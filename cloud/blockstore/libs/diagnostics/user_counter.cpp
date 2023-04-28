#include "user_counter.h"

#include <cloud/blockstore/libs/service/request.h>

#include <cloud/storage/core/libs/diagnostics/histogram_types.h>

#include <array>

namespace NCloud::NBlockStore::NUserCounter {

using namespace NMonitoring;

namespace {

////////////////////////////////////////////////////////////////////////////////

constexpr TStringBuf DISK_READ_OPS                    = "disk.read_ops";
constexpr TStringBuf DISK_READ_OPS_BURST              = "disk.read_ops_burst";
constexpr TStringBuf DISK_READ_OPS_IN_FLIGHT          = "disk.read_ops_in_flight";
constexpr TStringBuf DISK_READ_OPS_IN_FLIGHT_BURST    = "disk.read_ops_in_flight_burst";
constexpr TStringBuf DISK_READ_BYTES                  = "disk.read_bytes";
constexpr TStringBuf DISK_READ_BYTES_BURST            = "disk.read_bytes_burst";
constexpr TStringBuf DISK_READ_BYTES_IN_FLIGHT        = "disk.read_bytes_in_flight";
constexpr TStringBuf DISK_READ_BYTES_IN_FLIGHT_BURST  = "disk.read_bytes_in_flight_burst";
constexpr TStringBuf DISK_READ_ERRORS                 = "disk.read_errors";
constexpr TStringBuf DISK_READ_LATENCY                = "disk.read_latency";
constexpr TStringBuf DISK_READ_THROTTLER_DELAY        = "disk.read_throttler_delay";
constexpr TStringBuf DISK_WRITE_OPS                   = "disk.write_ops";
constexpr TStringBuf DISK_WRITE_OPS_BURST             = "disk.write_ops_burst";
constexpr TStringBuf DISK_WRITE_OPS_IN_FLIGHT         = "disk.write_ops_in_flight";
constexpr TStringBuf DISK_WRITE_OPS_IN_FLIGHT_BURST   = "disk.write_ops_in_flight_burst";
constexpr TStringBuf DISK_WRITE_BYTES                 = "disk.write_bytes";
constexpr TStringBuf DISK_WRITE_BYTES_BURST           = "disk.write_bytes_burst";
constexpr TStringBuf DISK_WRITE_BYTES_IN_FLIGHT       = "disk.write_bytes_in_flight";
constexpr TStringBuf DISK_WRITE_BYTES_IN_FLIGHT_BURST = "disk.write_bytes_in_flight_burst";
constexpr TStringBuf DISK_WRITE_ERRORS                = "disk.write_errors";
constexpr TStringBuf DISK_WRITE_LATENCY               = "disk.write_latency";
constexpr TStringBuf DISK_WRITE_THROTTLER_DELAY       = "disk.write_throttler_delay";
constexpr TStringBuf DISK_IO_QUOTA                    = "disk.io_quota_utilization_percentage";
constexpr TStringBuf DISK_IO_QUOTA_BURST              = "disk.io_quota_utilization_percentage_burst";

struct TUserSumCounterWrapper
    : public IUserCounter
{
    TVector<TIntrusivePtr<NMonitoring::TCounterForPtr>> Counters;
    NMonitoring::EMetricType Type = NMonitoring::EMetricType::UNKNOWN;

    void GetType(NMonitoring::IMetricConsumer* consumer) const override
    {
        consumer->OnMetricBegin(Type);
    }

    void GetValue(
        TInstant time,
        NMonitoring::IMetricConsumer* consumer) const override
    {
        int64_t sum = 0;

        for (const auto& counter: Counters) {
            sum += counter->Val();
        }

        consumer->OnInt64(time, sum);
    }
};

////////////////////////////////////////////////////////////////////////////////

struct TBucketDescr
{
    TBucketBound Bound;
    TString Name;
};

static constexpr size_t BUCKETS_COUNT = 25;

using TBuckets = std::array<TBucketDescr, BUCKETS_COUNT>;

template <typename THistogramType>
TBuckets MakeBuckets(auto convertBound)
{
    static_assert(BUCKETS_COUNT == THistogramType::BUCKETS_COUNT, "");

    TBuckets result;
    const auto names = THistogramType::MakeNames();
    for (size_t i = 0; i < names.size(); ++i) {
        result[i].Bound = convertBound(THistogramType::Buckets[i]);
        result[i].Name = names[i];
    }
    return result;
}

const TBuckets MS_BUCKETS = MakeBuckets<TRequestMsTimeBuckets>(
    [](double data) {return data;});
const TBuckets US_BUCKETS = MakeBuckets<TRequestUsTimeBuckets>(
    [](double data) {return data == std::numeric_limits<double>::max()
        ? data : data / 1000.;});

////////////////////////////////////////////////////////////////////////////////

struct TUserSumHistogramWrapper
    : public IUserCounter
{
    static constexpr size_t IGNORE_BUCKETS_COUNT = 10;

    TVector<TIntrusivePtr<TDynamicCounters>> Counters;
    TIntrusivePtr<TExplicitHistogramSnapshot> Histogram;
    const TBuckets& Buckets;
    EMetricType Type = EMetricType::UNKNOWN;

    TUserSumHistogramWrapper(const TBuckets& buckets)
        : Histogram(TExplicitHistogramSnapshot::New(
            buckets.size() - IGNORE_BUCKETS_COUNT))
        , Buckets(buckets)
    {
        for (size_t i = IGNORE_BUCKETS_COUNT; i < Buckets.size(); ++i) {
            (*Histogram)[i - IGNORE_BUCKETS_COUNT].first = Buckets[i].Bound;
        }
    }

    void Clear() const
    {
        for (size_t i = IGNORE_BUCKETS_COUNT; i < Buckets.size(); ++i) {
            (*Histogram)[i - IGNORE_BUCKETS_COUNT].second = 0;
        }
    }

    void GetType(NMonitoring::IMetricConsumer* consumer) const override
    {
        consumer->OnMetricBegin(Type);
    }

    void GetValue(
        TInstant time,
        NMonitoring::IMetricConsumer* consumer) const override
    {
        Clear();

        for (auto& histogram: Counters) {
            for (ui32 i = 0; i < IGNORE_BUCKETS_COUNT; ++i) {
                if (auto countSub = histogram->GetCounter(Buckets[i].Name)) {
                    (*Histogram)[0].second += countSub->Val();
                }
            }

            for (ui32 i = IGNORE_BUCKETS_COUNT; i < Buckets.size(); ++i) {
                if (auto countSub = histogram->GetCounter(Buckets[i].Name)) {
                    (*Histogram)[i - IGNORE_BUCKETS_COUNT].second += countSub->Val();
                }
            }
        }

        consumer->OnHistogram(time, Histogram);
    }
};

////////////////////////////////////////////////////////////////////////////////

using TBaseDynamicCounters = std::pair<TDynamicCounterPtr, TString>;

void AddUserMetric(
    TUserCounterSupplier& dsc,
    const TLabels& commonLabels,
    const TVector<TBaseDynamicCounters>& baseCounters,
    TStringBuf newName
)
{
    std::shared_ptr<TUserSumCounterWrapper> wrapper =
        std::make_shared<TUserSumCounterWrapper>();

    for (auto& counter: baseCounters) {
        if (counter.first) {
            if (auto countSub =
                counter.first->FindCounter(counter.second))
            {
                wrapper->Counters.push_back(countSub);
                wrapper->Type = countSub->ForDerivative()
                    ? EMetricType::RATE
                    : EMetricType::GAUGE;
            }
        }
    }

    if (wrapper->Type != NMonitoring::EMetricType::UNKNOWN) {
        dsc.AddUserMetric(
            commonLabels,
            newName,
            TUserCounter(wrapper));
    }
}

auto AddHistogramUserMetric(
    const TBuckets& buckets,
    TUserCounterSupplier& dsc,
    const TLabels& commonLabels,
    const TVector<TBaseDynamicCounters>& baseCounters,
    TStringBuf newName)
{
    auto wrapper = std::make_shared<TUserSumHistogramWrapper>(buckets);

    wrapper->Type = EMetricType::HIST_RATE;

    for (auto& counter: baseCounters) {
        if (counter.first) {
            auto histogram =
                counter.first->FindSubgroup("histogram", counter.second);
            if (histogram) {
                wrapper->Counters.push_back(histogram);
            }
        }
    }

    dsc.AddUserMetric(
        commonLabels,
        newName,
        TUserCounter(wrapper));
}

TLabels MakeVolumeLabels(
    const TString& cloudId,
    const TString& folderId,
    const TString& diskId)
{
    return {
        {"service", "compute"},
        {"project", cloudId},
        {"cluster", folderId},
        {"disk", diskId}};
}

TLabels MakeVolumeInstanceLabels(
    const TString& cloudId,
    const TString& folderId,
    const TString& diskId,
    const TString& instanceId)
{
    auto volumeLabels = MakeVolumeLabels(
        cloudId,
        folderId,
        diskId);
    volumeLabels.Add("instance", instanceId);

    return volumeLabels;
}

} // namespace

////////////////////////////////////////////////////////////////////////////////

TUserCounter::TUserCounter(std::shared_ptr<IUserCounter> counter)
    : Counter(std::move(counter))
{}

void TUserCounter::Accept(
    const TLabels& baseLabels,
    TInstant time,
    IMetricConsumer* consumer) const
{
    if (!Counter || !consumer) {
        return;
    }

    Counter->GetType(consumer);

    consumer->OnLabelsBegin();

    for (const auto& label: baseLabels) {
        consumer->OnLabel(label.Name(), label.Value());
    }

    consumer->OnLabelsEnd();

    Counter->GetValue(time, consumer);

    consumer->OnMetricEnd();
}

///////////////////////////////////////////////////////////////////////////////

void TUserCounterSupplier::Accept(
    TInstant time,
    IMetricConsumer* consumer) const
{
    if (!consumer) {
        return;
    }

    consumer->OnStreamBegin();
    {
        TReadGuard g{Lock};
        for (const auto& it: Metrics) {
            it.second.Accept(it.first, time, consumer);
        }
    }
    consumer->OnStreamEnd();
}

void TUserCounterSupplier::Append(
    TInstant time,
    IMetricConsumer* consumer) const
{
    TReadGuard g{Lock};
    for (const auto& it: Metrics) {
        it.second.Accept(it.first, time, consumer);
    }
}

void TUserCounterSupplier::AddUserMetric(
    TLabels labels,
    TStringBuf name,
    TUserCounter metric)
{
    labels.Add("name", name);

    TWriteGuard g{Lock};
    Metrics.emplace(std::move(labels), std::move(metric));
}

void TUserCounterSupplier::RemoveUserMetric(
    TLabels labels,
    TStringBuf name)
{
    labels.Add("name", name);

    TWriteGuard g{Lock};
    Metrics.erase(labels);
}

////////////////////////////////////////////////////////////////////////////////

void RegisterServiceVolume(
    TUserCounterSupplier& dsc,
    const TString& cloudId,
    const TString& folderId,
    const TString& diskId,
    TDynamicCounterPtr src)
{
    const auto commonLabels =
        MakeVolumeLabels(cloudId, folderId, diskId);

    AddUserMetric(
        dsc,
        commonLabels,
        { { src, "UsedQuota" } },
        DISK_IO_QUOTA);
    AddUserMetric(
        dsc,
        commonLabels,
        { { src, "MaxUsedQuota" } },
        DISK_IO_QUOTA_BURST);

    auto readSub = src->FindSubgroup("request", "ReadBlocks");
    AddHistogramUserMetric(
        US_BUCKETS,
        dsc,
        commonLabels,
        { { readSub, "ThrottlerDelay" } },
        DISK_READ_THROTTLER_DELAY);

    auto writeSub = src->FindSubgroup("request", "WriteBlocks");
    auto zeroSub = src->FindSubgroup("request", "ZeroBlocks");
    AddHistogramUserMetric(
        US_BUCKETS,
        dsc,
        commonLabels,
        { { writeSub, "ThrottlerDelay" }, { zeroSub, "ThrottlerDelay" } },
        DISK_WRITE_THROTTLER_DELAY);
}

void UnregisterServiceVolume(
    TUserCounterSupplier& dsc,
    const TString& cloudId,
    const TString& folderId,
    const TString& diskId)
{
    const auto commonLabels =
        MakeVolumeLabels(cloudId, folderId, diskId);

    dsc.RemoveUserMetric(commonLabels, DISK_READ_THROTTLER_DELAY);
    dsc.RemoveUserMetric(commonLabels, DISK_WRITE_THROTTLER_DELAY);
    dsc.RemoveUserMetric(commonLabels, DISK_IO_QUOTA);
    dsc.RemoveUserMetric(commonLabels, DISK_IO_QUOTA_BURST);
}

void RegisterServerVolumeInstance(
    TUserCounterSupplier& dsc,
    const TString& cloudId,
    const TString& folderId,
    const TString& diskId,
    const TString& instanceId,
    TDynamicCounterPtr src)
{
    auto commonLabels =
        MakeVolumeInstanceLabels(cloudId, folderId, diskId, instanceId);

    auto readSub = src->FindSubgroup("request", "ReadBlocks");
    AddUserMetric(
        dsc,
        commonLabels,
        { { readSub, "Count" } },
        DISK_READ_OPS);
    AddUserMetric(
        dsc,
        commonLabels,
        { { readSub, "MaxCount" } },
        DISK_READ_OPS_BURST);
    AddUserMetric(
        dsc,
        commonLabels,
        { { readSub, "Errors/Fatal" } },
        DISK_READ_ERRORS);
    AddUserMetric(
        dsc,
        commonLabels,
        { { readSub, "RequestBytes" } },
        DISK_READ_BYTES);
    AddUserMetric(
        dsc,
        commonLabels,
        { { readSub, "MaxRequestBytes" } },
        DISK_READ_BYTES_BURST);
    AddUserMetric(
        dsc,
        commonLabels,
        { { readSub, "InProgress" } },
        DISK_READ_OPS_IN_FLIGHT);
    AddUserMetric(
        dsc,
        commonLabels,
        { { readSub, "MaxInProgress" } },
        DISK_READ_OPS_IN_FLIGHT_BURST);
    AddUserMetric(
        dsc,
        commonLabels,
        { { readSub, "InProgressBytes" } },
        DISK_READ_BYTES_IN_FLIGHT);
    AddUserMetric(
        dsc,
        commonLabels,
        { { readSub, "MaxInProgressBytes" } },
        DISK_READ_BYTES_IN_FLIGHT_BURST);
    AddHistogramUserMetric(
        MS_BUCKETS,
        dsc,
        commonLabels,
        { { readSub, "Time" } },
        DISK_READ_LATENCY);

    auto writeSub = src->FindSubgroup("request", "WriteBlocks");
    auto zeroSub = src->FindSubgroup("request", "ZeroBlocks");

    AddUserMetric(
        dsc,
        commonLabels,
        { { writeSub, "Count" }, { zeroSub, "Count" } },
        DISK_WRITE_OPS);
    AddUserMetric(
        dsc,
        commonLabels,
        { { writeSub, "MaxCount" }, { zeroSub, "MaxCount" } },
        DISK_WRITE_OPS_BURST);
    AddUserMetric(
        dsc,
        commonLabels,
        { { writeSub, "Errors/Fatal" }, { zeroSub, "Errors/Fatal" } },
        DISK_WRITE_ERRORS);
    AddUserMetric(
        dsc,
        commonLabels,
        { { writeSub, "RequestBytes" }, { zeroSub, "RequestBytes" } },
        DISK_WRITE_BYTES);
    AddUserMetric(
        dsc,
        commonLabels,
        { { writeSub, "MaxRequestBytes" }, { zeroSub, "MaxRequestBytes" } },
        DISK_WRITE_BYTES_BURST);
    AddUserMetric(
        dsc,
        commonLabels,
        { { writeSub, "InProgress" }, { zeroSub, "InProgress" } },
        DISK_WRITE_OPS_IN_FLIGHT);
    AddUserMetric(
        dsc,
        commonLabels,
        { { writeSub, "MaxInProgress" }, { zeroSub, "MaxInProgress" } },
        DISK_WRITE_OPS_IN_FLIGHT_BURST);
    AddUserMetric(
        dsc,
        commonLabels,
        { { writeSub, "InProgressBytes" }, { zeroSub, "InProgressBytes" } },
        DISK_WRITE_BYTES_IN_FLIGHT);
    AddUserMetric(
        dsc,
        commonLabels,
        { { writeSub, "MaxInProgressBytes" }, { zeroSub, "MaxInProgressBytes" } },
        DISK_WRITE_BYTES_IN_FLIGHT_BURST);
    AddHistogramUserMetric(
        MS_BUCKETS,
        dsc,
        commonLabels,
        { { writeSub, "Time" }, { zeroSub, "Time" } },
        DISK_WRITE_LATENCY);
}

void UnregisterServerVolumeInstance(
    TUserCounterSupplier& dsc,
    const TString& cloudId,
    const TString& folderId,
    const TString& diskId,
    const TString& instanceId)
{
    const auto commonLabels =
        MakeVolumeInstanceLabels(cloudId, folderId, diskId, instanceId);

    dsc.RemoveUserMetric(commonLabels, DISK_READ_OPS);
    dsc.RemoveUserMetric(commonLabels, DISK_READ_OPS_BURST);
    dsc.RemoveUserMetric(commonLabels, DISK_READ_OPS_IN_FLIGHT);
    dsc.RemoveUserMetric(commonLabels, DISK_READ_OPS_IN_FLIGHT_BURST);
    dsc.RemoveUserMetric(commonLabels, DISK_READ_ERRORS);
    dsc.RemoveUserMetric(commonLabels, DISK_READ_BYTES);
    dsc.RemoveUserMetric(commonLabels, DISK_READ_BYTES_BURST);
    dsc.RemoveUserMetric(commonLabels, DISK_READ_BYTES_IN_FLIGHT);
    dsc.RemoveUserMetric(commonLabels, DISK_READ_BYTES_IN_FLIGHT_BURST);
    dsc.RemoveUserMetric(commonLabels, DISK_READ_LATENCY);

    dsc.RemoveUserMetric(commonLabels, DISK_WRITE_OPS);
    dsc.RemoveUserMetric(commonLabels, DISK_WRITE_OPS_BURST);
    dsc.RemoveUserMetric(commonLabels, DISK_WRITE_OPS_IN_FLIGHT);
    dsc.RemoveUserMetric(commonLabels, DISK_WRITE_OPS_IN_FLIGHT_BURST);
    dsc.RemoveUserMetric(commonLabels, DISK_WRITE_ERRORS);
    dsc.RemoveUserMetric(commonLabels, DISK_WRITE_BYTES);
    dsc.RemoveUserMetric(commonLabels, DISK_WRITE_BYTES_BURST);
    dsc.RemoveUserMetric(commonLabels, DISK_WRITE_BYTES_IN_FLIGHT);
    dsc.RemoveUserMetric(commonLabels, DISK_WRITE_BYTES_IN_FLIGHT_BURST);
    dsc.RemoveUserMetric(commonLabels, DISK_WRITE_LATENCY);
}

} // NCloud::NBlockStore::NUserCounter
