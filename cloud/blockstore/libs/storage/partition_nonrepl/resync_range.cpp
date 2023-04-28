#include "resync_range.h"

#include <cloud/blockstore/libs/common/sglist.h>
#include <cloud/blockstore/libs/diagnostics/block_digest.h>
#include <cloud/blockstore/libs/kikimr/components.h>
#include <cloud/blockstore/libs/kikimr/helpers.h>
#include <cloud/blockstore/libs/storage/api/disk_agent.h>
#include <cloud/blockstore/libs/storage/core/probes.h>
#include <cloud/blockstore/libs/storage/disk_agent/public.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

LWTRACE_USING(BLOCKSTORE_STORAGE_PROVIDER);

////////////////////////////////////////////////////////////////////////////////

TResyncRangeActor::TResyncRangeActor(
        TRequestInfoPtr requestInfo,
        ui32 blockSize,
        TBlockRange64 range,
        TVector<TResyncReplica> replicas,
        TString writerSessionId,
        IBlockDigestGeneratorPtr blockDigestGenerator)
    : RequestInfo(std::move(requestInfo))
    , BlockSize(blockSize)
    , Range(range)
    , Replicas(std::move(replicas))
    , WriterSessionId(std::move(writerSessionId))
    , BlockDigestGenerator(std::move(blockDigestGenerator))
{
    ActivityType = TBlockStoreActivities::PARTITION_WORKER;
}

void TResyncRangeActor::Bootstrap(const TActorContext& ctx)
{
    TRequestScope timer(*RequestInfo);

    Become(&TThis::StateWork);

    LWTRACK(
        RequestReceived_PartitionWorker,
        RequestInfo->CallContext->LWOrbit,
        "ResyncRange",
        RequestInfo->CallContext->RequestId);

    ChecksumBlocks(ctx);
}

void TResyncRangeActor::ChecksumBlocks(const TActorContext& ctx) {
    for (size_t i = 0; i < Replicas.size(); ++i) {
        ChecksumReplicaBlocks(ctx, i);
    }

    ChecksumStartTs = ctx.Now();
}

void TResyncRangeActor::ChecksumReplicaBlocks(const TActorContext& ctx, int idx)
{
    auto request = std::make_unique<TEvNonreplPartitionPrivate::TEvChecksumBlocksRequest>();
    request->Record.SetStartIndex(Range.Start);
    request->Record.SetBlocksCount(Range.Size());
    request->Record.SetSessionId(TString(BackgroundOpsSessionId));
    request->Record.MutableHeaders()->SetIsBackgroundRequest(true);

    auto traceId = RequestInfo->TraceId.Clone();
    BLOCKSTORE_TRACE_SENT(ctx, &traceId, this, request);

    TAutoPtr<IEventHandle> event(
        new IEventHandle(
            Replicas[idx].ActorId,
            ctx.SelfID,
            request.get(),
            IEventHandle::FlagForwardOnNondelivery,
            idx,  // cookie
            &ctx.SelfID,    // forwardOnNondelivery
            std::move(traceId)
        )
    );
    Y_UNUSED(request.release());

    ctx.Send(event);
}

void TResyncRangeActor::CompareChecksums(const TActorContext& ctx) {
    THashMap<ui64, ui32> checksumCount;
    ui32 majorCount = 0;
    ui64 majorChecksum = 0;
    int majorIdx = 0;

    for (const auto& [idx, checksum]: Checksums) {
        if (++checksumCount[checksum] > majorCount) {
            majorCount = checksumCount[checksum];
            majorChecksum = checksum;
            majorIdx = idx;
        }
    }

    if (majorCount == Replicas.size()) {
        // all checksums match
        Done(ctx);
        return;
    }

    LOG_WARN(ctx, TBlockStoreComponents::PARTITION,
        "[%s] Resync range %s: majority replica %lu, checksum %lu, count %u of %u",
        Replicas[0].Name.c_str(),
        DescribeRange(Range).c_str(),
        majorIdx,
        majorChecksum,
        majorCount,
        Replicas.size());

    for (const auto& [idx, checksum]: Checksums) {
        if (checksum != majorChecksum) {
            LOG_WARN(ctx, TBlockStoreComponents::PARTITION,
                "[%s] Replica %lu block range %s checksum %lu differs from majority checksum %lu",
                Replicas[0].Name.c_str(),
                idx,
                DescribeRange(Range).c_str(),
                checksum,
                majorChecksum);

            ActorsToResync.push_back(idx);
        }
    }

    ReadBlocks(ctx, majorIdx);
}

