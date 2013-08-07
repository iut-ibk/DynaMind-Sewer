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
    this->network.addAttribute("strahler");

    views.push_back(this->network);
    this->addData("City", views);
}

std::string NetworkAnalysis::getHelpUrl()
{
    return "https://github.com/iut-ibk/DynaMind-Sewer/blob/master/doc/Sewer.md";
}

void NetworkAnalysis::run() {
    DM::System * city = this->getData("City");

    std::vector<std::string> names = city->getUUIDsOfComponentsInView(this->network);

    std::map<DM::Node *, std::vector<DM::Edge*> > StartNodeSortedEdges;
    std::map<DM::Node *, std::vector<DM::Edge*> > EndNodeSortedEdges;
    std::map<DM::Node *, std::vector<DM::Edge*> > ConnectedEdges;
    std::vector<DM::Node *> StartNodes;
    std::vector<DM::Node *> nodes;

    //Create Connection List

    foreach(std::string name , names)  {
        DM::Edge * e = city->getEdge(name);
        DM::Node * startnode = city->getNode(e->getStartpointName());
        std::vector<DM::Edge*> v = ConnectedEdges[startnode];
        v.push_back(e);
        ConnectedEdges[startnode] = v;
        nodes.push_back(city->getNode(e->getStartpointName()));

        DM::Node * Endnode = city->getNode(e->getEndpointName());
        v = ConnectedEdges[Endnode];
        v.push_back(e);
        ConnectedEdges[Endnode] = v;
        nodes.push_back(city->getNode(e->getEndpointName()));
    }

    int counterChanged = 0;
    DM::Logger(DM::Debug) << "CHANGEDDIR " << counterChanged;


    //Find StartNodes
    foreach(std::string name , names)  {
        DM::Edge * e = city->getEdge(name);
        DM::Node * startnode = city->getNode(e->getStartpointName());
        std::vector<DM::Edge*> v = StartNodeSortedEdges[startnode];
        v.push_back(e);
        StartNodeSortedEdges[startnode] = v;


        DM::Node * Endnode = city->getNode(e->getEndpointName());
        v = EndNodeSortedEdges[Endnode];
        v.push_back(e);
        EndNodeSortedEdges[Endnode] = v;

    }

    foreach(std::string name , names)  {
        DM::Edge * e = city->getEdge(name);
        e->addAttribute("strahler", 0);
        DM::Node * startnode = city->getNode(e->getStartpointName());
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
        DM::Node * n2 = city->getNode(e->getEndpointName());

        DM::Node *  nextID = 0;
        if (n2 != StartID)
            nextID = n2;
        if (nextID == 0) {
            DM::Logger(DM::Error) << "Something went Wrong while calculation next Point";
            continue;
        }

        int prevStrahler = 1;

        std::vector<DM::Node * > visitedNodes;
        do {
            DM::Node * nextid_tmp = 0;
            DM::Edge * outgoing_id = 0;
            visitedNodes.push_back(nextID);
            std::vector<DM::Edge*>  downstreamEdges = StartNodeSortedEdges[nextID];
            std::vector<DM::Edge*>  upstreamEdges = EndNodeSortedEdges[nextID];
            //o---o---x
            if (downstreamEdges.size() == 1) {
                DM::Edge * e = downstreamEdges[0];
                if (city->getNode(e->getEndpointName()) != nextID) {
                    nextid_tmp = city->getNode(e->getEndpointName());
                    outgoing_id = e;
                }
            }
            //Select Element with max Diameter
            if (downstreamEdges.size() > 1) {
                double maxDia = -1;
                foreach (DM::Edge * e, downstreamEdges) {
                    if (maxDia < e->getAttribute("DIAMETER")->getDouble()) {
                        maxDia =  e->getAttribute("DIAMETER")->getDouble();
                        if (city->getNode(e->getEndpointName()) != nextID) {
                            nextid_tmp = city->getNode(e->getEndpointName());
                            outgoing_id = e;
                        }
                    }
                }

            }


            if (nextid_tmp == 0 || outgoing_id == 0) {
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
            nextID = nextid_tmp;
            if (nextID->getAttribute("existing")->getDouble() > 0.01) {
                nextID = 0;
            }


            foreach (DM::Node * visited,visitedNodes) {
                if (nextID == visited) {
                    DM::Logger(DM::Error) << "Endless Loop";
                    nextID = 0;
                }

            }

        } while (nextID != 0);

    }
}


