LIBRARY()

SRCS(
    scheme_helpers.cpp
)

PEERDIR(
    contrib/ydb/core/base
    contrib/ydb/core/kqp/provider
    contrib/ydb/core/protos
)

YQL_LAST_ABI_VERSION()

END()
