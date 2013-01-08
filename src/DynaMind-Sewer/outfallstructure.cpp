#include "outfallstructure.h"

DM_DECLARE_NODE_NAME(OutfallStructure, Sewer)

OutfallStructure::OutfallStructure()
{
    view_outlets = DM::View("OUTLET", DM::NODE, DM::READ);
    view_outfalls = DM::View("OUTFALL", DM::NODE, DM::WRITE);
    view_conduits = DM::View("CONDUIT", DM::EDGE, DM::WRITE);

    std::vector<DM::View> datastream;

    datastream.push_back(view_outlets);
    datastream.push_back(view_outfalls);
    datastream.push_back(view_conduits);

    this->addData("city", datastream);

}

void OutfallStructure::run()
{
    DM::System * city = this->getData("city");

    std::vector<std::string> outlet_uuids = city->getUUIDs(view_outlets);
    foreach (std::string o_uuid, outlet_uuids) {
        DM::Node * n = city->getNode(o_uuid);

        DM::Node offset(15,15,0);

        DM::Node * outfall = city->addNode(*n + offset, view_outfalls);

        DM::Edge * e = city->addEdge(n, outfall, view_conduits);
        e->addAttribute("Diameter", 4000);
        e->addAttribute("Length", 15.);




    }
}
