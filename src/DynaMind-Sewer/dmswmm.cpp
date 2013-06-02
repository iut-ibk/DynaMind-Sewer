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
#include <drainagehelper.h>


using namespace DM;
DM_DECLARE_NODE_NAME(DMSWMM, Sewer)
DMSWMM::DMSWMM()
{

    GLOBAL_Counter = 1;
    internalTimestep = 0;

    conduit = DM::View("CONDUIT", DM::EDGE, DM::READ);
    conduit.getAttribute("Diameter");
    conduit.addAttribute("capacity");
    conduit.addAttribute("velocity");

    inlet = DM::View("INLET", DM::NODE, DM::READ);
    inlet.getAttribute("CATCHMENT");


    junctions = DM::View("JUNCTION", DM::NODE, DM::READ);
    junctions.getAttribute("D");
    junctions.addAttribute("flooding_V");
    junctions.addAttribute("node_depth");

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

    globals = DM::View("CITY", DM::COMPONENT, DM::READ);
    globals.addAttribute("SWMM_ID");
    globals.addAttribute("Vr");
    globals.addAttribute("Vp");
    globals.addAttribute("Vout");
    globals.addAttribute("Vwwtp");
    globals.addAttribute("drainage_capacity");

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
    this->use_linear_cf = true;
    this->writeResultFile = false;
    this->climateChangeFactorFromCity = false;
    this->calculationTimestep = 1;
    this->consider_built_time = false;
    years = 0;

    this->isCombined = false;
    this->addParameter("Folder", DM::STRING, &this->FileName);
    this->addParameter("RainFile", DM::FILENAME, &this->RainFile);
    this->addParameter("ClimateChangeFactor", DM::DOUBLE, & this->climateChangeFactor);
    this->addParameter("use euler", DM::BOOL, & this->use_euler);
    this->addParameter("return period", DM::DOUBLE, &this->return_period);
    this->addParameter("combined system", DM::BOOL, &this->isCombined);
    this->addParameter("use_linear_cf", DM::BOOL, &this->use_linear_cf);
    this->addParameter("writeResultFile", DM::BOOL, &this->writeResultFile);
    this->addParameter("climateChangeFactorFromCity", DM::BOOL, &this->climateChangeFactorFromCity);
    this->addParameter("calculationTimestep", DM::INT, & this->calculationTimestep);
    this->addParameter("consider_build_time", DM::BOOL, & this->consider_built_time);

    counterRain = 0;
    this->addData("City", views);
    unique_name = QUuid::createUuid().toString().toStdString();

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

    if (!QDir(QString::fromStdString(this->FileName)).exists()) {
        DM::Logger(DM::Warning) <<  this->FileName << "  does not exist!";
    }
}


void DMSWMM::run() {

    if (!QDir(QString::fromStdString(this->FileName)).exists()){
        DM::Logger(DM::Error) <<  this->FileName << "  does not exist!";
        return;
    }
    city = this->getData("City");

    std::vector<std::string>v_cities = city->getUUIDs(globals);
    DM::Component * c = city->getComponent(v_cities[0]);
    if (!c) {
        DM::Logger(DM::Error) << "City not found ";
        return;
    }

    double cf = this->climateChangeFactor;

    if (this->climateChangeFactorFromCity) {
        cf = c->getAttribute("climate_change_factor")->getDouble();
    }

    if (this->use_linear_cf) cf = 1. + years / 20. * (this->climateChangeFactor - 1.);

    this->years++;

    if (this->internalTimestep == this->calculationTimestep) this->internalTimestep = 0;
    this->internalTimestep++;

    if (this->internalTimestep != 1) {
        return;
    }

    SWMMWriteAndRead * swmm;

    if (!this->use_euler)
        swmm = new SWMMWriteAndRead(city, this->RainFile, this->FileName);
    else {
        std::stringstream rfile;
        rfile << "/tmp/rain_"<< QUuid::createUuid().toString().toStdString();
        DrainageHelper::CreateEulerRainFile(60, 5, this->return_period, cf, rfile.str());
        swmm = new SWMMWriteAndRead(city, rfile.str(), this->FileName);
    }

    swmm->setBuildYearConsidered(this->consider_built_time);
    swmm->setClimateChangeFactor(cf);
    swmm->setupSWMM();
    swmm->runSWMM();
    swmm->readInReportFile();

    std::vector<std::pair<std::string, double> > flooding = swmm->getFloodedNodes();
    for (uint i = 0; i < flooding.size(); i++) {
        std::pair<std::string, double> flo = flooding[i];
        DM::Component * c = city->getComponent(flo.first);
        c->addAttribute("flooding_V", flo.second);
    }

    std::vector<std::pair<std::string, double> > node_depth = swmm->getNodeDepthSummery();
    for (uint i = 0; i < node_depth.size(); i++) {
        std::pair<std::string, double> no = node_depth[i];
        DM::Component * c = city->getComponent(no.first);
        c->addAttribute("node_depth", no.second);
    }

    std::vector<std::pair<std::string, double> > capacity = swmm->getLinkFlowSummeryCapacity();
    for (uint i = 0; i < capacity.size(); i++) {
        std::pair<std::string, double> cap = capacity[i];
        DM::Component * c = city->getComponent(cap.first);
        c->addAttribute("capacity", cap.second);
    }

    std::vector<std::pair<std::string, double> > velocity = swmm->getLinkFlowSummeryVelocity();
    for (uint i = 0; i < velocity.size(); i++) {
        std::pair<std::string, double> velo = velocity[i];
        DM::Component * c = city->getComponent(velo.first);
        c->addAttribute("velocity", velo.second);
    }

    c->addAttribute("drainage_capacity", swmm->getAverageCapacity());
    c->addAttribute("SWMM_ID", swmm->getSWMMUUIDPath());
    c->addAttribute("Vr", swmm->getVSurfaceRunoff());
    c->addAttribute("Vp", swmm->getVp());
    c->addAttribute("Vwwtp", swmm->getVwwtp());
    c->addAttribute("Vout", swmm->getVout());


    if (!writeResultFile) {
        delete swmm;
        return;
    }

    int current_year = c->getAttribute("year")->getDouble();

    DM::Logger(DM::Debug) << "Start write output files";
    std::map<std::string, std::string> additionalParameter;
    additionalParameter["year"] = QString::number(current_year).toStdString();
    additionalParameter["population_growth"] = QString::number(city->getAttribute("pop_growth")->getDouble()).toStdString();
    additionalParameter["climate_change_factor\t"] = QString::number(cf).toStdString();
    additionalParameter["return_period"] =  QString::number(this->return_period).toStdString();
    additionalParameter["renewal_rate"] =  QString::number(c->getAttribute("renewal_rate")->getDouble()).toStdString();

    std::stringstream fname;
    Logger(Standard) << "Start Write Report File " <<fname.str();
    fname << this->FileName << "/"  <<  current_year << "_" << unique_name << ".cvs";

    DrainageHelper::WriteOutputFiles(fname.str(), city, *swmm, additionalParameter);

    delete swmm;
}
