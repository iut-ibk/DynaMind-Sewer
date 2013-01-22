#ifndef LINKELEMENTWITHNEARESTPOINT_H
#define LINKELEMENTWITHNEARESTPOINT_H

#include <dm.h>

class DM_HELPER_DLL_EXPORT LinkElementWithNearestPoint : public DM::Module
{
    DM_DECLARE_NODE(LinkElementWithNearestPoint)
private:
    DM::View view_nearestPoints;
    DM::View view_linkElements;

    std::string name_nearestPoints;
    std::string name_linkElements;

    double treshhold = 200;
    bool onSignal = false;

public:
    LinkElementWithNearestPoint();
    void run();
    void init();
};

#endif // LINKELEMENTWITHNEARESTPOINT_H
