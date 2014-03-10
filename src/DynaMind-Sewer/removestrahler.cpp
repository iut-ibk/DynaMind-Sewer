/**
 * @file
 * @author  Chrisitan Urich <christian.urich@gmail.com>
 * @version 1.0
 * @section LICENSE
 *
 * This file is part of DynaMind
 *
 * Copyright (C) 2011  Christian Urich

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


#include "removestrahler.h"

DM_DECLARE_NODE_NAME(RemoveStrahler, Sewer)
RemoveStrahler::RemoveStrahler()
{
	this->junctions = DM::View("JUNCTION", DM::NODE, DM::MODIFY);
	this->junctions.addAttribute("number_of_inlets", DM::Attribute::DOUBLE, DM::WRITE);

	this->catchments = DM::View("CATCHMENT", DM::FACE, DM::READ);
	this->catchments.addAttribute("id_junction", DM::Attribute::DOUBLE, DM::WRITE);

	this->conduits = DM::View("CONDUIT", DM::EDGE, DM::MODIFY);
	this->inlets = DM::View("INLET", DM::NODE, DM::READ);
	this->conduits.addAttribute("strahler", DM::Attribute::DOUBLE, DM::READ);
	until = 1;
	this->addParameter("StrahlerNumber", DM::INT,&until);

	std::vector<DM::View> data;
	data.push_back(this->junctions);
	data.push_back(this->conduits);
	data.push_back(this->catchments);
	data.push_back(this->inlets);

	this->addData("SEWER", data);
}


void RemoveStrahler::run()
{
	DM::System * sys = this->getData("SEWER");

	std::vector<DM::Component*> inletCmps = sys->getAllComponentsInView(inlets);

	std::map<DM::Node*, int> visitedNodes;
	std::map<DM::Node *, DM::Edge*> startNodeMap;

	foreach(DM::Component* cmp, sys->getAllComponentsInView(conduits))
	{
		DM::Edge * e = (DM::Edge*)cmp;

		if (e->getAttribute("existing")->getDouble() >0.01)
			continue;

		startNodeMap[e->getStartNode()] = e;
	}

	for (int i = 1; i <= until; i++) 
	{
		foreach(DM::Component* n, inletCmps)
		{
			const std::vector<DM::Component*>& links = n->getAttribute("JUNCTION")->getLinkedComponents();
			DM::Node* id = links.size() > 0 ? (DM::Node*)links[0] : NULL;

			while (id != 0 ) 
			{
				DM::Edge * e = startNodeMap[id];
				if (!e)
					break;

				if (e->getAttribute("existing")->getDouble() > 0.01)
				{
					DM::Logger(DM::Standard) << "EEEEEEERRRROROORO";
					break;
				}

				if (e->getAttribute("strahler")->getDouble() > i)
					break;

				sys->removeComponentFromView(e, conduits);

				DM::Node * new_junction_inlet = e->getEndNode();

				DM::Attribute* junction_link_attribute = new_junction_inlet->getAttribute("INLET");

				foreach(DM::Component* inlet, id->getAttribute("INLET")->getLinkedComponents())
				{
					DM::Attribute* linkAttribute = inlet->getAttribute("JUNCTION");
					linkAttribute->clearLinks();
					linkAttribute->addLink(e->getEndNode(), "JUNCTION");

					junction_link_attribute->addLink(inlet, "INLET");
				}

				new_junction_inlet->addAttribute("number_of_inlets", junction_link_attribute->getLinkedComponents().size());

				sys->removeComponentFromView(id, junctions);
				if (visitedNodes[id] > 0) 
				{
					DM::Logger(DM::Debug) << "reviste node";
					break;
				}

				visitedNodes[id]+=1;
				id = new_junction_inlet;
			}
		}
	}

	foreach(DM::Component* inlet, inletCmps)
	{
		std::vector<DM::Component*> cLinks = inlet->getAttribute("CATCHMENT")->getLinkedComponents();
		std::vector<DM::Component*> jLinks = inlet->getAttribute("JUNCTION")->getLinkedComponents();

		if (cLinks.size() == NULL || cLinks[0] == NULL || jLinks.size() == NULL || jLinks[0] == NULL)
			continue;

		double id = jLinks[0]->getAttribute("id")->getDouble();
		jLinks[0]->addAttribute("id_catchment", id);
		DM::Logger(DM::Debug) << "Set " << id;
	}
}
