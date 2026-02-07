#ifndef PEDESTRIANSEMAPHORE_PWM_DEVICEDRIVER_H
#define PEDESTRIANSEMAPHORE_PWM_DEVICEDRIVER_H

#include <cstdint>
#include <stdbool.h>

class PWM_DeviceDriver
{
private:
    int file_descriptor;
    static bool insertKernelModule();
    static bool removeKernelModule();
    int open_dd();
    int close_dd();
public:
    PWM_DeviceDriver();
    ~PWM_DeviceDriver();
    [[nodiscard]]int setValue(int32_t freq, int duty) const;
    void disable() const;
};

#endif //PEDESTRIANSEMAPHORE_PWM_DEVICEDRIVER_H