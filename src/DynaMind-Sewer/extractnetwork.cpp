/**
 * @file
 * @author  Chrisitan Urich <christian.urich@gmail.com>
 * @version 1.0
 * @section LICENSE
 *
 * This file is part of VIBe2
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
#include "extractnetwork.h"
#include <sstream>
#include "csg_s_operations.h"
#include "tbvectordata.h"
#include <dmgeometry.h>

DM_DECLARE_NODE_NAME(ExtractNetwork, Sewer)
void ExtractNetwork::AgentExtraxtor::run() {

    this->path.clear();
    for (int i = 0; i < this->steps; i++) {
        this->path.push_back(currentPos);

        double currentH = this->Topology->getCell(currentPos.x, currentPos.y);
        double hcurrent = currentPos.h;
        //Influence Topology
        ConnectivityField->getMoorNeighbourhood(neigh, currentPos.x,currentPos.y);
        int index = GenerateSewerNetwork::indexOfMinValue(neigh);
        for (int i = 0; i < 9; i++) {

            if (index == i) {
                decissionVector[i] = 1;
            } else {
                decissionVector[i] = 0;
            }
        }

        //Forbidden Areas
        ForbiddenAreas->getMoorNeighbourhood(neigh, currentPos.x,currentPos.y);
        for (int i = 0; i < 9; i++) {
            decissionVector[i]*=neigh[i];
        }


        if (index == -1) {
            this->alive = false;
            break;
        }

        int direction = index;
        this->currentPos.x+=csg_s::csg_s_operations::returnPositionX(direction);
        this->currentPos.y+=csg_s::csg_s_operations::returnPositionY(direction);

        double deltaH = currentH - this->Topology->getCell(currentPos.x, currentPos.y);

        if (deltaH > 0) {
            currentPos.h = hcurrent - deltaH;
            if (currentPos.h < 3) {
                currentPos.h = 3;
            }
        } else {
            currentPos.h=hcurrent-deltaH;
        }

        if (currentPos.x < 0 || currentPos.y < 0 || currentPos.x > MarkPath->getWidth()-2 || currentPos.y >  MarkPath->getHeight()-2) {
            this->alive = false;
            break;


        }
        //if (Goals->getCell(currentPos.x, currentPos.y ) > 0 || this->MarkPath->getCell(currentPos.x, currentPos.y) > 0) {
        if (Goals->getCell(currentPos.x, currentPos.y ) > 0) {
            if (currentPos.h < Hmin) {
                this->alive = false;
                this->successful = true;
                this->path.push_back(currentPos);
                break;
            }
        }


    }

    this->alive = false;
}

ExtractNetwork::ExtractNetwork()
{
    this->steps = 1000;
    this->ConduitLength = 200;
    this->Hmin = 3.1;


    this->addParameter("Steps", DM::LONG, & this->steps);
    this->addParameter("MaxDeph", DM::DOUBLE, &this->Hmin);
    this->addParameter("ConduitLength", DM::DOUBLE, &this->ConduitLength);

    confield = DM::View("ConnectivityField_in", DM::RASTERDATA, DM::READ);
    path =DM::View("Path", DM::RASTERDATA, DM::READ);
    forb = DM::View("ForbiddenAreas", DM::RASTERDATA, DM::READ);
    goals = DM::View("Goals", DM::RASTERDATA, DM::READ);

    std::vector<DM::View> sewerGen;
    sewerGen.push_back(confield);
    sewerGen.push_back(path);
    sewerGen.push_back(forb);
    sewerGen.push_back(goals);
    this->addData("sewerGeneration", sewerGen);

    std::vector<DM::View> city;
    topo = DM::View("Topology", DM::RASTERDATA, DM::READ);
    Conduits = DM::View("CONDUIT", DM::EDGE, DM::WRITE);
    Conduits.addAttribute("New");
    Inlets= DM::View("INLET",  DM::NODE, DM::READ);
    Inlets.modifyAttribute("New");
    Inlets.addAttribute("Used");
    Inlets.addAttribute("Connected");
    Inlets.addLinks("JUNCTION", Junction);
    Junction= DM::View("JUNCTION",  DM::NODE, DM::WRITE);
    Junction.addAttribute("D");
    Junction.addAttribute("Z");
    Junction.addAttribute("id");
    EndPoint = DM::View("OUTLET", DM::NODE, DM::READ);

    city.push_back(topo);
    city.push_back(Conduits);
    city.push_back(Inlets);
    city.push_back(Junction);
    /** todo easy way out, needs a proper bugfix */
    //city.push_back(EndPoint);
    offset = 10;
    this->addParameter("Offset", DM::DOUBLE, &offset);

    this->addData("City", city);

}
void ExtractNetwork::run() {
    ///nodeListToCompare.clear();
    this->city = this->getData("City");

    this->ConnectivityField = this->getRasterData("sewerGeneration", confield);
    this->Goals = this->getRasterData("sewerGeneration", goals);
    this->Path =  this->getRasterData("sewerGeneration", path);
    this->ForbiddenAreas = this->getRasterData("sewerGeneration", forb);
    this->Topology = this->getRasterData("City", topo);

    long width = this->ConnectivityField->getWidth();
    long height = this->ConnectivityField->getHeight();
    offsetX = this->ConnectivityField->getXOffset();
    offsetY = this->ConnectivityField->getYOffset();
    double cellSizeX = this->ConnectivityField->getCellSizeX();
    double cellSizeY = this->ConnectivityField->getCellSizeY();

    cellsize = cellSizeX;
    this->Path->setSize(width, height, cellSizeX,cellSizeY,offsetX,offsetY);
    this->Path->clear();

    std::vector<AgentExtraxtor * > agents;

    std::vector<DM::Node*> StartPos;
    std::vector<std::string> uuid_inlets = city->getUUIDsOfComponentsInView(Inlets);
    int counter = 0;
    foreach (std::string inlet, uuid_inlets)  {
        counter++;
        //Logger(Standard) << counter << "/" << uuid_inlets.size();
        DM::Node * n = city->getNode(inlet);
        std::string ID_CA = n->getAttribute("CATCHMENT")->getLink().uuid;
        DM::Face * catchment = city->getFace(ID_CA);
        //Just For Now
        n->changeAttribute("New", 0);
        if (catchment->getAttribute("Active")->getDouble() > 0.1) {
            n->changeAttribute("New", 1);
            n->addAttribute("BuildYear", catchment->getAttribute("BuildYear")->getDouble());
            StartPos.push_back(n);
        }
    }

    //Create Agents

    foreach(DM::Node * p, StartPos) {
        long x = (long) (p->getX() - offsetX)/cellSizeX;
        long y = (long) (p->getY() - offsetY)/cellSizeY;
        AgentExtraxtor * a = new AgentExtraxtor(GenerateSewerNetwork::Pos(x,y));
        a->startNode = p;
        a->Topology = this->Topology;
        a->MarkPath = this->Path;
        a->ConnectivityField = this->ConnectivityField;
        a->ForbiddenAreas = this->ForbiddenAreas;
        a->Goals = this->Goals;
        a->AttractionTopology = 0;
        a->AttractionConnectivity =0;
        a->steps = this->steps;
        a->Hmin = this->Hmin;
        agents.push_back(a);

    }
    Logger(Debug) << "Number of agents" << agents.size();
    long successfulAgents = 0;
    double multiplier;
    multiplier = this->ConnectivityField->getCellSizeX();
    //Extract Conduits

    Logger(Debug) << "Done with the Agent Based Model";

    mforeach(Component *c, city->getAllComponentsInView(Conduits))
            c->changeAttribute("New", 1);

    Logger(Debug) << "multiplier" << multiplier;

    std::vector<std::vector<Node> > Points_After_Agent_Extraction;
    //Extract Netoworks

    std::vector<std::string> endNodeList;
    DM::SpatialNodeHashMap existing_nodes(city, 1000, true, Junction);

    Logger(Standard) << "Junctions" << existing_nodes.size();
    for (int j = 0; j < agents.size(); j++) {
        AgentExtraxtor * a = agents[j];
        if (a->alive) {
            a->run();
        } else {
            Logger(Debug) << "Agent Path Length" << a->path.size();
        }
        if (a->successful) {
            successfulAgents++;
            std::vector<Node> points_for_total;
            for (int i = 0; i < a->path.size(); i++) {
                this->Path->setCell(a->path[i].x, a->path[i].y, 1);


                //Find connecting Node
                if (i == a->path.size()-1) {
                    DM::Node * n = existing_nodes.findNode(a->path[i].x * multiplier + offset + this->offsetX, a->path[i].y * multiplier + offset + this->offsetY, cellsize-0.001);
                    if (!n){
                        Logger(Error) << "Couldn't find endnode";
                        continue;
                    }
                    points_for_total.push_back(Node(n->getX(), n->getY(), a->path[i].h));
                    if (find(endNodeList.begin(), endNodeList.end(), n->getUUID()) == endNodeList.end()) {
                        endNodeList.push_back(n->getUUID());
                    }
                } else

                    points_for_total.push_back(Node(a->path[i].x * multiplier + offset + this->offsetX, a->path[i].y * multiplier + offset + this->offsetY,a->path[i].h));
            }
            Logger(Debug) << "Successful Agent Path Length" << a->path.size();
            Points_After_Agent_Extraction.push_back(points_for_total);
            //Set Inlet Point to Used

            Node * start = a->startNode;
            start->changeAttribute("Used",1);
            start->changeAttribute("New", 0);
            start->changeAttribute("Connected", 1);
            Logger(Standard) << "Existing Links " << start->getAttribute("JUNCTION")->getLinks().size();

            std::vector<LinkAttribute> links;
            start->getAttribute("JUNCTION")->setLinks(links);
            start->getAttribute("INLET")->setLinks(links);

            start->getAttribute("JUNCTION")->setLink("JUNCTION", start->getUUID());
            start->getAttribute("INLET")->setLink("INLET", start->getUUID());
        }
    }

    Logger(DM::Standard) << "Successful " << agents.size() << "/" << successfulAgents;


    for (unsigned int j = 0; j < agents.size(); j++) {
        delete agents[j];
    }

    agents.clear();


    Logger(Debug) << "Done with the agents Junctions";

    std::vector<std::vector<DM::Node> > PointsToPlace = Points_After_Agent_Extraction;

    //Export Inlets
    Logger(Debug) << "Export Junctions";
    std::vector<std::vector<Node *> > Points_For_Conduits;


    DM::SpatialNodeHashMap spnh(city, 100, false);
    counter = 0;
    foreach (std::string uuid, endNodeList) {
        counter++;
        Logger(Debug)  << "export " << counter << "/" << endNodeList.size();
        spnh.addNodeToSpatialNodeHashMap(city->getNode(uuid));
    }

    DM::SpatialNodeHashMap spnh_inlets(city, 100, true, Inlets);

    Logger(Debug) << "Start with points to place";
    foreach (std::vector<Node> pl, PointsToPlace) {
        Node * n = 0;
        n  = spnh_inlets.findNode( pl[0].getX(), pl[0].getY(), offset);

        if (n == 0) {
            n = city->addNode(pl[0], Junction);
            Logger(Warning) << "Starting point is not an inlet";
        }

        city->addComponentToView(n, Junction);
        std::vector<DM::Node * > nl;
        nl.push_back(n);
        Points_For_Conduits.push_back(nl);
    }

    Logger(Debug) << "Done with points to place";


    //Point extraction starts from 1
    std::vector<std::vector<Node *> > Points_For_Conduits_tmp;
    for(int j = 0; j < PointsToPlace.size(); j++){
        std::vector<Node> pl = PointsToPlace[j];
        std::vector<DM::Node * > nl = Points_For_Conduits[j];
        for (int i = 1; i < pl.size(); i++) {
            bool foundNode = false;
            if (spnh.findNode(pl[i].getX(), pl[i].getY(), cellsize-0.0001)) {
                foundNode = true;
            }
            DM::Node * n = spnh.addNode(pl[i].getX(), pl[i].getY(), pl[i].getZ(), cellsize-0.0001, Junction);
            if (n->getAttribute("D")->getDouble() > pl[i].getZ()) {
                n->setZ(n->getAttribute("D")->getDouble());
            }
            n->setZ(n->getAttribute("D")->getDouble());

            if (n->getUUID() == this->EndNode){
                Logger(Debug) << "EndNode found";
            }
            n->changeAttribute("D", pl[i].getZ());
            nl.push_back(n);
            if (foundNode && i != 0)
                break;
        }
        Points_For_Conduits_tmp.push_back(nl);
    }
    Points_For_Conduits = Points_For_Conduits_tmp;
    Logger(Debug) << "Done with point extraction";
    foreach (std::vector<Node*> pl, Points_For_Conduits) {
        for (int i = 1; i < pl.size(); i++) {
            if (TBVectorData::getEdge(this->city, Conduits, pl[i-1], pl[i]) != 0)
                continue;

            DM::Edge * e = this->city->addEdge(pl[i-1], pl[i], Conduits);
            e->addAttribute("New", 0);
            if (pl[i-1]->isInView(this->Inlets)) {
                DM::Node * n = pl[i-1];
                e->addAttribute("PLAN_DATE",  n->getAttribute("BuildYear")->getDouble());
            }

        }
    }
    int id = 1;
    Logger(Debug) << "Done with adding plan date extraction";
    std::vector<std::string> JunctionNames =  this->city->getUUIDsOfComponentsInView(Junction);
    counter = 0;
    foreach (std::string name, JunctionNames) {
        counter++;
        Logger(Debug) << JunctionNames.size() << "/" << counter;
        DM::Node * n =this->city->getNode(name);
        int x = (n->getX() - offsetX)/cellSizeX;
        int y = (n->getY() - offsetY)/cellSizeY;
        Logger(Debug) << "start topo";
        double z = this->Topology->getCell(x,y);
        Logger(Debug) << "end topo";

        if (n->getAttribute("existing")->getDouble() >0.01)
            continue;

        n->changeAttribute("Z", z);
        n->addAttribute("id", id++);
    }

    Logger(Debug) << "Done with adding junctions date extraction";
    std::vector<std::string> EndNames =  this->city->getUUIDsOfComponentsInView(EndPoint);
    counter = 0;
    foreach (std::string name, EndNames) {
        counter++;
        Logger(Debug) << EndNames.size() << "/" << counter;
        DM::Node * n =this->city->getNode(name);
        if (n->getAttribute("existing")->getDouble() >0.01)
            continue;
        int x = (n->getX() -  offsetX)/cellSizeX;
        int y = (n->getY() - offsetY)/cellSizeY;
        double z = this->Topology->getCell(x,y);
        n->changeAttribute("Z", z-3);
    }
    Logger(Debug) << "Start smoothing";
    smoothNetwork();

}

