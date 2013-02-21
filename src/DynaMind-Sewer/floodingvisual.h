#ifndef FLOODINGVISUAL_H
#define FLOODINGVISUAL_H

#include <dm.h>

class DM_HELPER_DLL_EXPORT FloodingVisual : public DM::Module
{
    DM_DECLARE_NODE(FloodingVisual)
    private:
        DM::View view_junctions;
        DM::View view_junction_flooding;
        double radius;
        double scaling;

public:
    FloodingVisual();
    void run();
};

#endif // FLOODINGVISUAL_H
