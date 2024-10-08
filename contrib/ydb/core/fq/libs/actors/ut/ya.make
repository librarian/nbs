UNITTEST_FOR(contrib/ydb/core/fq/libs/actors)

SIZE(MEDIUM)

FORK_SUBTESTS()

PEERDIR(
    library/cpp/retry
    library/cpp/testing/unittest
    contrib/ydb/core/fq/libs/actors
    contrib/ydb/core/testlib/default
)

YQL_LAST_ABI_VERSION()

SRCS(
    database_resolver_ut.cpp
)

END()
