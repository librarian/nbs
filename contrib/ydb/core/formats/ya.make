RECURSE(
    arrow
)

LIBRARY()

PEERDIR(
    contrib/ydb/core/scheme
)

YQL_LAST_ABI_VERSION()

SRCS(
    clickhouse_block.h
    clickhouse_block.cpp
    factory.h
)

END()
