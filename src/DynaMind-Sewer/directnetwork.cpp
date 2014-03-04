/**
 * @file
 * @author  Chrisitan Urich <christian.urich@gmail.com>
 * @version 1.0
 * @section LICENSE
 *
 * This file is part of DynaMind
 *
 * Copyright (C) 2012  Christian Urich
 
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

#include "directnetwork.h"
#include <algorithm>

DM_DECLARE_NODE_NAME(DirectNetwork, Sewer)
DirectNetwork::DirectNetwork()
{
	std::vector<DM::View> views;
	this->conduits = DM::View("NETWORK", DM::EDGE, DM::MODIFY);
	this->outfalls = DM::View("ENDPOINT", DM::NODE, DM::MODIFY);
	this->startpoints = DM::View("STARTPOINT", DM::NODE, DM::WRITE);

	views.push_back(conduits);
	views.push_back(outfalls);

	this->addData("Vec", views);
}

void DirectNetwork::run() {

	sys = this->getData("Vec");

	std::vector<DM::Node*> outfalls;
	foreach(DM::Component* c, sys->getAllComponentsInView(this->outfalls))
		outfalls.push_back((DM::Node*)c);

	foreach(DM::Component* c, sys->getAllComponentsInView(this->conduits))
	{
		DM::Edge* e = (DM::Edge*)c;
		e->addAttribute("VISITED", 0);
		std::vector<DM::Edge*>& v1 = ConnectionList[e->getStartNode()];
		v1.push_back(e);
		std::vector<DM::Edge*>& v2 = ConnectionList[e->getEndNode()];
		v2.push_back(e);
	}

	foreach(DM::Component* c, sys->getAllComponentsInView(this->outfalls))
		NextEdge((DM::Node*)c);
}

void DirectNetwork::NextEdge(DM::Node* startnode) {
	std::vector<DM::Edge*> connected;
	if (ConnectionList[startnode].size() == 1)
		sys->addComponentToView(startnode, this->startpoints);

	foreach (DM::Edge* e, ConnectionList[startnode]) {
		if (std::find(visitedEdges.begin(), visitedEdges.end(), e) ==   visitedEdges.end()) {
			connected.push_back(e);
		}
	}
	//Diameter Sorted max first
	std::vector<DM::Edge*> connected_new;
	while (connected.size() > 0) 
	{
		DM::Edge* maxID = connected.size() > 0 ? connected[0] : NULL;
		double maxDiameter = 0;
		for (int i = 0; i < connected.size(); i++) 
		{
			double d = connected[i]->getAttribute("DIAMETER")->getDouble();
			if (d > maxDiameter) {
				maxDiameter = d;
				maxID = connected[i];
			}
		}
		connected.erase(std::find(connected.begin(), connected.end(), maxID));
		connected_new.push_back(maxID);
	}

	connected = connected_new;

	foreach (DM::Edge* e, connected) 
	{
		e->getAttribute("VISITED")->setDouble(1);
		if (e->getStartNode() == startnode) 
		{
			e->setStartpoint(e->getEndNode());
			e->setEndpoint(startnode);
		}
		this->visitedEdges.push_back(e);
		NextEdge(e->getStartNode());
	}
}



