#include "Buzzer.hpp"
#include <stdexcept>

using namespace std;

#define STANDART_DUTY 50

Buzzer::Buzzer(const int freq): frequency(freq){
    active = false;
    driver = new PWM_DeviceDriver;
}

// void activateBuzzer (int frequency = 2);
void Buzzer::activateBuzzer (const int freq){
    if (active)
        deactivateBuzzer();
    if (freq < 0)
        throw runtime_error  ("Frequency must be > 0");

    frequency = freq;

    // Call DD function
    if (driver->setValue(frequency, STANDART_DUTY) < 0)
        throw runtime_error  ("Error setting frequency");

    active = true;
}

void Buzzer::deactivateBuzzer (){

    // Call DD function
    if (driver->setValue(0, 0) < 0)
        throw runtime_error  ("Error setting frequency");

    active = false;
}

 int Buzzer::getFrequency() const{
    return frequency;
}

 bool Buzzer::getState() const{
    return active;
}
