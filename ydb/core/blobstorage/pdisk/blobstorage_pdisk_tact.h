#pragma once
#include "defs.h"

namespace NKikimr {
namespace NPDisk {

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 2-tact work cycle types
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum class ETact {
    TactLc,
    TactCl,
    TactLl,
    TactCc
};

} // NPDisk
} // NKikimr

