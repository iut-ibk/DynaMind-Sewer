#ifndef PIPEAGE_H
#define PIPEAGE_H

#include <dm.h>


class DM_HELPER_DLL_EXPORT PipeAge : public DM::Module
{   
    DM_DECLARE_NODE (PipeAge)

    DM::View view_inlet = DM::View("INLET", DM::NODE, DM::READ);
    DM::View view_catchment = DM::View("CATCHMENT", DM::COMPONENT, DM::READ);
    DM::View view_conduit = DM::View("CONDUIT", DM::EDGE, DM::READ);

public:
        void run();
        PipeAge();
};

#endif // PIPEAGE_H
