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
#include "reconstructparameter.h"

DM_DECLARE_NODE_NAME(ReconstructParameter, Sewer)
ReconstructParameter::ReconstructParameter()
{
	this->network = DM::View("CONDUIT", DM::EDGE, DM::READ);
	this->network.addAttribute("CONYEAR", DM::Attribute::DOUBLE, DM::WRITE);
	this->network.addAttribute("PLAN_YEAR", DM::Attribute::DOUBLE, DM::WRITE);
	std::vector<DM::View> views;
	views.push_back(this->network);
	this->addData("City", views);
}

void ReconstructParameter::run() {
	DM::System * city = this->getData("City");

	std::vector<DM::Component*> networkCmps = city->getAllComponentsInView(this->network);
	double offset = 10;

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
		StartNodeSortedEdges[e->getStartNode()].push_back(e);
		StartNodeSortedEdges[e->getEndNode()].push_back(e);
	}

	foreach(DM::Component* cmp, networkCmps)
	{
		DM::Edge * e = (DM::Edge*)cmp;
		e->addAttribute("CONYEAR", e->getAttribute("PLAN_YEAR")->getDouble());
		StartNodes.push_back(e->getStartNode());
	}
	DM::Logger(DM::Debug) << "Number of StartNodes" << StartNodes.size();


	foreach(DM::Node * StartID, StartNodes) 
	{
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

		int PLAN_DATE = (int) e->getAttribute("PLAN_YEAR")->getDouble();
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
				if (e->getEndNode() != next) 
				{
					next_tmp = e->getEndNode();
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
						maxDia =  e->getAttribute("DIAMETER")->getDouble();
						if (e->getEndNode() != next)
						{
							next_tmp = e->getEndNode();
							outgoing_id = e;
						}
					}
				}
			}

			if (next_tmp == 0 || outgoing_id == 0)
				break;

			int constructionAgeOutGoing =  (int) outgoing_id->getAttribute("PLAN_YEAR")->getDouble();
			int conYear =  (int) outgoing_id->getAttribute("CONYEAR")->getDouble();


			if (PLAN_DATE > constructionAgeOutGoing || PLAN_DATE == 0)
				if (constructionAgeOutGoing != 0)
					PLAN_DATE = constructionAgeOutGoing;

			if  (PLAN_DATE < conYear && PLAN_DATE != 0 )
				outgoing_id->getAttribute("CONYEAR")->setDouble( PLAN_DATE );
			if (conYear == 0 && PLAN_DATE != 0)
				outgoing_id->getAttribute("CONYEAR")->setDouble( PLAN_DATE );


			next = next_tmp;

			foreach (DM::Node * visited,visitedNodes)
				if (next == visited)
					next = 0;

		} while (next != 0);

	}
}

string ReconstructParameter::getHelpUrl()
{
	return "https://docs.google.com/document/d/10Ftd_63V442jVI-M6BQPPjBr3BTfV0jx3OKXDvfpIgc/edit";
}
