# Generated by devtools/yamaker from nixpkgs 22.11.

LIBRARY()

LICENSE(
    BSL-1.0 AND
    Mit-Old-Style
)

LICENSE_TEXTS(.yandex_meta/licenses.list.txt)

VERSION(1.84.0)

ORIGINAL_SOURCE(https://github.com/boostorg/multi_index/archive/boost-1.84.0.tar.gz)

PEERDIR(
    contrib/restricted/boost/assert
    contrib/restricted/boost/bind
    contrib/restricted/boost/config
    contrib/restricted/boost/container_hash
    contrib/restricted/boost/core
    contrib/restricted/boost/integer
    contrib/restricted/boost/iterator
    contrib/restricted/boost/move
    contrib/restricted/boost/mpl
    contrib/restricted/boost/preprocessor
    contrib/restricted/boost/smart_ptr
    contrib/restricted/boost/static_assert
    contrib/restricted/boost/throw_exception
    contrib/restricted/boost/tuple
    contrib/restricted/boost/type_traits
    contrib/restricted/boost/utility
)

ADDINCL(
    GLOBAL contrib/restricted/boost/multi_index/include
)

NO_COMPILER_WARNINGS()

NO_UTIL()

END()
