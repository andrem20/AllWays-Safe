#include "PedestrianSemaphore.hpp"
#include "../GPIOHandling/rasp_gpio.hpp"

#include <stdexcept>

#define EM_FREQ 4000 // Hz


/*
 * PEDESTRIAN SEMAPHORE
 */

PedestrianSemaphore::PedestrianSemaphore(
    Mediator* mediator, std::atomic<bool>& shutdownRequested, const int loc,
    const PedestrianFeatures features,  const int gpio_red, const int gpio_green, const int gpio_button, const int button_threshold) :
    Semaphore(loc), Component(mediator), _shutdown_requested(shutdownRequested)
{
    buttonEventCounter = hasFeature(features, PedestrianFeatures::Button) ? 0 : -1;
    configureLight(TrafficColour::RED, gpio_red/*, 0*/);
    configureLight(TrafficColour::GREEN, gpio_green/*, 0*/);

    /* HANDLE ATTRIBUTES - create if required */
    if (hasFeature(features, PedestrianFeatures::Button))
    {
        if (!button_threshold)      // if the configuration says it has a button but sets button threshold to 0
            throw runtime_error("Pedestrian features are erroneously configured");

        button =  std::make_unique<Button>(gpio_button, button_threshold, _shutdown_requested,
            // Define Callback Function to allow Notification from internal module Button
            [this]
                {
                    if (this->currentState == TrafficColour::RED)
                    {
                        buttonEventCounter++;
                        PedestrianButtonEvent pedestrianEvent = {
                            .location=this->getLocation()
                        };

                        this->mediator->notify(this, pedestrianEvent);
                    }
                });
    }

    if (hasFeature(features, PedestrianFeatures::Buzzer))
        buzzer = std::make_unique<Buzzer>(); // Default pin, limited to one, HW constraint

    if (hasFeature(features, PedestrianFeatures::CardReader)) // Default pins, limited to one RFID, HW constraint
        cardReader = std::make_unique<MFRC522>(
        // Define Callback Function to allow Notification from internal module MFRC522
            [this](const uint32_t uuid)
                {
                    PedestrianRFIDEvent pedestrianEvent = {
                      .location = this->getLocation(),
                      .uuid = uuid
                    };
                    this->mediator->notify(this, pedestrianEvent);
                },
                _shutdown_requested);
}

void PedestrianSemaphore::start () const
{
    if (button != nullptr)
        button->start();
    if (cardReader != nullptr)
        cardReader->start();
}

void PedestrianSemaphore::stop()
{
    if (button != nullptr)
        button->stop();
    if (cardReader != nullptr)
        cardReader->stop();
}


// Switch to a specific color
void  PedestrianSemaphore::switch_nextLight(const TrafficColour colour, const bool emergency) {
    // Validate that this light exists
    if (!lights.contains(colour) || !lights[colour].gpio_pin)
        throw runtime_error("PSEM: Light colour or pin not configured");

    // Turn off current light
    rasp_gpio_clear(lights[currentState].gpio_pin);

    // Turn on new light
    rasp_gpio_set(lights[colour].gpio_pin);

    currentState = colour;

    // Set Buzzer function based on state
    if (buzzer != nullptr)
    {
        if (colour == TrafficColour::GREEN)
            buzzer->activateBuzzer(2);
        else if (colour == TrafficColour::RED && emergency)
            buzzer->activateBuzzer(EM_FREQ); // EMERGENCY CONFIGURATION
        else
            buzzer->deactivateBuzzer();
    }
}

int PedestrianSemaphore::getButtonEventCounter() const
{
    if (button != nullptr)
        return buttonEventCounter;

    return -1;
}

void PedestrianSemaphore::resetButtonEventCounter()
{
    if (button != nullptr)
    {
        buttonEventCounter = 0;
        button->resetCount();
    }
}