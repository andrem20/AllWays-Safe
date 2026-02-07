
#include "Battery.hpp"

#include <iostream>
#include <csignal>
#include <sys/stat.h>
#include <sys/time.h>

#define STANDART_DUTY 50
#define time_between_adcREAD 120

static Battery* instance = nullptr;

/*---Constructor/Destructor---------------------------------------------------------------------------------------*/

Battery::Battery(std::atomic<bool>& shutdown_requested_, const float minBat, const int freq)
    : adc(std::make_unique<ADS1115>()),
      led(std::make_unique<PWM_DeviceDriver>()),
      frequency(freq),
      minBatteryVoltage(minBat),
      alarmTriggered(false),
      shutdown_requested_(shutdown_requested_)
{

    adcMutex = std::make_unique<CppWrapper::Mutex>();
    condMutex = std::make_unique<CppWrapper::Mutex>();
    condReadADC = std::make_unique<CppWrapper::CondVar>(*condMutex);

    voltageQueue = std::make_unique<CppWrapper::MQueue>("/battery_mq", CppWrapper::MQueue::OpenMode::ReadWrite);

    deactivateWarning();

    instance = this;
}

Battery::~Battery()
{
    stop();

    signal(SIGALRM, SIG_DFL);
    instance = nullptr;

    voltageQueue->unlink();
}

/*---System Handling----------------------------------------------------------------------------------------------*/

void Battery::start()
{
    signal(SIGALRM, Battery::sigalrmHandler);

    batteryMonitorThread = std::make_unique<CppWrapper::Thread>(t_batteryMonitor);
    batteryMonitorThread->setPriority(60);
    batteryMonitorThread->run(this);

    ledWarningThread = std::make_unique<CppWrapper::Thread>(t_ledWarning);
    ledWarningThread->setPriority(30);
    ledWarningThread->run(this);

    itimerval timer{};
    timer.it_value.tv_sec = time_between_adcREAD;
    timer.it_interval.tv_sec = time_between_adcREAD;
    setitimer(ITIMER_REAL, &timer, nullptr);
}

/*---Helper methods-----------------------------------------------------------------------------------------------*/

void Battery::monitorBattery()
{
    while (!shutdown_requested_.load())
    {
        condMutex->LockMutex();
        while (!alarmTriggered)
            condReadADC->condWait();

        alarmTriggered = false;
        condMutex->UnlockMutex();

        float voltage;
        adcMutex->LockMutex();
        voltage = adc->readBatteryVoltage();
        adcMutex->UnlockMutex();

        std::cout << "[Battery] Current Battery Voltage: " << voltage << " V" << std::endl;

        voltageQueue->send(voltage);
    }
}

void Battery::monitorLedWarning()
{
    while (!shutdown_requested_.load())
    {
        if (auto voltage = voltageQueue->receive<float>(); isBatteryOK(voltage))
            deactivateWarning();
        else
            activateWarning(2);
    }
}

bool Battery::isBatteryOK(const float voltage) const
{
    return voltage > minBatteryVoltage;
}

void Battery::activateWarning( int freq)
{
    frequency = freq;
    int ret = led->setValue(frequency, STANDART_DUTY);
    if (ret < 0)
        throw std::runtime_error("Battery::activateWarning failed: invalid parameters");
}

void Battery::deactivateWarning() const {
    int ret = led->setValue(0, 0);
    if (ret < 0)
        throw std::runtime_error("Battery::deactivateWarning failed: invalid parameters");
}

void Battery::sigalrmHandler(int)
{
    if (!instance) return;

    instance->condMutex->LockMutex();
    instance->alarmTriggered = true;
    instance->condReadADC->condBroadcast();
    instance->condMutex->UnlockMutex();
}

void Battery::stop()
{
    itimerval timer{};
    setitimer(ITIMER_REAL, &timer, nullptr);

    condMutex->LockMutex();
    alarmTriggered = true;
    condReadADC->condBroadcast();
    condMutex->UnlockMutex();

    voltageQueue->send(0.0f);
    batteryMonitorThread->join();
    ledWarningThread->join();
}

/*---Threading & Synchronization Resources------------------------------------------------------------------------*/

void * Battery::t_ledWarning(void *arg)
{
    static_cast<Battery *>(arg)->monitorLedWarning();
    return nullptr;
}

void * Battery::t_batteryMonitor(void *arg)
{
    static_cast<Battery *>(arg)->monitorBattery();
    return nullptr;
}