//TODO: Smooth Networks
void ExtractNetwork::smoothNetwork() {
    std::map<DM::Node *, std::vector<DM::Edge*> > StartNodeSortedEdges;
    std::map<DM::Node *, std::vector<DM::Edge*> > EndNodeSortedEdges;
    std::map<DM::Node *, std::vector<DM::Edge*> > ConnectedEdges;
    std::vector<std::string> InletNames;
    InletNames = city->getUUIDsOfComponentsInView(this->Inlets);
    std::vector<std::string> ConduitNames;
    ConduitNames = city->getUUIDsOfComponentsInView(this->Conduits);
    //Create Connection List
    foreach(std::string name , ConduitNames)  {
        DM::Edge * e = city->getEdge(name);
        DM::Node * startnode = city->getNode(e->getStartpointName());
        std::vector<DM::Edge*> v = ConnectedEdges[startnode];
        v.push_back(e);
        ConnectedEdges[startnode] = v;

        DM::Node * Endnode = city->getNode(e->getEndpointName());
        v = ConnectedEdges[Endnode];
        v.push_back(e);
        ConnectedEdges[Endnode] = v;
    }


    foreach(std::string name , ConduitNames)  {
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

    Logger(Debug) << "Start Smoothing Netowrk";
    //find WWTP


    double internalcounter = 0;
    Logger(Debug) << "Endpoint Thing";

    std::vector<DM::Node *> EndPoints;

    std::vector<std::string> endPoints =  this->city->getUUIDsOfComponentsInView(this->EndPoint);

    foreach (std::string n,endPoints) {
        EndPoints.push_back(this->city->getNode(n));
    }
    while (EndPoints.size() > 0) {
        std::vector<DM::Node*> new_endPointList;
        foreach (DM::Node * p, EndPoints) {
            if (internalcounter > 100000) {
                Logger(Error) << "Endless loop";
                return;
            }
            internalcounter++;

            double z_lower = p->getAttribute("Z")->getDouble() - p->getAttribute("D")->getDouble();

            std::vector<DM::Edge*> upstreamconnections = EndNodeSortedEdges[p];

            foreach (DM::Edge * e, upstreamconnections)  {

                DM::Node * p_upper = this->city->getNode(e->getStartpointName());
                double z_upper = p_upper->getAttribute("Z")->getDouble() - p_upper->getAttribute("D")->getDouble();
                if (z_lower >= z_upper) {
                    z_upper = z_lower + 0.00005;
                }
                internalcounter++;
                if (internalcounter > 100000) {
                    Logger(Error) << "Endless loop";
                    return;
                }
                p_upper->changeAttribute("D", p_upper->getAttribute("Z")->getDouble() - z_upper);
                new_endPointList.push_back(p_upper);
            }
        }
        internalcounter++;
        if (internalcounter > 100000) {
            Logger(Error) << "Endless loop";
            return;
        }
        EndPoints = new_endPointList;
    }
    Logger(Debug) << "Done Endpoint Thing";



}
