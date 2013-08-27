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
#include "generatesewernetwork.h"
#include "csg_s_operations.h"
#include <dmgroup.h>
#ifdef _OPENMP
#include <omp.h>
#endif


DM_DECLARE_NODE_NAME(GenerateSewerNetwork, Sewer)

GenerateSewerNetwork::Agent::Agent(Pos StartPos) {
	this->successful = false;
	this->alive = true;
	this->currentPos.x = StartPos.x;
	this->currentPos.y = StartPos.y;
	this->currentPos.z = 0;
	this->currentPos.h = 3;
	this->HConnection = 3;
	this->lastdir = -1;
	this->neigh = std::vector<double>(9);
	this->decissionVector = std::vector<double>(9);
	this->ProbabilityVector = std::vector<double>(9);
}

void GenerateSewerNetwork::Agent::run() {
	this->path.clear();
	this->successful = false;
	double noData =  Topology->getNoValue();
	for (int i = 0; i < this->steps; i++) {
		this->currentPos.z = this->Topology->getCell(currentPos.x, currentPos.y);
		double Hcurrent = this->currentPos.z;
		double hcurrent = this->currentPos.h;
		this->path.push_back(currentPos);


		//Influence Connectifity flield
		ConnectivityField->getMoorNeighbourhood(neigh, currentPos.x,currentPos.y);
		double currentHeight = neigh[4];
		int index = GenerateSewerNetwork::indexOfMinValue(neigh, noData);
		for (int i = 0; i < 9; i++) {
			if (currentHeight > neigh[i]  && neigh[i] != noData) {
				if (index == i)
					decissionVector[i] = MultiplyerCenterCon*this->AttractionConnectivity;
				else
					decissionVector[i] = 1*this->AttractionConnectivity;
			} else {
				decissionVector[i] = 0;
			}
		}

		//Influence Topology add psossible dem connection, set probability to 0 if no suitable neighbour can be found
		Topology->getMoorNeighbourhood(neigh, currentPos.x,currentPos.y);
		currentHeight = neigh[4];
		index = GenerateSewerNetwork::indexOfMinValue(neigh,noData);
		for (int i = 0; i < 9; i++) {
			if (currentHeight + (this->Hmin - this->currentPos.h) >= neigh[i] && neigh[i] != noData) {
				if (index == i )
					decissionVector[i] += MultiplyerCenterTop*this->AttractionTopology;
				else
					decissionVector[i] += 1*this->AttractionTopology;

			} else { //set zerro if a connection is not possbile
				decissionVector[i] = 0;
			}

		}


		//Forbidden Areas
		ForbiddenAreas->getMoorNeighbourhood(neigh, currentPos.x,currentPos.y);
		for (int i = 0; i < 9; i++) {
			decissionVector[i]*=neigh[i];
		}
		if (lastdir > -1){
			decissionVector[lastdir]= decissionVector[lastdir]* StablizierLastDir;
		}

		double sumVec = 0;
		for (int i = 0; i < 9; i++) {
			sumVec+=decissionVector[i];
		}

		for (int i = 0; i < 9; i++) {
			ProbabilityVector[i] = decissionVector[i] /sumVec * 100.;
		}


		int ra = rand()%100+1; // +1 otherwise if ra == 0 possbile that agent picks a pos with 0% probability

		double prob = 0;
		int direction = -1;
		for (int i = 0; i < 9; i++) {
			prob += ProbabilityVector[i];

			if (ra <= (int) prob) {
				direction = i;
				break;
			}
		}

		lastdir = direction;

		if(direction == -1) {
			this->alive = false;
			break;
		}

		this->currentPos.x+=csg_s::csg_s_operations::returnPositionX(direction);
		this->currentPos.y+=csg_s::csg_s_operations::returnPositionY(direction);
		double deltaH = Hcurrent - this->Topology->getCell(currentPos.x, currentPos.y);
		if (deltaH > 0) {
			currentPos.h = hcurrent - deltaH;
			if (currentPos.h < 3) {
				currentPos.h = 3;
			}
		} else {
			currentPos.h=hcurrent-deltaH;
		}

		if (MarkPath) {
			MarkPath->setCell(currentPos.x, currentPos.y, 1);
			Trace->setCell(currentPos.x, currentPos.y,Trace->getCell(currentPos.x, currentPos.y)+1);
		}
		if (currentPos.x < 0 || currentPos.y < 0 || currentPos.x > Topology->getWidth()-2 || currentPos.y >  Topology->getHeight()-2) {
			this->alive = false;
			break;
		}

		//Check current Pos is < 3 to secure connections
		if (Goals->getCell(currentPos.x, currentPos.y ) > 0.01 && currentPos.h <= this->HConnection) {
			this->alive = false;
			this->successful = true;
			this->path.push_back(currentPos);
			break;
		}


	}

	this->alive = false;
}

