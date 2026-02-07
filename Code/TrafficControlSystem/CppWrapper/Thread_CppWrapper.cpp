#include "CppWrapper.hpp"

#include <sched.h>
#include <cerrno>
#include <stdexcept>

using namespace CppWrapper;

int Thread::sched_policy = SCHED_RR; // SCHED_OTHER, SCHED_FIFO

// Init internal variables with default values
// thread is joinable (default)
Thread::Thread(void*(*ep)(void *)) : thread(), attr(), isRunning(false), isDetachable(false), userArg(nullptr)
{
    entry_point = ep; // Set executing function

    int s = pthread_attr_init(&attr);
    if (s != 0)
        throw std::runtime_error("Thread: pthread_attr_init");

    s = pthread_attr_setstacksize(&attr, 20 * 1024 * 1024); // Set to 8MB
    if (s != 0)
            throw std::runtime_error("Thread: pthread_attr_setstacksize");
    s = pthread_attr_setschedpolicy(&attr, sched_policy);
    if (s != 0)
        throw std::runtime_error("Thread: pthread_attr_setschedpolicy");
}

Thread::~Thread ()
{
  /*  if (isRunning && isDetachable)
        pthread_detach(thread);*/

    pthread_attr_destroy(&attr);      /* No longer needed */
}

// Sets priority of the thread. If not used, priority = default SCHED_RR/SCHED_FIFO (min) = 1, SCHED_OTHER = 0
int Thread::setPriority(const int priority)
{
    if (isRunning)
        return -EPERM;

    int min_prio = sched_get_priority_min(sched_policy);
    int max_prio = sched_get_priority_max(sched_policy);

    if (priority < min_prio || priority > max_prio)
        return -EINVAL;

    const struct sched_param sp ={.sched_priority = priority};

    int s = pthread_attr_setschedparam(&attr, &sp);
    if (s != 0)
        throw std::runtime_error("Thread: pthread_attr_setschedparam");

    return 0;
}

int Thread::setDetachAttribute()
{
    if (isRunning)
        return -EPERM;

    int s = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (s != 0)
        throw std::runtime_error("Thread: pthread_attr_setdetachstate");

    isDetachable = true;
    return 0;
}

// Argument is of type Thread
void* Thread::runFromInside(void* arg)
{
    auto caller_thread = static_cast<Thread*>(arg);
    void* ret = caller_thread->entry_point(caller_thread->userArg);
    // Code will reach this point upon Thread function completion
    caller_thread->isRunning = false;

    return ret;
}

// creates the thread (it starts running) sets isRunning.
// &Thread::runFromInside  is passed to ensure variable isRunning is updated correctly
// arg is the executing function received argument
int Thread::run(void* arg)
{
    if (isRunning)
        return -EPERM;

    userArg = arg;

    int s = pthread_create(&thread, &attr, &Thread::runFromInside, this); // this: is the argument to the runFromInside function
    if (s != 0)
        throw std::runtime_error("Thread: pthread_create");

    isRunning = true;

    return 0;
}

void Thread::join() const
{
    int ret = pthread_join(thread, nullptr);
    if (ret != 0)
        throw std::runtime_error("Thread: pthread_join");

}

int Thread::detach()
{
    if (!isDetachable)
        return -EPERM;

    int ret = pthread_detach(thread);
    if (ret != 0)
        throw std::runtime_error("Thread: pthread_detach");

    isDetachable = false;
    return 0;
}