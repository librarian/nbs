#pragma once

#include "public.h"

#include <cloud/blockstore/public/api/protos/volume.pb.h>

#include <cloud/blockstore/libs/kikimr/components.h>
#include <cloud/blockstore/libs/kikimr/events.h>
#include <cloud/blockstore/libs/storage/core/disk_counters.h>
#include <cloud/blockstore/libs/storage/protos/volume.pb.h>
#include <cloud/blockstore/libs/storage/protos_ydb/volume.pb.h>

namespace NCloud::NBlockStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

#define BLOCKSTORE_VOLUME_REQUESTS(xxx, ...)                                   \
    xxx(AddClient,                __VA_ARGS__)                                 \
    xxx(RemoveClient,             __VA_ARGS__)                                 \
    xxx(WaitReady,                __VA_ARGS__)                                 \
    xxx(DescribeBlocks,           __VA_ARGS__)                                 \
    xxx(GetPartitionInfo,         __VA_ARGS__)                                 \
    xxx(CompactRange,             __VA_ARGS__)                                 \
    xxx(GetCompactionStatus,      __VA_ARGS__)                                 \
    xxx(ReallocateDisk,           __VA_ARGS__)                                 \
    xxx(GetVolumeLoadInfo,        __VA_ARGS__)                                 \
    xxx(GetUsedBlocks,            __VA_ARGS__)                                 \
    xxx(DeleteCheckpointData,     __VA_ARGS__)                                 \
    xxx(UpdateUsedBlocks,         __VA_ARGS__)                                 \
    xxx(RebuildMetadata,          __VA_ARGS__)                                 \
    xxx(GetRebuildMetadataStatus, __VA_ARGS__)                                 \
    xxx(ScanDisk,                 __VA_ARGS__)                                 \
    xxx(GetScanDiskStatus,        __VA_ARGS__)                                 \
    xxx(GetVolumeInfo,            __VA_ARGS__)                                 \
    xxx(UpdateVolumeParams,       __VA_ARGS__)                                 \
    xxx(ChangeStorageConfig,      __VA_ARGS__)                                 \
// BLOCKSTORE_VOLUME_REQUESTS

// requests forwarded from service to volume
#define BLOCKSTORE_VOLUME_REQUESTS_FWD_SERVICE(xxx, ...)                       \
    xxx(ReadBlocks,           __VA_ARGS__)                                     \
    xxx(WriteBlocks,          __VA_ARGS__)                                     \
    xxx(ZeroBlocks,           __VA_ARGS__)                                     \
    xxx(StatVolume,           __VA_ARGS__)                                     \
    xxx(CreateCheckpoint,     __VA_ARGS__)                                     \
    xxx(DeleteCheckpoint,     __VA_ARGS__)                                     \
    xxx(GetChangedBlocks,     __VA_ARGS__)                                     \
    xxx(GetCheckpointStatus,  __VA_ARGS__)                                     \
    xxx(ReadBlocksLocal,      __VA_ARGS__)                                     \
    xxx(WriteBlocksLocal,     __VA_ARGS__)                                     \
// BLOCKSTORE_VOLUME_REQUESTS_FWD_SERVICE

// responses which are forwarded back via volume (volume has handlers for these)
#define BLOCKSTORE_VOLUME_HANDLED_RESPONSES(xxx, ...)                          \
    xxx(DescribeBlocks,           __VA_ARGS__)                                 \
    xxx(GetPartitionInfo,         __VA_ARGS__)                                 \
    xxx(CompactRange,             __VA_ARGS__)                                 \
    xxx(GetCompactionStatus,      __VA_ARGS__)                                 \
    xxx(GetUsedBlocks,            __VA_ARGS__)                                 \
    xxx(DeleteCheckpointData,     __VA_ARGS__)                                 \
    xxx(RebuildMetadata,          __VA_ARGS__)                                 \
    xxx(GetRebuildMetadataStatus, __VA_ARGS__)                                 \
    xxx(ScanDisk,                 __VA_ARGS__)                                 \
    xxx(GetScanDiskStatus,        __VA_ARGS__)                                 \
// BLOCKSTORE_VOLUME_HANDLED_RESPONSES

