#pragma once

#include <contrib/ydb/library/yql/providers/common/transform/yql_visit.h>
#include <contrib/ydb/library/yql/core/yql_expr_type_annotation.h>

#include <util/generic/ptr.h>

namespace NYql {

THolder<TVisitorTransformerBase> CreateDqsDataSinkTypeAnnotationTransformer(TTypeAnnotationContext* typeCtx, bool enableDqReplicate);

} // NYql
