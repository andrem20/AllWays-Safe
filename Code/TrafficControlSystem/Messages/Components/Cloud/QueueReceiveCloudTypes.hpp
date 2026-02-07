#ifndef TRAFFICCONTROLSYSTEM_QUEUERECEIVECLOUDTYPES_HPP
#define TRAFFICCONTROLSYSTEM_QUEUERECEIVECLOUDTYPES_HPP

#include <variant>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

using json = nlohmann::json;

/***********************************************************************************************************************
 * Message Queue RECEIVE from Cloud Types
 **********************************************************************************************************************/
namespace rx_cloud
{
    struct TSEM_data
    {
        std::shared_ptr<json> file;
    };

    struct PSEM_data
    {
        std::shared_ptr<json> file;
    };

    struct RFID_Validation
    {
        bool isIDvalid = false;
        int location;
    };
}

// Receive from Cloud Messages Type
using CloudReceiveType = std::variant<rx_cloud::TSEM_data, rx_cloud::PSEM_data, rx_cloud::RFID_Validation>;

#endif //TRAFFICCONTROLSYSTEM_QUEUERECEIVECLOUDTYPES_HPP