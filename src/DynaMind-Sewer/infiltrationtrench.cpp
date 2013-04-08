#include "infiltrationtrench.h"

DM_DECLARE_NODE_NAME(InfiltrationTrench, Sewer)
InfiltrationTrench::InfiltrationTrench()
{
    R = 200; //l/s.ha (DWA-A 138)
    D = 10;

    this->addParameter("R", DM::DOUBLE, &this->R);
    this->addParameter("D", DM::DOUBLE, &this->D);



    view_building = DM::View("BUILDING", DM::COMPONENT, DM::READ);
    view_building.getAttribute("roof_area");

    view_parcel = DM::View("PARCEL", DM::COMPONENT, DM::READ);
    view_parcel.getAttribute("CATCHMENT");

    view_parcel.getAttribute("BUILDING");
    view_parcel.addLinks("INFILTRATION_SYSTEM", view_infitration_system);

    view_infitration_system = DM::View("INFILTRATION_SYSTEM", DM::COMPONENT, DM::WRITE);
    view_infitration_system.addAttribute("treated_area");
    view_infitration_system.addAttribute("area");
    view_infitration_system.addAttribute("h");
    view_infitration_system.addAttribute("kf");

    view_catchment = DM::View("CATCHMENT", DM::COMPONENT, DM::READ);
    view_catchment.addLinks("INFILTRATION_SYSTEM", view_infitration_system);


    view_parcel.addLinks("INFILTRATION_SYSTEM", view_infitration_system);
    view_infitration_system.addLinks("PARCEL", view_parcel);

    std::vector<DM::View> datastream;

    datastream.push_back(view_building);
    datastream.push_back(view_parcel);
    datastream.push_back(view_infitration_system);
    datastream.push_back(view_catchment);

    this->addData("city", datastream);
}

void InfiltrationTrench::run() {
    DM::System * city = this->getData("city");

    std::vector<std::string> parcel_uuids = city->getUUIDs(view_parcel);

    foreach (std::string uuid, parcel_uuids) {
        DM::Face * parcel = city->getFace(uuid);
        DM::Component * catchment =  city->getComponent(parcel->getAttribute("CATCHMENT")->getLink().uuid);
        if (!catchment)
            continue;
        //Get Buildings on Parcel
        double roof_area_tot = 0;
        std::vector<DM::LinkAttribute> l_buildings = parcel->getAttribute("BUILDING")->getLinks();
        foreach (DM::LinkAttribute l_b, l_buildings) {
            DM::Component * building = city->getComponent(l_b.uuid);
            roof_area_tot += building->getAttribute("roof_area")->getDouble();

        }

        //Design Infitration System
        double Qzu = roof_area_tot * R *(1.e-7);//m3/s
        double kf = 1.e-4;
        double h = 1;
        double D = this->D*60;

        double As = Qzu /(h/D+0.5*kf);

        DM::Component * infiltration_system = new DM::Component();
        city->addComponent(infiltration_system, view_infitration_system);
        DM::Logger(DM::Debug) << roof_area_tot;
        infiltration_system->addAttribute("treated_area", roof_area_tot);
        infiltration_system->addAttribute("area", As);
        infiltration_system->addAttribute("h", h);
        infiltration_system->addAttribute("kf", kf);
        infiltration_system->getAttribute("PARCEL")->setLink("PARCEL", uuid);

        catchment->getAttribute("INFILTRATION_SYSTEM")->setLink("INFILTRATION_SYSTEM", infiltration_system->getUUID());



    }





}
