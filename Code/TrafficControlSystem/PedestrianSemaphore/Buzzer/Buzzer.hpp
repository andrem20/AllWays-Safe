#ifndef TRAFFICCONTROLSYSTEM_PEDESTRIANSEMAPHORE_BUZZER_HPP
#define TRAFFICCONTROLSYSTEM_PEDESTRIANSEMAPHORE_BUZZER_HPP

#include "PWM_DeviceDriver.hpp"

class Buzzer {
private:
    int frequency;  // Hz unit
    bool active;
    PWM_DeviceDriver *driver;
public:
    explicit Buzzer(int freq = 2);
    ~Buzzer()=default;
    void activateBuzzer (int freq = 2);
    void deactivateBuzzer ();
    [[nodiscard]] int getFrequency() const;
    [[nodiscard]] bool getState() const;
};

#endif