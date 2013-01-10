#ifndef INFITRATIONTRENCH_H
#define INFITRATIONTRENCH_H

#include <dm.h>

class DM_HELPER_DLL_EXPORT InfiltrationTrench : public DM::Module
{
    DM_DECLARE_NODE(InfiltrationTrench)
private:
        double R;
        double D;

        DM::View view_building;
        DM::View view_parcel;
        DM::View view_infitration_system;

public:
    InfiltrationTrench();
    void run();

};

#endif // INFITRATIONTRENCH_H
