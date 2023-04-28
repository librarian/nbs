#include "verbs.h"

#include <cloud/storage/core/libs/common/error.h>

#include <util/network/address.h>
#include <util/stream/format.h>
#include <util/string/builder.h>
#include <util/string/cast.h>
#include <util/system/error.h>

namespace NCloud::NBlockStore::NRdma::NVerbs {

////////////////////////////////////////////////////////////////////////////////

// ibdrv replaces ibv_reg_mr with macro that uses ibv_reg_mr_iova2 if
// IBV_ACCESS_OPTIONAL_RANGE flag is present or (for some reason) if compiler
// can't deduce whether `flags` is constant or not. This will crash on older
// versions where ibv_reg_mr_iova2 is not present, but makes no difference on
// newer ones, since it calls _iova2 variant anyway

#ifdef ibv_reg_mr
#undef ibv_reg_mr
#endif

#define RDMA_THROW_ERROR(method)                                               \
    throw TServiceError(MAKE_SYSTEM_ERROR(LastSystemError()))                  \
        << method << " failed with error " << LastSystemError()                \
        << ": " << LastSystemErrorText()                                       \
// RDMA_THROW_ERROR

////////////////////////////////////////////////////////////////////////////////

struct TVerbs
    : IVerbs
{
    TDeviceListPtr GetDeviceList() override
    {
        auto** list = ibv_get_device_list(nullptr);
        return WrapPtr(list);
    }

    TContextPtr OpenDevice(ibv_device* device) override
    {
        auto* context = ibv_open_device(device);
        if (!context) {
            RDMA_THROW_ERROR("ibv_open_device");
        }

        return WrapPtr(context);
    }

    void QueryDevice(
        ibv_context* context,
        ibv_device_attr_ex* attr) override
    {
        int res = rdma_seterrno(ibv_query_device_ex(context, nullptr, attr));
        if (res < 0) {
            RDMA_THROW_ERROR("ibv_query_device_ex");
        }
    }

    ui64 GetDeviceTimestamp(ibv_context* context) override
    {
        ibv_values_ex values = {
            .comp_mask = IBV_VALUES_MASK_RAW_CLOCK,
        };

        int res = rdma_seterrno(ibv_query_rt_values_ex(context, &values));
        if (res < 0) {
            RDMA_THROW_ERROR("ibv_query_rt_values_ex");
        }

        return (ui64)values.raw_clock.tv_sec * 1000000000
             + (ui64)values.raw_clock.tv_nsec;
    }

    TProtectionDomainPtr CreateProtectionDomain(ibv_context* context) override
    {
        auto* pd = ibv_alloc_pd(context);
        if (!pd) {
            RDMA_THROW_ERROR("ibv_alloc_pd");
        }

        return WrapPtr(pd);
    }

    TMemoryRegionPtr RegisterMemoryRegion(
        ibv_pd* pd,
        void* addr,
        size_t length,
        int flags) override
    {
        auto* mr = ibv_reg_mr(pd, addr, length, flags);
        if (!mr) {
            RDMA_THROW_ERROR("ibv_reg_mr");
        }

        return WrapPtr(mr);
    }

    TCompletionChannelPtr CreateCompletionChannel(ibv_context* context) override
    {
        auto* channel = ibv_create_comp_channel(context);
        if (!channel) {
            RDMA_THROW_ERROR("ibv_create_comp_channel");
        }

        return WrapPtr(channel);
    }

    TCompletionQueuePtr CreateCompletionQueue(
        ibv_context* context,
        ibv_cq_init_attr_ex* attr) override
    {
        auto* cq_ex = ibv_create_cq_ex(context, attr);
        if (!cq_ex) {
            RDMA_THROW_ERROR("ibv_create_cq_ex");
        }

        return WrapPtr(ibv_cq_ex_to_cq(cq_ex));
    }

    void RequestCompletionEvent(ibv_cq* cq, int solicitedOnly) override
    {
        int res = rdma_seterrno(ibv_req_notify_cq(cq, solicitedOnly));
        if (res < 0) {
            RDMA_THROW_ERROR("ibv_req_notify_cq");
        }
    }

    void* GetCompletionEvent(ibv_cq* cq) override
    {
        ibv_cq* ev_cq;
        void* ev_ctx;

        int res = ibv_get_cq_event(cq->channel, &ev_cq, &ev_ctx);
        if (res < 0) {
            RDMA_THROW_ERROR("ibv_get_cq_event");
        }

        Y_VERIFY(ev_cq == cq);
        return ev_ctx;
    }

    void AckCompletionEvents(ibv_cq* cq, unsigned int count) override
    {
        ibv_ack_cq_events(cq, count);
    }

    int PollCompletionQueue(ibv_cq_ex* cq, ICompletionHandler* handler) override
    {
        ibv_poll_cq_attr attr = {};

        int rc = ibv_start_poll(cq, &attr);
        if (rc != 0) {
            return rc;
        }

        TCompletion wc = {};
        do {
            wc.wr_id = cq->wr_id;
            wc.status = cq->status;
            wc.opcode = ibv_wc_read_opcode(cq);
            wc.ts = ibv_wc_read_completion_ts(cq);
            handler->HandleCompletionEvent(wc);

            rc = ibv_next_poll(cq);
        } while (rc == 0);

        ibv_end_poll(cq);

        // return 0 in case we actually handled some requests
        return (rc == ENOENT) ? 0 : rc;
    }

    void PostSend(ibv_qp* qp, ibv_send_wr* wr) override
    {
        ibv_send_wr* bad = nullptr;

        int res = rdma_seterrno(ibv_post_send(qp, wr, &bad));
        if (res < 0) {
            RDMA_THROW_ERROR("ibv_post_send");
        }
    }

    void PostRecv(ibv_qp* qp, ibv_recv_wr* wr) override
    {
        ibv_recv_wr* bad = nullptr;

        int res = rdma_seterrno(ibv_post_recv(qp, wr, &bad));
        if (res < 0) {
            RDMA_THROW_ERROR("ibv_post_recv");
        }
    }

    ////////////////////////////////////////////////////////////////////////////////

    TAddressInfoPtr GetAddressInfo(
        const TString& host,
        ui32 port,
        rdma_addrinfo* hints) override
    {
        rdma_addrinfo* addr = nullptr;

        int res = rdma_getaddrinfo(
            host.c_str(),
            ToString(port).c_str(),
            hints,
            &addr);
        if (res < 0) {
            RDMA_THROW_ERROR("rdma_getaddrinfo");
        }

        return WrapPtr(addr);
    }

    TEventChannelPtr CreateEventChannel() override
    {
        auto* channel = rdma_create_event_channel();
        if (!channel) {
            RDMA_THROW_ERROR("rdma_create_event_channel");
        }

        return WrapPtr(channel);
    }

    TConnectionEventPtr GetConnectionEvent(rdma_event_channel* channel) override
    {
        rdma_cm_event* event = nullptr;

        int res = rdma_get_cm_event(channel, &event);
        if (res < 0) {
            if (errno != EAGAIN && errno != EINTR && errno != EWOULDBLOCK) {
                RDMA_THROW_ERROR("rdma_get_cm_event");
            }
            return NullPtr;
        }

        return WrapPtr(event);
    }

    TConnectionPtr CreateConnection(
        rdma_event_channel* channel,
        void* context,
        rdma_port_space ps) override
    {
        rdma_cm_id* id = nullptr;

        int res = rdma_create_id(channel, &id, context, ps);
        if (res < 0) {
            RDMA_THROW_ERROR("rdma_create_id");
        }

        return WrapPtr(id);
    }

    void BindAddress(rdma_cm_id* id, sockaddr* addr) override
    {
        int res = rdma_bind_addr(id, addr);
        if (res < 0) {
            RDMA_THROW_ERROR("rdma_bind_addr");
        }
    }

    void ResolveAddress(
        rdma_cm_id* id,
        sockaddr* src,
        sockaddr* dst,
        TDuration timeout) override
    {
        int res = rdma_resolve_addr(id, src, dst, timeout.MilliSeconds());
        if (res < 0) {
            RDMA_THROW_ERROR("rdma_resolve_addr");
        }
    }

    void ResolveRoute(rdma_cm_id* id, TDuration timeout) override
    {
        int res = rdma_resolve_route(id, timeout.MilliSeconds());
        if (res < 0) {
            RDMA_THROW_ERROR("rdma_resolve_route");
        }
    }

    void Listen(rdma_cm_id* id, int backlog) override
    {
        int res = rdma_listen(id, backlog);
        if (res < 0) {
            RDMA_THROW_ERROR("rdma_listen");
        }
    }

    void Connect(rdma_cm_id* id, rdma_conn_param* param) override
    {
        int res = rdma_connect(id, param);
        if (res < 0) {
            RDMA_THROW_ERROR("rdma_connect");
        }
    }

    int Accept(rdma_cm_id* id, rdma_conn_param* param) override
    {
        return rdma_accept(id, param) ? errno : 0;
    }

    int Reject(
        rdma_cm_id* id,
        const void* privateData,
        ui8 privateDataLen) override
    {
        return rdma_reject(id, privateData, privateDataLen) ? errno : 0;
    }

    void CreateQP(rdma_cm_id* id, ibv_qp_init_attr* attr) override
    {
        int res = rdma_create_qp(id, id->pd, attr);
        if (res < 0) {
            RDMA_THROW_ERROR("rdma_create_qp");
        }
    }

    void DestroyQP(rdma_cm_id* id) override
    {
        rdma_destroy_qp(id);
    }
};

////////////////////////////////////////////////////////////////////////////////

IVerbsPtr CreateVerbs()
{
    return std::make_shared<TVerbs>();
}

////////////////////////////////////////////////////////////////////////////////

const char* GetOpcodeName(ibv_wc_opcode opcode)
{
    static const char* names[] = {
        "IBV_WC_SEND",
        "IBV_WC_RDMA_WRITE",
        "IBV_WC_RDMA_READ",
        "IBV_WC_COMP_SWAP",
        "IBV_WC_FETCH_ADD",
        "IBV_WC_BIND_MW",
        "IBV_WC_LOCAL_INV",
        "IBV_WC_TSO",
    };

    static const char* names2[] = {
        "IBV_WC_RECV",
        "IBV_WC_RECV_RDMA_WITH_IMM",
        "IBV_WC_TM_ADD",
        "IBV_WC_TM_DEL",
        "IBV_WC_TM_SYNC",
        "IBV_WC_TM_RECV",
        "IBV_WC_TM_NO_TAG",
        "IBV_WC_DRIVER1",
        "IBV_WC_DRIVER2",
        "IBV_WC_DRIVER3",
    };

    if ((size_t)opcode < Y_ARRAY_SIZE(names)) {
        return names[(size_t)opcode];
    }

    if ((size_t)opcode - IBV_WC_RECV < Y_ARRAY_SIZE(names2)) {
        return names2[(size_t)opcode - IBV_WC_RECV];
    }

    return "IBV_WC_UNKNOWN";
}

const char* GetStatusString(ibv_wc_status status)
{
    static const char *const strings[] = {
        "IBV_WC_SUCCESS",
        "IBV_WC_LOC_LEN_ERR",
        "IBV_WC_LOC_QP_OP_ERR",
        "IBV_WC_LOC_EEC_OP_ERR",
        "IBV_WC_LOC_PROT_ERR",
        "IBV_WC_WR_FLUSH_ERR",
        "IBV_WC_MW_BIND_ERR",
        "IBV_WC_BAD_RESP_ERR",
        "IBV_WC_LOC_ACCESS_ERR",
        "IBV_WC_REM_INV_REQ_ERR",
        "IBV_WC_REM_ACCESS_ERR",
        "IBV_WC_REM_OP_ERR",
        "IBV_WC_RETRY_EXC_ERR",
        "IBV_WC_RNR_RETRY_EXC_ERR",
        "IBV_WC_LOC_RDD_VIOL_ERR",
        "IBV_WC_REM_INV_RD_REQ_ERR",
        "IBV_WC_REM_ABORT_ERR",
        "IBV_WC_INV_EECN_ERR",
        "IBV_WC_INV_EEC_STATE_ERR",
        "IBV_WC_FATAL_ERR",
        "IBV_WC_RESP_TIMEOUT_ERR",
        "IBV_WC_GENERAL_ERR",
        "IBV_WC_TM_ERR",
        "IBV_WC_TM_RNDV_INCOMPLETE",
    };

    if ((size_t)status >= Y_ARRAY_SIZE(strings)) {
        return "IBV_WC_UNKNOWN";
    }

    return strings[status];
}

const char* GetEventName(rdma_cm_event_type event)
{
    static const char* names[] = {
        "RDMA_CM_EVENT_ADDR_RESOLVED",
        "RDMA_CM_EVENT_ADDR_ERROR",
        "RDMA_CM_EVENT_ROUTE_RESOLVED",
        "RDMA_CM_EVENT_ROUTE_ERROR",
        "RDMA_CM_EVENT_CONNECT_REQUEST",
        "RDMA_CM_EVENT_CONNECT_RESPONSE",
        "RDMA_CM_EVENT_CONNECT_ERROR",
        "RDMA_CM_EVENT_UNREACHABLE",
        "RDMA_CM_EVENT_REJECTED",
        "RDMA_CM_EVENT_ESTABLISHED",
        "RDMA_CM_EVENT_DISCONNECTED",
        "RDMA_CM_EVENT_DEVICE_REMOVAL",
        "RDMA_CM_EVENT_MULTICAST_JOIN",
        "RDMA_CM_EVENT_MULTICAST_ERROR",
        "RDMA_CM_EVENT_ADDR_CHANGE",
        "RDMA_CM_EVENT_TIMEWAIT_EXIT"
    };

    if ((size_t)event < Y_ARRAY_SIZE(names)) {
        return names[(size_t)event];
    }

    return "RDMA_CM_EVENT_UNKNOWN";
}

TString PrintAddress(const sockaddr* addr)
{
    return NAddr::PrintHostAndPort(NAddr::TOpaqueAddr(addr));
}

TString PrintConnectionParams(const rdma_conn_param* conn)
{
    return TStringBuilder()
        << "[private_data=" << Hex((uintptr_t)conn->private_data)
        << ", private_data_len=" << (uint32_t)conn->private_data_len
        << ", responder_resources=" << (uint32_t)conn->responder_resources
        << ", initiator_depth=" << (uint32_t)conn->initiator_depth
        << ", flow_control=" << (uint32_t)conn->flow_control
        << ", retry_count=" << (uint32_t)conn->retry_count
        << ", rnr_retry_count=" << (uint32_t)conn->rnr_retry_count
        << ", srq=" << (uint32_t)conn->srq
        << ", qp_num=" << (uint32_t)conn->qp_num
        << "]";
}

}   // namespace NCloud::NBlockStore::NRdma::NVerbs
