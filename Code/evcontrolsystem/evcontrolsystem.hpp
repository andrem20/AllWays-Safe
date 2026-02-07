
#ifndef EVCONTROLSYSTEM_EVCONTROLSYSTEM_HPP
#define EVCONTROLSYSTEM_EVCONTROLSYSTEM_HPP

#include "Battery/Battery.hpp"
#include "Publisher/DDSPublisher.hpp"
#include "CloudInterface/CloudInterface.hpp"

#include <memory>

class evcontrolsystem
{
private:
    /*---Attributes---------------------------------------------------------------------------------------------------*/
    static evcontrolsystem* instance;
    bool initialized_;

    Battery* battery_;
    eprosima::fastdds::examples::emergencyMSG::DDSPublisher* publisher_;
    CloudInterface cloud_;

    /*---Singleton Constructor----------------------------------------------------------------------------------------*/
    evcontrolsystem();

    /*---Helper methods-----------------------------------------------------------------------------------------------*/
    static void signalHandler(int signum);
    [[nodiscard]] bool receive_SSIDs(const std::string& tmcName) const;

public:
    /*---Attributes---------------------------------------------------------------------------------------------------*/
    static std::atomic<bool> shutdown_requested_;

    /*---Singleton----------------------------------------------------------------------------------------*/
    static evcontrolsystem& getInstance();
    ~evcontrolsystem();

    /*---System Handling----------------------------------------------------------------------------------------------*/
    bool initializeSystem(std::string& cloud_url, const std::string& tmcName);
    void runSystem() const;
    void shutdownSystem();
};


#endif //EVCONTROLSYSTEM_EVCONTROLSYSTEM_HPP