
#include "evcontrolsystem.hpp"

#include <iostream>
#include <csignal>
#include <nlohmann/json.hpp>

evcontrolsystem* evcontrolsystem::instance = nullptr;

std::atomic<bool> evcontrolsystem::shutdown_requested_{false};

/*---Singleton----------------------------------------------------------------------------------------*/

evcontrolsystem& evcontrolsystem::getInstance()
{
    static evcontrolsystem instance;
    return instance;
}

evcontrolsystem::~evcontrolsystem() = default;

/*---System Handling----------------------------------------------------------------------------------------------*/

bool evcontrolsystem::initializeSystem(std::string& cloud_url, const std::string& tmcName)
{
    if (initialized_) return true;

    cloud_ = CloudInterface(cloud_url);
    cloud_.cloudConnect();

    battery_ = new Battery(shutdown_requested_);
    publisher_ = new eprosima::fastdds::examples::emergencyMSG::DDSPublisher(shutdown_requested_, 1, 1, "EmergencyAlert");

    /*if (!receive_SSIDs(tmcName))
        return false;*/

    signal(SIGINT, evcontrolsystem::signalHandler);
    signal(SIGTERM, evcontrolsystem::signalHandler);
    signal(SIGHUP, evcontrolsystem::signalHandler);

    initialized_ = true;

    std::cout << "EVControlSystem initialized successfully." << std::endl;
    return true;
}

void evcontrolsystem::runSystem() const
{
    if (!initialized_) return;
    battery_->start();
    publisher_->start();
}

void evcontrolsystem::shutdownSystem()
{
    delete battery_;
    battery_ = nullptr;

    delete publisher_;
    publisher_ = nullptr;
}

/*---Singleton Constructor----------------------------------------------------------------------------------------*/

evcontrolsystem::evcontrolsystem()
    : initialized_(false), battery_(nullptr), publisher_(nullptr), cloud_("http://172.20.10.3:3000") {}

/*---Helper methods-----------------------------------------------------------------------------------------------*/

void evcontrolsystem::signalHandler(const int signum)
{
    std::cout << "\nSignal " << signum << " received, stopping EVControlSystem..." << std::endl;
    shutdown_requested_.store(true);
}

bool evcontrolsystem::receive_SSIDs(const std::string& tmcName) const
{
    cloud_.clearNetworks();

    auto networks = cloud_.getWifiNetworksByTMC(tmcName);
    if (networks.empty()) return false;

    for (const auto& network : networks)
        cloud_.addNetwork(network);

    cloud_.reinitializeWifiNetworks();

    return true;
}