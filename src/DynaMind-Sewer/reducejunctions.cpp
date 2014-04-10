/**
 * @file
 * @author  Chrisitan Urich <christian.urich@gmail.com>
 * @version 1.0
 * @section LICENSE
 *
 * This file is part of DynaMind
 *
 * Copyright (C) 2013  Christian Urich

 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "reducejunctions.h"
#include "tbvectordata.h"

DM_DECLARE_NODE_NAME(ReduceJunctions, Sewer)

ReduceJunctions::ReduceJunctions()
{
	junctions = DM::View("JUNCTION", DM::NODE, DM::MODIFY);
	junctions.addAttribute("end_counter", DM::Attribute::DOUBLE, DM::WRITE);
	conduits = DM::View("CONDUIT", DM::EDGE, DM::MODIFY);
	conduits.addAttribute("strahler", DM::Attribute::DOUBLE, DM::READ);
	inlets = DM::View("INLET", DM::NODE, DM::READ);
	inlets.addAttribute("JUNCTION", "JUNCTION", DM::READ);
	conduit_new = DM::View("CONDUIT", DM::EDGE, DM::WRITE);
	catchment = DM::View("CATCHMENT", DM::READ, DM::READ);
	catchment.addAttribute("junction_id", DM::Attribute::DOUBLE, DM::WRITE);
	std::vector<DM::View> datastream;
	datastream.push_back(junctions);
	datastream.push_back(conduit_new);
	datastream.push_back(conduits);
	datastream.push_back(inlets);
	datastream.push_back(catchment);

	distance = 50.0;

	this->addData("city", datastream);

	this->addParameter("Distance", DM::DOUBLE, &distance);
}

void ReduceJunctions::createJunctions(DM::System * city, std::vector<DM::Node *> &nodes,  std::vector<DM::Node *> &r_nodes, int strahlerNumber)
{
	int numberOfEdges = nodes.size() - 1;
	for (int i = 0; i < numberOfEdges; i++) 
	{
		if (nodes[i] == nodes[i+1])
			continue;
		junction_map_Counter[nodes[i]] = 1;
		junction_map_Counter[nodes[i+1]] = 1;
		DM::Edge * e = city->addEdge(nodes[i], nodes[i+1], conduit_new);
		e->addAttribute("strahler", strahlerNumber);
	}

	DM::Node * lastNode = nodes[numberOfEdges];
	//move all inlet connections to the end and remove junctions
	foreach (DM::Node * n, r_nodes) 
	{
		DM::Attribute junction_link_attribute("INLET", DM::Attribute::LINK);
		foreach(DM::Component* inlet, n->getAttribute("INLET")->getLinkedComponents())
		{
			DM::Attribute attr("JUNCTION", DM::Attribute::LINK);
			attr.addLink(lastNode, "JUNCTION");
			inlet->changeAttribute(attr);
			junction_link_attribute.addLink(inlet, "INLET");
		}

		foreach(DM::Component* linkedCmp, junction_link_attribute.getLinkedComponents())
			lastNode->getAttribute("INLET")->addLink(linkedCmp, "INLET");
		
		if (n->getAttribute("existing")->getDouble() > 0.01)
			continue;

		if ( junction_map_Counter[n] > 0)
			continue;

		city->removeComponentFromView(n, junctions);
	}

	nodes.clear();
	r_nodes.clear();

}

void ReduceJunctions::run()
{
	DM::System * city = this->getData("city");
	std::vector<DM::Component*> junctionCmps = city->getAllComponentsInView(junctions);
	std::vector<DM::Component*> conduitCmps = city->getAllComponentsInView(conduits);
	//std::vector<std::string> inlet_uuids = city->getUUIDs(inlets);
	std::vector<DM::Node*> startNodes;
	std::map<DM::Node*, DM::Edge*> startNodeMap;
	std::map<DM::Node*, int> endPointCounter;

	foreach(DM::Component* cmp, conduitCmps)
	{
		DM::Edge * c = (DM::Edge*)cmp;

		startNodeMap[c->getStartNode()] = c;
		endPointCounter[c->getEndNode()]++;

		//Remove old
		if (c->getAttribute("existing")->getDouble() > 0.01)
			continue;

		city->removeComponentFromView(c, conduits);
	}

	foreach(DM::Component* cmp, junctionCmps)
	{
		DM::Node * jun = (DM::Node*)cmp;
		int counter = endPointCounter[jun];
		jun->addAttribute("end_counter",(double) counter);
		jun->addAttribute("visited",0); // reset when used in loop
		if (jun->getAttribute("existing")->getDouble() > 0.01 )
			continue;
		if (counter!=0)
			continue;
		startNodes.push_back(jun);

	}

	//Only fix point is change in strahler Number
	foreach(DM::Node* current_j, startNodes)
	{
		if (!current_j)
			continue;

		std::vector<DM::Node*> new_conduits;
		std::vector<DM::Node*> removed_junctions;
		new_conduits.push_back(current_j);
		int currrentStrahler = -1;
		double segment_length = 0;
		while (current_j) 
		{
			DM::Edge * c = startNodeMap[current_j];
			if (!c || current_j->getAttribute("visited")->getDouble() > 0.01) {
				//last node
				new_conduits.push_back(current_j);
				this->createJunctions(city, new_conduits, removed_junctions, currrentStrahler);
				current_j = 0;
				continue;
			}
			if (current_j->getAttribute("existing")->getDouble() > 0.01){
				new_conduits.push_back(current_j);
				this->createJunctions(city, new_conduits, removed_junctions, currrentStrahler);
				current_j = 0;
				continue;

			}

			if (currrentStrahler == -1) {
				currrentStrahler = (int) c->getAttribute("strahler")->getDouble();
			}

			current_j->addAttribute("visited",1);
			current_j = c->getEndNode();
			int connectingNodes = endPointCounter[current_j];

			if ( connectingNodes == 1 && segment_length < this->distance ) {
				removed_junctions.push_back(current_j);
				segment_length+= TBVectorData::calculateDistance(c->getStartNode(), c->getEndNode());
				continue;
			}

			new_conduits.push_back(current_j);
			this->createJunctions(city, new_conduits, removed_junctions, currrentStrahler);

			//current node is start for next conduit
			new_conduits.push_back(current_j);
			currrentStrahler =  (int) c->getAttribute("strahler")->getDouble();
			segment_length = 0;
		}
	}
	//Update Catchment UUID
	foreach(DM::Component * c, city->getAllComponentsInView(catchment))
	{
		std::vector<DM::Component*> links = c->getAttribute("INLET")->getLinkedComponents();

		if (links.size() < 1)
			continue;

		DM::Component * inlet = links[0];

		if (!inlet)
			continue;

		links = c->getAttribute("JUNCTION")->getLinkedComponents();

		if (links.size() < 1)
			continue;

		DM::Component * jun = links[0];
		if (!jun)
			continue;

		c->addAttribute("junction_id", jun->getAttribute("id")->getDouble());
	}
}
