#ifndef TRAFFICCONTROLSYSTEM_PEDESTRIANEVENT_H
#define TRAFFICCONTROLSYSTEM_PEDESTRIANEVENT_H

#include <cstdint>

typedef struct
{
    int location;
}PedestrianButtonEvent;

typedef struct
{
    int location;
    uint32_t uuid;
}PedestrianRFIDEvent;

#endif //TRAFFICCONTROLSYSTEM_PEDESTRIANEVENT_H