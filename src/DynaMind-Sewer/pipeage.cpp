#include "pipeage.h"

DM_DECLARE_NODE_NAME(PipeAge, Sewer)

PipeAge::PipeAge()
{


    view_inlet = DM::View("INLET", DM::NODE, DM::READ);
    view_catchment = DM::View("CATCHMENT", DM::COMPONENT, DM::READ);
    view_conduit = DM::View("CONDUIT", DM::EDGE, DM::READ);
    view_junction = DM::View("JUNCTION", DM::NODE, DM::READ);
    view_inlet.getAttribute("CATCHMENT");
    view_inlet.getAttribute("JUNCTION");
    view_junction.modifyAttribute("built_year");
    view_catchment.getAttribute("built_year");
    view_catchment.getAttribute("INLET");
    view_conduit.modifyAttribute("built_year");

    std::vector<DM::View> datastream;
    datastream.push_back(view_inlet);
    datastream.push_back(view_catchment);
    datastream.push_back(view_conduit);
    datastream.push_back(view_junction);

    this->addData("city", datastream);
}


void PipeAge::run()
{
    DM::System * city = this->getData("city");
    std::vector<std::string> edge_uuids = city->getUUIDs(view_conduit);
    std::map<DM::Node *, DM::Edge* > StartNodeSortedEdges;
    foreach(std::string name , edge_uuids)  {
        DM::Edge * e = city->getEdge(name);
        DM::Node * startnode = city->getNode(e->getStartpointName());
        StartNodeSortedEdges[startnode] = e;
    }
    std::vector<std::string> catchment_uuids  = city->getUUIDs(view_catchment);
    foreach(std::string name , catchment_uuids)  {
        DM::Component * c = city->getComponent(name);
        double startYear = c->getAttribute("built_year")->getDouble();
        if (startYear < 0.01)
            continue;
        std::string inlet_uuid = c->getAttribute("INLET")->getLink().uuid;
        DM::Component * inlet = city->getComponent(inlet_uuid);
        if (!inlet) continue;

        std::string junction_uuid = inlet->getAttribute("JUNCTION")->getLink().uuid;
        DM::Node * id = city->getNode(junction_uuid);
        if (!id) continue;
        if (id->getAttribute("built_year")->getDouble() < 0.001)
            id->changeAttribute("built_year", startYear);
        while (id!=0) {
            DM::Edge * e = StartNodeSortedEdges[id];
            if (!e)
                break;
            double e_year = e->getAttribute("built_year")->getDouble();
            e->addAttribute("built_year", startYear);
            if (e_year > 0.01 && e_year < startYear) {
                id = 0;
                continue;
            }
            e->addAttribute("built_year", startYear);
            if (id->getAttribute("built_year")->getDouble() < 0.001)
                id->changeAttribute("built_year", startYear);

            id = city->getNode(e->getEndpointName());

        }
    }

}


