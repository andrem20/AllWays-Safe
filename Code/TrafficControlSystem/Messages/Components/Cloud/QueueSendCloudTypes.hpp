#ifndef TRAFFICCONTROLSYSTEM_EVENTTYPES_HPP
#define TRAFFICCONTROLSYSTEM_EVENTTYPES_HPP

#include <cstdint>
#include <string>
#include <variant>

#define TABLE_TSEM  "t_semaphore"
#define TABLE_PSEM  "p_semaphore"

/***********************************************************************************************************************
 * Message Queue SEND to Cloud Types
 **********************************************************************************************************************/
namespace tx_cloud
{
    struct Configure
    {
        std::string psem_table = TABLE_PSEM;
        std::string tsem_table = TABLE_TSEM;
    };

    struct EmergencyContext {                               // POST - post em vehicle
        std::string EmVehicleID;    // license_plate
        uint8_t Origin;
        uint8_t Destination;
        uint8_t priority;
    };

    struct ValidateRFID                                        // GET - query
    {
        int location;
        uint32_t uuid;
    };

    struct TrafficSemaphoreUpdate {                         // PATCH - patch
        std::string table = TABLE_TSEM;
        int location;
        int status; // 0(, 1), 2
    };

    struct PedestrianSemaphoreUpdate {                       // PATCH - patch
        std::string table = TABLE_PSEM;
        int location;
        int status; // 0, 2
    };
}

// Send Event, Polymorphic Type
using CloudSendType = std::variant<
    tx_cloud:: Configure,
    tx_cloud::EmergencyContext,
    tx_cloud::ValidateRFID,
    tx_cloud::TrafficSemaphoreUpdate,
    tx_cloud::PedestrianSemaphoreUpdate
>;

#endif //TRAFFICCONTROLSYSTEM_EVENTTYPES_HPP