#ifndef INFITRATIONTRENCH_H
#define INFITRATIONTRENCH_H

#include <dm.h>

class DM_HELPER_DLL_EXPORT InfiltrationTrench : public DM::Module
{
	DM_DECLARE_NODE(InfiltrationTrench)
private:
	double R;
	double D;
	bool useClimateChangeFactorInfiltration;

	DM::View view_building;
	DM::View view_parcel;
	DM::View view_infitration_system;
	DM::View view_catchment;
	DM::View view_city;

public:
	void init();
	InfiltrationTrench();
	void run();
	std::string getHelpUrl();

};

#endif // INFITRATIONTRENCH_H
