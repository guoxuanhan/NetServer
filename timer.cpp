#include <sys/time.h>
#include "timer.h"
#include "timermanager.h"

Timer::Timer(int timeout, TimerType timertype, const CallBack &timercallback) :
    timeout_(timeout),
    timertype_(timertype),
    timercallback_(timercallback),
    rotation(0),
    timeslot(0),
    prev(nullptr),
    next(nullptr)
{
    if(timeout < 0)
    {
        return;
    }
    // struct timeval tv;
    // gettimeofday(&tv, NULL);
    // timeout_ = (tv.tv_sec % 10000) * 1000 + tv.tv_usec / 1000 + timeout;
}

Timer::~Timer()
{
    Stop();
}

void Timer::Start()
{
    TimerManager::GetTimerManagerInstance()->AddTimer(this);
}

void Timer::Stop()
{
    TimerManager::GetTimerManagerInstance()->RemoveTimer(this);
}

void Timer::Adjust(int timeout, Timer::TimerType timertype, const CallBack &timercallback)
{
    timeout_ = timeout;
    timertype_ = timertype;
    timercallback_ = timercallback;
    TimerManager::GetTimerManagerInstance()->AdjustTimer(this);
}
