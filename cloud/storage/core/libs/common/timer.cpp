#include "timer.h"

#include <util/datetime/cputimer.h>

namespace NCloud {

namespace {

////////////////////////////////////////////////////////////////////////////////

class TWallClockTimer final
    : public ITimer
{
public:
    TInstant Now() override
    {
        return TInstant::Now();
    }
};

////////////////////////////////////////////////////////////////////////////////

TInstant InitTime = TInstant::Now();
ui64 InitCycleCount = GetCycleCount();

class TCpuCycleTimer final
    : public ITimer
{
    TInstant Now() override
    {
        return InitTime + CyclesToDurationSafe(GetCycleCount() - InitCycleCount);
    }
};

}   // namespace

////////////////////////////////////////////////////////////////////////////////

ITimerPtr CreateWallClockTimer()
{
    return std::make_shared<TWallClockTimer>();
}

ITimerPtr CreateCpuCycleTimer()
{
    return std::make_shared<TCpuCycleTimer>();
}

}   // namespace NCloud
