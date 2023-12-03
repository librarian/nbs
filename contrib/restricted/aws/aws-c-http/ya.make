# Generated by devtools/yamaker from nixpkgs 23.05.

LIBRARY()

LICENSE(Apache-2.0)

LICENSE_TEXTS(.yandex_meta/licenses.list.txt)

VERSION(0.7.6)

ORIGINAL_SOURCE(https://github.com/awslabs/aws-c-http/archive/v0.7.6.tar.gz)

PEERDIR(
    contrib/restricted/aws/aws-c-cal
    contrib/restricted/aws/aws-c-common
    contrib/restricted/aws/aws-c-compression
    contrib/restricted/aws/aws-c-io
)

ADDINCL(
    GLOBAL contrib/restricted/aws/aws-c-http/include
)

NO_COMPILER_WARNINGS()

NO_RUNTIME()

CFLAGS(
    -DAWS_CAL_USE_IMPORT_EXPORT
    -DAWS_COMMON_USE_IMPORT_EXPORT
    -DAWS_COMPRESSION_USE_IMPORT_EXPORT
    -DAWS_HTTP_USE_IMPORT_EXPORT
    -DAWS_IO_USE_IMPORT_EXPORT
    -DAWS_USE_EPOLL
    -DHAVE_SYSCONF
    -DS2N_CLONE_SUPPORTED
    -DS2N_CPUID_AVAILABLE
    -DS2N_FALL_THROUGH_SUPPORTED
    -DS2N_FEATURES_AVAILABLE
    -DS2N_KYBER512R3_AVX2_BMI2
    -DS2N_LIBCRYPTO_SUPPORTS_EVP_MD5_SHA1_HASH
    -DS2N_LIBCRYPTO_SUPPORTS_EVP_MD_CTX_SET_PKEY_CTX
    -DS2N_LIBCRYPTO_SUPPORTS_EVP_RC4
    -DS2N_MADVISE_SUPPORTED
    -DS2N_PLATFORM_SUPPORTS_KTLS
    -DS2N_STACKTRACE
    -DS2N___RESTRICT__SUPPORTED
)

SRCS(
    source/connection.c
    source/connection_manager.c
    source/connection_monitor.c
    source/h1_connection.c
    source/h1_decoder.c
    source/h1_encoder.c
    source/h1_stream.c
    source/h2_connection.c
    source/h2_decoder.c
    source/h2_frames.c
    source/h2_stream.c
    source/hpack.c
    source/hpack_decoder.c
    source/hpack_encoder.c
    source/hpack_huffman_static.c
    source/http.c
    source/http2_stream_manager.c
    source/proxy_connection.c
    source/proxy_strategy.c
    source/random_access_set.c
    source/request_response.c
    source/statistics.c
    source/strutil.c
    source/websocket.c
    source/websocket_bootstrap.c
    source/websocket_decoder.c
    source/websocket_encoder.c
)

END()
