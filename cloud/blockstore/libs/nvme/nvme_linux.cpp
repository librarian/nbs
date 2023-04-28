#include "nvme.h"

#include <cloud/storage/core/libs/common/task_queue.h>
#include <cloud/storage/core/libs/common/thread_pool.h>
#include <util/generic/yexception.h>
#include <util/system/file.h>

#include <linux/nvme_ioctl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include <cerrno>

namespace NCloud::NBlockStore::NNvme {

using namespace NThreading;

namespace {

////////////////////////////////////////////////////////////////////////////////

nvme_ctrlr_data NVMeIdentifyCtrl(TFileHandle& device)
{
    nvme_ctrlr_data ctrl = {};

    nvme_admin_cmd cmd = {
        .opcode = NVME_OPC_IDENTIFY,
        .addr = static_cast<ui64>(reinterpret_cast<uintptr_t>(&ctrl)),
        .data_len = sizeof(ctrl),
        .cdw10 = NVME_IDENTIFY_CTRLR
    };

    int err = ioctl(device, NVME_IOCTL_ADMIN_CMD, &cmd);

    if (err) {
        int err = errno;
        ythrow TServiceError(MAKE_SYSTEM_ERROR(err))
            << "NVMeIdentifyCtrl failed: " << strerror(err);
    }

    return ctrl;
}

nvme_ns_data NVMeIdentifyNs(TFileHandle& device, ui32 nsId)
{
    nvme_ns_data ns = {};

    nvme_admin_cmd cmd = {
        .opcode = NVME_OPC_IDENTIFY,
        .nsid = nsId,
        .addr = static_cast<ui64>(reinterpret_cast<uintptr_t>(&ns)),
        .data_len = sizeof(ns),
        .cdw10 = NVME_IDENTIFY_NS
    };

    int err = ioctl(device, NVME_IOCTL_ADMIN_CMD, &cmd);

    if (err) {
        int err = errno;
        ythrow TServiceError(MAKE_SYSTEM_ERROR(err))
            << "NVMeIdentifyNs failed: " << strerror(err);
    }

    return ns;
}

void NVMeFormatImpl(
    TFileHandle& device,
    ui32 nsId,
    nvme_format format,
    TDuration timeout)
{
    nvme_admin_cmd cmd = {
        .opcode = NVME_OPC_FORMAT_NVM,
        .nsid = nsId,
        .timeout_ms = static_cast<ui32>(timeout.MilliSeconds())
    };

    memcpy(&cmd.cdw10, &format, sizeof(ui32));

    int err = ioctl(device, NVME_IOCTL_ADMIN_CMD, &cmd);

    if (err) {
        int err = errno;
        ythrow TServiceError(MAKE_SYSTEM_ERROR(err))
            << "NVMeFormatImpl failed: " << strerror(err);
    }
}

bool IsBlockOrCharDevice(TFileHandle& device)
{
    struct stat deviceStat = {};

    if (fstat(device, &deviceStat) < 0) {
        int err = errno;
        ythrow TServiceError(MAKE_SYSTEM_ERROR(err))
            << "fstat error: " << strerror(err);
    }

    return S_ISCHR(deviceStat.st_mode) || S_ISBLK(deviceStat.st_mode);
}

class TNvmeManager final
    : public INvmeManager
{
private:
    ITaskQueuePtr Executor;
    TDuration Timeout;  // admin command timeout

    void FormatImpl(
        const TString& path,
        nvme_secure_erase_setting ses)
    {
        TFileHandle device(path, OpenExisting | RdOnly);

        Y_ENSURE(IsBlockOrCharDevice(device), "expected block or character device");

        nvme_ctrlr_data ctrl = NVMeIdentifyCtrl(device);

        Y_ENSURE(ctrl.fna.format_all_ns == 0, "can't format single namespace");
        Y_ENSURE(ctrl.fna.erase_all_ns == 0, "can't erase single namespace");
        Y_ENSURE(
            ses != NVME_FMT_NVM_SES_CRYPTO_ERASE || ctrl.fna.crypto_erase_supported == 1,
            "cryptographic erase is not supported");

        const int nsId = ioctl(device, NVME_IOCTL_ID);

        Y_ENSURE(nsId > 0, "unexpected namespace id");

        nvme_ns_data ns = NVMeIdentifyNs(device, static_cast<ui32>(nsId));

        Y_ENSURE(ns.lbaf[ns.flbas.format].ms == 0, "unexpected metadata");

        nvme_format format {
            .lbaf = ns.flbas.format,
            .ses = ses
        };

        NVMeFormatImpl(device, nsId, format, Timeout);
    }

public:
    TNvmeManager(ITaskQueuePtr executor, TDuration timeout)
        : Executor(executor)
        , Timeout(timeout)
    {}

    TFuture<NProto::TError> Format(
        const TString& path,
        nvme_secure_erase_setting ses) override
    {
        return Executor->Execute([=] {
            try {
                FormatImpl(path, ses);
                return NProto::TError();
            } catch (...) {
                return MakeError(E_FAIL, CurrentExceptionMessage());
            }
        });
    }

    TResultOrError<TString> GetSerialNumber(const TString& path) override
    {
        return SafeExecute<TResultOrError<TString>>([&] {
            TFileHandle device(path, OpenExisting | RdOnly);

            nvme_ctrlr_data ctrl = NVMeIdentifyCtrl(device);

            auto* sn = reinterpret_cast<const char*>(ctrl.sn);
            auto end = std::find(sn, sn + sizeof(ctrl.sn), '\0');

            return TString(sn, end);
        });
    }
};

}   // namespace

////////////////////////////////////////////////////////////////////////////////

INvmeManagerPtr CreateNvmeManager(TDuration timeout)
{
    return std::make_shared<TNvmeManager>(
        CreateLongRunningTaskExecutor("SecureErase"),
        timeout);
}

}   // namespace NCloud::NBlockStore::NNvme
