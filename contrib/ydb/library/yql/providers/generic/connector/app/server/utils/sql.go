package utils

import (
	"context"

	"github.com/apache/arrow/go/v13/arrow/array"
	api_common "github.com/ydb-platform/nbs/contrib/ydb/library/yql/providers/generic/connector/api/common"
	api_service_protos "github.com/ydb-platform/nbs/contrib/ydb/library/yql/providers/generic/connector/libgo/service/protos"
	"github.com/ydb-platform/nbs/library/go/core/log"
	"github.com/ydb-platform/ydb-go-genproto/protos/Ydb"
)

type Connection interface {
	Query(ctx context.Context, query string, args ...any) (Rows, error)
	Close() error
}

type Rows interface {
	Close() error
	Err() error
	Next() bool
	Scan(dest ...any) error
	MakeTransformer(ydbTypes []*Ydb.Type) (Transformer, error)
}

// Transformer accepts values from a row, converts them and appends to arrow builders
type Transformer interface {
	// Return acceptors for scan values from a row
	GetAcceptors() []any

	// Convert and append to arrow buiders accepted values
	AppendToArrowBuilders(builder []array.Builder) error
}

type ConnectionManager interface {
	Make(ctx context.Context, logger log.Logger, dataSourceInstance *api_common.TDataSourceInstance) (Connection, error)
	Release(logger log.Logger, connection Connection)
}

type ConnectionManagerBase struct {
	QueryLoggerFactory QueryLoggerFactory
}

type SQLFormatter interface {
	GetDescribeTableQuery(request *api_service_protos.TDescribeTableRequest) (string, []any)

	// Get placeholder for n'th argument (starting from 0) for prepared statement
	GetPlaceholder(n int) string

	// Sanitize names of databases, tables, columns, views, schemas
	SanitiseIdentifier(ident string) string

	// Support for high level expression (without subexpressions, they are checked separately)
	SupportsPushdownExpression(expression *api_service_protos.TExpression) bool
}
