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
    this->junctions.addAttribute("number_of_inlets");

    this->catchments = DM::View("CATCHMENT", DM::FACE, DM::READ);
    this->catchments.addAttribute("id_junction");

    this->conduits = DM::View("CONDUIT", DM::EDGE, DM::MODIFY);
    this->inlets = DM::View("INLET", DM::NODE, DM::READ);
    this->conduits.getAttribute("strahler");
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

    std::vector<std::string> condis = sys->getUUIDsOfComponentsInView(conduits);
    std::vector<std::string> inlet_uuids = sys->getUUIDs(inlets);

    std::map<DM::Node*, int> visitedNodes;
    std::map<DM::Node *, DM::Edge*> startNodeMap;

    foreach (std::string s, condis) {
        DM::Edge * c = sys->getEdge(s);
        if (c->getAttribute("existing")->getDouble() >0.01)
            continue;
        startNodeMap[sys->getNode(c->getStartpointName())] = c;
    }

    for (int i = 1; i <= until; i++) {
        foreach (std::string s, inlet_uuids) {
            DM::Component * n = sys->getComponent(s);
            DM::Node * id = sys->getNode(n->getAttribute("JUNCTION")->getLink().uuid);

            while (id != 0 ) {

                DM::Edge * e = startNodeMap[id];
                if (!e) {
                    id = 0;
                    continue;
                }

                if (e->getAttribute("existing")->getDouble() > 0.01){
                    id = 0;
                    DM::Logger(DM::Standard) << "EEEEEEERRRROROORO";
                    continue;
                }
                int currentStrahler = (int) e->getAttribute("strahler")->getDouble();
                if (currentStrahler > i) {
                    id = 0;
                    continue;
                }

                sys->removeComponentFromView(e, conduits);

                std::vector<DM::LinkAttribute> links = id->getAttribute("INLET")->getLinks();
                DM::Attribute junction_link_attribute("INLET");
                foreach (DM::LinkAttribute l, links) {
                    std::string uuid_inlet = l.uuid;
                    std::string uuid_new_junction = e->getEndpointName();

                    DM::Component * inlet = sys->getComponent(uuid_inlet);
                    DM::Attribute attr("JUNCTION");
                    attr.setLink("JUNCTION", uuid_new_junction);
                    inlet->changeAttribute(attr);
                    junction_link_attribute.setLink("INLET", inlet->getUUID());


                }
                DM::Node * new_junction_inlet = sys->getNode(e->getEndpointName());
                foreach(DM::LinkAttribute l_attr, junction_link_attribute.getLinks()) {
                    new_junction_inlet->getAttribute("INLET")->setLink(l_attr.viewname,l_attr.uuid);
                }
                new_junction_inlet->addAttribute("number_of_inlets",new_junction_inlet->getAttribute("INLET")->getLinks().size());

                sys->removeComponentFromView(id, junctions);
                if (visitedNodes[id] > 0) {
                    id = 0;
                    DM::Logger(DM::Debug) << "reviste node";
                    continue;
                }

                visitedNodes[id]+=1;
                id = new_junction_inlet;

            }

        }
    }

    foreach (std::string s, inlet_uuids) {
        DM::Component * inlet = sys->getComponent(s);
        std::string c_uuid = inlet->getAttribute("CATCHMENT")->getLink().uuid;
        std::string j_uuid = inlet->getAttribute("JUNCTION")->getLink().uuid;

         DM::Component * catchment = sys->getComponent(c_uuid);
         DM::Component * junction = sys->getComponent(j_uuid);
         if (!catchment || !junction)
                 continue;

         double id = junction->getAttribute("id")->getDouble();
         catchment->addAttribute("id_catchment", id);
         DM::Logger(DM::Debug) << "Set " << id;

    }

}
