GO_LIBRARY()

LICENSE(Apache-2.0)

SRCS(
    debug.go
    decode.go
    encode.go
    error.go
    header.go
    header_value.go
    message.go
)

GO_TEST_SRCS(
    decode_test.go
    encode_test.go
    header_test.go
    header_value_test.go
    shared_test.go
)

END()

RECURSE(
    eventstreamapi
    eventstreamtest
    gotest
)
