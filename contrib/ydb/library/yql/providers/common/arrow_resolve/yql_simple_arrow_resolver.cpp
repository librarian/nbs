#include "yql_simple_arrow_resolver.h"

#include <contrib/ydb/library/yql/minikql/arrow/mkql_functions.h>
#include <contrib/ydb/library/yql/minikql/mkql_program_builder.h>
#include <contrib/ydb/library/yql/minikql/mkql_type_builder.h>
#include <contrib/ydb/library/yql/minikql/mkql_function_registry.h>
#include <contrib/ydb/library/yql/providers/common/mkql/yql_type_mkql.h>

#include <util/stream/null.h>

namespace NYql {

using namespace NKikimr::NMiniKQL;

class TSimpleArrowResolver: public IArrowResolver {
public:
    TSimpleArrowResolver(const IFunctionRegistry& functionRegistry)
        : FunctionRegistry_(functionRegistry)
    {}

private:
    EStatus LoadFunctionMetadata(const TPosition& pos, TStringBuf name, const TVector<const TTypeAnnotationNode*>& argTypes,
        const TTypeAnnotationNode* returnType, TExprContext& ctx) const override
    {
        try {
            TScopedAlloc alloc(__LOCATION__);
            TTypeEnvironment env(alloc);
            TProgramBuilder pgmBuilder(env, FunctionRegistry_);
            TNullOutput null;
            TVector<TType*> mkqlInputTypes;
            for (const auto& type : argTypes) {
                auto mkqlType = NCommon::BuildType(*type, pgmBuilder, null);
                YQL_ENSURE(mkqlType, "Failed to convert type " << *type << " to MKQL type");
                mkqlInputTypes.emplace_back(mkqlType);
            }
            TType* mkqlOutputType = NCommon::BuildType(*returnType, pgmBuilder, null);
            bool found = FindArrowFunction(name, mkqlInputTypes, mkqlOutputType, *FunctionRegistry_.GetBuiltins());
            return found ? EStatus::OK : EStatus::NOT_FOUND;
        } catch (const std::exception& e) {
            ctx.AddError(TIssue(pos, e.what()));
            return EStatus::ERROR;
        }
    }

    EStatus HasCast(const TPosition& pos, const TTypeAnnotationNode* from, const TTypeAnnotationNode* to, TExprContext& ctx) const override {
        try {
            TScopedAlloc alloc(__LOCATION__);
            TTypeEnvironment env(alloc);
            TProgramBuilder pgmBuilder(env, FunctionRegistry_);
            TNullOutput null;
            auto mkqlFromType = NCommon::BuildType(*from, pgmBuilder, null);
            auto mkqlToType = NCommon::BuildType(*to, pgmBuilder, null);
            return HasArrowCast(mkqlFromType, mkqlToType) ? EStatus::OK : EStatus::NOT_FOUND;
        } catch (const std::exception& e) {
            ctx.AddError(TIssue(pos, e.what()));
            return EStatus::ERROR;
        }
    }

    EStatus AreTypesSupported(const TPosition& pos, const TVector<const TTypeAnnotationNode*>& types, TExprContext& ctx) const override {
        try {
            TScopedAlloc alloc(__LOCATION__);
            TTypeEnvironment env(alloc);
            TProgramBuilder pgmBuilder(env, FunctionRegistry_);
            for (const auto& type : types) {
                TNullOutput null;
                auto mkqlType = NCommon::BuildType(*type, pgmBuilder, null);
                std::shared_ptr<arrow::DataType> arrowType;
                if (!ConvertArrowType(mkqlType, arrowType)) {
                    return EStatus::NOT_FOUND;
                }
            }
            return EStatus::OK;
        } catch (const std::exception& e) {
            ctx.AddError(TIssue(pos, e.what()));
            return EStatus::ERROR;
        }
    }

private:
    const IFunctionRegistry& FunctionRegistry_;
};

IArrowResolver::TPtr MakeSimpleArrowResolver(const IFunctionRegistry& functionRegistry) {
    return new TSimpleArrowResolver(functionRegistry);
}

} // namespace NYql
