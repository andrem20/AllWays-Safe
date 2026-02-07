#include <csignal>

#include "CppWrapper.hpp"

using namespace CppWrapper;

/* Timer has a Mutex and has a condition variable (the latter is related to the former) */

Timer::Timer (): timerid(), condTimer(mutexTimer), fired (0)
{
    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_notify_function = timerCallback;
    sev.sigev_value.sival_ptr = this;
    /*  */
    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = 0;
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 0;

    if (timer_create(CLOCK_MONOTONIC, &sev, &timerid) != 0)
        throw std::runtime_error("Timer: timer_create()");
}

Timer::~Timer()
{
    timer_delete(timerid);
}

void Timer::timerRun(const long sec, const long nsec)
{
    mutexTimer.LockMutex();
    fired = 0;
    mutexTimer.UnlockMutex();

    its.it_value.tv_sec = sec;  its.it_value.tv_nsec = nsec;
    /* period for periodic timer expirations. */
    its.it_interval.tv_sec = 0;  its.it_interval.tv_nsec = 0;

    timer_settime(timerid, 0, &its, nullptr);
}

void Timer::timerWait()
{
    mutexTimer.LockMutex();
    // while (!fired)
    // {
    condTimer.condWait();
    // }
    mutexTimer.UnlockMutex();
}

// Called when timer stops
void Timer::timerCallback(union sigval sv)
{
    const auto self = static_cast<Timer*>(sv.sival_ptr);
    self->mutexTimer.LockMutex();
    self->fired = 1;
    self->condTimer.condBroadcast();

    self->mutexTimer.UnlockMutex();
}