#include "part_nonrepl_rdma_actor.h"
#include "part_nonrepl_common.h"

#include <cloud/blockstore/libs/common/block_checksum.h>
#include <cloud/blockstore/libs/rdma/protocol.h>
#include <cloud/blockstore/libs/rdma/protobuf.h>
#include <cloud/blockstore/libs/service_local/rdma_protocol.h>
#include <cloud/blockstore/libs/storage/api/disk_agent.h>
#include <cloud/blockstore/libs/storage/core/block_handler.h>
#include <cloud/blockstore/libs/storage/core/config.h>
#include <cloud/blockstore/libs/storage/core/probes.h>

#include <util/generic/map.h>
#include <util/generic/string.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

using namespace NKikimr;

LWTRACE_USING(BLOCKSTORE_STORAGE_PROVIDER);

namespace {

////////////////////////////////////////////////////////////////////////////////

struct TDeviceChecksumRequestContext: TRdmaContext
{
    ui64 RangeStartIndex = 0;
    ui32 RangeSize = 0;
};

////////////////////////////////////////////////////////////////////////////////

using TResponse = TEvNonreplPartitionPrivate::TEvChecksumBlocksResponse;

////////////////////////////////////////////////////////////////////////////////

struct TPartialChecksum
{
    ui64 Value;
    ui64 Size;
};

////////////////////////////////////////////////////////////////////////////////

struct TRdmaRequestContext: NRdma::IClientHandler
{
    TActorSystem* ActorSystem;
    TNonreplicatedPartitionConfigPtr PartConfig;
    TRequestInfoPtr RequestInfo;
    TAtomic Responses;
    TMap<ui64, TPartialChecksum> Checksums;
    NProto::TError Error;
    const ui32 RequestBlockCount;
    NActors::TActorId ParentActorId;
    ui64 RequestId;

    TRdmaRequestContext(
            TActorSystem* actorSystem,
            TNonreplicatedPartitionConfigPtr partConfig,
            TRequestInfoPtr requestInfo,
            TAtomicBase requests,
            ui32 requestBlockCount,
            NActors::TActorId parentActorId,
            ui64 requestId)
        : ActorSystem(actorSystem)
        , PartConfig(std::move(partConfig))
        , RequestInfo(std::move(requestInfo))
        , Responses(requests)
        , RequestBlockCount(requestBlockCount)
        , ParentActorId(parentActorId)
        , RequestId(requestId)
    {}

    void HandleResult(TDeviceChecksumRequestContext& dc, TStringBuf buffer)
    {
        auto* serializer = TBlockStoreProtocol::Serializer();
        auto [result, err] = serializer->Parse(buffer);

        if (HasError(err)) {
            Error = std::move(err);
        }

        const auto& concreteProto =
            static_cast<NProto::TChecksumDeviceBlocksResponse&>(*result.Proto);
        if (HasError(concreteProto.GetError())) {
            Error = concreteProto.GetError();
            return;
        }

        Checksums[dc.RangeStartIndex] = {
            concreteProto.GetChecksum(),
            dc.RangeSize};
    }

    void HandleResponse(
        NRdma::TClientRequestPtr req,
        ui32 status,
        size_t responseBytes) override
    {
        auto* dc = static_cast<TDeviceChecksumRequestContext*>(req->Context);
        auto buffer = req->ResponseBuffer.Head(responseBytes);

        if (status == NRdma::RDMA_PROTO_OK) {
            HandleResult(*dc, buffer);
        } else {
            HandleError(PartConfig, buffer, Error);
        }

        dc->Endpoint->FreeRequest(std::move(req));

        delete dc;

        if (AtomicDecrement(Responses) == 0) {
            ProcessError(*ActorSystem, *PartConfig, Error);

            TBlockChecksum checksum;
            for (const auto& [_, partialChecksum]: Checksums) {
                checksum.Combine(partialChecksum.Value, partialChecksum.Size);
            }

            auto response = std::make_unique<TResponse>(Error);
            response->Record.SetChecksum(checksum.GetValue());
            TAutoPtr<IEventHandle> event(
                new IEventHandle(
                    RequestInfo->Sender,
                    {},
                    response.get(),
                    0,
                    RequestInfo->Cookie));
            response.release();
            ActorSystem->Send(event);

            using TCompletionEvent =
                TEvNonreplPartitionPrivate::TEvChecksumBlocksCompleted;
            auto completion =
                std::make_unique<TCompletionEvent>(std::move(Error));
            auto& counters = *completion->Stats.MutableSysChecksumCounters();
            completion->TotalCycles = RequestInfo->GetTotalCycles();

            counters.SetBlocksCount(RequestBlockCount);
            TAutoPtr<IEventHandle> completionEvent(
                new IEventHandle(
                    ParentActorId,
                    {},
                    completion.get(),
                    0,
                    RequestId));

            completion.release();
            ActorSystem->Send(completionEvent);

            delete this;
        }
    }
};

}   // namespace

