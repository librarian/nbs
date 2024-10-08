UNITTEST_FOR(contrib/ydb/library/yql/providers/s3/range_helpers)

SRCS(
    file_tree_builder_ut.cpp
    path_list_reader_ut.cpp
)

PEERDIR(
    contrib/ydb/library/yql/providers/common/provider
    contrib/ydb/library/yql/public/udf/service/exception_policy
    contrib/ydb/library/yql/sql/pg_dummy
)

YQL_LAST_ABI_VERSION()

END()
