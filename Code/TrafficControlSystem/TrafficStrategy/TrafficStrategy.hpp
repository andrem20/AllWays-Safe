#ifndef TRAFFICCONTROLSYSTEM_TRAFFICSTRATEGY_HPP
#define TRAFFICCONTROLSYSTEM_TRAFFICSTRATEGY_HPP

/*
 *   Implementation of Strategy Design Pattern to control the system,
 *  depending on the current state
 *   Referencing will allow for the separation of the Strategy from
 *  the rest of the code - there is no need for it to know about
 *  the Components
 *
 * These methods implement the Traffic Control System functions which control the system's
 * components, using different attributes => different controlling strategies
 * They only check the event they desire
 */

#include "../TrafficControlSystem.hpp"

class TrafficControlSystem;

/*--- STRATEGY PATTERN INTERFACE -------------------------------------------------------------------------------------*/
class I_TrafficStrategy
{
public:
  virtual ~I_TrafficStrategy()=default;
  virtual void controlOperation (TrafficControlSystem* tcs, Event& event)=0;
};

/*--- STRATEGY PATTERN IMPLEMENTATIONS -------------------------------------------------------------------------------*/

/* Strategies developed for the following system states/operational modes:
   * SET UP
   * NORMAL
   * EMERGENCY
   * FAILURE
 */

class StrategySetUp : public I_TrafficStrategy
{
  static int event_counter;
public:
  void controlOperation (TrafficControlSystem* tcs, Event& event) override;
};

class StrategyNormal : public I_TrafficStrategy
{
  static void handleInternalEvent(TrafficControlSystem* tcs, const InternalEvent& receive);
  static void handleDDSEvent(TrafficControlSystem* tcs, const DDSEvent& receive);
  static void handlePedestrianButtonEvent(TrafficControlSystem* tcs, const PedestrianButtonEvent& event);
  static void handlePedestrianRFIDEvent(TrafficControlSystem* tcs, const PedestrianRFIDEvent& event);
  static void handleCloudReceiveEvent(TrafficControlSystem* tcs, const CloudReceiveType& event);

public:
  void controlOperation (TrafficControlSystem* tcs, Event& event) override;
};

class StrategyEmergency : public I_TrafficStrategy
{
  static void handleInternalEvent(TrafficControlSystem* tcs, const InternalEvent& receive);
  static void handleDDSEvent(TrafficControlSystem* tcs, const DDSEvent& receive);
public:
  void controlOperation (TrafficControlSystem* tcs, Event& event) override;
};

class StrategyFailure : public I_TrafficStrategy
{
public:
  void controlOperation (TrafficControlSystem* tcs, Event& event) override;
};

#endif //TRAFFICCONTROLSYSTEM_TRAFFICSTRATEGY_HPP