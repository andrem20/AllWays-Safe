#ifndef TRAFFICCONTROLSYSTEM_BUTTON_HPP
#define TRAFFICCONTROLSYSTEM_BUTTON_HPP

#include <atomic>
#include <chrono>
#include <functional>

#include "../../CppWrapper/CppWrapper.hpp"

using namespace std;

class Button {

    int gpio_pin;
    int pressCount;
    int threshold;
    bool current_state;
    bool last_state;
    std::chrono::steady_clock::time_point lastPressedTime;

    [[nodiscard]] int isThresholdReached() const;

    /*---Threading & Synchronization Resources------------------------------------------------------------------------*/
    CppWrapper::Thread threadButton;
    static void*(t_Button)(void*arg);

    CppWrapper::Timer timerButton;

    using ThresholdFunc = std::function<void()>;
    ThresholdFunc onThreshold;

    std::atomic<bool>& _shutdown_requested;

public:
    Button(int pin, int threshold, std::atomic<bool>& shutdownRequested, ThresholdFunc thresholdCallback);
    ~Button();

    void start ();
    void stop ();
    static int debounce(int, unsigned int, const struct timespec*, void*);

    [[nodiscard]] int getPressCount()const;
    void resetCount();


};

#endif //TRAFFICCONTROLSYSTEM_BUTTON_HPP