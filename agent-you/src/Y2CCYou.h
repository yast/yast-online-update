// -*- c++ -*-

#ifndef Y2CCYou_h
#define Y2CCYou_h

#include "Y2.h"

class Y2CCYou : public Y2ComponentCreator
{
 public:
    /**
     * Creates a new Y2CCYou object.
     */
    Y2CCYou();
    
    /**
     * Returns true: The You agent is a server component.
     */
    bool isServerCreator() const;
    
    /**
     * Creates a new @ref Y2SCRComponent, if name is "ag_you".
     */
    Y2Component *create(const char *name) const;
};

#endif
