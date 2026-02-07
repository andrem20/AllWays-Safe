
#ifndef EVCONTROLSYSTEM_BATTERY_HPP
#define EVCONTROLSYSTEM_BATTERY_HPP


#include "ADS1115.hpp"
#include "PWM_DeviceDriver/PWM_DeviceDriver.hpp"
#include "CppWrapper_pthreads/CppWrapper.hpp"

#include <atomic>
#include <memory>


class Battery
{
private:
    /*---Attributes---------------------------------------------------------------------------------------------------*/
    std::unique_ptr<ADS1115> adc;
    std::unique_ptr<PWM_DeviceDriver> led;

    int frequency;
    const float minBatteryVoltage; //min input voltage in step down to output 5V
    bool alarmTriggered;

    /*---Helper methods-----------------------------------------------------------------------------------------------*/
    void monitorBattery();
    void monitorLedWarning();
    [[nodiscard]] bool isBatteryOK(float voltage) const;
    void activateWarning(int freq);
    void deactivateWarning() const;
    static void sigalrmHandler(int);
    void stop();

    /*---Threading & Synchronization Resources------------------------------------------------------------------------*/
    std::unique_ptr<CppWrapper::Thread> batteryMonitorThread;
    std::unique_ptr<CppWrapper::Thread> ledWarningThread;

    std::unique_ptr<CppWrapper::Mutex> adcMutex;
    std::unique_ptr<CppWrapper::Mutex> condMutex;

    std::unique_ptr<CppWrapper::CondVar> condReadADC;

    std::unique_ptr<CppWrapper::MQueue> voltageQueue;

    static void* t_ledWarning(void* arg);
    static void* t_batteryMonitor(void* arg);

    std::atomic<bool>& shutdown_requested_;

public:
    /*---Constructor/Destructor---------------------------------------------------------------------------------------*/
    explicit Battery(std::atomic<bool>& shutdown_requested_, float minBat = 6.25, int freq = 2);
    ~Battery();

    /*---Disable Copying (unique ownership)---------------------------------------------------------------------------*/
    Battery(const Battery&) = delete;
    Battery& operator=(const Battery&) = delete;

    /*---System Handling----------------------------------------------------------------------------------------------*/
    void start();
};

#endif //EVCONTROLSYSTEM_BATTERY_HPP