void TResyncRangeActor::ReadBlocks(const TActorContext& ctx, int idx)
{
    Buffer = TGuardedBuffer(TString::Uninitialized(Range.Size() * BlockSize));

    auto sgList = Buffer.GetGuardedSgList();
    auto sgListOrError = SgListNormalize(sgList.Acquire().Get(), BlockSize);
    Y_VERIFY(!HasError(sgListOrError));
    SgList.SetSgList(sgListOrError.ExtractResult());

    auto request = std::make_unique<TEvService::TEvReadBlocksLocalRequest>();
    request->Record.SetStartIndex(Range.Start);
    request->Record.SetBlocksCount(Range.Size());
    request->Record.SetSessionId(TString(BackgroundOpsSessionId));
    request->Record.BlockSize = BlockSize;
    request->Record.Sglist = SgList;
    request->Record.MutableHeaders()->SetIsBackgroundRequest(true);

    auto traceId = RequestInfo->TraceId.Clone();
    BLOCKSTORE_TRACE_SENT(ctx, &traceId, this, request);

    TAutoPtr<IEventHandle> event(
        new IEventHandle(
            Replicas[idx].ActorId,
            ctx.SelfID,
            request.get(),
            IEventHandle::FlagForwardOnNondelivery,
            idx,  // cookie
            &ctx.SelfID,    // forwardOnNondelivery
            std::move(traceId)
        )
    );
    Y_UNUSED(request.release());

    ctx.Send(event);

    ReadStartTs = ctx.Now();
}

void TResyncRangeActor::WriteBlocks(const TActorContext& ctx) {
    for (int idx: ActorsToResync) {
        WriteReplicaBlocks(ctx, idx);
    }

    WriteStartTs = ctx.Now();
}

void TResyncRangeActor::WriteReplicaBlocks(const TActorContext& ctx, int idx)
{
    auto request = std::make_unique<TEvService::TEvWriteBlocksLocalRequest>();
    request->Record.SetStartIndex(Range.Start);
    request->Record.SetSessionId(
        WriterSessionId ? WriterSessionId : TString(BackgroundOpsSessionId));
    request->Record.BlocksCount = Range.Size();
    request->Record.BlockSize = BlockSize;
    request->Record.Sglist = SgList;
    request->Record.MutableHeaders()->SetIsBackgroundRequest(true);

    for (const auto blockIndex: xrange(Range)) {
        auto* data = Buffer.Get().Data() + (blockIndex - Range.Start) * BlockSize;

        const auto digest = BlockDigestGenerator->ComputeDigest(
            blockIndex,
            TBlockDataRef(data, BlockSize)
        );

        if (digest.Defined()) {
            AffectedBlockInfos.push_back({blockIndex, *digest});
        }
    }

    auto traceId = RequestInfo->TraceId.Clone();
    BLOCKSTORE_TRACE_SENT(ctx, &traceId, this, request);

    TAutoPtr<IEventHandle> event(
        new IEventHandle(
            Replicas[idx].ActorId,
            ctx.SelfID,
            request.get(),
            IEventHandle::FlagForwardOnNondelivery,
            idx,  // cookie
            &ctx.SelfID,    // forwardOnNondelivery
            std::move(traceId)
        )
    );
    Y_UNUSED(request.release());

    LOG_WARN(ctx, TBlockStoreComponents::PARTITION,
        "[%s] Replica %lu Overwrite block range %s during resync",
        Replicas[0].Name.c_str(),
        idx,
        DescribeRange(Range).c_str());

    ctx.Send(event);
}

void TResyncRangeActor::Done(const TActorContext& ctx)
{
    auto response = std::make_unique<TEvNonreplPartitionPrivate::TEvRangeResynced>(
        std::move(Error),
        Range,
        ChecksumStartTs,
        ChecksumDuration,
        ReadStartTs,
        ReadDuration,
        WriteStartTs,
        WriteDuration,
        std::move(AffectedBlockInfos)
    );

    BLOCKSTORE_TRACE_SENT(ctx, &RequestInfo->TraceId, this, response);

    LWTRACK(
        ResponseSent_PartitionWorker,
        RequestInfo->CallContext->LWOrbit,
        "ResyncRange",
        RequestInfo->CallContext->RequestId);

    NCloud::Reply(ctx, *RequestInfo, std::move(response));

    Die(ctx);
}

