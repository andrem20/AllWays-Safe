#include "Mediator.hpp"

Component::Component(Mediator* mediator)
{
   this->mediator = mediator;
}