// responses for the requests forwarded from service which are forwarded back
// via volume (volume has handlers for these)
#define BLOCKSTORE_VOLUME_HANDLED_RESPONSES_FWD_SERVICE(xxx, ...)              \
    xxx(ReadBlocks,           __VA_ARGS__)                                     \
    xxx(WriteBlocks,          __VA_ARGS__)                                     \
    xxx(ZeroBlocks,           __VA_ARGS__)                                     \
    xxx(CreateCheckpoint,     __VA_ARGS__)                                     \
    xxx(DeleteCheckpoint,     __VA_ARGS__)                                     \
    xxx(GetChangedBlocks,     __VA_ARGS__)                                     \
    xxx(GetCheckpointStatus,  __VA_ARGS__)                                     \
    xxx(ReadBlocksLocal,      __VA_ARGS__)                                     \
    xxx(WriteBlocksLocal,     __VA_ARGS__)                                     \
// BLOCKSTORE_VOLUME_HANDLED_RESPONSES_FWD_SERVICE

////////////////////////////////////////////////////////////////////////////////

struct TEvVolume
{
    //
    // ReacquireDisk
    //

    struct TReacquireDisk
    {
    };

    //
    // UpdateMigrationState
    //

    struct TUpdateMigrationState
    {
        ui64 MigrationIndex;

        TUpdateMigrationState(ui64 migrationIndex)
            : MigrationIndex(migrationIndex)
        {
        }
    };

    //
    // MigrationStateUpdated
    //

    struct TMigrationStateUpdated
    {
    };

    //
    // RWClientIdChanged
    //

    struct TRWClientIdChanged
    {
        TString RWClientId;

        TRWClientIdChanged(TString rwClientId)
            : RWClientId(std::move(rwClientId))
        {
        }
    };

    //
    // DiskRegistryBasedPartitionCounters
    //

    struct TDiskRegistryBasedPartitionCounters
    {
        TPartitionDiskCountersPtr DiskCounters;
        ui64 NetworkBytes = 0;
        TDuration CpuUsage;

        TDiskRegistryBasedPartitionCounters(
                TPartitionDiskCountersPtr diskCounters)
            : DiskCounters(std::move(diskCounters))
        {
        }
    };

    //
    // RdmaUnavailable
    //

    struct TRdmaUnavailable
    {
    };

    //
    // UpdateResyncState
    //

    struct TUpdateResyncState
    {
        ui64 ResyncIndex;

        TUpdateResyncState(ui64 resyncIndex)
            : ResyncIndex(resyncIndex)
        {
        }
    };

    //
    // ResyncStateUpdated
    //

    struct TResyncStateUpdated
    {
    };

    //
    // ResyncFinished
    //

    struct TResyncFinished
    {
    };

    //
    // MapBaseDiskIdToTabletId
    //
    struct TMapBaseDiskIdToTabletId
    {
        const TString BaseDiskId;
        const ui64 BaseTabletId;

        TMapBaseDiskIdToTabletId(TString baseDiskId, ui64 baseTabletId)
            : BaseDiskId(std::move(baseDiskId))
            , BaseTabletId(baseTabletId)
        {}
    };

    //
    // ClearBaseDiskIdToTabletIdMapping
    //
    struct TClearBaseDiskIdToTabletIdMapping
    {
        const TString BaseDiskId;

        explicit TClearBaseDiskIdToTabletIdMapping(TString baseDiskId)
            : BaseDiskId(std::move(baseDiskId))
        {}
    };

    //
    // Events declaration
    //

    enum EEvents
    {
        EvBegin = TBlockStoreEvents::VOLUME_START,

        EvAddClientRequest = EvBegin + 5,
        EvAddClientResponse = EvBegin + 6,

        EvRemoveClientRequest = EvBegin + 7,
        EvRemoveClientResponse = EvBegin + 8,

        EvWaitReadyRequest = EvBegin + 9,
        EvWaitReadyResponse = EvBegin + 10,

        EvDescribeBlocksRequest = EvBegin + 11,
        EvDescribeBlocksResponse = EvBegin + 12,

        EvGetPartitionInfoRequest = EvBegin + 13,
        EvGetPartitionInfoResponse = EvBegin + 14,

        EvCompactRangeRequest = EvBegin + 15,
        EvCompactRangeResponse = EvBegin + 16,

        EvGetCompactionStatusRequest = EvBegin + 17,
        EvGetCompactionStatusResponse = EvBegin + 18,

        EvReacquireDisk = EvBegin + 19,

