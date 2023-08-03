#include "volume_actor.h"

#include <cloud/blockstore/libs/storage/core/config.h>
#include <cloud/blockstore/libs/storage/core/proto_helpers.h>
#include <cloud/storage/core/libs/common/verify.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

using namespace NKikimr;

////////////////////////////////////////////////////////////////////////////////

void TVolumeActor::HandleUpdateVolumeParams(
    const TEvVolume::TEvUpdateVolumeParamsRequest::TPtr& ev,
    const TActorContext& ctx)
{
    STORAGE_VERIFY(State, TWellKnownEntityTypes::TABLET, TabletID());

    BLOCKSTORE_VOLUME_COUNTER(UpdateVolumeParams);

    LOG_DEBUG(ctx, TBlockStoreComponents::VOLUME,
        "[%lu] Received UpdateVolumeParams request",
        TabletID());

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        MakeIntrusive<TCallContext>());

    auto* msg = ev->Get();

    THashMap<TString, TVolumeParamsValue> volumeParams;
    for (const auto& [key, param]: msg->Record.GetVolumeParams()) {
        volumeParams.try_emplace(
            key,
            TVolumeParamsValue{
                key,
                param.GetValue(),
                ctx.Now() + TDuration::MilliSeconds(param.GetTtlMs())
            }
        );
    }

    ExecuteTx<TUpdateVolumeParams>(
        ctx,
        std::move(requestInfo),
        std::move(volumeParams));

    auto response = std::make_unique<TEvVolume::TEvUpdateVolumeParamsResponse>();
    NCloud::Reply(ctx, *ev, std::move(response));
}

void TVolumeActor::HandleRemoveExpiredVolumeParams(
    const TEvVolumePrivate::TEvRemoveExpiredVolumeParams::TPtr& ev,
    const TActorContext& ctx)
{
    STORAGE_VERIFY(State, TWellKnownEntityTypes::TABLET, TabletID());

    LOG_DEBUG(ctx, TBlockStoreComponents::VOLUME,
        "[%lu] Received RemoveExpiredVolumeParams request",
        TabletID());

    RemoveExpiredVolumeParamsScheduled = false;

    auto expiredKeys = State->GetVolumeParams().GetExpiredKeys(ctx.Now());
    if (expiredKeys.empty()) {
        // means that ttl was changed and override is still valid
        ScheduleRegularUpdates(ctx);
        return;
    }

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        MakeIntrusive<TCallContext>());

    ExecuteTx<TDeleteVolumeParams>(
        ctx,
        std::move(requestInfo),
        std::move(expiredKeys));
}

////////////////////////////////////////////////////////////////////////////////

bool TVolumeActor::PrepareUpdateVolumeParams(
    const TActorContext& ctx,
    ITransactionBase::TTransactionContext& tx,
    TTxVolume::TUpdateVolumeParams& args)
{
    Y_UNUSED(ctx);
    Y_UNUSED(tx);
    Y_UNUSED(args);

    return true;
}

void TVolumeActor::ExecuteUpdateVolumeParams(
    const TActorContext& ctx,
    ITransactionBase::TTransactionContext& tx,
    TTxVolume::TUpdateVolumeParams& args)
{
    Y_UNUSED(ctx);

    // This might lead to 'dirty' commit, if the same values are updated
    // from different transactions simultaneously, but we accept that risk.
    State->MergeVolumeParams(args.VolumeParams);

    TVector<TVolumeParamsValue> volumeParamsVec;
    for (const auto& [key, value]: args.VolumeParams) {
        volumeParamsVec.emplace_back(value);
    }

    TVolumeDatabase db(tx.DB);
    db.WriteVolumeParams(volumeParamsVec);
}

void TVolumeActor::CompleteUpdateVolumeParams(
    const TActorContext& ctx,
    TTxVolume::TUpdateVolumeParams& args)
{
    LOG_DEBUG(ctx, TBlockStoreComponents::VOLUME,
        "[%lu] Sending OK response for UpdateVolumeParams",
        TabletID());

    auto response = std::make_unique<TEvVolume::TEvUpdateVolumeParamsResponse>();
    NCloud::Reply(ctx, *args.RequestInfo, std::move(response));

    StopPartitions(ctx);
    SendVolumeConfigUpdated(ctx);
    StartPartitionsForUse(ctx);
    ResetServicePipes(ctx);
}

////////////////////////////////////////////////////////////////////////////////

bool TVolumeActor::PrepareDeleteVolumeParams(
    const TActorContext& ctx,
    ITransactionBase::TTransactionContext& tx,
    TTxVolume::TDeleteVolumeParams& args)
{
    Y_UNUSED(ctx);
    Y_UNUSED(tx);
    Y_UNUSED(args);

    return true;
}

void TVolumeActor::ExecuteDeleteVolumeParams(
    const TActorContext& ctx,
    ITransactionBase::TTransactionContext& tx,
    TTxVolume::TDeleteVolumeParams& args)
{
    Y_UNUSED(ctx);

    TVolumeDatabase db(tx.DB);
    db.DeleteVolumeParams(args.Keys);
}

void TVolumeActor::CompleteDeleteVolumeParams(
    const TActorContext& ctx,
    TTxVolume::TDeleteVolumeParams& args)
{
    Y_UNUSED(args);

    StopPartitions(ctx);
    SendVolumeConfigUpdated(ctx);
    StartPartitionsForUse(ctx);
    ResetServicePipes(ctx);
}

}   // namespace NCloud::NBlockStore::NStorage
