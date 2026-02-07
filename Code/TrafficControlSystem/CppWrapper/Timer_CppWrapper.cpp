#include <csignal>

#include "CppWrapper.hpp"

#define FIRE_TIMER_IMMEDIATELY 1e-9

using namespace CppWrapper;

/* Timer has a Mutex and has a condition variable (the latter is related to the former) */

Timer::Timer (): timerid(), condTimer(mutexTimer), fired (0)
{
    std::cerr << "Timer Created at " << this << std::endl;
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
    std::cerr << "Timer Deleted at " << this << std::endl;
    timer_delete(timerid);
}

std::pair<long, long> splitNumber(const double value)
{
    long integer = static_cast<long>(value);
    double decimal = value - integer;

    long decimal_scaled = static_cast<long>(decimal * 1e9);
    return {integer, decimal_scaled};
}

/*
 * According to The Linux Programming Interface
        To arm a timer, we make a call to timer_settime() in which either or both of the
    subfields of value.it_value are nonzero. If the timer was previously armed, timer_settime()
    replaces the previous settings.
 *
 */

void Timer::timerRun(const double value)
{
    LockGuard lock (mutexTimer);

    auto [seconds, nanoseconds] = splitNumber(value);

    std::cerr << "Timer RUN at " << this << std::endl;

    fired = 0;

    its.it_value.tv_sec = seconds;  its.it_value.tv_nsec = nanoseconds;
    /* period for periodic timer expirations. */
    its.it_interval.tv_sec = 0;  its.it_interval.tv_nsec = 0;

    timer_settime(timerid, 0, &its, nullptr);
}

void Timer::fireImmediately()
{
    timerRun(FIRE_TIMER_IMMEDIATELY);
}

void Timer::timerWait()
{
    LockGuard lock (mutexTimer);
    while (!fired)
        condTimer.condWait();

}

// Called when timer stops
void Timer::timerCallback(union sigval sv)
{
    const auto self = static_cast<Timer*>(sv.sival_ptr);

    LockGuard lock (self->mutexTimer);
    self->fired = 1;
    self->condTimer.condBroadcast();
}

int Timer::getTime()
{
    timer_gettime(this->timerid, &its);
    return its.it_value.tv_sec;
}
