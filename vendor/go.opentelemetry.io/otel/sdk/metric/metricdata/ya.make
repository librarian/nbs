GO_LIBRARY()

SUBSCRIBER(g:go-contrib)

LICENSE(Apache-2.0)

SRCS(
    data.go
    temporality.go
    temporality_string.go
)

END()

RECURSE(
    metricdatatest
)
