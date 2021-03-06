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
	double noData = this->Topology->getNoValue();
	this->path.clear();
	for (int i = 0; i < this->steps; i++) {
		this->path.push_back(currentPos);

		double currentH = this->Topology->getCell(currentPos.x, currentPos.y);
		double hcurrent = currentPos.h;
		//Influence Topology
		ConnectivityField->getMoorNeighbourhood(neigh, currentPos.x,currentPos.y);
		int index = GenerateSewerNetwork::indexOfMinValue(neigh, noData);
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

		if (currentPos.x < 0 || currentPos.y < 0 || currentPos.x > Topology->getWidth()-2 || currentPos.y >  Topology->getHeight()-2) {
			this->alive = false;
			break;


		}
		if (Goals->getCell(currentPos.x, currentPos.y )> 0.01 && currentPos.h <= this->Hmin) {
			this->alive = false;
			this->successful = true;
			this->path.push_back(currentPos);
			break;

		}


	}

	this->alive = false;
}

ExtractNetwork::ExtractNetwork()
{
	this->steps = 1000;
	this->Hmin = 3.1;
	smooth = true;


	this->addParameter("Steps", DM::LONG, & this->steps);
	this->addParameter("MaxDeph", DM::DOUBLE, &this->Hmin);
	this->addParameter("Smooth", DM::BOOL, &this->smooth);

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
	Conduits.addAttribute("New", DM::Attribute::DOUBLE, DM::WRITE);
	Inlets= DM::View("INLET",  DM::NODE, DM::READ);
	Inlets.addAttribute("New", DM::Attribute::DOUBLE, DM::MODIFY);
	Inlets.addAttribute("Used", DM::Attribute::DOUBLE, DM::WRITE);
	Inlets.addAttribute("Connected", DM::Attribute::DOUBLE, DM::WRITE);
	Inlets.addAttribute("success", DM::Attribute::DOUBLE, DM::READ);
	Inlets.addAttribute("JUNCTION", "Junction", DM::WRITE);
	Junction= DM::View("JUNCTION",  DM::NODE, DM::WRITE);
	Junction.addAttribute("D", DM::Attribute::DOUBLE, DM::WRITE);
	Junction.addAttribute("Z", DM::Attribute::DOUBLE, DM::WRITE);
	Junction.addAttribute("invert_elevation", DM::Attribute::DOUBLE, DM::WRITE);
	Junction.addAttribute("id", DM::Attribute::DOUBLE, DM::WRITE);
	EndPoint = DM::View("OUTLET", DM::NODE, DM::READ);

	city.push_back(topo);
	city.push_back(Conduits);
	city.push_back(Inlets);
	city.push_back(Junction);
	/** todo easy way out, needs a proper bugfix */
	//city.push_back(EndPoint);
	offset = 10;
	//this->addParameter("Offset", DM::DOUBLE, &offset);

	this->addData("City", city);

}
void ExtractNetwork::run() {
	this->city = this->getData("City");

	this->ConnectivityField = this->getRasterData("sewerGeneration", confield);
	this->Goals = this->getRasterData("sewerGeneration", goals);
	this->ForbiddenAreas = this->getRasterData("sewerGeneration", forb);
	this->Topology = this->getRasterData("City", topo);

	offsetX = this->ConnectivityField->getXOffset();
	offsetY = this->ConnectivityField->getYOffset();
	double cellSizeX = this->ConnectivityField->getCellSizeX();
	double cellSizeY = this->ConnectivityField->getCellSizeY();
	cellsize = cellSizeX;
	offset = this->cellsize/2;


	std::vector<AgentExtraxtor * > agents;

	std::vector<DM::Node*> StartPos;
	int counter = 0;
	foreach(DM::Component* cmp, city->getAllComponentsInView(Inlets))  
	{
		counter++;
		//Logger(Standard) << counter << "/" << uuid_inlets.size();
		DM::Node * n = (DM::Node*)cmp;
		DM::Face * catchment = (DM::Face*)n->getAttribute("CATCHMENT")->getLinkedComponents()[0];
		//Just For Now
		if ( n->getAttribute("success")->getDouble()   > 0.01) {
			n->addAttribute("BuildYear", catchment->getAttribute("BuildYear")->getDouble());
			StartPos.push_back(n);
		}
	}

	//Create Agents

	foreach(DM::Node * p, StartPos) 
	{
		long x = (long) (p->getX() - offsetX)/cellSizeX;
		long y = (long) (p->getY() - offsetY)/cellSizeY;
		AgentExtraxtor * a = new AgentExtraxtor(GenerateSewerNetwork::Pos(x,y));
		a->startNode = p;
		a->Topology = this->Topology;
		a->MarkPath = 0;
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

	foreach(Component *c, city->getAllComponentsInView(Conduits))
			c->changeAttribute("New", 1);

	Logger(Debug) << "multiplier" << multiplier;

	std::vector<std::vector<Node> > Points_After_Agent_Extraction;
	//Extract Netoworks

	std::vector<DM::Node*> endNodeList;
	DM::SpatialNodeHashMap existing_nodes(city, 1000, true, Junction);

	Logger(Standard) << "Junctions" << existing_nodes.size();
	for (int j = 0; j < agents.size(); j++) 
	{
		AgentExtraxtor * a = agents[j];
		if (a->alive)
			a->run();
		else
			Logger(Debug) << "Agent Path Length" << a->path.size();

		if (a->successful) 
		{
			successfulAgents++;
			std::vector<Node> points_for_total;
			for (int i = 0; i < a->path.size(); i++) 
			{
				if (i == a->path.size()-1) 
				{
					DM::Node * n = existing_nodes.findNode(a->path[i].x * multiplier + offset + this->offsetX, a->path[i].y * multiplier + offset + this->offsetY, cellsize-0.001);
					if (!n){
						Logger(Error) << "Couldn't find endnode";
						continue;
					}
					n->getAttribute("D")->setDouble(-1);
					points_for_total.push_back(Node(n->getX(), n->getY(), a->path[i].h));

					if (!vector_contains(&endNodeList, n))
						endNodeList.push_back(n);

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
			Logger(Debug) << "Existing Links " << start->getAttribute("JUNCTION")->getLinkedComponents().size();

			start->getAttribute("JUNCTION")->clearLinks();
			start->getAttribute("INLET")->clearLinks();

			start->getAttribute("JUNCTION")->addLink(start, "JUNCTION");
			start->getAttribute("INLET")->addLink(start, "INLET");
		}
	}

	Logger(DM::Standard) << "Successful " << successfulAgents << "/" <<  agents.size();


	for (unsigned int j = 0; j < agents.size(); j++)
		delete agents[j];

	agents.clear();


	Logger(Debug) << "Done with the agents Junctions";

	std::vector<std::vector<DM::Node> > PointsToPlace = Points_After_Agent_Extraction;

	//Export Inlets
	Logger(Debug) << "Export Junctions";
	std::vector<std::vector<Node *> > Points_For_Conduits;


	DM::SpatialNodeHashMap spnh(city, 100, false);
	counter = 0;
	foreach (DM::Node* n, endNodeList) 
	{
		counter++;
		Logger(Debug)  << "export " << counter << "/" << endNodeList.size();
		spnh.addNodeToSpatialNodeHashMap(n);
	}

	DM::SpatialNodeHashMap spnh_inlets(city, 100, true, Inlets);

	Logger(Debug) << "Start with points to place";
	foreach (std::vector<Node> pl, PointsToPlace) {
		Node * n = 0;
		n  = spnh_inlets.findNode( pl[0].getX(), pl[0].getY(), offset);

		if (n == 0) {
			n = city->addNode(pl[0], Junction);
			Logger(Warning) << "Starting point is not an inlet " << pl[0].getX() << "/" << pl[0].getY();
		}

		city->addComponentToView(n, Junction);
		std::vector<DM::Node * > nl;
		nl.push_back(n);
		Points_For_Conduits.push_back(nl);
	}

	Logger(Debug) << "Done with points to place";


	//Point extraction starts from 1
	//goes through all strains extracted from the agent based model.
	//and creates new map for the conduit generation.
	std::vector<std::vector<Node *> > Points_For_Conduits_tmp;
	for(int j = 0; j < PointsToPlace.size(); j++){
		std::vector<Node> pl = PointsToPlace[j];
		bool foundNode = false;
		std::vector<DM::Node * > nl = Points_For_Conduits[j];
		bool placeNode = true;
		for (int i = 1; i < pl.size(); i++) {
			if (foundNode) placeNode = false;

			if (spnh.findNode(pl[i].getX(), pl[i].getY(), cellsize-0.0001)) {
				foundNode = true;
			}
			DM::Node * n = spnh.addNode(pl[i].getX(), pl[i].getY(), pl[i].getZ(), cellsize-0.0001, Junction);
			if (n->getAttribute("D")->getDouble() < pl[i].getZ()) {
				n->getAttribute("D")->setDouble( pl[i].getZ());
			}

			if (n->getUUID() == this->EndNode){
				Logger(Debug) << "EndNode found";
			}

			if (placeNode)
				nl.push_back(n);

		}
		Points_For_Conduits_tmp.push_back(nl);
	}
	Points_For_Conduits = Points_For_Conduits_tmp;
	Logger(Debug) << "Done with point extraction";

	std::vector<DM::Component*> inletComponents = city->getAllComponentsInView(this->Inlets);

	foreach (std::vector<Node*> pl, Points_For_Conduits) 
	{
		for (int i = 1; i < pl.size(); i++) {
			if (TBVectorData::getEdge(this->city, Conduits, pl[i-1], pl[i]) != 0)
				continue;

			DM::Edge * e = this->city->addEdge(pl[i-1], pl[i], Conduits);
			e->addAttribute("New", 0);

			if (vector_contains(&inletComponents, (Component*)pl[i-1]))
			{
				DM::Node * n = pl[i-1];
				e->addAttribute("PLAN_DATE",  n->getAttribute("BuildYear")->getDouble());
			}

		}
	}
	int id = 1;
	Logger(Debug) << "Done with adding plan date extraction";
	std::vector<DM::Component*> junctions =  this->city->getAllComponentsInView(Junction);
	counter = 0;

	foreach(DM::Component* cmp, junctions)
	{
		counter++;
		Logger(Debug) << junctions.size() << "/" << counter;
		DM::Node * n = (DM::Node*)cmp;
		int x = (n->getX() - offsetX)/cellSizeX;
		int y = (n->getY() - offsetY)/cellSizeY;
		Logger(Debug) << "start topo";
		double z = this->Topology->getCell(x,y);
		Logger(Debug) << "end topo";

		if (n->getAttribute("existing")->getDouble() >0.01)
			continue;

		n->changeAttribute("Z", z);
		double invert_elevation = n->getAttribute("Z")->getDouble() - n->getAttribute("D")->getDouble();
		n->getAttribute("invert_elevation")->setDouble(invert_elevation);
		n->addAttribute("id", id++);
	}

	/*Logger(Debug) << "Done with adding junctions date extraction";
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
		n->changeAttribute("Z", z);
		double invert_elevation = n->getAttribute("Z")->getDouble() - 3.;
		n->getAttribute("invert_elevation")->setDouble(invert_elevation);
	}*/


	if( smooth ) {
		Logger(Standard) << "Start smoothing";
		smoothNetwork();
	}

}

//TODO: Smooth Networks
void ExtractNetwork::smoothNetwork() {
	std::map<DM::Node *, std::vector<DM::Edge*> > StartNodeSortedEdges;
	std::map<DM::Node *, std::vector<DM::Edge*> > EndNodeSortedEdges;
	std::map<DM::Node *, std::vector<DM::Edge*> > ConnectedEdges;

	std::vector<DM::Component*> inlets = city->getAllComponentsInView(this->Inlets);
	std::vector<DM::Component*> conduits = city->getAllComponentsInView(this->Conduits);

	//Create Connection List
	//foreach(std::string name , ConduitNames)  {

	foreach(DM::Component* cmp, conduits)
	{
		DM::Edge * e = (Edge*)cmp;
		DM::Node * startnode = e->getStartNode();
		DM::Node * endnode = e->getEndNode();

		ConnectedEdges[startnode].push_back(e);
		ConnectedEdges[endnode].push_back(e);
		StartNodeSortedEdges[startnode].push_back(e);
		EndNodeSortedEdges[endnode].push_back(e);
	}

	Logger(Debug) << "Start Smoothing Netowrk";
	//find WWTP


	double internalcounter = 0;
	Logger(Debug) << "Endpoint Thing";

	std::vector<DM::Node *> EndPoints;


	foreach (DM::Component* cmp, city->getAllComponentsInView(this->EndPoint))
		EndPoints.push_back((Node*)cmp);

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

				DM::Node * p_upper = e->getStartNode();
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