void GenerateSewerNetwork::addRadiusValue(int x, int y, RasterData * layer, int rmax, double value, double ** stamp, double nodeata) {

	if (rmax > 500) {
		rmax = 500;
	}
	int i = x - rmax;
	int j = y - rmax;

	if (value > 1000) {
		Logger(Debug) << "Really Big";
	}

	int i_small = 0;
	int limitx =  rmax+x;
	int limity =  rmax+y;
	for (; i < limitx;  i++ ) {
		j = y - rmax;
		int j_small = -1;
		for (;  j < limity; j++) {
			j_small++;
			if (i < 0 || j < 0)
				continue;
			double val =  stamp[i_small][j_small] * value;
			double cur_val = layer->getCell(i,j);
			if (layer->getCell(i,j) > val || cur_val == nodeata ) {
				layer->setCell(i,j,val );
			}

		}
		i_small++;
	}
}

void GenerateSewerNetwork::MarkPathWithField(RasterData * ConnectivityField, int ConnectivityWidth) {


	RasterData Buffer;
	double noData = ConnectivityField->getNoValue();
	Buffer.setSize(ConnectivityField->getWidth(), ConnectivityField->getHeight(), ConnectivityField->getCellSizeX(),ConnectivityField->getCellSizeY(),ConnectivityField->getXOffset(),ConnectivityField->getYOffset());
	Buffer.clear();


	//Mark Field
	int level = ConnectivityWidth;
	int rmax = ConnectivityWidth;
	if (rmax > 500) {
		rmax = 500;
	}


	double** stamp = new double*[rmax*2];
	for (int i = 0; i < rmax*2; i++) {
		stamp[i] = new double[rmax*2];
	}
	int x_p = rmax;
	for (int i = 0; i < rmax*2;  i++) {
		for (int j = 0; j < rmax*2;  j++) {
			if (i != x_p || j != x_p) {
				double r = sqrt(double((i-x_p)*(i-x_p) + (j-x_p)*(j-x_p)));
				double val = (-level/10. * 1./r);
				stamp[i][j]  = val;
			} else {
				double val = (-level/10. *( 2.) );
				stamp[i][j] = val;
			}
		}
	}

	for (std::map<std::pair<int,int>, Pos>::const_iterator it = agentPathMap.begin(); it != agentPathMap.end(); ++it ) {
		Pos p = it->second;
		GenerateSewerNetwork::addRadiusValue(p.x,  p.y, ConnectivityField, ConnectivityWidth,  p.val, stamp, noData);
	}


	for (int i = 0; i < rmax*2; i++) {
		delete[] stamp[i];
	}
	delete[] stamp;
}

