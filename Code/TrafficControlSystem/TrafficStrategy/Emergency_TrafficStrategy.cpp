#include "TrafficStrategy.hpp"

void StrategyEmergency::handleInternalEvent(TrafficControlSystem* tcs, const InternalEvent& receive)
{
    if (receive == InternalEvent::NEW_STATE_ENTERED)
    {
        if (const int ret = tcs->EVneedChangeConfiguration(); ret >= 0)
        {
            TrafficControlSystem::SwitchLightsData newConfiguration = tcs->organizeNextConfiguration(ret);
            auto sendData = newConfiguration;

            tcs->timerSwitchLight.fireImmediately(); // NOT WORKING WHEN YELLOW
            tcs->switchLightQueue.send(std::move(sendData));
        }
        tcs->sendToCloud(tcs->getEmergency());

        tcs->popEmergency();
    }
    // This case happens when an Emergency Vehicle Passes on a Yellow Light transition
    else if (receive == InternalEvent::YELLOW_TIMEOUT)
    {
        tcs->timerSwitchLight.fireImmediately();
    }
}

void StrategyEmergency::handleDDSEvent(TrafficControlSystem* tcs, const DDSEvent& receive)
{
    if (receive.qualifier == DDS_Event_Qualifier::EMERGENCY_FINISH)
    {
        TrafficControlSystem::SwitchLightsData outConfiguration = tcs->organizeNextConfiguration();
        outConfiguration.time = 5;
        //tcs->timerSwitchLight.timerRun(0);
        auto sendData = outConfiguration;
        tcs->switchLightQueue.send(std::move(sendData)); // trigger Queue -> next configuration
        tcs->switch_state (TrafficControlSystem::SystemState::NORMAL);
    }
}


/* Emergency Strategy
 *  - Identify all the currently ON semaphores which interfere with the EV passage
 *  - Switch them off, and warn of the situation where it should be warned
 */
void StrategyEmergency::controlOperation(TrafficControlSystem* tcs, Event& event)
{

    auto visitor = [tcs](auto&& receive)
    {
        using T = std::decay_t<decltype(receive)>;

        if constexpr (std::is_same_v<T, InternalEvent>)
            handleInternalEvent(tcs, receive);
        else if constexpr (std::is_same_v<T, DDSEvent>)
            handleDDSEvent(tcs, receive);
    };

    std::visit(visitor, event);
}
