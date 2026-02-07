#include "../Semaphore/Semaphore.hpp"
#include "../GPIOHandling/rasp_gpio.hpp"

#include <stdexcept>
#include <iostream>
#include <ostream>
#include <cerrno>

Semaphore::Semaphore(const int loc):location(loc), currentState(TrafficColour::RED){}

Semaphore::~Semaphore()
{
    // Turn off all lights before destruction
    for (auto& [SemaphoreColour, LightConfiguration] : lights) {
        if (LightConfiguration.gpio_pin) // Check if there is an attributed pin (0 is not a pin)
            rasp_gpio_clear(LightConfiguration.gpio_pin);
    }

    std::cout << "Semaphore destroyed" << std::endl;
}

int  Semaphore::setDuration(const TrafficColour colour, const int duration_s){
    if (!lights.contains(colour))
    {
        std::cout << "WARNING: Light colour not configured" << std::endl;
        return -EPERM;
    }

    if (duration_s < 0) // 0 is allowed for pre-configuration of pins
    {
        std::cout << "WARNING: Duration must be positive" << std::endl;
        return -EPERM;
    }

    lights[colour].duration_s = duration_s;

    return 0;
}

// Configure a specific light with its GPIO pin and duration
void Semaphore::configureLight(TrafficColour colour, int pin/*, int duration_s*/) {
    if (!pin)
        throw invalid_argument("GPIO pin cannot be null");
/*
    if (duration_s <= 0)
        throw invalid_argument("Duration must be positive");
*/
    lights[colour] = {pin, 0/*duration_s*/};

    // Initialize the GPIO pin to LOW (off) and sets it up as output
    set_output_mode(pin);
    rasp_gpio_clear(pin);
    cout << "light created" << endl;
}

int  Semaphore::getDuration(const TrafficColour colour) const{
    if (!lights.contains(colour)) {
        std::cout << "WARNING: Light colour not configured" << std::endl;
        return -EPERM;
    }
    return lights.at(colour).duration_s;
}

Semaphore::TrafficColour Semaphore::getCurrentState() const
{
    return currentState;
}

int  Semaphore::getLocation() const{
    return location;
}