#ifndef SEMAPHORE_SEMAPHORE_HPP
#define SEMAPHORE_SEMAPHORE_HPP

#include <map>

using namespace std;

class Semaphore {
public:
    enum class TrafficColour {
        RED = 0,
        GREEN = 1,
        YELLOW = 2
    };
    struct LightConfig {
        int gpio_pin;
        int duration_s;
    };

    explicit Semaphore(int loc);
    virtual ~Semaphore();
    // Configure a specific light with its GPIO pin and duration
    void configureLight(TrafficColour colour, int pin/*, int duration_s*/);
    // Switch to a specific color
    virtual void switch_nextLight(TrafficColour colour, bool emergency) = 0;
    // Set duration for a specific color (in seconds)
    int setDuration(TrafficColour colour, int duration_s);
    // Get duration for a specific color
    [[nodiscard]]int getDuration(TrafficColour colour) const;
    // Get current active light
    [[nodiscard]]TrafficColour getCurrentState() const;
    // Get semaphore Location
    [[nodiscard]]int getLocation() const;

protected:
    int location;
    TrafficColour currentState;
    map<TrafficColour, LightConfig> lights; // stores allowed colors and their durations
};

#endif //SEMAPHORE_SEMAPHORE_HPP
