#include "kqp_scan_compute_manager.h"
#include <contrib/ydb/library/wilson_ids/wilson.h>
#include <util/string/builder.h>

namespace NKikimr::NKqp::NScanPrivate {

TShardState::TPtr TInFlightShards::Put(TShardState&& state) {
    TScanShardsStatistics::OnScansDiff(Shards.size(), GetScansCount());
    MutableStatistics(state.TabletId).MutableStatistics(0).SetStartInstant(Now());

    TShardState::TPtr result = std::make_shared<TShardState>(std::move(state));
    AFL_ENSURE(Shards.emplace(result->TabletId, result).second);
    return result;
}

std::vector<std::unique_ptr<TComputeTaskData>> TShardScannerInfo::OnReceiveData(TEvKqpCompute::TEvScanData& data, const std::shared_ptr<TShardScannerInfo>& selfPtr) {
    if (!data.Finished) {
        AFL_ENSURE(!NeedAck);
        NeedAck = true;
    } else {
        Finished = true;
    }
    if (data.IsEmpty()) {
        AFL_ENSURE(data.Finished);
        return {};
    }
    AFL_ENSURE(ActorId);
    AFL_ENSURE(!DataChunksInFlightCount);
    std::vector<std::unique_ptr<TComputeTaskData>> result;
    if (data.SplittedBatches.size() > 1) {
        ui32 idx = 0;
        AFL_ENSURE(data.ArrowBatch);
        for (auto&& i : data.SplittedBatches) {
            result.emplace_back(std::make_unique<TComputeTaskData>(selfPtr, std::make_unique<TEvScanExchange::TEvSendData>(data.ArrowBatch, TabletId, std::move(i)), idx++));
        }
    } else if (data.ArrowBatch) {
        result.emplace_back(std::make_unique<TComputeTaskData>(selfPtr, std::make_unique<TEvScanExchange::TEvSendData>(data.ArrowBatch, TabletId)));
    } else {
        result.emplace_back(std::make_unique<TComputeTaskData>(selfPtr, std::make_unique<TEvScanExchange::TEvSendData>(std::move(data.Rows), TabletId)));
    }
    AFL_DEBUG(NKikimrServices::KQP_COMPUTE)("event", "receive_data")("count_chunks", result.size());
    DataChunksInFlightCount = result.size();
    return result;
}

}
