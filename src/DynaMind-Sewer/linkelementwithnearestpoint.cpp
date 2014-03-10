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

	view_linkElements.addAttribute(this->name_nearestPoints, this->name_nearestPoints, DM::WRITE);
	view_linkElements.addAttribute(this->name_linkElements, this->name_linkElements, DM::WRITE);

	std::vector<DM::View> datastream;
	datastream.push_back(view_linkElements);
	datastream.push_back(view_nearestPoints);

	this->addData("city", datastream);
}

string LinkElementWithNearestPoint::getHelpUrl()
{
	return "https://github.com/iut-ibk/DynaMind-Sewer/blob/master/doc/LinkElementWithNearestPoint.md";
}

void LinkElementWithNearestPoint::run()
{
	DM::System * city = this->getData("city");

	std::vector<DM::Node*> nodes;
	foreach (DM::Component* cmp, city->getAllComponentsInView(view_nearestPoints))
		nodes.push_back((DM::Node*)cmp);

	int numberOfLinks = 0;
	SpatialSearchNearestNodes ssnn(city, nodes);

	foreach(DM::Component* cmp, city->getAllComponentsInView(view_linkElements))
	{
		DM::Node * query = (DM::Node*)cmp;
		double signal = query->getAttribute("new")->getDouble();
		if (onSignal && signal < 0.01)
			continue;
		DM::Node * n = ssnn.findNearestNode(query, this->treshhold);

		if (!n)
			continue;
		numberOfLinks++;

		query->getAttribute(name_nearestPoints)->addLink(n, name_nearestPoints);
		n->getAttribute(name_linkElements)->addLink(query, name_linkElements);
	}

	DM::Logger(DM::Standard) << "Linked Elements " << numberOfLinks;
}



