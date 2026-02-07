#include "TrafficStrategy.hpp"

#include "../Messages/Components/Cloud/QueueSendCloudTypes.hpp"
#include "../Messages/EventsType.hpp"

#define SET_UP_CONFIGS 2

int StrategySetUp::event_counter = 0;

// Sets up the System: Based only on CloudReceiveType
void StrategySetUp::controlOperation (TrafficControlSystem* tcs, Event& event)
{
    if (std::holds_alternative<InternalEvent>(event))
    {
        const auto& receive = std::get<InternalEvent>(event);
        if (receive == InternalEvent::NEW_STATE_ENTERED)
        {
            tx_cloud::Configure configuration;
            tcs->sendToCloud(configuration);
        }
    }

    if (std::holds_alternative<CloudReceiveType>(event))
    {
        const auto& receive = std::get<CloudReceiveType>(event);

        if (std::holds_alternative<rx_cloud::PSEM_data>(receive))
        {
            event_counter ++;
            const auto& data = std::get<rx_cloud::PSEM_data>(receive);
            tcs->createComponents(data.file);
        }
        else if (std::holds_alternative<rx_cloud::TSEM_data>(receive))
        {
            event_counter ++;
            const auto& data = std::get<rx_cloud::TSEM_data>(receive);
            tcs->createComponents(data.file);
        }
    }

    // If all HW configurations (2) are SET UP, find system configurations and then move to operational mode
    if (event_counter == SET_UP_CONFIGS)
    {
        tcs->findConfigurations();
        // For components who have just been set up and require threads
        tcs->startComponents();

        // Put System in Initially Known State -  ALL RED
        TrafficControlSystem::SwitchLightsData startConfiguration = tcs->systemWarning();
        tcs->switchLightQueue.send(std::move(startConfiguration));

        // Finally, switch state to NORMAL execution
        tcs->switch_state (TrafficControlSystem::SystemState::NORMAL);
    }
}
