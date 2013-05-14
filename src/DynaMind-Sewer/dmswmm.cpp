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
#include "dmswmm.h"
#include <fstream>

#include <QDir>
#include <QUuid>
#include <QProcess>
#include <QTextStream>
#include <QSettings>
#include <math.h>
#include <algorithm>
#include <swmmwriteandread.h>
#include <swmmreturnperiod.h>



using namespace DM;
DM_DECLARE_NODE_NAME(DMSWMM, Sewer)
DMSWMM::DMSWMM()
{

    GLOBAL_Counter = 1;
    conduit = DM::View("CONDUIT", DM::EDGE, DM::READ);
    conduit.getAttribute("Diameter");
    conduit.addAttribute("capacity");
    conduit.addAttribute("velocity");

    inlet = DM::View("INLET", DM::NODE, DM::READ);
    inlet.getAttribute("CATCHMENT");


    junctions = DM::View("JUNCTION", DM::NODE, DM::READ);
    junctions.getAttribute("D");
    junctions.addAttribute("flooding_V");

    endnodes = DM::View("OUTFALL", DM::NODE, DM::READ);

    catchment = DM::View("CATCHMENT", DM::FACE, DM::READ);
    catchment.getAttribute("WasteWater");
    catchment.getAttribute("area");
    catchment.getAttribute("Impervious");

    outfalls= DM::View("OUTFALL", DM::NODE, DM::READ);

    weir = DM::View("WEIR", DM::EDGE, DM::READ);
    weir.getAttribute("crest_height");
    wwtp = DM::View("WWTP", DM::NODE, DM::READ);

    pumps = DM::View("PUMPS", DM::EDGE, DM::READ);

    storage = DM::View("STORAGE", DM::NODE, DM::READ);
    storage.getAttribute("Z");
    //storage.getAttribute("Storage");


    globals = DM::View("CITY", DM::COMPONENT, DM::READ);
    globals.addAttribute("SWMM_ID");
    globals.addAttribute("Vr");
    globals.addAttribute("Vp");
    globals.addAttribute("Vwwtp");

    std::vector<DM::View> views;

    views.push_back(conduit);
    views.push_back(inlet);
    views.push_back(junctions);
    views.push_back(endnodes);
    views.push_back(catchment);
    views.push_back(outfalls);
    views.push_back(weir);
    views.push_back(wwtp);
    views.push_back(storage);
    views.push_back(globals);

    this->FileName = "";
    this->climateChangeFactor = 1;
    this->RainFile = "";
    this->use_euler = true;
    this->return_period = 1;

    years = 0;

    this->isCombined = false;
    this->addParameter("Folder", DM::STRING, &this->FileName);
    this->addParameter("RainFile", DM::FILENAME, &this->RainFile);
    this->addParameter("ClimateChangeFactor", DM::DOUBLE, & this->climateChangeFactor);
    this->addParameter("use euler", DM::BOOL, & this->use_euler);
    this->addParameter("return period", DM::DOUBLE, &this->return_period);
    this->addParameter("combined system", DM::BOOL, &this->isCombined);

    counterRain =0;


    this->addData("City", views);

}

void DMSWMM::init() {

    std::vector<DM::View> views;
    views.push_back(conduit);
    views.push_back(inlet);
    views.push_back(junctions);
    views.push_back(endnodes);
    views.push_back(catchment);
    views.push_back(outfalls);

    if (isCombined){
        views.push_back(weir);
        views.push_back(wwtp);
        views.push_back(storage);
    }
    views.push_back(globals);

    this->addData("City", views);
}


void DMSWMM::run() {
    curves.str("");
    city = this->getData("City");

    SWMMWriteAndRead * swmm;
    double cf = 1. + years / 20. * (this->climateChangeFactor - 1.);

    if (!this->use_euler)
        swmm = new SWMMWriteAndRead(city, this->RainFile, this->FileName);


    else {
        std::stringstream rfile;
        rfile << "/tmp/rain_"<< this->getUuid();
        SWMMReturnPeriod::CreateEulerRainFile(60, 5, this->return_period, 1, rfile.str());
        swmm = new SWMMWriteAndRead(city, rfile.str(), this->FileName);
    }

    swmm->setClimateChangeFactor(cf);
    swmm->setupSWMM();
    swmm->runSWMM();
    swmm->readInReportFile();

    std::vector<std::pair<std::string, double> > capacity = swmm->getLinkFlowSummeryCapacity();
    for (int i = 0; i < capacity.size(); i++) {
        std::pair<std::string, double> cap = capacity[i];
        DM::Component * c = city->getComponent(cap.first);
        c->addAttribute("capacity", cap.second);
    }

    std::vector<std::pair<std::string, double> > velocity = swmm->getLinkFlowSummeryVelocity();
    for (int i = 0; i < velocity.size(); i++) {
        std::pair<std::string, double> velo = velocity[i];
        DM::Component * c = city->getComponent(velo.first);
        c->addAttribute("velocity", velo.second);
    }

    delete swmm;

    this->years++;
}
