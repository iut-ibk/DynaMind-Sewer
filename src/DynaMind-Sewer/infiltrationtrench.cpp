#include "infiltrationtrench.h"

DM_DECLARE_NODE_NAME(InfiltrationTrench, Sewer)
InfiltrationTrench::InfiltrationTrench()
{
    R = 200; //l/s.ha (DWA-A 138)
    D = 10;

    this->addParameter("R", DM::DOUBLE, &this->R);
    this->addParameter("D", DM::DOUBLE, &this->D);

    view_infitration_system = DM::View("INFILTRATION_SYSTEM", DM::COMPONENT, DM::WRITE);
    view_infitration_system.addAttribute("treated_area");
    view_infitration_system.addAttribute("area");
    view_infitration_system.addAttribute("h");
    view_infitration_system.addAttribute("kf");

    view_catchment = DM::View("CATCHMENT", DM::COMPONENT, DM::READ);
    view_catchment.getAttribute("roof_area_treated");
    view_catchment.getAttribute("Impervious");
    view_catchment.getAttribute("area");
    view_catchment.addLinks("INFILTRATION_SYSTEM", view_infitration_system);

    std::vector<DM::View> datastream;

    datastream.push_back(view_infitration_system);
    datastream.push_back(view_catchment);

    this->addData("city", datastream);
}

void InfiltrationTrench::run() {
    DM::System * city = this->getData("city");

    std::vector<std::string> catchment_uuids = city->getUUIDs(view_catchment);
    int numberOfPlacedSystem = 0;

    foreach (std::string uuid, catchment_uuids) {
        DM::Face * catchment = city->getFace(uuid);

        std::vector<DM::LinkAttribute> exinsting_infils = catchment->getAttribute("INFILTRATION_SYSTEM")->getLinks();
        double already_treated = 0;
        foreach (DM::LinkAttribute la,exinsting_infils) {
            DM::Component * inf = city->getComponent(la.uuid);
            already_treated+=inf->getAttribute("treated_area")->getDouble();
        }
        double roof_area_tot = catchment->getAttribute("roof_area_treated")->getDouble();

        double toTreat = roof_area_tot - already_treated;
        //DM::Logger(DM::Standard) << roof_area_tot;
        //DM::Logger(DM::Standard) << already_treated;

        if (toTreat < 5) continue;

        double imp = catchment->getAttribute("Impervious")->getDouble();
        double avalible_space = (1.-imp) * catchment->getAttribute("area")->getDouble();

        //Design Infitration System
        double Qzu = toTreat * R *(1.e-7);//m3/s
        double kf = 1.e-4;
        double h = 1;
        double D = this->D*60;

        double As = Qzu /(h/D+0.5*kf);

        if(avalible_space < As){
            DM::Logger(DM::Warning) << "Not enough space can't place infil system";
        }

        //catchment->getAttribute("INFILTRATION_SYSTEM")->setLinks(std::vector<DM::LinkAttribute>());

        DM::Component * infiltration_system = new DM::Component();
        city->addComponent(infiltration_system, view_infitration_system);
        DM::Logger(DM::Debug) << toTreat;
        infiltration_system->addAttribute("treated_area", toTreat);
        infiltration_system->addAttribute("area", As);
        infiltration_system->addAttribute("h", h);
        infiltration_system->addAttribute("kf", kf);
        infiltration_system->getAttribute("Footprint")->setLink("Footprint", uuid);

        catchment->getAttribute("INFILTRATION_SYSTEM")->setLink("INFILTRATION_SYSTEM", infiltration_system->getUUID());

        numberOfPlacedSystem++;

    }
    DM::Logger(DM::Standard) << "Number of placed infiltration systems " << numberOfPlacedSystem;
}

string InfiltrationTrench::getHelpUrl()
{
    return "https://github.com/iut-ibk/DynaMind-Sewer/blob/master/doc/InfiltrationTrench.md";
}
