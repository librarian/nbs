LIBRARY()

SRCS(
    yql_yt_url_mapper.cpp
)

PEERDIR(
    contrib/ydb/library/yql/providers/common/proto
    library/cpp/regex/pcre
    library/cpp/uri
    library/cpp/cgiparam
)

END()