int GenerateSewerNetwork::indexOfMinValue(const vector<double> &vec, double noValue) {
	double val = noValue;
	int index = -1;

	for (int i = 0; i < 9; i++) {
		if (val == noValue && vec[i] != noValue) {
			val =  vec[i];
			index = i;
			continue;
		}
		if (vec[i] < val && vec[i] != noValue) {
			val = vec[i];
			index = i;
		}
	}

	//Check if alone
	std::vector<int> indizes;
	for (int i = 0; i < 9; i++) {
		if (vec[i] == val) {
			indizes.push_back(i);
		}
	}

	//if index exists more often return random
	if (indizes.size() > 1) {
		int i_ran = rand() % indizes.size();
		index = indizes[i_ran];
	}
	return index;
}

void GenerateSewerNetwork::reducePath(std::vector<GenerateSewerNetwork::Pos> & path)
{
	QMutexLocker ml(&mMutex);
	if (path.size() < 1) {
		Logger(DM::Debug) << "MarkPathWithField: Path Size = 0" ;
		return;
	}

	//Cost Function for Length
	int last = path.size() - 1;

	double x = path[0].x - path[last].x;
	double y = path[0].y - path[last].y;

	double r_opt = sqrt(x * x + y * y);

	int path_size =  path.size();
	for (int i = 0; i < path_size; i++) {
		//Calculate Optimal Length

		double r = last+1;
		double val = ((double) i / (double) path_size);
		val = val * r_opt / r;

		int x_1 = path[i].x;
		int y_1 = path[i].y;
		std::pair<int,int> hash(x_1,y_1);
		Pos p = path[i];
		p.val = val;


		if (agentPathMap.find(hash) == agentPathMap.end()) {
			agentPathMap[hash] = p;
		} else {
			if (agentPathMap[hash].val < p.val ) {
				agentPathMap[hash] = p;
			}
		}



	}

}

GenerateSewerNetwork::GenerateSewerNetwork() : mMutex()
{


	this->ConnectivityWidth = 9;
	this->AttractionTopology = 1;
	this->AttractionConnectivity = 1;
	this->IdentifierStartPoins = "";
	this->steps = 1000;
	this->Hmin = 8;
	this->HConnection = 3;
	this->DebugMode = false;
	MultiplyerCenterCon = 1;
	MultiplyerCenterTop = 1;
	StablizierLastDir = 1;
	internalCounter = 0;


	this->addParameter("MaxDeph", DM::DOUBLE, &this->Hmin);
	this->addParameter("MaxDepthConnection", DM::DOUBLE, &this->HConnection);
	this->addParameter("Steps", DM::LONG, & this->steps);
	this->addParameter("ConnectivityWidth", DM::INT, & this->ConnectivityWidth);
	this->addParameter("AttractionTopology", DM::DOUBLE, & this->AttractionTopology);
	this->addParameter("AttractionConnectivity", DM::DOUBLE, & this->AttractionConnectivity);
	this->addParameter("MultiplyerCenterCon", DM::DOUBLE, & this->MultiplyerCenterCon);
	this->addParameter("MultiplyerCenterTop", DM::DOUBLE, & this->MultiplyerCenterTop);
	this->addParameter("StablizierLastDir", DM::INT, &this->StablizierLastDir);
	this->addParameter("Debug", DM::BOOL, &this->DebugMode);


	Topology = DM::View("Topology", DM::RASTERDATA, DM::READ);
	std::vector<DM::View> city;

	Inlets = DM::View("INLET", DM::NODE, DM::READ);
	Inlets.modifyAttribute("New");

	Inlets.getAttribute("CATCHMENT");
	Inlets.addAttribute("success");


	catchment = DM::View("CATCHMENT", DM::FACE, DM::READ);
	catchment.getAttribute("Active");

	city.push_back(Topology);
	city.push_back(Inlets);
	city.push_back(catchment);

	this->addData("City", city);




	ForbiddenAreas = DM::View("ForbiddenAreas", DM::RASTERDATA, DM::READ);
	Goals = DM::View("Goals", DM::RASTERDATA, DM::READ);

	std::vector<DM::View> sewerGeneration_in;
	sewerGeneration_in.push_back(ForbiddenAreas);
	sewerGeneration_in.push_back(Goals);
	this->addData("sewerGeneration_In", sewerGeneration_in);

	std::vector<DM::View> sewerGeneration_con;
	ConnectivityField_in = DM::View("ConnectivityField_in", DM::RASTERDATA, DM::READ);
	sewerGeneration_con.push_back(ConnectivityField_in);
	this->addData("sewerGeneration_con", sewerGeneration_con);

	std::vector<DM::View> sewerGeneration_out;

	ConnectivityField = DM::View("ConnectivityField_in", DM::RASTERDATA, DM::WRITE);
	Path = DM::View("Path", DM::RASTERDATA, DM::WRITE);
	sewerGeneration_out.push_back(Path);
	sewerGeneration_out.push_back(ConnectivityField);
	sewerGeneration_out.push_back(DM::View("Trace", DM::RASTERDATA, DM::WRITE));

	sewerGeneration_out.push_back(DM::View("PathSuccess", DM::RASTERDATA, DM::WRITE));
	this->addData("sewerGeneration_Out", sewerGeneration_out);

}

