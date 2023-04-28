#pragma once

#include "public.h"

#include "config.h"

#include <cloud/blockstore/libs/storage/core/public.h>

namespace NCloud::NBlockStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

struct TReplicaInfo
{
    TNonreplicatedPartitionConfigPtr Config;
    google::protobuf::RepeatedPtrField<NProto::TDeviceMigration> Migrations;
};

}   // namespace NCloud::NBlockStore::NStorage
