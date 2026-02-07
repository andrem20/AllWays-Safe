#ifndef PEDESTRIANSEMAPHORE_PEDESTRIANSEMAPHORE_H
#define PEDESTRIANSEMAPHORE_PEDESTRIANSEMAPHORE_H

#include <atomic>
#include <memory>

#include "../Mediator.hpp"
#include "../Semaphore/Semaphore.hpp"
#include "Button/Button.hpp"
#include "RFID/MFRC522.hpp"
#include "Buzzer/Buzzer.hpp"

enum class PedestrianFeatures : std::uint8_t {
    None       = 0,
    Button     = 1 << 0,
    CardReader = 1 << 1,
    Buzzer     = 1 << 2
};

/*
 * PEDESTRIAN FEATURES
 */

/* OPERATOR OVERLOAD */
inline PedestrianFeatures operator | (PedestrianFeatures lhs, PedestrianFeatures rhs)
{
    using T = std::underlying_type_t<PedestrianFeatures>;
    return (static_cast<PedestrianFeatures>(static_cast<T>(lhs) | static_cast<T>(rhs)));
}

inline PedestrianFeatures& operator |= (PedestrianFeatures& lhs, PedestrianFeatures rhs)
{
    lhs = lhs | rhs;
    return lhs;
}

/* COMPARE */
inline bool hasFeature (PedestrianFeatures in, PedestrianFeatures compare)
{
    using T = std::underlying_type_t<PedestrianFeatures>;
    return static_cast<T>(in) & static_cast<T>(compare);
}

class PedestrianSemaphore : public Semaphore, public Component
{
    std::unique_ptr<Button> button;
    std::unique_ptr<MFRC522> cardReader;
    std::unique_ptr<Buzzer> buzzer;

    std::atomic<bool>& _shutdown_requested;

    int buttonEventCounter;

 public:
    PedestrianSemaphore(Mediator* mediator, std::atomic<bool>& shutdownRequested, int loc,
        PedestrianFeatures features, int gpio_red=0, int gpio_green=0, int gpio_button=0, int button_threshold=0);
    ~PedestrianSemaphore() override=default;
/* Interface Implementation */
    void start() const;
    void stop();
    void switch_nextLight (TrafficColour colour,  bool emergency) override;
/* Helper Methods */
    [[nodiscard]]int getButtonEventCounter() const;
    void resetButtonEventCounter();
};

#endif //PEDESTRIANSEMAPHORE_PEDESTRIANSEMAPHORE_H