////////////////////////////////////////////////////////////////////////////////

void TNonreplicatedPartitionRdmaActor::HandleChecksumBlocks(
    const TEvNonreplPartitionPrivate::TEvChecksumBlocksRequest::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    auto requestInfo = CreateRequestInfo<TEvNonreplPartitionPrivate::TChecksumBlocksMethod>(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    TRequestScope timer(*requestInfo);

    BLOCKSTORE_TRACE_RECEIVED(ctx, &requestInfo->TraceId, this, msg);

    LWTRACK(
        RequestReceived_Partition,
        requestInfo->CallContext->LWOrbit,
        "ChecksumBlocks",
        requestInfo->CallContext->RequestId);

    auto blockRange = TBlockRange64::WithLength(
        msg->Record.GetStartIndex(),
        msg->Record.GetBlocksCount());

    TVector<TDeviceRequest> deviceRequests;
    bool ok = InitRequests<TEvNonreplPartitionPrivate::TChecksumBlocksMethod>(
        ctx,
        *requestInfo,
        blockRange,
        &deviceRequests
    );

    if (!ok) {
        return;
    }

    const auto requestId = RequestsInProgress.GenerateRequestId();

    auto requestContext = std::make_unique<TRdmaRequestContext>(
        ctx.ActorSystem(),
        PartConfig,
        requestInfo,
        deviceRequests.size(),
        msg->Record.GetBlocksCount(),
        SelfId(),
        requestId);

    auto* serializer = TBlockStoreProtocol::Serializer();

    struct TDeviceRequestInfo
    {
        NRdma::IClientEndpointPtr Endpoint;
        NRdma::TClientRequestPtr ClientRequest;
        std::unique_ptr<TDeviceChecksumRequestContext> DeviceRequestContext;
    };

    TVector<TDeviceRequestInfo> requests;

    for (auto& r: deviceRequests) {
        auto& ep = AgentId2Endpoint[r.Device.GetAgentId()];
        Y_VERIFY(ep);
        auto dc = std::make_unique<TDeviceChecksumRequestContext>();
        dc->Endpoint = ep;
        dc->RequestHandler = &*requestContext;
        dc->RangeStartIndex = r.BlockRange.Start;
        dc->RangeSize = r.DeviceBlockRange.Size() * PartConfig->GetBlockSize();

        NProto::TChecksumDeviceBlocksRequest deviceRequest;
        deviceRequest.MutableHeaders()->CopyFrom(msg->Record.GetHeaders());
        deviceRequest.SetDeviceUUID(r.Device.GetDeviceUUID());
        deviceRequest.SetStartIndex(r.DeviceBlockRange.Start);
        deviceRequest.SetBlocksCount(r.DeviceBlockRange.Size());
        deviceRequest.SetBlockSize(PartConfig->GetBlockSize());
        deviceRequest.SetSessionId(msg->Record.GetSessionId());

        auto [req, err] = ep->AllocateRequest(
            &*dc,
            serializer->MessageByteSize(deviceRequest, 0),
            4_KB);

        if (HasError(err)) {
            LOG_ERROR(ctx, TBlockStoreComponents::PARTITION,
                "Failed to allocate rdma memory for ChecksumDeviceBlocksRequest"
                ", error: %s",
                FormatError(err).c_str());

            for (auto& request: requests) {
                request.Endpoint->FreeRequest(std::move(request.ClientRequest));
            }

            NCloud::Reply(
                ctx,
                *requestInfo,
                std::make_unique<TResponse>(std::move(err)));

            return;
        }

        serializer->Serialize(
            req->RequestBuffer,
            TBlockStoreProtocol::ChecksumDeviceBlocksRequest,
            deviceRequest,
            TContIOVector(nullptr, 0));

        requests.push_back({ep, std::move(req), std::move(dc)});
    }

    for (auto& request: requests) {
        request.Endpoint->SendRequest(
            std::move(request.ClientRequest),
            requestInfo->CallContext);
        request.DeviceRequestContext.release();
    }

    RequestsInProgress.AddReadRequest(requestId);
    requestContext.release();
}

}   // namespace NCloud::NBlockStore::NStorage
