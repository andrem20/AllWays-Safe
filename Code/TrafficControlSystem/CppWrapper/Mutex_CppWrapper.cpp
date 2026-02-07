#include "CppWrapper.hpp"

#include <stdexcept>

using namespace CppWrapper;

int Mutex::mutex_type = PTHREAD_MUTEX_NORMAL; //PTHREAD_MUTEX_ERRORCHECK;

Mutex::Mutex() : mtx(), mtxAttr()
{
    // Mutex dynamic Initialization

    int s = pthread_mutexattr_init(&mtxAttr);
    if (s != 0)
        throw std::runtime_error("Mutex: pthread_mutexattr_init");

    s = pthread_mutexattr_settype(&mtxAttr, mutex_type);
    if (s != 0)
        throw std::runtime_error("Mutex: pthread_mutexattr_settype");

    s = pthread_mutex_init(&mtx, &mtxAttr);
    if (s != 0)
        throw std::runtime_error("Mutex: pthread_mutex_init");
}

Mutex::~Mutex()
{
    pthread_mutexattr_destroy(&mtxAttr); /* No longer needed */
    pthread_mutex_destroy(&mtx);
}

int Mutex::TryLockMutex()
{
    const int ret = pthread_mutex_trylock(&mtx);
    if (ret == EBUSY)
    {
        // Mutex is already locked
        return EBUSY;
    }
    if (ret != 0)
        throw std::runtime_error("Mutex: pthread_mutex_trylock failed");
    return 0;
}

void Mutex::LockMutex()
{
    const int ret = pthread_mutex_lock(&mtx);
    if (ret != 0)
        throw std::runtime_error("Mutex: pthread_mutex_lock");
}

void Mutex::UnlockMutex()
{
    const int ret = pthread_mutex_unlock(&mtx);
    if (ret != 0)
        throw std::runtime_error("Mutex: pthread_mutex_unlock");
}

pthread_mutex_t& Mutex::getType()
{
    return mtx;
}
