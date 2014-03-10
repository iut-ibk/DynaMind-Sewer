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
#include "timeareamethod.h"
#include "tbvectordata.h"
#include <math.h>

DM_DECLARE_NODE_NAME(TimeAreaMethod, Sewer)
TimeAreaMethod::TimeAreaMethod()
{
	combinedSystem = false;
	conduit = DM::View("CONDUIT", DM::EDGE, DM::READ);
	conduit.addAttribute("Diameter", DM::Attribute::DOUBLE, DM::WRITE);
	conduit.addAttribute("Length", DM::Attribute::DOUBLE, DM::WRITE);
	conduit.addAttribute("JUNCTION", "JUNCTION", DM::WRITE);

	inlet = DM::View("INLET", DM::NODE, DM::READ);
	inlet.addAttribute("Connected", DM::Attribute::DOUBLE, DM::READ);
	inlet.addAttribute("WasteWater", DM::Attribute::DOUBLE, DM::WRITE);
	inlet.addAttribute("InfiltrationWater", DM::Attribute::DOUBLE, DM::WRITE);
	inlet.addAttribute("Area", DM::Attribute::DOUBLE, DM::WRITE);
	inlet.addAttribute("QrKrit", DM::Attribute::DOUBLE, DM::WRITE);
	inlet.addAttribute("Impervious", DM::Attribute::DOUBLE, DM::WRITE);
	inlet.addAttribute("CATCHMENT", "CITYBLOCKS", DM::READ);

	shaft = DM::View("JUNCTION", DM::NODE, DM::READ);
	shaft.addAttribute("WasteWaterPerShaft", DM::Attribute::DOUBLE, DM::WRITE);
	shaft.addAttribute("InfiltrationWaterPerShaft", DM::Attribute::DOUBLE, DM::WRITE);
	shaft.addAttribute("QrKritPerShaft", DM::Attribute::DOUBLE, DM::WRITE);
	shaft.addAttribute("AreaPerShaft", DM::Attribute::DOUBLE, DM::WRITE);

	wwtps = DM::View("WWTP", DM::NODE, DM::READ);
	catchment = DM::View("CATCHMENT", DM::FACE, DM::READ);

	storage = DM::View("STORAGE", DM::NODE, DM::READ);
	storage.addAttribute("StorageV", DM::Attribute::DOUBLE, DM::WRITE);
	storage.addAttribute("Storage", DM::Attribute::DOUBLE, DM::WRITE);
	storage.addAttribute("StorageA", DM::Attribute::DOUBLE, DM::WRITE);
	storage.addAttribute("ConnectedStorageArea", DM::Attribute::DOUBLE, DM::WRITE);

	catchment.addAttribute("Population", DM::Attribute::DOUBLE, DM::READ);
	catchment.addAttribute("Area", DM::Attribute::DOUBLE, DM::READ);
	catchment.addAttribute("Impervious", DM::Attribute::DOUBLE, DM::READ);
	catchment.addAttribute("WasteWater", DM::Attribute::DOUBLE, DM::READ);

	std::vector<DM::View> views;

	outfalls= DM::View("OUTFALL", DM::NODE, DM::WRITE);
	weir = DM::View("WEIR", DM::NODE, DM::WRITE);
	weir.addAttribute("InletOffset", DM::Attribute::DOUBLE, DM::WRITE);

	globals = DM::View("CITY", DM::COMPONENT, DM::READ);
	globals.addAttribute("CONNECTEDPOP", DM::Attribute::DOUBLE, DM::WRITE);
	globals.addAttribute("CONNECTEDAREA", DM::Attribute::DOUBLE, DM::WRITE);

	views.push_back(conduit);
	views.push_back(inlet);
	views.push_back(shaft);
	views.push_back(wwtps);
	views.push_back(catchment);
	views.push_back(outfalls);
	views.push_back(weir);
	views.push_back(storage);
	views.push_back(globals);

	this->v = 1;
	this->r15 = 150;

	this->addParameter("v", DM::DOUBLE, & this->v);
	this->addParameter("r15", DM::DOUBLE, & this->r15);
	this->addParameter("combined system", DM::BOOL, &this->combinedSystem);

	this->addData("City", views);
}

void TimeAreaMethod::init() {
	std::vector<DM::View> views;
	if (combinedSystem) {
		views.push_back(conduit);
		views.push_back(inlet);
		views.push_back(shaft);
		views.push_back(wwtps);
		views.push_back(catchment);
		views.push_back(outfalls);
		views.push_back(weir);
		views.push_back(storage);
		views.push_back(globals);
	} else {
		views.push_back(conduit);
		views.push_back(inlet);
		views.push_back(shaft);
		views.push_back(catchment);
		views.push_back(outfalls);
		views.push_back(globals);
	}
	this->addData("City", views);
}

