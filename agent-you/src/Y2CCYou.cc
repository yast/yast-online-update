

#include "Y2CCYou.h"
#include "Y2YouComponent.h"


Y2CCYou::Y2CCYou()
    : Y2ComponentCreator(Y2ComponentBroker::BUILTIN)
{
}


bool
Y2CCYou::isServerCreator() const
{
    return true;
}


Y2Component *
Y2CCYou::create(const char *name) const
{
    if (!strcmp(name, "ag_you")) return new Y2YouComponent();
    else return 0;
}


Y2CCYou g_y2ccag_you;
