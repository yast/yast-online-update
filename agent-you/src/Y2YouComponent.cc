

#include "Y2YouComponent.h"
#include <scr/SCRInterpreter.h>
#include "YouAgent.h"


Y2YouComponent::Y2YouComponent()
    : interpreter(0),
      agent(0)
{
}


Y2YouComponent::~Y2YouComponent()
{
    if (interpreter) {
        delete interpreter;
        delete agent;
    }
}


bool
Y2YouComponent::isServer() const
{
    return true;
}


string
Y2YouComponent::name() const
{
    return "ag_you";
}


YCPValue
Y2YouComponent::evaluate(const YCPValue& value)
{
    if (!interpreter)
    {
	getSCRAgent ();
    }
    
    return interpreter->evaluate(value);
}

SCRAgent*
Y2YouComponent::getSCRAgent ()
{
    if (!interpreter)
    {
	agent = new YouAgent ();
	interpreter = new SCRInterpreter (agent);
    }
    return agent;
}