        EvReallocateDiskRequest = EvBegin + 20,
        EvReallocateDiskResponse = EvBegin + 21,

        EvGetVolumeLoadInfoRequest = EvBegin + 22,
        EvGetVolumeLoadInfoResponse = EvBegin + 23,

        EvGetUsedBlocksRequest = EvBegin + 24,
        EvGetUsedBlocksResponse = EvBegin + 25,

        EvDeleteCheckpointDataRequest = EvBegin + 26,
        EvDeleteCheckpointDataResponse = EvBegin + 27,

        EvUpdateUsedBlocksRequest = EvBegin + 28,
        EvUpdateUsedBlocksResponse = EvBegin + 29,

        EvUpdateMigrationState = EvBegin + 30,
        EvMigrationStateUpdated = EvBegin + 31,

        EvRWClientIdChanged = EvBegin + 32,

        EvDiskRegistryBasedPartitionCounters = EvBegin + 33,

        EvRebuildMetadataRequest = EvBegin + 34,
        EvRebuildMetadataResponse = EvBegin + 35,

        EvGetRebuildMetadataStatusRequest = EvBegin + 36,
        EvGetRebuildMetadataStatusResponse = EvBegin + 37,

        EvRdmaUnavailable = EvBegin + 38,

        EvUpdateResyncState = EvBegin + 39,
        EvResyncStateUpdated = EvBegin + 40,

        EvScanDiskRequest = EvBegin + 41,
        EvScanDiskResponse = EvBegin + 42,

        EvGetScanDiskStatusRequest = EvBegin + 43,
        EvGetScanDiskStatusResponse = EvBegin + 44,

        EvResyncFinished = EvBegin + 45,

        EvGetVolumeInfoRequest = EvBegin + 46,
        EvGetVolumeInfoResponse = EvBegin + 47,

        EvMapBaseDiskIdToTabletId = EvBegin + 48,

        EvSetupChannelsRequest = EvBegin + 49,
        EvSetupChannelsResponse = EvBegin + 50,

        EvClearBaseDiskIdToTabletIdMapping = EvBegin + 51,

        EvUpdateVolumeParamsRequest = EvBegin + 52,
        EvUpdateVolumeParamsResponse = EvBegin + 53,

        EvChangeStorageConfigRequest = EvBegin + 54,
        EvChangeStorageConfigResponse = EvBegin + 55,

        EvEnd
    };

    static_assert(EvEnd < (int)TBlockStoreEvents::VOLUME_END,
        "EvEnd expected to be < TBlockStoreEvents::VOLUME_END");

    BLOCKSTORE_VOLUME_REQUESTS(BLOCKSTORE_DECLARE_PROTO_EVENTS)

    using TEvReacquireDisk = TRequestEvent<
        TReacquireDisk,
        EvReacquireDisk
    >;

    using TEvUpdateMigrationState = TRequestEvent<
        TUpdateMigrationState,
        EvUpdateMigrationState
    >;

    using TEvMigrationStateUpdated = TRequestEvent<
        TMigrationStateUpdated,
        EvMigrationStateUpdated
    >;

    using TEvRWClientIdChanged = TRequestEvent<
        TRWClientIdChanged,
        EvRWClientIdChanged
    >;

    using TEvDiskRegistryBasedPartitionCounters = TRequestEvent<
        TDiskRegistryBasedPartitionCounters,
        EvDiskRegistryBasedPartitionCounters
    >;

    using TEvRdmaUnavailable = TRequestEvent<
        TRdmaUnavailable,
        EvRdmaUnavailable
    >;

    using TEvUpdateResyncState = TRequestEvent<
        TUpdateResyncState,
        EvUpdateResyncState
    >;

    using TEvResyncStateUpdated = TRequestEvent<
        TResyncStateUpdated,
        EvResyncStateUpdated
    >;

    using TEvResyncFinished = TRequestEvent<
        TResyncFinished,
        EvResyncFinished
    >;

    using TEvMapBaseDiskIdToTabletId = TRequestEvent<
        TMapBaseDiskIdToTabletId,
        EvMapBaseDiskIdToTabletId
    >;

    using TEvClearBaseDiskIdToTabletIdMapping = TRequestEvent<
        TClearBaseDiskIdToTabletIdMapping,
        EvClearBaseDiskIdToTabletIdMapping
    >;
};

}   // namespace NCloud::NBlockStore::NStorage
