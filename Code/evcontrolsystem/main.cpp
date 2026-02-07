
#include "evcontrolsystem.hpp"

int main()
{
    try
    {
        evcontrolsystem *evSystem = &evcontrolsystem::getInstance();
        std::string cloud_url = "http://172.20.10.3:3000";
        std::string tmcName = "tmc1";
        evSystem->initializeSystem(cloud_url, tmcName);
        evSystem->runSystem();

        //allows to shut down in terminal and destroy objects and threads
        while (!evcontrolsystem::shutdown_requested_.load()) {}

        evSystem->shutdownSystem();
    }
    catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}