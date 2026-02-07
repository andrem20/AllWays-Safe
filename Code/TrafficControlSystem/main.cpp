#include <iostream>

#include "TrafficControlSystem.hpp"

int main()
{
    try
    {
        TrafficControlSystem& tcs = TrafficControlSystem::getInstance();
        tcs.start();
        tcs.waitStop();
    }catch (const std::runtime_error& e) {
        std::cerr << "Runtime error: " << e.what() << '\n';
    }
    std::cout<< "exit main" << std::endl;
    return 0;
}