double TimeAreaMethod::caluclateAPhi(DM::Component * attr, double r15)  const {
	double n = 1.;
	double T = 10.;
	double phi10 = (38./(T+9.)*(pow(n,(-1/4))-0,369));

	T = attr->getAttribute("Time")->getDouble() / 60.;
	if (T < 10) {
		T = 10;
	}
	double phiT = (38./(T+9.)*(pow(n,(-1/4))-0,369));

	return phiT/phi10;
}


void TimeAreaMethod::run() {

	DM::System *city= this->getData("City");

	std::vector<DM::Component*> inletCmps = city->getAllComponentsInView(inlet);
	std::vector<DM::Component*> conduitCmps = city->getAllComponentsInView(conduit);

	//Create Connection List
	foreach(DM::Component* cmp, conduitCmps)
	{
		DM::Edge * e = (DM::Edge*)cmp;
		if (e->getAttribute("existing")->getDouble() > 0.01)
			continue;

		ConnectedEdges[e->getStartNode()].push_back(e);
		ConnectedEdges[e->getEndNode()].push_back(e);
	}

	foreach(DM::Component* cmp, conduitCmps)
	{
		DM::Edge * e = (DM::Edge*)cmp;
		if (e->getAttribute("existing")->getDouble() > 0.01)
			continue;

		StartNodeSortedEdges[e->getStartNode()].push_back(e);
		EndNodeSortedEdges[e->getEndNode()].push_back(e);
	}

	//Calculate Waste Water
	double Population_sum = 0;
	double area_sum = 0;
	double WasterWaterPerPerson = 0.0052;
	double InfiltrationWater =  0.003;

	//Calculate Water Water per Node
	foreach(DM::Component* inlet_attr, inletCmps)
	{
		if (inlet_attr->getAttribute("Connected")->getDouble() < 0.01 )
			continue;

		DM::Component* catchment_attr = inlet_attr->getAttribute("CATCHMENT")->getLinkedComponents()[0];

		double pop =  catchment_attr->getAttribute("Population")->getDouble();
		double area = catchment_attr->getAttribute("Area")->getDouble();
		double imp = catchment_attr->getAttribute("Impervious")->getDouble();
		if(imp < 0.2)
			imp = 0.2;

		inlet_attr->addAttribute("InfiltrationWater", pop * InfiltrationWater);
		inlet_attr->addAttribute("WasterWater",  pop * WasterWaterPerPerson);
		inlet_attr->addAttribute("Area",area*imp);
		inlet_attr->addAttribute("QrKrit", 15.*area/10000*imp);
		inlet_attr->addAttribute("Impervious",imp);

		Population_sum += catchment_attr->getAttribute("Population")->getDouble();
		area_sum += catchment_attr->getAttribute("Area")->getDouble();
	}

	int nOutfalls = city->getAllComponentsInView(outfalls).size();
	foreach(DM::Component * sg, city->getAllComponentsInView(globals))
	{
		sg->addAttribute("CONNECTEDPOP", Population_sum);
		sg->addAttribute("CONNECTEDAREA", area_sum);
		sg->addAttribute("CSOs", nOutfalls);
	}

	//AddStorageToWWtp
	/*std::vector<std::string> endPointNames = city->getUUIDsOfComponentsInView(wwtps);
	foreach(std::string name, endPointNames) {
		this->EndPointList.push_back(city->getNode(name));
		DM::Node * p = city->getNode(name);
		p->addAttribute("StorageV",0);
		p->addAttribute("Storage",1);
		city->addComponentToView(p,storage);
	}*/

	foreach(DM::Component* cmp, conduitCmps)
	{
		DM::Edge* con = (DM::Edge*)cmp;
		DM::Node* start = con->getStartNode();
		DM::Node* end = con->getEndNode();
		DM::Node dp = *(start) - *(end);

		double l = sqrt(dp.getX()*dp.getX() + dp.getY()*dp.getY() + dp.getZ()*dp.getZ());
		con->addAttribute("Length", l);
	}

	std::vector<DM::Component*> storageCmps = city->getAllComponentsInView(storage);

	foreach(DM::Component* inlet_attr, inletCmps)
	{
		double wastewater = inlet_attr->getAttribute("WasterWater")->getDouble();
		double infiltreationwater = inlet_attr->getAttribute("InfiltrationWater")->getDouble();
		double area = inlet_attr->getAttribute("Area")->getDouble();
		double QrKrit = inlet_attr->getAttribute("QrKrit")->getDouble();

		bool ReachedEndPoint = false;
		DM::Node * idPrev = 0;
		//Length is reset if Outfall is hit
		double StrangL = 0;
		//Length is reset if total length
		double StrangL_Total = 0;
		double QKrit = 0;
		double DeltaA = 0;
		double DeltaStorage = 0;

		//DM::Logger(DM::Debug) << "JUNCTION " << inlet_attr->getAttribute("JUNCTION")->getLink().uuid;
		std::vector<DM::Component*> links = inlet_attr->getAttribute("JUNCTION")->getLinkedComponents();
		if (links.size() == 0 || links[0] == NULL)
		{
			DM::Logger(DM::Debug) << "Inlet not connected to junction";
			continue;
		}
		DM::Node * id = (DM::Node*)links[0];

		do 
		{
			DM::Attribute* a;

#define INC(att, value) a = id->getAttribute(att); a->setDouble(a->getDouble() + value);

			INC("WasteWaterPerShaft", wastewater)
			INC("InfiltrationWaterPerShaft", infiltreationwater)
			INC("AreaPerShaft", area)
			INC("QrKritPerShaft", QrKrit)

			if (idPrev != 0) {
				DM::Node dp = *(idPrev) - *(id);

				StrangL += sqrt(dp.getX()*dp.getX() + dp.getY()*dp.getY() + dp.getZ()*dp.getZ());
				StrangL_Total += sqrt(dp.getX()*dp.getX() + dp.getY()*dp.getY() + dp.getZ()*dp.getZ());
			}

			//Search for Outfall
			if (id->getAttribute("Outfall")->getDouble() > 0.01) {
				StrangL = 0;
				QKrit =  id->getAttribute("QrKritPerShaft")->getDouble();
				DeltaA = id->getAttribute("AreaPerShaft")->getDouble();

				if (vector_contains(&storageCmps, (DM::Component*)id)) 
				{
					//Storage Capacity
					if (area > 0.01) {
						double capacity = id->getAttribute("Storage")->getDouble();
						QKrit =  id->getAttribute("QrKritPerShaft")->getDouble() * (1-capacity);
						DeltaStorage =  area * (capacity);
						area = area * capacity * (1-capacity);
						id->addAttribute("ConnectedStorageArea", DeltaStorage + id->getAttribute("ConnectedStorageArea")->getDouble());
						StrangL_Total = 0;
					}

				}

			}

			if ( id->getAttribute("StrangLength")->getDouble() < StrangL)
				id->getAttribute("StrangLength")->setDouble(StrangL);

			if ( id->getAttribute("StrangLengthTotal")->getDouble() < StrangL_Total)
				id->getAttribute("StrangLengthTotal")->setDouble(StrangL_Total);

			id->getAttribute("QrKritPerShaft_total")->setDouble(QKrit);
			id->getAttribute("Area_total")->setDouble(id->getAttribute("AreaPerShaft")->getDouble() - DeltaA);
			DeltaStorage = 0;


			idPrev = id;


			DM::Node * nextid_tmp = 0;
			DM::Edge * outgoing_id = 0;

			std::vector<DM::Edge*>  downstreamEdges = StartNodeSortedEdges[id];
			//o---o---x
			if (downstreamEdges.size() == 1) 
			{
				DM::Edge* e = downstreamEdges[0];
				DM::Node* endnode = e->getEndNode();
				if (endnode != id)
				{
					nextid_tmp = endnode;
					outgoing_id = e;
				}
			}

			id = nextid_tmp;

			if (id == 0) {
				break;
			}

			foreach(DM::Node * p, this->EndPointList) {
				if (id == p ){
					ReachedEndPoint = true;
					id->getAttribute("WasteWaterPerShaft")->setDouble(id->getAttribute("WasteWaterPerShaft")->getDouble() + wastewater);
					id->getAttribute("InfiltrationWaterPerShaft")->setDouble(id->getAttribute("InfiltrationWaterPerShaft")->getDouble() + infiltreationwater);
					id->getAttribute("AreaPerShaft")->setDouble(id->getAttribute("AreaPerShaft")->getDouble() + area);
					id->addAttribute("ConnectedStorageArea", id->getAttribute("AreaPerShaft")->getDouble() + area);

					if (idPrev != 0) {
						DM::Node dp = *(idPrev) - *(id);
						StrangL += sqrt(dp.getX()*dp.getX() + dp.getY()*dp.getY() + dp.getZ()*dp.getZ());
						StrangL_Total += sqrt(dp.getX()*dp.getX() + dp.getY()*dp.getY() + dp.getZ()*dp.getZ());
					}

					id->getAttribute("StrangLength")->getDouble();
					if ( id->getAttribute("StrangLength")->getDouble() < StrangL)
						id->getAttribute("StrangLength")->setDouble(StrangL);
					if ( id->getAttribute("StrangLengthTotal")->getDouble() < StrangL_Total)
						id->getAttribute("StrangLengthTotal")->setDouble(StrangL_Total);
					break;
				}
			}

		}while (!ReachedEndPoint && id != 0);
		if (id == 0)
			continue;
	}

	//Write Data to shaft
	std::vector<DM::Component*> junctionnames = city->getAllComponentsInView(shaft);

	foreach (DM::Component* c, junctionnames)
	{
		DM::Node * p = (DM::Node*)c;

		p->getAttribute("Time")->setDouble( p->getAttribute("StrangLength")->getDouble()/this->v + 1*60);
		p->getAttribute("TimeTotal")->setDouble( p->getAttribute("StrangLengthTotal")->getDouble()/this->v + 1*60);
		double QrKritPerShaft_total =  p->getAttribute("QrKritPerShaft_total")->getDouble();


		p->addAttribute("APhi", this->caluclateAPhi(p, this->r15));
		//rkrit can be reduced to 7.5 if T > 120min
		if ( p->getAttribute("TimeTotal")->getDouble()/60 > 120) {
			p->addAttribute("QrKrit_total", QrKritPerShaft_total/2. );
		} else {
			p->addAttribute("QrKrit_total", QrKritPerShaft_total*120/(p->getAttribute("TimeTotal")->getDouble()/60+120) );

		}
	}

	//Dimensioning
	foreach(DM::Component* c, conduitCmps) 
	{
		DM::Edge * e = (DM::Edge*)c;
		if (e->getAttribute("existing")->getDouble() > 0.01)
			continue;
		DM::Node * attr = e->getStartNode();
		double QWasteWater = attr->getAttribute("WasteWaterPerShaft")->getDouble() +  attr->getAttribute("InfiltrationWaterPerShaft")->getDouble();
		double QRainWater =  attr->getAttribute("Area_total")->getDouble()*attr->getAttribute("APhi")->getDouble()*this->r15/10000. +  attr->getAttribute("QrKrit_total")->getDouble();
		double Area_tot = attr->getAttribute("Area_total")->getDouble();
		double APhi = attr->getAttribute("APhi")->getDouble();
		if (QRainWater < 0)
			QRainWater = 0;
		double QBem = (QRainWater + QWasteWater)/1000.; //m³

		double QWasteWater_2 = 10*(attr->getAttribute("WasteWaterPerShaft")->getDouble() +  attr->getAttribute("InfiltrationWaterPerShaft")->getDouble())/1000;

		if (QWasteWater_2 > QBem) {
			QBem = QWasteWater_2;
		}
		e->addAttribute("Diameter", this->chooseDiameter(sqrt((QBem)/3.14*4)) / 1000.); //in mm
		e->addAttribute("QBem", QBem);
		e->addAttribute("Area_tot", Area_tot);
		e->addAttribute("QrKrittotal", (attr->getAttribute("QrKrit_total")->getDouble()));

		DM::Logger(DM::Debug) << e->getStartpointName() << "\tArea total:\t" <<attr->getAttribute("Area_total")->getDouble() << "\tQrKrit total:\t" <<attr->getAttribute("QrKrit_total")->getDouble()<< "\tDiameter:\t" <<e->getAttribute("Diameter")->getDouble() << "Q:\t" <<e->getAttribute("QBem")->getDouble() ;

	}

	//Dimensioning of Outfalls
	foreach(DM::Component* c, city->getAllComponentsInView(weir))
	{
		DM::Edge * weir = (DM::Edge*)c;
		if (weir->getAttribute("existing")->getDouble() > 0.01)
			continue;
		DM::Node * StartNode = weir->getStartNode();

		//Get Upstream Nodes

		std::vector<DM::Edge*> upstream = EndNodeSortedEdges[StartNode];
		std::vector<DM::Edge*> downstream = StartNodeSortedEdges[StartNode];
		double maxDiameter = 0;
		double minDiameter = -1;
		foreach (DM::Edge * c, upstream) {
			if (c->getAttribute("Diameter")->getDouble() > maxDiameter)
				maxDiameter = c->getAttribute("Diameter")->getDouble();
			if (c->getAttribute("Diameter")->getDouble() < minDiameter || minDiameter < 0)
				minDiameter = c->getAttribute("Diameter")->getDouble();
		}
		foreach (DM::Edge * c, downstream) {
			if (c->getAttribute("Diameter")->getDouble() > maxDiameter)
				maxDiameter = c->getAttribute("Diameter")->getDouble();
			if (c->getAttribute("Diameter")->getDouble() < minDiameter || minDiameter < 0)
				minDiameter = c->getAttribute("Diameter")->getDouble();
		}

		double inletOffset = 0;
		//If storage

		if (StartNode->getAttribute("ConnectedStorageArea")->getDouble() > 0.01) {
			inletOffset = 0.6/1000 * maxDiameter;
		} else {
			inletOffset = 0.85/1000 * maxDiameter;
		}
		if (minDiameter > inletOffset)
			inletOffset = maxDiameter*.80/1000;

		if (StartNode->getAttribute("Area_total")->getDouble() < 0.01) {
			inletOffset = maxDiameter/1000;
		}

		weir->addAttribute("InletOffset", inletOffset);
	}

	//Dimensiong Pipe to WWTP
	foreach(DM::Component* c, city->getAllComponentsInView(wwtps)) 
	{
		DM::Node* wwtp = (DM::Node*)c;
		if (wwtp->getAttribute("existing")->getDouble() > 0.01)
			continue;
		std::vector<DM::Edge*> edges = this->StartNodeSortedEdges[wwtp];
		if (edges.size() == 1) {
			double QWasteWater = 2*wwtp->getAttribute("WasteWaterPerShaft")->getDouble() +  wwtp->getAttribute("InfiltrationWaterPerShaft")->getDouble();
			DM::Edge * e = edges[0];
			double diameter = sqrt(QWasteWater)/3.14*4;
			e->addAttribute("Diameter", this->chooseDiameter(diameter)/1000.);

		}
	}

	//Dimensioning Storage
	foreach(DM::Component* c, city->getAllComponentsInView(storage)) 
	{
		DM::Node* storage = (DM::Node*)c;
		if (storage->getAttribute("existing")->getDouble() > 0.01)
			continue;
		std::vector<DM::Edge*> edges = this->EndNodeSortedEdges[storage];
		double minDiameter = -1;
		double maxDiameter = 0;
		foreach (DM::Edge * c, edges) {
			if (c->getAttribute("Diameter")->getDouble() > maxDiameter)
				maxDiameter = c->getAttribute("Diameter")->getDouble();
			if (c->getAttribute("Diameter")->getDouble() < minDiameter || minDiameter < 0)
				minDiameter = c->getAttribute("Diameter")->getDouble();
		}

		std::vector<DM::Edge*> upstream = StartNodeSortedEdges[storage];
		foreach (DM::Edge * c, upstream) {
			if (c->getAttribute("Diameter")->getDouble() > maxDiameter)
				maxDiameter = c->getAttribute("Diameter")->getDouble();
			if (c->getAttribute("Diameter")->getDouble() < minDiameter || minDiameter < 0)
				minDiameter = c->getAttribute("Diameter")->getDouble();
		}


		double VStorage = storage->getAttribute("ConnectedStorageArea")->getDouble() * 15/10000;


		//Connected Weir
		double maxdepth = maxDiameter*0.6/1000;

		double area = VStorage / (maxdepth);
		storage->addAttribute("StorageA", area);
		storage->addAttribute("StorageV", VStorage);

	}

	DM::Logger(DM::Standard) << "Sum over Population " << Population_sum;

}

double TimeAreaMethod::chooseDiameter(double diameter) {
	QVector<double> vd;
	/*vd.append(150);
	vd.append(200);*/
	vd.append(250);
	vd.append(300);
	vd.append(350);
	vd.append(400);
	vd.append(450);
	vd.append(500);
	vd.append(600);
	vd.append(700);
	vd.append(800);
	vd.append(900);
	vd.append(1000);
	vd.append(1100);
	vd.append(1200);
	vd.append(1300);
	vd.append(1400);
	vd.append(1500);
	vd.append(1600);
	vd.append(1800);
	vd.append(2000);
	vd.append(2200);
	vd.append(2400);
	vd.append(2600);
	vd.append(2800);
	vd.append(3000);
	vd.append(3200);
	vd.append(3400);
	vd.append(3600);
	vd.append(3800);
	vd.append(4000);

	double d = 0;
	for (int i = 0; i < vd.size(); i++) {
		if (d == 0 && diameter*1000 <= vd.at(i) ) {
			d =  vd.at(i);
		}
	}
	if (d == 0.) {
		d = 4000;
	}
	return d;

};
