#include "yql_generic_provider_impl.h"

#include <ydb/library/yql/core/expr_nodes/yql_expr_nodes.h>
#include <ydb/library/yql/core/yql_opt_utils.h>
#include <ydb/library/yql/providers/common/provider/yql_data_provider_impl.h>
#include <ydb/library/yql/providers/common/provider/yql_provider.h>
#include <ydb/library/yql/providers/common/provider/yql_provider_names.h>
#include <ydb/library/yql/providers/common/transform/yql_optimize.h>
#include <ydb/library/yql/providers/generic/expr_nodes/yql_generic_expr_nodes.h>
#include <ydb/library/yql/providers/dq/expr_nodes/dqs_expr_nodes.h>
#include <ydb/library/yql/utils/log/log.h>

namespace NYql {

    using namespace NNodes;

    namespace {

        class TGenericPhysicalOptProposalTransformer: public TOptimizeTransformerBase {
        public:
            TGenericPhysicalOptProposalTransformer(TGenericState::TPtr state)
                : TOptimizeTransformerBase(state->Types, NLog::EComponent::ProviderYdb, {})
                , State_(state)
            {
#define HNDL(name) "PhysicalOptimizer-" #name, Hndl(&TGenericPhysicalOptProposalTransformer::name)
                AddHandler(0, &TCoLeft::Match, HNDL(TrimReadWorld));
                AddHandler(0, &TCoNarrowMap::Match, HNDL(ReadZeroColumns));
#undef HNDL
            }

            TMaybeNode<TExprBase> TrimReadWorld(TExprBase node, TExprContext& ctx) const {
                Y_UNUSED(ctx);

                const auto& maybeRead = node.Cast<TCoLeft>().Input().Maybe<TGenReadTable>();
                if (!maybeRead) {
                    return node;
                }

                return TExprBase(maybeRead.Cast().World().Ptr());
            }

            TMaybeNode<TExprBase> ReadZeroColumns(TExprBase node, TExprContext& ctx) const {
                const auto& narrow = node.Maybe<TCoNarrowMap>();
                if (const auto& wide = narrow.Cast().Input().Maybe<TDqReadWideWrap>()) {
                    if (const auto& maybe = wide.Cast().Input().Maybe<TGenReadTable>()) {
                        if (!wide.Cast()
                                 .Ref()
                                 .GetTypeAnn()
                                 ->Cast<TFlowExprType>()
                                 ->GetItemType()
                                 ->Cast<TMultiExprType>()
                                 ->GetSize()) {
                            const auto& read = maybe.Cast();

                            // Get table metadata
                            const auto [tableMeta, issue] = State_->GetTable(
                                read.DataSource().Cluster().Value(),
                                read.Table().Value(),
                                ctx.GetPosition(node.Pos()));
                            if (issue.has_value()) {
                                ctx.AddError(issue.value());
                                return node;
                            }

                            const auto structType = tableMeta.value()->ItemType;
                            YQL_ENSURE(structType->GetSize());
                            auto columns =
                                ctx.NewList(read.Pos(), {ctx.NewAtom(read.Pos(), GetLightColumn(*structType)->GetName())});

                            // clang-format off
                            return Build<TCoNarrowMap>(ctx, narrow.Cast().Pos())
                                .Input<TDqReadWideWrap>()
                                    .InitFrom(wide.Cast())
                                    .Input<TGenReadTable>()
                                        .InitFrom(read)
                                        .Columns(std::move(columns))
                                    .Build()
                                .Build()
                                .Lambda()
                                    .Args({"stub"})
                                    .Body<TCoAsStruct>().Build()
                                .Build()
                            .Done();
                            // clang-format on
                        }
                    }
                }

                return node;
            }

        private:
            const TGenericState::TPtr State_;
        };

    }

    THolder<IGraphTransformer> CreateGenericPhysicalOptProposalTransformer(TGenericState::TPtr state) {
        return MakeHolder<TGenericPhysicalOptProposalTransformer>(state);
    }

} // namespace NYql
