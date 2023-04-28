#pragma once

#include "command.h"

namespace NCloud::NBlockStore::NClient {

////////////////////////////////////////////////////////////////////////////////

TCommandPtr NewDestroyVolumeCommand(IBlockStorePtr client);

}   // namespace NCloud::NBlockStore::NClient
