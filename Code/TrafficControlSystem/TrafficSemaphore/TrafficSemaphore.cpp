#include "TrafficSemaphore.hpp"

#include <iostream>

#include "../GPIOHandling/rasp_gpio.hpp"

#include <stdexcept>

TrafficSemaphore::TrafficSemaphore( Mediator* mediator, const int loc, const std::unordered_set<int> &dir,
    const int gpio_red, const int gpio_green, const int gpio_yellow)
    : Semaphore(loc), direction(dir){

    configureLight(TrafficColour::RED, gpio_red/*, 0*/);
    configureLight(TrafficColour::GREEN, gpio_green/*, 0*/);
    configureLight(TrafficColour::YELLOW, gpio_yellow/*, 0*/);
}

void TrafficSemaphore::switch_nextLight(TrafficColour colour, bool emergency) {
    // arg  emergency  is not used

    // Validate that this light exists
    if (!lights.contains(colour) || !lights[colour].gpio_pin)
        throw runtime_error("TSEM: Light colour or pin not configured ");

    // Turn off current light
    rasp_gpio_clear(lights[currentState].gpio_pin);

    // Turn on new light
    rasp_gpio_set(lights[colour].gpio_pin);

    currentState = colour;
}

std::unordered_set<int> TrafficSemaphore::getDirection() const {
    return direction;
}