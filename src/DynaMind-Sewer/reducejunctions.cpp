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

DM_DECLARE_NODE_NAME(ReduceJunctions, Sewer)

ReduceJunctions::ReduceJunctions()
{
    junctions = DM::View("JUNCTION", DM::NODE, DM::MODIFY);
    junctions.addAttribute("end_counter");
    conduits = DM::View("CONDUIT", DM::EDGE, DM::MODIFY);
    conduits.getAttribute("Strahler");
    inlets = DM::View("INLET", DM::NODE, DM::READ);
    inlets.getAttribute("JUNCTION");
    conduit_new = DM::View("CONDUIT", DM::EDGE, DM::WRITE);
    catchment = DM::View("CATCHMENT", DM::READ, DM::READ);
    catchment.addAttribute("junction_id");
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
    for (int i = 0; i < numberOfEdges; i++) {
        if (nodes[i] == nodes[i+1])
            continue;
        DM::Edge * e = city->addEdge(nodes[i], nodes[i+1], conduit_new);
        e->addAttribute("Strahler", strahlerNumber);

    }
    DM::Node * lastNode = nodes[numberOfEdges];
    //move all inlet connections to the end and remove junctions
    foreach (DM::Node * n, r_nodes) {

        std::vector<DM::LinkAttribute> links = n->getAttribute("INLET")->getLinks();
        DM::Attribute junction_link_attribute("INLET");
        foreach (DM::LinkAttribute l, links) {
            std::string uuid_inlet = l.uuid;
            DM::Component * inlet = city->getComponent(uuid_inlet);
            DM::Attribute attr("JUNCTION");
            attr.setLink("JUNCTION", lastNode->getUUID());
            inlet->changeAttribute(attr);
            junction_link_attribute.setLink("INLET", inlet->getUUID());
        }

        foreach(DM::LinkAttribute l_attr, junction_link_attribute.getLinks()) {
            lastNode->getAttribute("INLET")->setLink(l_attr.viewname,l_attr.uuid);
        }
        city->removeComponentFromView(n, junctions);
    }

    nodes.clear();
    r_nodes.clear();

}

void ReduceJunctions::run()
{
    DM::System * city = this->getData("city");
    std::vector<std::string> junction_uuids = city->getUUIDs(junctions);
    std::vector<std::string> conduit_uuids = city->getUUIDs(conduits);
    //std::vector<std::string> inlet_uuids = city->getUUIDs(inlets);
    std::vector<std::string> start_uuids;
    std::map<DM::Node*, DM::Edge*> startNodeMap;
    std::map<DM::Node*, int> endPointCounter;

    foreach (std::string c_uuid, conduit_uuids) {
        DM::Edge * c = city->getEdge(c_uuid);
        startNodeMap[city->getNode(c->getStartpointName())] = c;
        endPointCounter[city->getNode(c->getEndpointName())] =  endPointCounter[city->getNode(c->getEndpointName())] + 1;

        //Remove old
        city->removeComponentFromView(c, conduits);
    }
    foreach (std::string j_uuid, junction_uuids) {
        DM::Node * jun = city->getNode(j_uuid);
        int counter = endPointCounter[jun];
        jun->addAttribute("end_counter",(double) counter);
        if (counter==0)
            start_uuids.push_back(jun->getUUID());

    }

    //Only fix point is change in strahler Number
    foreach (std::string s_uuid, start_uuids) {
        DM::Node * current_j = city->getNode(s_uuid);
        if (!current_j)
            continue;
        std::vector<DM::Node*> new_conduits;
        std::vector<DM::Node*> removed_junctions;
        new_conduits.push_back(current_j);
        int currrentStrahler = -1;

        while (current_j) {
            DM::Edge * c = startNodeMap[current_j];
            if (!c || current_j->getAttribute("visited")->getDouble() > 0.01) {
                //last node
                new_conduits.push_back(current_j);
                this->createJunctions(city, new_conduits, removed_junctions, currrentStrahler);
                current_j = 0;
                continue;
            }
            if (currrentStrahler == -1) {
                currrentStrahler = (int) c->getAttribute("Strahler")->getDouble();
            }

            current_j->addAttribute("visited",1);
            current_j = city->getNode(c->getEndpointName());
            int connectingNodes = endPointCounter[current_j];

            if ( connectingNodes == 1) {
                removed_junctions.push_back(current_j);
                continue;
            }

            new_conduits.push_back(current_j);
            this->createJunctions(city, new_conduits, removed_junctions, currrentStrahler);

            //current node is start for next conduit
            new_conduits.push_back(current_j);
            currrentStrahler =  (int) c->getAttribute("Strahler")->getDouble();


        }
    }
    //Update Catchment UUID
    std::vector<std::string> catchment_uuids = city->getUUIDs(catchment);
    foreach (std::string uuid, catchment_uuids) {
        DM::Component * c = city->getComponent(uuid);
        DM::Component * inlet = city->getComponent(c->getAttribute("INLET")->getLink().uuid);
        if (!inlet)
            continue;
        DM::Component * jun = city->getComponent(inlet->getAttribute("JUNCTION")->getLink().uuid);
        if (!jun)
            continue;
        c->addAttribute("junction_id", jun->getAttribute("id")->getDouble());

    }




}
