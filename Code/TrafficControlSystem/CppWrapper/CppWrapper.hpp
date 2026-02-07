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
#include <iostream>
#include <list>
#include <queue>

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
        Thread(const Thread&)=delete;
        Thread& operator=(const Thread&)=delete;

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
        Mutex (const Mutex&)=delete;
        Mutex& operator= (const Mutex&)=delete;
        ~Mutex();
        int TryLockMutex();
        void LockMutex();
        void UnlockMutex();
        [[nodiscard]] pthread_mutex_t& getType ();
    };

    class LockGuard {
        Mutex& m_mtx;
    public:
        // Lock on construction
        explicit LockGuard(Mutex& m) : m_mtx(m) {
            m_mtx.LockMutex();
        }

        // Unlock on destruction (automatic)
        ~LockGuard() {
            m_mtx.UnlockMutex();
        }

        LockGuard(const LockGuard&) = delete;
        LockGuard& operator=(const LockGuard&) = delete;
    };

    class CondVar
    {
        pthread_cond_t cv;
        pthread_condattr_t cvAttr;
        Mutex& associated_mtx;

    public:
        explicit CondVar(Mutex& mtx);
        CondVar(const CondVar&)=delete;
        CondVar& operator= (const CondVar&)=delete;
        ~CondVar();

        void condSignal();
        void condBroadcast();
        void condWait();
        void condTimedWaitMs(uint32_t timeout_ms);
    };

    // Requires used data to be Trivially Copiable
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

        Mutex mutexMQueue;
        CondVar condMQueue;

    public:

        explicit MQueue(std::string  name, OpenMode opmode=OpenMode::ReadWrite);
        MQueue(const MQueue&) = delete;
        MQueue& operator= (const MQueue&) = delete;
        ~MQueue()noexcept(false);

        template <typename T>
        void send(const T& data)
        {
            mutexMQueue.LockMutex();
            static_assert(std::is_trivially_copyable_v<T>, "MQueue: messages must be trivially copyable"); // C++ Verification

            if (mq_send(mqd,
                        reinterpret_cast<const char*>(&data),
                        sizeof(T),
                        0) == -1)
                throw std::runtime_error("MQueue: mq_send failed");
            mutexMQueue.UnlockMutex();
        }

        template <typename T>
        T receive()
        {
            mutexMQueue.LockMutex();
            condMQueue.condWait();
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

            mutexMQueue.UnlockMutex();
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
        Timer(const Timer&) = delete;
        Timer& operator=(const Timer&) = delete;
        ~Timer();

        void timerRun(double value);
        void timerWait();
        void fireImmediately();
        static void timerCallback(union sigval sv);
        int getTime();
    };

    // Queue allows to have non-trivially copiable data, contrary to MQueue
    template <typename T>
    class Queue
    {
        Mutex mutexQueue;
        CondVar condQueue;

        bool _interrupted;

        std::queue<T> queueData;
    public:
        Queue(): condQueue(mutexQueue), _interrupted(false){};
        Queue(const Queue& queue) = delete;
        Queue& operator= (Queue& queue) = delete;
        ~Queue() {
            std::cerr << "Queue destroyed at " << this << std::endl;
        }

        void send (T&& data)
        {
            std::cerr << "send() on Queue " << this << std::endl;
            CppWrapper::LockGuard lock(mutexQueue);
            try {
                // 2. Use emplace instead of push to construct in-place
                queueData.emplace(std::forward<T>(data));
                condQueue.condBroadcast();
            } catch (const std::exception& e) {
                std::cerr << "Exception during queue push: " << e.what() << std::endl;
                throw;
            }
        }

        T receive ()
        {
            CppWrapper::LockGuard lock(mutexQueue);

            while (queueData.empty()) // Prevent Spurious Wake-Ups
            {
                condQueue.condWait();
            }

            T data = std::move(queueData.front());

            queueData.pop(); // pops front => FIFO

            return data;
        }

        void interrupt ()
        {
            CppWrapper::LockGuard lock(mutexQueue);
            _interrupted = true;
            condQueue.condBroadcast();
        }
    };
}

#endif //PTHREADS_CPPWRAPPER_HPP