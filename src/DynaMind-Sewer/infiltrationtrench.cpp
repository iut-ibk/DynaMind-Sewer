#include "infiltrationtrench.h"

DM_DECLARE_NODE_NAME(InfiltrationTrench, Sewer)
InfiltrationTrench::InfiltrationTrench()
{
	R = 200; //l/s.ha (DWA-A 138)
	D = 10;
	useClimateChangeFactorInfiltration = false;

	this->addParameter("R", DM::DOUBLE, &this->R);
	this->addParameter("D", DM::DOUBLE, &this->D);
	this->addParameter("useClimateCFInfiltration", DM::BOOL, &this->useClimateChangeFactorInfiltration);

	view_infitration_system = DM::View("INFILTRATION_SYSTEM", DM::COMPONENT, DM::WRITE);
	view_infitration_system.addAttribute("treated_area", DM::Attribute::DOUBLE, DM::WRITE);
	view_infitration_system.addAttribute("area", DM::Attribute::DOUBLE, DM::WRITE);
	view_infitration_system.addAttribute("h", DM::Attribute::DOUBLE, DM::WRITE);
	view_infitration_system.addAttribute("kf", DM::Attribute::DOUBLE, DM::WRITE);

	view_catchment = DM::View("CATCHMENT", DM::COMPONENT, DM::READ);
	view_catchment.addAttribute("roof_area_treated", DM::Attribute::DOUBLE, DM::READ);
	view_catchment.addAttribute("Impervious", DM::Attribute::DOUBLE, DM::READ);
	view_catchment.addAttribute("area", DM::Attribute::DOUBLE, DM::READ);
	view_catchment.addAttribute("INFILTRATION_SYSTEM", "INFILTRATION_SYSTEM", DM::WRITE);

	view_city = DM::View("CITY", DM::COMPONENT, DM::READ); //Not added in init
	view_city.addAttribute("CFInfiltration", DM::Attribute::DOUBLE, DM::READ);


	std::vector<DM::View> datastream;

	datastream.push_back(view_infitration_system);
	datastream.push_back(view_catchment);

	this->addData("city", datastream);
}

void InfiltrationTrench::init() {


	std::vector<DM::View> datastream;

	datastream.push_back(view_infitration_system);
	datastream.push_back(view_catchment);

	if (useClimateChangeFactorInfiltration) {
		datastream.push_back(view_city);
	}

	this->addData("city", datastream);
}

void InfiltrationTrench::run() {
	DM::System * city = this->getData("city");

	int numberOfPlacedSystem = 0;
	double cf = 1;

	if (useClimateChangeFactorInfiltration)
		foreach ( DM::Component * cmp, city->getAllComponentsInView(view_city) )
			cf = cmp->getAttribute("CFInfiltration")->getDouble();

	foreach(DM::Component* cmp, city->getAllComponentsInView(view_catchment)) 
	{
		DM::Face * catchment = (DM::Face*)cmp;

		double already_treated = 0;
		foreach(DM::Component* inf, catchment->getAttribute("INFILTRATION_SYSTEM")->getLinkedComponents())
			already_treated += inf->getAttribute("treated_area")->getDouble();

		double roof_area_tot = catchment->getAttribute("roof_area_treated")->getDouble();

		double toTreat = roof_area_tot - already_treated;

		if (toTreat < 5)
			continue;

		double imp = catchment->getAttribute("Impervious")->getDouble();
		double avalible_space = (1.-imp) * catchment->getAttribute("area")->getDouble();

		//Design Infitration System
		double Qzu = toTreat * R * cf *(1.e-7);//m3/s
		double kf = 1.e-4;
		double h = 1;
		double D = this->D*60;

		double As = Qzu /(h/D+0.5*kf);

		if(avalible_space < As){
			DM::Logger(DM::Warning) << "Not enough space can't place infil system";
			continue;
		}

		//catchment->getAttribute("INFILTRATION_SYSTEM")->setLinks(std::vector<DM::LinkAttribute>());

		DM::Component * infiltration_system = new DM::Component();
		city->addComponent(infiltration_system, view_infitration_system);
		DM::Logger(DM::Debug) << toTreat;
		infiltration_system->addAttribute("treated_area", toTreat);
		infiltration_system->addAttribute("area", As);
		infiltration_system->addAttribute("h", h);
		infiltration_system->addAttribute("kf", kf);

		DM::Attribute* link = infiltration_system->getAttribute("Footprint");
		link->clearLinks();
		link->addLink(catchment, "Footprint");

		link = infiltration_system->getAttribute("INFILTRATION_SYSTEM");
		link->clearLinks();
		link->addLink(infiltration_system, "INFILTRATION_SYSTEM");

		numberOfPlacedSystem++;

	}
	DM::Logger(DM::Standard) << "Number of placed infiltration systems " << numberOfPlacedSystem;
}

string InfiltrationTrench::getHelpUrl()
{
	return "https://github.com/iut-ibk/DynaMind-Sewer/blob/master/doc/InfiltrationTrench.md";
}