////////////////////////////////////////////////////////////////////////////////

void TResyncRangeActor::HandleChecksumUndelivery(
    const TEvNonreplPartitionPrivate::TEvChecksumBlocksRequest::TPtr& ev,
    const TActorContext& ctx)
{
    ChecksumDuration = ctx.Now() - ChecksumStartTs;

    Y_UNUSED(ev);

    Error = MakeError(E_REJECTED, "ChecksumBlocks request undelivered");

    Done(ctx);
}

void TResyncRangeActor::HandleChecksumResponse(
    const TEvNonreplPartitionPrivate::TEvChecksumBlocksResponse::TPtr& ev,
    const TActorContext& ctx)
{
    ChecksumDuration = ctx.Now() - ChecksumStartTs;

    auto* msg = ev->Get();

    BLOCKSTORE_TRACE_RECEIVED(ctx, &RequestInfo->TraceId, this, msg, &ev->TraceId);

    Error = msg->Record.GetError();

    if (HasError(Error)) {
        Done(ctx);
        return;
    }

    Checksums.insert({ev->Cookie, msg->Record.GetChecksum()});
    if (Checksums.size() == Replicas.size()) {
        CompareChecksums(ctx);
    }
}

void TResyncRangeActor::HandleReadUndelivery(
    const TEvService::TEvReadBlocksLocalRequest::TPtr& ev,
    const TActorContext& ctx)
{
    ReadDuration = ctx.Now() - ReadStartTs;

    Y_UNUSED(ev);

    Error = MakeError(E_REJECTED, "ReadBlocks request undelivered");

    Done(ctx);
}

void TResyncRangeActor::HandleReadResponse(
    const TEvService::TEvReadBlocksLocalResponse::TPtr& ev,
    const TActorContext& ctx)
{
    ReadDuration = ctx.Now() - ReadStartTs;

    auto* msg = ev->Get();

    BLOCKSTORE_TRACE_RECEIVED(ctx, &RequestInfo->TraceId, this, msg, &ev->TraceId);

    Error = msg->Record.GetError();

    if (HasError(Error)) {
        Done(ctx);
        return;
    }

    WriteBlocks(ctx);
}

void TResyncRangeActor::HandleWriteUndelivery(
    const TEvService::TEvWriteBlocksLocalRequest::TPtr& ev,
    const TActorContext& ctx)
{
    WriteDuration = ctx.Now() - WriteStartTs;

    Y_UNUSED(ev);

    Error = MakeError(E_REJECTED, "WriteBlocks request undelivered");

    Done(ctx);
}

void TResyncRangeActor::HandleWriteResponse(
    const TEvService::TEvWriteBlocksLocalResponse::TPtr& ev,
    const TActorContext& ctx)
{
    WriteDuration = ctx.Now() - WriteStartTs;

    auto* msg = ev->Get();

    BLOCKSTORE_TRACE_RECEIVED(ctx, &RequestInfo->TraceId, this, msg, &ev->TraceId);

    Error = msg->Record.GetError();

    if (HasError(Error)) {
        Done(ctx);
        return;
    }

    if (++ResyncedCount == ActorsToResync.size()) {
        Done(ctx);
    }
}

void TResyncRangeActor::HandlePoisonPill(
    const TEvents::TEvPoisonPill::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);

    Error = MakeError(E_REJECTED, "Dead");
    Done(ctx);
}

STFUNC(TResyncRangeActor::StateWork)
{
    TRequestScope timer(*RequestInfo);

    switch (ev->GetTypeRewrite()) {
        HFunc(TEvents::TEvPoisonPill, HandlePoisonPill);

        HFunc(TEvNonreplPartitionPrivate::TEvChecksumBlocksRequest, HandleChecksumUndelivery);
        HFunc(TEvNonreplPartitionPrivate::TEvChecksumBlocksResponse, HandleChecksumResponse);
        HFunc(TEvService::TEvReadBlocksLocalRequest, HandleReadUndelivery);
        HFunc(TEvService::TEvReadBlocksLocalResponse, HandleReadResponse);
        HFunc(TEvService::TEvWriteBlocksLocalRequest, HandleWriteUndelivery);
        HFunc(TEvService::TEvWriteBlocksLocalResponse, HandleWriteResponse);

        default:
            HandleUnexpectedEvent(
                ev,
                TBlockStoreComponents::PARTITION_WORKER);
            break;
    }
}

}   // namespace NCloud::NBlockStore::NStorage
