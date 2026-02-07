#ifndef PEDESTRIANSEMAPHORE_PWM_DEVICEDRIVER_H
#define PEDESTRIANSEMAPHORE_PWM_DEVICEDRIVER_H

#include <cstdint>

class PWM_DeviceDriver
{
private:
    /*---Attributes---------------------------------------------------------------------------------------------------*/
    int file_descriptor;

    /*---Helper Methods-----------------------------------------------------------------------------------------------*/
    static bool insertKernelModule();
    static bool removeKernelModule();
    int open_dd();
    int close_dd();

public:
    /*---Constructor/Destructor---------------------------------------------------------------------------------------*/
    PWM_DeviceDriver();
    ~PWM_DeviceDriver();

    /*---System Handling----------------------------------------------------------------------------------------------*/
    int setValue(int32_t freq, int duty) const;
    void disable() const;
};

#endif //PEDESTRIANSEMAPHORE_PWM_DEVICEDRIVER_H