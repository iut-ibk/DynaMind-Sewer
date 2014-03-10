#include "pipeage.h"

DM_DECLARE_NODE_NAME(PipeAge, Sewer)

PipeAge::PipeAge()
{


	view_inlet = DM::View("INLET", DM::NODE, DM::READ);
	view_catchment = DM::View("CATCHMENT", DM::COMPONENT, DM::READ);
	view_conduit = DM::View("CONDUIT", DM::EDGE, DM::READ);
	view_junction = DM::View("JUNCTION", DM::NODE, DM::READ);
	view_inlet.addAttribute("CATCHMENT", "CITYBLOCKS", DM::READ);
	view_inlet.addAttribute("JUNCTION", "Junction", DM::READ);
	view_junction.addAttribute("built_year", DM::Attribute::DOUBLE, DM::MODIFY);
	view_catchment.addAttribute("built_year", DM::Attribute::DOUBLE, DM::READ);
	view_catchment.addAttribute("INLET", "INLET", DM::READ);
	view_conduit.addAttribute("built_year", DM::Attribute::DOUBLE, DM::MODIFY);

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
	std::map<DM::Node *, DM::Edge* > StartNodeSortedEdges;
	foreach(DM::Component* cmp, city->getAllComponentsInView(view_conduit))  
	{
		DM::Edge * e = (DM::Edge*)cmp;
		StartNodeSortedEdges[e->getStartNode()] = e;
	}

	foreach(DM::Component* c, city->getAllComponentsInView(view_catchment))
	{
		double startYear = c->getAttribute("built_year")->getDouble();

		if (startYear < 0.01)
			continue;

		std::vector<DM::Component*> linkedCmps = c->getAttribute("INLET")->getLinkedComponents();
		DM::Component * inlet = linkedCmps.size() > 0 ? linkedCmps[0] : NULL;

		if (!inlet) 
			continue;

		linkedCmps = c->getAttribute("JUNCTION")->getLinkedComponents();
		DM::Node * id = linkedCmps.size() > 0 ? (DM::Node*)linkedCmps[0] : NULL;

		if (!id) 
			continue;

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

			id = e->getEndNode();
		}
	}

}


