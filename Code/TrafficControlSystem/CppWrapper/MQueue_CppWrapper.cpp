#include "CppWrapper.hpp"

#include <stdexcept>
#include <utility>

#define MAX_MSG 30
#define MSG_SIZE 1024
//                  owner r     w       group r      w      others r    w       rw-rw-rw
#define FILE_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)

using namespace CppWrapper;

int MQueue::toFlags(const OpenMode& mode)
{
    switch (mode)
    {
    case OpenMode::ReadOnly: return O_RDONLY;   // Open for reading only
    case OpenMode::WriteOnly: return O_WRONLY;  // Open for writing only
    case OpenMode::ReadWrite: return O_RDWR;    // Open for reading and writing
    }
    return O_RDWR;
}

MQueue::MQueue(std::string  name, OpenMode opmode):name(std::move(name)), condMQueue(mutexMQueue)
{
    int flags = toFlags(opmode) | O_CREAT;  // Create queue if it doesn’t already exist

    // Default parameters
    mq_attr attr{};
    attr.mq_maxmsg = MAX_MSG;
    attr.mq_msgsize = MSG_SIZE;
    mode_t perms = FILE_PERMS;  // 666

    mqd = mq_open(this->name.c_str(), flags, perms, &attr);

    if (mqd == (mqd_t)-1)
        throw std::runtime_error("MQueue: creation failed");
}

MQueue::~MQueue() noexcept(false)
{
    if (mqd != (mqd_t)-1)
        mq_close(mqd);
}

void MQueue::unlink() const
{
    if (mq_unlink(name.c_str()) == -1)
        throw std::runtime_error("MQueue: mq_unlink failed");
}