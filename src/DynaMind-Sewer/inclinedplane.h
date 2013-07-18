#ifndef INCLINEDPLANE_H
#define INCLINEDPLANE_H

#include <dm.h>

class DM_HELPER_DLL_EXPORT InclinedPlane : public DM::Module
{
    DM_DECLARE_NODE(InclinedPlane)
    private:
        long height;
    long width;
    double cellsize;
    double slope;

    DM::View v_plane;
    bool appendToStream;

public:
    InclinedPlane();
    void run();
    void init();



};

#endif // INCLINEDPLANE_H
