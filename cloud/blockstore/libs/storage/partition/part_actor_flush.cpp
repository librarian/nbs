#include "part_actor.h"

#include <cloud/blockstore/libs/diagnostics/block_digest.h>
#include <cloud/blockstore/libs/diagnostics/profile_log.h>
#include <cloud/blockstore/libs/service/request_helpers.h>
#include <cloud/blockstore/libs/storage/core/config.h>
#include <cloud/blockstore/libs/storage/core/probes.h>
#include <cloud/blockstore/libs/storage/partition/model/block.h>

#include <cloud/storage/core/libs/common/alloc.h>

#include <library/cpp/actors/core/actor_bootstrapped.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NCloud::NBlockStore::NStorage::NPartition {

using namespace NActors;

using namespace NKikimr;
using namespace NKikimr::NTabletFlatExecutor;

LWTRACE_USING(BLOCKSTORE_STORAGE_PROVIDER);

namespace {

////////////////////////////////////////////////////////////////////////////////

class TFlushActor final
    : public TActorBootstrapped<TFlushActor>
{
public:
    struct TRequest
    {
        TPartialBlobId BlobId;
        TGuardedBuffer<TBlockBuffer> BlobContent;
        TVector<TBlock> Blocks;

        TRequest(const TPartialBlobId& blobId,
                 TBlockBuffer blobContent,
                 TVector<TBlock> blocks)
            : BlobId(blobId)
            , BlobContent(std::move(blobContent))
            , Blocks(std::move(blocks))
        {}
    };

private:
    const TRequestInfoPtr RequestInfo;
    const ui32 BlockSize;
    const IBlockDigestGeneratorPtr BlockDigestGenerator;

    const TActorId Tablet;
    const ui64 CommitId;
    TFlushedCommitIds FlushedCommitIdsFromChannel;
    const ui32 FlushedFreshBlobCount;
    const ui64 FlushedFreshBlobByteCount;

    TVector<TRequest> Requests;
    TVector<TBlockRange64> AffectedRanges;
    TVector<IProfileLog::TBlockInfo> AffectedBlockInfos;
    size_t RequestsCompleted = 0;
    size_t BlocksCount = 0;

    TVector<TCallContextPtr> ForkedCallContexts;

    ui64 MaxExecCyclesFromWrite = 0;

public:
    TFlushActor(
        TRequestInfoPtr requestInfo,
        ui32 blockSize,
        IBlockDigestGeneratorPtr blockDigestGenerator,
        const TActorId& tablet,
        ui64 commitId,
        TFlushedCommitIds flushedCommitIdsFromChannel,
        ui32 flushedFreshBlobCount,
        ui64 flushedFreshBlobByteCount,
        TVector<TRequest> requests);

    void Bootstrap(const TActorContext& ctx);

private:
    void WriteBlobs(const TActorContext& ctx);
    void AddBlobs(const TActorContext& ctx);

    void NotifyCompleted(const TActorContext& ctx, const NProto::TError& error);
    bool HandleError(const TActorContext& ctx, const NProto::TError& error);

    void ReplyAndDie(
        const TActorContext& ctx,
        std::unique_ptr<TEvPartitionPrivate::TEvFlushResponse> response);

private:
    STFUNC(StateWork);

    void HandleWriteBlobResponse(
        const TEvPartitionPrivate::TEvWriteBlobResponse::TPtr& ev,
        const TActorContext& ctx);

    void HandleAddBlobsResponse(
        const TEvPartitionPrivate::TEvAddBlobsResponse::TPtr& ev,
        const TActorContext& ctx);

    void HandlePoisonPill(
        const TEvents::TEvPoisonPill::TPtr& ev,
        const TActorContext& ctx);
};

////////////////////////////////////////////////////////////////////////////////

TFlushActor::TFlushActor(
        TRequestInfoPtr requestInfo,
        ui32 blockSize,
        IBlockDigestGeneratorPtr blockDigestGenerator,
        const TActorId& tablet,
        ui64 commitId,
        TFlushedCommitIds flushedCommitIdsFromChannel,
        ui32 flushedFreshBlobCount,
        ui64 flushedFreshBlobByteCount,
        TVector<TRequest> requests)
    : RequestInfo(std::move(requestInfo))
    , BlockSize(blockSize)
    , BlockDigestGenerator(std::move(blockDigestGenerator))
    , Tablet(tablet)
    , CommitId(commitId)
    , FlushedCommitIdsFromChannel(std::move(flushedCommitIdsFromChannel))
    , FlushedFreshBlobCount(flushedFreshBlobCount)
    , FlushedFreshBlobByteCount(flushedFreshBlobByteCount)
    , Requests(std::move(requests))
{
    ActivityType = TBlockStoreActivities::PARTITION_WORKER;
}

void TFlushActor::Bootstrap(const TActorContext& ctx)
{
    TRequestScope timer(*RequestInfo);

    Become(&TThis::StateWork);

    LWTRACK(
        RequestReceived_PartitionWorker,
        RequestInfo->CallContext->LWOrbit,
        "Flush",
        RequestInfo->CallContext->RequestId);

    TBlockRange64Builder rangeBuilder(AffectedRanges);
    for (const auto& request: Requests) {
        auto blockContent = request.BlobContent.Get().GetBlocks().begin();

        if (request.BlobContent.Get()) {
            // BlobContent is empty only for zero blocks
            Y_VERIFY_DEBUG(request.BlobContent.Get().GetBlocks().size()
                == request.Blocks.size());
        }

        for (const auto& block: request.Blocks) {
            rangeBuilder.OnBlock(block.BlockIndex);

            auto digest = BlockDigestGenerator->ComputeDigest(
                block.BlockIndex,
                request.BlobContent.Get()
                    ? *blockContent
                    : TBlockDataRef::CreateZeroBlock(BlockSize)
            );

            if (digest.Defined()) {
                AffectedBlockInfos.push_back({block.BlockIndex, *digest});
            }

            if (request.BlobContent.Get()) {
                ++blockContent;
            }
        }
    }

    WriteBlobs(ctx);

    if (RequestsCompleted == Requests.size()) {
        AddBlobs(ctx);
    }
}

void TFlushActor::WriteBlobs(const TActorContext& ctx)
{
    for (auto& req: Requests) {
        if (!req.BlobContent.Get()) {
            ++RequestsCompleted;
            continue;
        }

        auto request = std::make_unique<TEvPartitionPrivate::TEvWriteBlobRequest>(
            req.BlobId,
            req.BlobContent.GetGuardedSgList(),
            true);  // async

        auto traceId = RequestInfo->TraceId.Clone();
        BLOCKSTORE_TRACE_SENT(ctx, &traceId, this, request);

        if (!RequestInfo->CallContext->LWOrbit.Fork(request->CallContext->LWOrbit)) {
            LWTRACK(
                ForkFailed,
                RequestInfo->CallContext->LWOrbit,
                "TEvPartitionPrivate::TEvWriteBlobRequest",
                RequestInfo->CallContext->RequestId);
        }

        ForkedCallContexts.emplace_back(request->CallContext);

        NCloud::Send(
            ctx,
            Tablet,
            std::move(request),
            0,  // cookie
            std::move(traceId));
    }
}

void TFlushActor::AddBlobs(const TActorContext& ctx)
{
    TVector<TAddFreshBlob> freshBlobs(Reserve(Requests.size()));

    for (auto& req: Requests) {
        BlocksCount += req.Blocks.size();
        freshBlobs.emplace_back(req.BlobId, std::move(req.Blocks));
    }

    auto request = std::make_unique<TEvPartitionPrivate::TEvAddBlobsRequest>(
        RequestInfo->CallContext,
        CommitId,
        TVector<TAddMixedBlob>(),
        TVector<TAddMergedBlob>(),
        freshBlobs,
        ADD_FLUSH_RESULT
    );

    auto traceId = RequestInfo->TraceId.Clone();
    BLOCKSTORE_TRACE_SENT(ctx, &traceId, this, request);

    NCloud::Send(
        ctx,
        Tablet,
        std::move(request),
        0,  // cookie
        std::move(traceId));
}

void TFlushActor::NotifyCompleted(
    const TActorContext& ctx,
    const NProto::TError& error)
{
    using TEvent = TEvPartitionPrivate::TEvFlushCompleted;
    auto ev = std::make_unique<TEvent>(
        error,
        FlushedFreshBlobCount,
        FlushedFreshBlobByteCount,
        std::move(FlushedCommitIdsFromChannel));

    ev->ExecCycles = RequestInfo->GetExecCycles();
    ev->TotalCycles = RequestInfo->GetTotalCycles();

    ev->CommitId = CommitId;

    {
        auto execTime = CyclesToDurationSafe(RequestInfo->GetExecCycles());
        auto waitTime = CyclesToDurationSafe(RequestInfo->GetWaitCycles());

        auto& counters = *ev->Stats.MutableSysWriteCounters();
        counters.SetRequestsCount(1);
        counters.SetBlocksCount(BlocksCount);
        counters.SetExecTime(execTime.MicroSeconds());
        counters.SetWaitTime(waitTime.MicroSeconds());
    }

    ev->AffectedRanges = std::move(AffectedRanges);
    ev->AffectedBlockInfos = std::move(AffectedBlockInfos);

    NCloud::Send(ctx, Tablet, std::move(ev));
}

bool TFlushActor::HandleError(
    const TActorContext& ctx,
    const NProto::TError& error)
{
    if (FAILED(error.GetCode())) {
        ReplyAndDie(
            ctx,
            std::make_unique<TEvPartitionPrivate::TEvFlushResponse>(
                error
            )
        );
        return true;
    }
    return false;
}

void TFlushActor::ReplyAndDie(
    const TActorContext& ctx,
    std::unique_ptr<TEvPartitionPrivate::TEvFlushResponse> response)
{
    NotifyCompleted(ctx, response->GetError());

    BLOCKSTORE_TRACE_SENT(ctx, &RequestInfo->TraceId, this, response);

    LWTRACK(
        ResponseSent_Partition,
        RequestInfo->CallContext->LWOrbit,
        "Flush",
        RequestInfo->CallContext->RequestId);

    NCloud::Reply(ctx, *RequestInfo, std::move(response));
    Die(ctx);
}

////////////////////////////////////////////////////////////////////////////////

void TFlushActor::HandleWriteBlobResponse(
    const TEvPartitionPrivate::TEvWriteBlobResponse::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    MaxExecCyclesFromWrite = Max(MaxExecCyclesFromWrite, msg->ExecCycles);

    BLOCKSTORE_TRACE_RECEIVED(ctx, &RequestInfo->TraceId, this, msg, &ev->TraceId);

    if (HandleError(ctx, msg->GetError())) {
        return;
    }

    Y_VERIFY(RequestsCompleted < Requests.size());
    if (++RequestsCompleted < Requests.size()) {
        return;
    }

    RequestInfo->AddExecCycles(MaxExecCyclesFromWrite);

    for (auto context: ForkedCallContexts) {
        RequestInfo->CallContext->LWOrbit.Join(context->LWOrbit);
    }

    AddBlobs(ctx);
}

void TFlushActor::HandlePoisonPill(
    const TEvents::TEvPoisonPill::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);

