#pragma once

#include "public.h"

#include <util/generic/strbuf.h>
#include <util/system/defaults.h>

namespace NCloud {

////////////////////////////////////////////////////////////////////////////////

bool TryParseSourceFd(const TStringBuf& peer, ui32* fd);

size_t SetExecutorThreadsLimit(size_t count);

}   // namespace NCloud
