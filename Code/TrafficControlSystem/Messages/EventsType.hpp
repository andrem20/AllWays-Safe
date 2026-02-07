#ifndef TRAFFICCONTROLSYSTEM_EVENTSTYPE_HPP
#define TRAFFICCONTROLSYSTEM_EVENTSTYPE_HPP

#include <variant>

#include "Components/PedestrianEvent.hpp"
#include "Components/DDSEvent.hpp"
#include "Components/Cloud/QueueReceiveCloudTypes.hpp"
#include "InternalEvent.hpp"


using Event = std::variant<PedestrianButtonEvent, PedestrianRFIDEvent,
                           DDSEvent, InternalEvent, CloudReceiveType>;
       // rx_cloud::TSEM_data, rx_cloud::PSEM_data, rx_cloud::RFID_ID_data>;//

#endif //TRAFFICCONTROLSYSTEM_EVENTSTYPE_HPP