// -*- c++ -*-

#ifndef Y2YouComponent_h
#define Y2YouComponent_h

#include "Y2.h"

class SCRInterpreter;
class YouAgent;

class Y2YouComponent : public Y2Component
{
    SCRInterpreter *interpreter;
    YouAgent *agent;
    
public:
    
    /**
     * Create a new Y2YouComponent
     */
    Y2YouComponent();
    
    /**
     * Cleans up
     */
    ~Y2YouComponent();
    
    /**
     * Returns true: The scr is a server component
     */
    bool isServer() const;
    
    /**
     * Returns "ag_you": This is the name of the you component
     */
    string name() const;
    
    /**
     * Evalutas a command to the scr
     */
    YCPValue evaluate(const YCPValue& command);

    /**
     * Returns the SCRAgent of the Y2Component, which of course is a
     * YouAgent.
     */
    SCRAgent* getSCRAgent ();    
};

#endif
