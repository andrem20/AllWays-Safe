#include "CppWrapper.hpp"

#include <stdexcept>
#include <cerrno>

using namespace CppWrapper;

// Good practise that the Condition variable is associated w/ only one mutex
CondVar::CondVar(Mutex& mtx): associated_mtx(mtx), cv(), cvAttr()
{
    int ret = pthread_condattr_init(&cvAttr);
    if (ret != 0)
        throw std::runtime_error("CondVar: pthread_condattr_init");

    ret = pthread_cond_init(&cv, &cvAttr);
    if (ret != 0)
        throw std::runtime_error("CondVar: pthread_cond_init");
}

CondVar::~CondVar()
{
    pthread_cond_destroy(&cv);
}


void CondVar::condSignal()
{
    int ret = pthread_cond_signal(&cv);
    if (ret != 0)
        throw std::runtime_error("CondVar: pthread_cond_signal");
}


void CondVar::condBroadcast()
{
    int ret = pthread_cond_broadcast(&cv);
    if (ret != 0)
        throw std::runtime_error("CondVar: pthread_cond_broadcast");
}

void CondVar::condWait()
{
    // Mutex must already be LOCKED HERE
    int ret = pthread_cond_wait(&cv, &associated_mtx.getType());
    if (ret != 0)
        throw std::runtime_error("CondVar: pthread_cond_wait");
}

void CondVar::condTimedWaitMs(uint32_t timeout_ms) {
    struct timespec ts{};
    clock_gettime(CLOCK_REALTIME, &ts);

    ts.tv_sec += timeout_ms / 1000;
    ts.tv_nsec += (timeout_ms % 1000) * 1000000;

    if (ts.tv_nsec >= 1000000000) {
        ts.tv_sec++;
        ts.tv_nsec -= 1000000000;
    }

    int ret = pthread_cond_timedwait(&cv, &associated_mtx.getType(), &ts);

    if (ret == ETIMEDOUT) {
        // Timeout occurred - this is expected behavior, not an error
        return ;
    }

    if (ret != 0)
        throw std::runtime_error("CondVar: pthread_cond_timedwait");

}
