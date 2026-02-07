#ifndef TRAFFICCONTROLSYSTEM_INTERNALEVENT_HPP
#define TRAFFICCONTROLSYSTEM_INTERNALEVENT_HPP

// Any internally generated relevant event
enum class InternalEvent
{
    NEW_STATE_ENTERED,      /* "on State Change" Generated Event */
    YELLOW_TIMEOUT,
    LIGHTS_TIMEOUT
};

#endif //TRAFFICCONTROLSYSTEM_INTERNALEVENT_HPP