#include "TrafficStrategy.hpp"

#define MIN_SECONDS     10
#define REDUCE_SECONDS  5

void StrategyNormal::handleInternalEvent(TrafficControlSystem* tcs, const InternalEvent& receive)
{
    static TrafficControlSystem::SwitchLightsData newConfiguration;
    bool isYellow = false; // Enables setting Intermediary State

    bool shouldQueue = false;

    switch (receive)
    {
  //  case InternalEvent::NEW_STATE_ENTERED:
    case InternalEvent::LIGHTS_TIMEOUT:
        newConfiguration = tcs->organizeNextConfiguration();
        isYellow = true;
        shouldQueue = true;
        break;

    case InternalEvent::YELLOW_TIMEOUT:
        // keep last Configuration, just switch out of yellow
        break;

    default:
        return;
    }

    auto sendData = newConfiguration;

    if (shouldQueue)
        tcs->switchLightQueue.send(std::move(sendData));
}

void StrategyNormal::handleDDSEvent(TrafficControlSystem* tcs, const DDSEvent& receive)
{
    if (receive.qualifier == DDS_Event_Qualifier::EMERGENCY_START)
    {
        const tx_cloud::EmergencyContext emergencyContext(
            receive.license_plate,
            receive.location,
            receive.direction,
            receive.priority);

        tcs->pushEmergency(emergencyContext); // Stores Emergency
        tcs->switch_state(TrafficControlSystem::SystemState::EMERGENCY);
    }
}

void StrategyNormal::handlePedestrianButtonEvent(TrafficControlSystem* tcs, const PedestrianButtonEvent& receive)
{
    std::cout << "PedestrianButtonEvent: loc " << receive.location << std::endl;

    if (!tcs->PSEM_Button_HasExtended(receive.location))
    {
        int remain = tcs->timerSwitchLight.getTime();
        if (remain > MIN_SECONDS)
            tcs->timerSwitchLight.timerRun(remain - REDUCE_SECONDS);
    }
}

void StrategyNormal::handlePedestrianRFIDEvent(TrafficControlSystem* tcs, const PedestrianRFIDEvent& receive)
{
    //   send to Cloud
    std::cout<<"PedestrianRFIDEvent:  loc "<< receive.location <<
                                    "  UUID: 0x" << std::hex << receive.uuid << "\n" << std::dec;

    // send to cloud mqueue
    tcs->sendToCloud(tx_cloud::ValidateRFID {receive.location, receive.uuid});
}

void StrategyNormal::handleCloudReceiveEvent(TrafficControlSystem* tcs, const CloudReceiveType& receive)
{
    // Receive will only happen if Card Data is Valid
    if (std::holds_alternative<rx_cloud::RFID_Validation>(receive))
    {
        const auto& value = std::get<rx_cloud::RFID_Validation> (receive);
        tcs->searchConfigurationForRFID(value.location);
    }
}

void StrategyNormal::controlOperation (TrafficControlSystem* tcs, Event& event)
{
    auto visitor = [tcs](auto&& receive) {
        using T = std::decay_t<decltype(receive)>;

        if constexpr (std::is_same_v<T, InternalEvent>)
            handleInternalEvent(tcs, receive);
        else if constexpr (std::is_same_v<T, DDSEvent>)
            handleDDSEvent(tcs, receive);
        else if constexpr (std::is_same_v<T, PedestrianButtonEvent>)
            handlePedestrianButtonEvent(tcs, receive);
        else if constexpr (std::is_same_v<T, PedestrianRFIDEvent>)
            handlePedestrianRFIDEvent(tcs, receive);
        else if constexpr (std::is_same_v<T, CloudReceiveType>)
            handleCloudReceiveEvent(tcs, receive);
    };

    std::visit (visitor, event);
}

