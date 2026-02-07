#ifndef PTHREADS_CPPWRAPPER_HPP
#define PTHREADS_CPPWRAPPER_HPP

#include <cstdint>
#include <pthread.h>
#include <mqueue.h>
#include <string>
#include <stdexcept>
#include <cstring>
#include <type_traits>
#include <ctime>

/*
 *  C++ Wrapper of PThreads/POSIX IPC/POSIX Interval Timers relevant mechanisms for the project
 * developed based on The Linux Programming Interface, by Michael Kerrisk
 *   *  Enables RAII mechanisms on PThreads
 *   *  Copy Constructor and attribution operator are deleted
 *
 *   *  Methods -> return -EPERM if operation can't be performed (when applicable)
 *              -> throw exception (std::runtime_error) if pthreads operation was not well succeeded
 */

#define MAX_MSG 30
#define MSG_SIZE 1024

namespace CppWrapper
{
    class Thread
    {
        static int sched_policy;
        pthread_t thread;
        pthread_attr_t attr;
        bool isDetachable;
        void* userArg;

    public:
        bool isRunning;              // Used internally for Error Checking
        void*(*entry_point)(void *); // function the Thread is going to run

        explicit Thread(void*(*ep)(void *));
        explicit Thread(Thread& thread)=delete;
        Thread& operator=(Thread& thread)=delete;

        ~Thread();

        int setPriority(int priority);
        int setDetachAttribute();

        static void* runFromInside(void* arg);
        int run(void* arg = nullptr);
        void join() const;
        [[nodiscard]] int detach();
    };

    class Mutex
    {
        static int mutex_type;
        pthread_mutex_t mtx;
        pthread_mutexattr_t mtxAttr;

    public:
        Mutex();
        explicit Mutex (Mutex& mtx)=delete;
        Mutex& operator= (Mutex& mtx)=delete;
        ~Mutex();
        int TryLockMutex();
        void LockMutex();
        void UnlockMutex();
        [[nodiscard]] pthread_mutex_t& getType ();
    };

    class CondVar
    {
        pthread_cond_t cv;
        pthread_condattr_t cvAttr;
        Mutex& associated_mtx;

    public:
        explicit CondVar(Mutex& mtx);
        explicit CondVar(CondVar& cv)=delete;
        CondVar& operator= (CondVar& cv)=delete;
        ~CondVar();

        void condSignal();
        void condBroadcast();
        void condWait();
        void condTimedWaitMs(uint32_t timeout_ms);
    };

    class MQueue
    {
    public:
        enum class OpenMode
        {
            ReadOnly,
            WriteOnly,
            ReadWrite
        };

    private:
        mqd_t mqd{-1};
        std::string name;

        [[nodiscard]] static int toFlags(const OpenMode& mode);

    public:

        explicit MQueue(std::string  name, OpenMode opmode);
        MQueue(MQueue& queue) = delete;
        MQueue& operator= (MQueue& queue) = delete;
        ~MQueue();

        template <typename T>
        void send(const T& data) const
        {
            static_assert(std::is_trivially_copyable_v<T>, "MQueue: messages must be trivially copyable"); // C++ Verification

            if (mq_send(mqd,
                        reinterpret_cast<const char*>(&data),
                        sizeof(T),
                        0) == -1)
                throw std::runtime_error("MQueue: mq_send failed");
        }

        template <typename T>
        T receive() const
        {
            //T data{};
            static_assert(std::is_trivially_copyable_v<T>, "MQueue: messages must be trivially copyable");

            if (sizeof(T) > MSG_SIZE)
                throw std::runtime_error("MQueue: mq_receive : Data larger than msg size");

            char buffer[MSG_SIZE];
            const ssize_t ret = mq_receive(mqd,
                                     buffer,
                                     MSG_SIZE,
                                     nullptr);
            if (ret < 0)
                throw std::runtime_error("MQueue: mq_receive failed");

            T data;
            std::memcpy(&data, buffer, sizeof(T));
            return data;
        }

        void unlink() const;  // removes name from system
    };

    class Timer
    {
        timer_t timerid;
        sigevent sev{};
        itimerspec its{};

        Mutex mutexTimer;
        CondVar condTimer;

        int fired;
    public:
        Timer();
        explicit Timer(Timer& timer) = delete;
        Timer& operator=(Timer& timer) = delete;
        ~Timer();

        void timerRun(long sec, long nsec=0);
        void timerWait();
        static void timerCallback(union sigval sv);
    };
}

#endif //PTHREADS_CPPWRAPPER_HPP