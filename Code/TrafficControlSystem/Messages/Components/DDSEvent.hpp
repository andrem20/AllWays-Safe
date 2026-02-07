#ifndef TRAFFICCONTROLSYSTEM_DDSEVENT_HPP
#define TRAFFICCONTROLSYSTEM_DDSEVENT_HPP

//#include <utility>

enum class DDS_Event_Qualifier
{
  EMERGENCY_START,    // "match" in DDS
  EMERGENCY_FINISH,      // no connection yet or "unmatch" in DDS
};

struct DDSEvent
{
  DDS_Event_Qualifier qualifier;
  // The following arguments aren't necessary when qualifier is EMERGENCY_END
  std::string license_plate = "";
  int location = -1;
  int direction = -1;
  int priority = -1;
};

#endif //TRAFFICCONTROLSYSTEM_DDSEVENT_HPP