void GenerateSewerNetwork::init()
{

}

void GenerateSewerNetwork::run() {
	Group* lg = dynamic_cast<Group*>(getOwner());
	if(lg) {
		this->internalCounter = lg->getGroupCounter();
		DM::Logger(DM::Debug) << "counter " << lg->getGroupCounter();
	}
	else
	{
		DM::Logger(DM::Debug) << "counter not found";
		this->internalCounter = 0;
	}

	this->city = this->getData("City");

	rTopology = this->getRasterData("City", Topology);
	rConnectivityField_in = this->getRasterData("sewerGeneration_con", ConnectivityField_in);
	rForbiddenAreas  = this->getRasterData("sewerGeneration_In", ForbiddenAreas);
	rGoals = this->getRasterData("sewerGeneration_In", Goals);
	Logger(Standard) << internalCounter;


	rConnectivityField = this->getRasterData("sewerGeneration_Out", ConnectivityField);

	rPath = 0;
	rTrace = 0;
	DM::RasterData * rSuccess;
	rSuccess = 0;


	long width = this->rTopology->getWidth();
	long height = this->rTopology->getHeight();
	double noValue = this->rConnectivityField_in->getNoValue();
	double cellSizeX = this->rTopology->getCellSizeX();
	double cellSizeY = this->rTopology->getCellSizeY();
	double OffsetX = this->rTopology->getXOffset();
	double OffsetY = this->rTopology->getYOffset();

	rasterSize = cellSizeX;

	this->rConnectivityField->setSize(width, height, cellSizeX,cellSizeY,OffsetX,OffsetY);
	Logger(Debug) << "Conn Max " << this->rConnectivityField_in->getMaxValue();
	Logger(Debug) << "Conn Min " << this->rConnectivityField_in->getMinValue();
	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++ ) {
			double val = this->rConnectivityField_in->getCell(i,j);
			if (val !=noValue )
				this->rConnectivityField->setCell(i,j, val*0.5);
		}
	}
	this->rConnectivityField->setDebugValue(rConnectivityField_in->getDebugValue()+1);

	if (this->DebugMode) {
		rTrace = this->getRasterData("sewerGeneration_Out", DM::View("Trace", DM::RASTERDATA, DM::WRITE));
		rSuccess = this->getRasterData("sewerGeneration_Out", DM::View("PathSuccess", DM::RASTERDATA, DM::WRITE));
		rPath  = this->getRasterData("sewerGeneration_Out", Path);

		this->rPath->setSize(width, height, cellSizeX,cellSizeY,OffsetX,OffsetY);
		this->rPath->clear();
		this->rTrace->setSize(width, height, cellSizeX,cellSizeY,OffsetX,OffsetY);
		rSuccess->setSize(width, height, cellSizeX,cellSizeY,OffsetX,OffsetY);

	}

	std::vector<Agent * > agents;


	std::vector<DM::Node*> StartPos;
	foreach (std::string inlet, city->getUUIDsOfComponentsInView(Inlets))  {
		DM::Node * n = city->getNode(inlet);
		std::string ID_CA = n->getAttribute("CATCHMENT")->getLink().uuid;
		DM::Face * catchment = city->getFace(ID_CA);
		if (!catchment) {
			Logger(Warning) << "Inlet without Catchment ID " << ID_CA;;
			continue;
		}
		n->changeAttribute("New", 0);
		n->changeAttribute("success", 0);
		if (catchment->getAttribute("Active")->getDouble() > 0.1 && n->getAttribute("Connected")->getDouble()  <  0.01) {
			n->changeAttribute("New", 1);
			StartPos.push_back(n);
		}
	}
	//Randomize Agents
	for (unsigned int i = 0; i < StartPos.size(); i++) {
		int i_rand_1 = rand() % StartPos.size();
		int i_rand_2 = rand() % StartPos.size();

		DM::Node * n_tmp = StartPos[i_rand_1];
		StartPos[i_rand_1] = StartPos[i_rand_2];
		StartPos[i_rand_2] = n_tmp;
	}

	//int internalCounter = rConnectivityField_in->getAttribute("agent counter")->getDouble();

	//Create Agents
	this->internalCounter++;
	int attrtopo = this->AttractionTopology - this->internalCounter;
	if (attrtopo < 0)
		attrtopo = 0;
	int attrcon = this->AttractionConnectivity + this->internalCounter;
	if (attrcon < 0)
		attrcon = 0;
	foreach(DM::Node * p, StartPos) {
		long x = (long) (p->getX()  - OffsetX )/cellSizeX;
		long y = (long) (p->getY() -  OffsetY) /cellSizeY;
		Agent * a = new Agent(Pos(x,y));
		a->Topology = this->rTopology;
		a->MarkPath = this->rPath;
		a->ConnectivityField = this->rConnectivityField;
		a->Goals = this->rGoals;
		a->AttractionTopology = attrtopo;
		a->AttractionConnectivity = attrcon;
		a->MultiplyerCenterCon = MultiplyerCenterCon;
		a->MultiplyerCenterTop = MultiplyerCenterTop;
		a->steps = this->steps;
		a->Hmin = this->Hmin;
		a->ForbiddenAreas = this->rForbiddenAreas;
		a->StablizierLastDir = this->StablizierLastDir;
		a->HConnection = this->HConnection;
		a->Trace = rTrace;
		agents.push_back(a);

	}
	long successfulAgents = 0;
	agentPathMap.clear();
	int sumLengthAgentPath = 0;
	int nov_agents = agents.size();
	for (int j = 0; j < nov_agents; j++)
	{
		Agent * a = agents[j];
		if (a->alive) {
			a->run();
			if (!a->successful || a->path.size() < 1)  {
				a->path.clear();
				continue;
			}

			this->reducePath(a->path);
			sumLengthAgentPath+=a->path.size();

			StartPos[j]->addAttribute("success",1);

			if (rSuccess) {
				foreach (Pos p , a->path) {
					rSuccess->setCell(p.x, p.y, 1);

				}
				rSuccess->setCell(  a->path[a->path.size()-1].x,   a->path[a->path.size()-1].y, 10);
			}

			a->path.clear();
			successfulAgents++;
		}


	}

	GenerateSewerNetwork::MarkPathWithField(this->rConnectivityField, this->ConnectivityWidth);
	Logger(DM::Standard) << "Successful " << successfulAgents << "/" << nov_agents;


	for (int j = 0; j < agents.size(); j++) {
		delete agents[j];
	}
	agents.clear();

}


GenerateSewerNetwork::Pos::Pos()
{
	x = 0;
	y = 0;
	z = 0;
	h = 0;
	val = 0;
}
