#include "Button.hpp"

#include <chrono>
#include <utility>

#include "../../GPIOHandling/rasp_gpio.hpp"

#define DEBOUNCE_TIME 200 // ms

Button::Button(const int pin, const int threshold, std::atomic<bool>& shutdownRequested,
    ThresholdFunc thresholdCallback) :
    gpio_pin(pin), current_state(false),
    last_state(false), pressCount(0),
    threshold(threshold),
    threadButton(t_Button),
    onThreshold(std::move(thresholdCallback)),
    _shutdown_requested(shutdownRequested)
{
    lastPressedTime = std::chrono::steady_clock::now();
}
Button::~Button() {
    threadButton.join();
    rasp_gpio_release(gpio_pin);
}

void Button::start()
{
    threadButton.run(this);
}

void Button::stop()
{
    threadButton.join();
}

int Button::getPressCount() const{
    return pressCount;
}

void Button::resetCount() {
    pressCount = 0;
}

//needs to be completed
int Button::isThresholdReached() const{
    if (getPressCount() >= threshold) {
        return 1;
    }
    return 0;
}

// this function is used for debounce after GPIO interrupt, triggered by libgpiod, checks first trigger only
int Button::debounce(int, unsigned int, const struct timespec*, void*data)
{
    auto self = static_cast<Button*>(data);

    auto current = std::chrono::steady_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                        current - self->lastPressedTime).count();

    if (duration > DEBOUNCE_TIME)
    {
        self->lastPressedTime = current;
        self->pressCount ++;
        std::cerr<< "count: "<< self->pressCount << std::endl;
        if (self->isThresholdReached())
        {
            self->resetCount();
            self->onThreshold(); // callback to announce Event
        }
    }
    return 0;
}

/*----Thread -----------------------------------------------------------------------------------*/
void* Button::t_Button(void*arg)
{
    const auto self = static_cast<Button*>(arg);

    if (int ret = rasp_gpio_reqInt(self->gpio_pin, self, debounce); ret < 0)
        std::cout << "ret raspgpioButton int req "<< ret << std::endl;

    return arg;
}