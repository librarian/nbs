#include "scheduler_test.h"

namespace NCloud {

////////////////////////////////////////////////////////////////////////////////

void TTestScheduler::Schedule(
    ITaskQueue* taskQueue,
    TInstant deadline,
    TCallback callback)
{
    Y_UNUSED(taskQueue);
    Y_UNUSED(deadline);

    with_lock (CallbacksLock) {
        Callbacks.push_back(callback);
    }
}

void TTestScheduler::RunAllScheduledTasks()
{
    TVector<TCallback> callbacks;
    with_lock (CallbacksLock) {
        callbacks = std::move(Callbacks);
    }

    for (auto& callback: callbacks) {
        callback();
    }
}

}   // namespace NCloud
