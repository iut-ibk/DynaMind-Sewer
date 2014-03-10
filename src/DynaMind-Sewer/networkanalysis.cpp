/**
 * @file
 * @author  Chrisitan Urich <christian.urich@gmail.com>
 * @version 1.0
 * @section LICENSE
 *
 * This file is part of DynaMind
 *
 * Copyright (C) 2011-2012  Christian Urich

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
#include "networkanalysis.h"
#include "tbvectordata.h"
#include <QUuid>

DM_DECLARE_NODE_NAME(NetworkAnalysis, Sewer)
NetworkAnalysis::NetworkAnalysis()
{

	this->EdgeName = "CONDUIT";
	this->addParameter("EdgeName", DM::STRING, &this->EdgeName);

	std::vector<DM::View> views;
	views.push_back(DM::View("dummy", DM::EDGE, DM::MODIFY));
	this->addData("City", views);
}

void NetworkAnalysis::init()
{
	std::vector<DM::View> views;
	this->network = DM::View(this->EdgeName, DM::EDGE, DM::READ);
	this->network.addAttribute("strahler", DM::Attribute::DOUBLE, DM::WRITE);

	views.push_back(this->network);
	this->addData("City", views);
}

std::string NetworkAnalysis::getHelpUrl()
{
	return "https://github.com/iut-ibk/DynaMind-Sewer/blob/master/doc/Sewer.md";
}

void NetworkAnalysis::run() {
	DM::System * city = this->getData("City");

	std::vector<DM::Component*> networkCmps = city->getAllComponentsInView(this->network);

	std::map<DM::Node *, std::vector<DM::Edge*> > StartNodeSortedEdges;
	std::map<DM::Node *, std::vector<DM::Edge*> > EndNodeSortedEdges;
	std::map<DM::Node *, std::vector<DM::Edge*> > ConnectedEdges;
	std::vector<DM::Node *> StartNodes;
	std::vector<DM::Node *> nodes;

	//Create Connection List

	foreach(DM::Component* cmp, networkCmps)  
	{
		DM::Edge * e = (DM::Edge*)cmp;
		ConnectedEdges[e->getStartNode()].push_back(e);
		ConnectedEdges[e->getEndNode()].push_back(e);
	}

	int counterChanged = 0;
	DM::Logger(DM::Debug) << "CHANGEDDIR " << counterChanged;


	//Find StartNodes
	foreach(DM::Component* cmp, networkCmps)
	{
		DM::Edge * e = (DM::Edge*)cmp;
		DM::Node * startnode = e->getStartNode();
		DM::Node * endnode = e->getEndNode();

		StartNodeSortedEdges[startnode].push_back(e);
		EndNodeSortedEdges[endnode].push_back(e);
	}

	foreach(DM::Component* cmp, networkCmps)
	{
		DM::Edge * e = (DM::Edge*)cmp;
		e->addAttribute("strahler", 0);
		DM::Node * startnode = e->getStartNode();
		//if (EndNodeSortedEdges[startnode].size() == 0)
		StartNodes.push_back(startnode);
	}
	DM::Logger(DM::Debug) << "Number of StartNodes" << StartNodes.size();

	foreach(DM::Node * StartID, StartNodes) {
		std::vector<DM::Edge*> ids = StartNodeSortedEdges[StartID];
		foreach(DM::Edge * id, ids) {
			id->getAttribute("strahler")->setDouble(1);
		}
	}

	foreach(DM::Node * StartID, StartNodes) {
		std::vector<DM::Edge*> ids = StartNodeSortedEdges[StartID];

		DM::Edge * e = ids[0];
		DM::Node * n2 = e->getEndNode();

		DM::Node *  next = 0;
		if (n2 != StartID)
			next = n2;
		if (next == 0) {
			DM::Logger(DM::Error) << "Something went Wrong while calculation next Point";
			continue;
		}

		int prevStrahler = 1;

		std::vector<DM::Node * > visitedNodes;
		do {
			DM::Node * next_tmp = 0;
			DM::Edge * outgoing_id = 0;
			visitedNodes.push_back(next);
			std::vector<DM::Edge*>  downstreamEdges = StartNodeSortedEdges[next];
			std::vector<DM::Edge*>  upstreamEdges = EndNodeSortedEdges[next];
			//o---o---x
			if (downstreamEdges.size() == 1) 
			{
				DM::Edge * e = downstreamEdges[0];
				DM::Node* endnode = e->getEndNode();
				if (endnode != next) 
				{
					next_tmp = endnode;
					outgoing_id = e;
				}
			}
			//Select Element with max Diameter
			if (downstreamEdges.size() > 1) 
			{
				double maxDia = -1;
				foreach (DM::Edge * e, downstreamEdges) 
				{
					if (maxDia < e->getAttribute("DIAMETER")->getDouble()) 
					{
						maxDia = e->getAttribute("DIAMETER")->getDouble();
						DM::Node* endnode = e->getEndNode();
						if (endnode != next) 
						{
							next_tmp = endnode;
							outgoing_id = e;
						}
					}
				}

			}


			if (next_tmp == 0 || outgoing_id == 0) {
				//DM::Logger(DM::Error) << "Unconnected Parts in Graph 2";
				break;
			}
			int maxStrahlerInNode = -1;
			int strahlerCounter = 0;
			foreach(DM::Edge * up, upstreamEdges) {
				if (e != outgoing_id) {
					if (maxStrahlerInNode < up->getAttribute("strahler")->getDouble()) {
						maxStrahlerInNode = up->getAttribute("strahler")->getDouble();
						strahlerCounter = 1;
					} else if (maxStrahlerInNode == up->getAttribute("strahler")->getDouble()) {
						strahlerCounter++;
					}

				}
			}
			if (strahlerCounter > 1) {
				if ( outgoing_id->getAttribute("strahler")->getDouble() < maxStrahlerInNode+1 )
					outgoing_id->getAttribute("strahler")->setDouble( maxStrahlerInNode+1 );

			} else {
				if (outgoing_id->getAttribute("strahler")->getDouble() < prevStrahler) {
					outgoing_id->getAttribute("strahler")->setDouble( prevStrahler );
				}
			}
			prevStrahler = outgoing_id->getAttribute("strahler")->getDouble();
			next = next_tmp;
			if (next->getAttribute("existing")->getDouble() > 0.01) {
				next = 0;
			}


			foreach (DM::Node * visited,visitedNodes) {
				if (next == visited) {
					DM::Logger(DM::Error) << "Endless Loop";
					next = 0;
				}

			}

		} while (next != 0);

	}
}


