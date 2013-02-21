#ifndef PIPEAGE_H
#define PIPEAGE_H

#include <dm.h>


class DM_HELPER_DLL_EXPORT PipeAge : public DM::Module
{   
    DM_DECLARE_NODE (PipeAge)

    DM::View view_inlet;
    DM::View view_catchment;
    DM::View view_conduit;
public:
        void run();
        PipeAge();
};

#endif // PIPEAGE_H
