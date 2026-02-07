#ifndef TRAFFICCONTROLSYSTEM_MEDIATOR_HPP
#define TRAFFICCONTROLSYSTEM_MEDIATOR_HPP

#include <nlohmann/json.hpp>

#include "Messages/EventsType.hpp"

using json = nlohmann::json;

class Mediator;

class Component
{
protected:
    Mediator *mediator;
public:
    explicit Component(Mediator *mediator);
    virtual ~Component()=default;
    // void setMediator (Mediator* mediator); // Mediator is set on creation and never changed
};

class Mediator
{
public:
    virtual ~Mediator() = default;
    virtual void notify (Component* sender, Event command)=0;
    //virtual void consume (queue)=0;
    virtual int createComponents (const std::shared_ptr<json>& data_file)=0;
};

#endif //TRAFFICCONTROLSYSTEM_MEDIATOR_HPP