#include "linkelementwithnearestpoint.h"
#include "spatialsearchnearestnodes.h"
DM_DECLARE_NODE_NAME(LinkElementWithNearestPoint, Sewer)

LinkElementWithNearestPoint::LinkElementWithNearestPoint()
{
     treshhold = 200;
     onSignal = false;

    this->name_linkElements = "";
    this->name_nearestPoints = "";

    this->addParameter("Points_to_Link", DM::STRING, & this->name_linkElements);
    this->addParameter("Point_Field", DM::STRING, & this->name_nearestPoints);
    this->addParameter("treshhold",DM::DOUBLE, & this->treshhold);
    this->addParameter("onSignal",DM::BOOL, & this->onSignal);

}

void LinkElementWithNearestPoint::init()
{
    if (name_linkElements.empty()) return;
    if (name_nearestPoints.empty()) return;

    view_linkElements = DM::View(this->name_linkElements, DM::NODE, DM::READ);

    view_nearestPoints = DM::View(this->name_nearestPoints, DM::NODE, DM::READ);

    view_linkElements.addLinks(this->name_nearestPoints, view_nearestPoints);
    view_nearestPoints.addLinks(this->name_nearestPoints, view_linkElements);

    std::vector<DM::View> datastream;
    datastream.push_back(view_linkElements);
    datastream.push_back(view_nearestPoints);

    this->addData("city", datastream);

}

void LinkElementWithNearestPoint::run()
{
    DM::System * city = this->getData("city");

    std::vector<std::string> search_uuids = city->getUUIDs(view_nearestPoints);

    std::vector<DM::Node*> nodes;
    foreach (std::string uuid, search_uuids)
        nodes.push_back(city->getNode(uuid));

    int numberOfLinks = 0;
    SpatialSearchNearestNodes ssnn(city, nodes);

    std::vector<std::string> link_uuids = city->getUUIDs(view_linkElements);
    int l_n = link_uuids.size();

    for (int i = 0; i < l_n; i++) {        
        DM::Node * query = city->getNode(link_uuids[i]);
        double signal = query->getAttribute("new")->getDouble();
        if (onSignal && signal < 0.01)
            continue;
        DM::Node * n = ssnn.findNearestNode(query, this->treshhold);

        if (!n)
            continue;
        numberOfLinks++;

        query->getAttribute(name_nearestPoints)->setLink(name_nearestPoints, n->getUUID());
        n->getAttribute(name_linkElements)->setLink(name_linkElements, query->getUUID());
    }

    DM::Logger(DM::Standard) << "Linked Elements " << numberOfLinks;
}



