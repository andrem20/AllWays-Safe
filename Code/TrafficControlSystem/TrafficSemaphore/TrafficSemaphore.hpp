#ifndef SEMAPHORE_TRAFFICSEMAPHORE_HPP
#define SEMAPHORE_TRAFFICSEMAPHORE_HPP

#include <string>
#include <unordered_set>
#include <vector>

#include "../Mediator.hpp"
#include "../Semaphore/Semaphore.hpp"

using namespace std;

class TrafficSemaphore : public Semaphore{
private:
    std::unordered_set <int> direction;

public:
    // Constructor: creates a semaphore with ID (location) and direction
    // There cannot be more than one semaphore in the same location
    TrafficSemaphore(Mediator* mediator,int loc, const std::unordered_set<int> &dir,
    int gpio_red=0, int gpio_green=0, int gpio_yellow=0);
    ~TrafficSemaphore() override = default;

    // Interface implementations
    void switch_nextLight(TrafficColour colour,  bool emergency) override;

    // Additional utility
    [[nodiscard]]std::unordered_set<int> getDirection() const;
};

#endif //SEMAPHORE_TRAFFICSEMAPHORE_HPP
