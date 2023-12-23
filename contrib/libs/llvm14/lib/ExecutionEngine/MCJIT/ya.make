# Generated by devtools/yamaker.

LIBRARY()

LICENSE(Apache-2.0 WITH LLVM-exception)

LICENSE_TEXTS(.yandex_meta/licenses.list.txt)

PEERDIR(
    contrib/libs/llvm14
    contrib/libs/llvm14/include
    contrib/libs/llvm14/lib/ExecutionEngine
    contrib/libs/llvm14/lib/ExecutionEngine/RuntimeDyld
    contrib/libs/llvm14/lib/IR
    contrib/libs/llvm14/lib/Object
    contrib/libs/llvm14/lib/Support
    contrib/libs/llvm14/lib/Target
)

ADDINCL(
    contrib/libs/llvm14/lib/ExecutionEngine/MCJIT
)

NO_COMPILER_WARNINGS()

NO_UTIL()

SRCS(
    MCJIT.cpp
)

END()