    auto respose = std::make_unique<TEvPartitionPrivate::TEvFlushResponse>(
        MakeError(E_REJECTED, "Tablet is dead"));

    ReplyAndDie(ctx, std::move(respose));
}

void TFlushActor::HandleAddBlobsResponse(
    const TEvPartitionPrivate::TEvAddBlobsResponse::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    RequestInfo->AddExecCycles(msg->ExecCycles);

    BLOCKSTORE_TRACE_RECEIVED(ctx, &RequestInfo->TraceId, this, msg, &ev->TraceId);

    if (HandleError(ctx, msg->GetError())) {
        return;
    }

    ReplyAndDie(
        ctx,
        std::make_unique<TEvPartitionPrivate::TEvFlushResponse>()
    );
}

STFUNC(TFlushActor::StateWork)
{
    TRequestScope timer(*RequestInfo);

    switch (ev->GetTypeRewrite()) {
        HFunc(TEvents::TEvPoisonPill, HandlePoisonPill);
        HFunc(TEvPartitionPrivate::TEvWriteBlobResponse, HandleWriteBlobResponse);
        HFunc(TEvPartitionPrivate::TEvAddBlobsResponse, HandleAddBlobsResponse);

        default:
            HandleUnexpectedEvent(ev, TBlockStoreComponents::PARTITION);
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////

struct TBlob
{
    TBlockBuffer BlobContent;
    TVector<TBlock> Blocks;

    TBlob(TBlockBuffer blobContent, TVector<TBlock> blocks)
        : BlobContent(std::move(blobContent))
        , Blocks(std::move(blocks))
    {
    }
};

////////////////////////////////////////////////////////////////////////////////

class TFlushBlocksVisitor final
    : public IFreshBlocksIndexVisitor
{
private:
    TVector<TBlob>& Blobs;
    const ui32 BlockSize;
    const ui32 FlushBlobSizeThreshold;
    const ui32 MaxBlobRangeSize;
    const ui32 MaxBlocksInBlob;

    TBlockBuffer BlobContent { TProfilingAllocator::Instance() };

    TVector<TBlock> Blocks;
    TVector<TBlock> ZeroBlocks;

public:
    TFlushBlocksVisitor(
            TVector<TBlob>& blobs,
            ui32 blockSize,
            ui32 flushBlobSizeThreshold,
            ui32 maxBlobRangeSize,
            ui32 maxBlocksInBlob)
        : Blobs(blobs)
        , BlockSize(blockSize)
        , FlushBlobSizeThreshold(flushBlobSizeThreshold)
        , MaxBlobRangeSize(maxBlobRangeSize)
        , MaxBlocksInBlob(maxBlocksInBlob)
    {}

    bool Visit(const TFreshBlock& block) override
    {
        if (block.Content) {
            // NBS-299: we do not want to mix blocks that are too far from each other
            if (GetBlobRangeSize(Blocks, block.Meta.BlockIndex)
                    > MaxBlobRangeSize / BlockSize)
            {
                Blobs.emplace_back(std::move(BlobContent), std::move(Blocks));
            }

            BlobContent.AddBlock({block.Content.Data(), block.Content.Size()});
            Blocks.emplace_back(block.Meta.BlockIndex, block.Meta.CommitId, block.Meta.IsStoredInDb);

            if (Blocks.size() == MaxBlocksInBlob) {
                Blobs.emplace_back(std::move(BlobContent), std::move(Blocks));
            }
        } else {
            const auto blobRangeSize =
                GetBlobRangeSize(ZeroBlocks, block.Meta.BlockIndex);
            if (blobRangeSize > MaxBlobRangeSize / BlockSize) {
                Blobs.emplace_back(TBlockBuffer(), std::move(ZeroBlocks));
            }

            ZeroBlocks.emplace_back(block.Meta.BlockIndex, block.Meta.CommitId, block.Meta.IsStoredInDb);

            if (ZeroBlocks.size() == MaxBlocksInBlob) {
                Blobs.emplace_back(TBlockBuffer(), std::move(ZeroBlocks));
            }
        }

        return true;
    }

    void Finish()
    {
        const auto dataSize = Blocks.size() * BlockSize;
        if (Blocks && (!Blobs || dataSize >= FlushBlobSizeThreshold)) {
            Blobs.emplace_back(std::move(BlobContent), std::move(Blocks));
        }

        if (ZeroBlocks && (!Blobs || ZeroBlocks.size() >= FlushBlobSizeThreshold)) {
            Blobs.emplace_back(TBlockBuffer(), std::move(ZeroBlocks));
        }
    }

private:
    static ui32 GetBlobRangeSize(const TVector<TBlock>& blocks, ui32 blockIndex)
    {
        if (blocks) {
            ui32 firstBlockIndex = blocks.front().BlockIndex;
            Y_VERIFY(firstBlockIndex <= blockIndex);
            return blockIndex - firstBlockIndex;
        }
        return 0;
    }
};

////////////////////////////////////////////////////////////////////////////////

TFlushedCommitIds BuildFlushedCommitIdsFromChannel(const TVector<TBlob>& blobs)
{
    TFlushedCommitIds result;
    TVector<ui64> commitIds;

    for (const auto& blob: blobs) {
        for (const auto& block: blob.Blocks) {
            if (!block.IsStoredInDb) {
                commitIds.push_back(block.CommitId);
            }
        }
    }

    if (!commitIds) {
        return {};
    }

    Sort(commitIds);

    ui64 cur = commitIds.front();
    ui32 cnt = 0;

    for (const auto commitId: commitIds) {
        if (commitId == cur) {
            ++cnt;
        } else {
            result.emplace_back(cur, cnt);
            cur = commitId;
            cnt = 1;
        }
    }

    result.emplace_back(cur, cnt);

    return result;
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

void TPartitionActor::EnqueueFlushIfNeeded(const TActorContext& ctx)
{
    if (State->GetFlushState().Status != EOperationStatus::Idle) {
        // already enqueued
        return;
    }

    const auto freshBlockByteCount =
        State->GetUnflushedFreshBlocksCount() * State->GetBlockSize();
    const auto freshBlobCount = State->GetUnflushedFreshBlobCount();
    const auto freshBlobByteCount = State->GetUnflushedFreshBlobByteCount();

    const bool shouldFlush = !State->IsLoadStateFinished()
        || freshBlockByteCount >= Config->GetFlushThreshold()
        || freshBlobCount >= Config->GetFreshBlobCountFlushThreshold()
        || freshBlobByteCount >= Config->GetFreshBlobByteCountFlushThreshold();

    if (!shouldFlush) {
        return;
    }

    State->GetFlushState().SetStatus(EOperationStatus::Enqueued);

    auto request = std::make_unique<TEvPartitionPrivate::TEvFlushRequest>(
        MakeIntrusive<TCallContext>(CreateRequestId()));

    auto traceId = NWilson::TTraceId::NewTraceId();
    BLOCKSTORE_TRACE_SENT(ctx, &traceId, this, request);

    NCloud::Send(
        ctx,
        SelfId(),
        std::move(request),
        0,  // cookie
        std::move(traceId));
}

void TPartitionActor::HandleFlush(
    const TEvPartitionPrivate::TEvFlushRequest::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    auto requestInfo = CreateRequestInfo<TEvPartitionPrivate::TFlushMethod>(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    TRequestScope timer(*requestInfo);

    BLOCKSTORE_TRACE_RECEIVED(ctx, &requestInfo->TraceId, this, msg);

    LWTRACK(
        BackgroundTaskStarted_Partition,
        requestInfo->CallContext->LWOrbit,
        "Flush",
        static_cast<ui32>(PartitionConfig.GetStorageMediaKind()),
        requestInfo->CallContext->RequestId,
        PartitionConfig.GetDiskId());

    if (State->GetFlushState().Status == EOperationStatus::Started) {
        auto response = std::make_unique<TEvPartitionPrivate::TEvFlushResponse>(
            MakeError(E_TRY_AGAIN, "flush already in progress"));

        BLOCKSTORE_TRACE_SENT(ctx, &requestInfo->TraceId, this, response);

        LWTRACK(
            ResponseSent_Partition,
            requestInfo->CallContext->LWOrbit,
            "Flush",
            requestInfo->CallContext->RequestId);

        UpdateCPUUsageStat(CyclesToDurationSafe(requestInfo->GetExecCycles()).MicroSeconds());
        UpdateExecutorStats(ctx);

        NCloud::Reply(ctx, *requestInfo, std::move(response));
        return;
    }

    ui64 blocksCount = State->GetUnflushedFreshBlocksCount();
    if (!blocksCount) {
        State->GetFlushState().SetStatus(EOperationStatus::Idle);

        auto response = std::make_unique<TEvPartitionPrivate::TEvFlushResponse>(
            MakeError(S_ALREADY, "nothing to flush"));

        BLOCKSTORE_TRACE_SENT(ctx, &requestInfo->TraceId, this, response);

        LWTRACK(
            ResponseSent_Partition,
            requestInfo->CallContext->LWOrbit,
            "Flush",
            requestInfo->CallContext->RequestId);

        UpdateCPUUsageStat(CyclesToDurationSafe(requestInfo->GetExecCycles()).MicroSeconds());
        UpdateExecutorStats(ctx);

        NCloud::Reply(ctx, *requestInfo, std::move(response));
        return;
    }

    ui64 commitId = State->GenerateCommitId();
    if (commitId == InvalidCommitId) {
        requestInfo->CancelRequest(ctx);
        RebootPartitionOnCommitIdOverflow(ctx, "Flush");
        return;
    }

    LOG_DEBUG(ctx, TBlockStoreComponents::PARTITION,
        "[%lu] Start flush @%lu (blocks: %lu)",
        TabletID(),
        commitId,
        blocksCount);

    State->GetFlushState().SetStatus(EOperationStatus::Started);

    TVector<TBlob> blobs;
    {
        auto flushBlobSizeThreshold = Config->GetFlushBlobSizeThreshold();
        if (State->GetUnflushedFreshBlobCount() > 0) {
            // ignore flushBlobSizeThreshold when there are any fresh blobs
            // to prevent situation, when some blocks were not flushed
            // but get trimmed in the future
            flushBlobSizeThreshold = 0;
        }

        TFlushBlocksVisitor visitor(
            blobs,
            State->GetBlockSize(),
            flushBlobSizeThreshold,
            Config->GetMaxBlobRangeSize(),
            State->GetMaxBlocksInBlob());

        State->FindFreshBlocks(visitor, TBlockRange32::Max(), commitId);

        visitor.Finish();
    }

    auto flushedCommitIdsFromChannel = BuildFlushedCommitIdsFromChannel(blobs);

    {
        auto& flushedCommitIdsInProgress = State->GetFlushedCommitIdsInProgress();
        Y_VERIFY(flushedCommitIdsInProgress.empty());

        for (const auto& blob: blobs) {
            for (const auto& block: blob.Blocks) {
                flushedCommitIdsInProgress.insert(block.CommitId);
            }
        }
    }

    TVector<TFlushActor::TRequest> requests(Reserve(blobs.size()));

    ui32 blobIndex = 0;
    for (auto& blob: blobs) {
        auto blobId = State->GenerateBlobId(
            EChannelDataKind::Mixed,
            EChannelPermission::UserWritesAllowed,
            commitId,
            blob.BlobContent.GetBytesCount(),
            blobIndex++);

        requests.emplace_back(
            blobId,
            std::move(blob.BlobContent),
            std::move(blob.Blocks));
    }

    Y_VERIFY(requests);

    if (Config->GetFlushToDevNull()) {
        TVector<TBlock> freshBlocks;
        for (const auto& request: requests) {
            for (const auto& block: request.Blocks) {
                freshBlocks.push_back(block);
            }
        }

        ExecuteTx(
            ctx,
            CreateTx<TFlushToDevNull>(
                requestInfo,
                std::move(freshBlocks)
            )
        );
    } else {
        State->GetCommitQueue().AcquireBarrier(commitId);
        State->GetGarbageQueue().AcquireBarrier(commitId);

        auto actor = NCloud::Register<TFlushActor>(
            ctx,
            requestInfo,
            State->GetBlockSize(),
            BlockDigestGenerator,
            SelfId(),
            commitId,
            std::move(flushedCommitIdsFromChannel),
            State->GetUnflushedFreshBlobCount(),
            State->GetUnflushedFreshBlobByteCount(),
            std::move(requests));

        Actors.insert(actor);
    }
}

bool TPartitionActor::PrepareFlushToDevNull(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxPartition::TFlushToDevNull& args)
{
    Y_UNUSED(ctx);
    Y_UNUSED(tx);
    Y_UNUSED(args);

    return true;
}

void TPartitionActor::ExecuteFlushToDevNull(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxPartition::TFlushToDevNull& args)
{
    Y_UNUSED(ctx);

    TPartitionDatabase db(tx.DB);

    for (const auto& block: args.FreshBlocks) {
        if (block.IsStoredInDb) {
            State->DeleteFreshBlock(db, block.BlockIndex, block.CommitId);
        } else {
            State->DeleteFreshBlock(block.BlockIndex, block.CommitId);
        }
    }

    db.WriteMeta(State->GetMeta());
}

void TPartitionActor::CompleteFlushToDevNull(
    const TActorContext& ctx,
    TTxPartition::TFlushToDevNull& args)
{
    Y_UNUSED(ctx);
    Y_UNUSED(args);

    State->GetFlushState().SetStatus(EOperationStatus::Idle);
}

void TPartitionActor::HandleFlushCompleted(
    const TEvPartitionPrivate::TEvFlushCompleted::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    ui64 commitId = msg->CommitId;
    LOG_DEBUG(ctx, TBlockStoreComponents::PARTITION,
        "[%lu] Complete flush @%lu",
        TabletID(),
        commitId);

    UpdateStats(msg->Stats);

    UpdateCPUUsageStat(CyclesToDurationSafe(msg->ExecCycles).MicroSeconds());
    UpdateExecutorStats(ctx);

    State->GetCommitQueue().ReleaseBarrier(commitId);
    State->GetGarbageQueue().ReleaseBarrier(commitId);

    for (const auto& i: msg->FlushedCommitIdsFromChannel) {
        State->GetTrimFreshLogBarriers().ReleaseBarrierN(i.CommitId, i.BlockCount);
    }

    State->DecrementUnflushedFreshBlobCount(msg->FlushedFreshBlobCount);
    State->DecrementUnflushedFreshBlobByteCount(msg->FlushedFreshBlobByteCount);

    State->GetFlushedCommitIdsInProgress().clear();
    State->GetFlushState().SetStatus(EOperationStatus::Idle);

    Actors.erase(ev->Sender);

    const auto d = CyclesToDurationSafe(msg->TotalCycles);
    Y_VERIFY_DEBUG(msg->Stats.GetSysReadCounters().GetBlocksCount() == 0);
    ui32 blocks = msg->Stats.GetSysWriteCounters().GetBlocksCount();
    PartCounters->RequestCounters.Flush.AddRequest(
        d.MicroSeconds(),
        blocks * State->GetBlockSize());

    const auto ts = ctx.Now() - d;

    {
        IProfileLog::TSysReadWriteRequest request;
        request.RequestType = ESysRequestType::Flush;
        request.Duration = d;
        request.Ranges = std::move(msg->AffectedRanges);

        IProfileLog::TRecord record;
        record.DiskId = State->GetConfig().GetDiskId();
        record.Ts = ts;
        record.Request = std::move(request);

        ProfileLog->Write(std::move(record));
    }

    if (msg->AffectedBlockInfos) {
        IProfileLog::TSysReadWriteRequestBlockInfos request;
        request.RequestType = ESysRequestType::Flush;
        request.BlockInfos = std::move(msg->AffectedBlockInfos);
        request.CommitId = commitId;

        IProfileLog::TRecord record;
        record.DiskId = State->GetConfig().GetDiskId();
        record.Ts = ts;
        record.Request = std::move(request);

        ProfileLog->Write(std::move(record));
    }

    EnqueueTrimFreshLogIfNeeded(ctx);
    EnqueueFlushIfNeeded(ctx);
    EnqueueCleanupIfNeeded(ctx);
    ProcessCommitQueue(ctx);
}

}   // namespace NCloud::NBlockStore::NStorage::NPartition
