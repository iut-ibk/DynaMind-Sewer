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

#ifndef SWMMWRITEANDREAD_H
#define SWMMWRITEANDREAD_H

#include <dmmodule.h>
#include <dm.h>
#include <QDir>
#include <sstream>

class SWMMWriteAndRead
{
public:
    SWMMWriteAndRead(DM::System * city, std::string rainfile, std::string filename  = "");
    void setRainFile(std::string rainfile);
    void setClimateChangeFactor(int cf);
    void writeSWMMFile();
    void setupSWMM();
    std::string getSWMMUUIDPath();
    void readInReportFile();
    void runSWMM();

    std::vector<std::pair<string, double> > getFloodedNodes();
    std::vector<std::pair<string, double> > getNodeDepthSummery();

    std::vector<std::pair<string, double> > getLinkFlowSummeryCapacity();
    std::vector<std::pair<string, double> > getLinkFlowSummeryVelocity();

    double getVp();
    double getVSurfaceRunoff();
    double getVSurfaceStorage();
    double getVwwtp();
    double getVout();
    double getTotalImpervious();
    double getContinuityError();
    double getImperiousInfiltration();
    double getAverageCapacity();

    double getWaterLeveleBelow0();
    double getWaterLeveleBelow10();
    double getWaterLeveleBelow20();


    /** @brief set caculation timestep for flow rounting in sec */
    void setCalculationTimeStep(int timeStep);

private:

    int setting_timestep;

    int GLOBAL_Counter;
    DM::System * city;
    DM::View conduit;
    DM::View inlet;
    DM::View junctions;
    DM::View endnodes;
    DM::View catchment;
    DM::View outfalls;
    DM::View weir;
    DM::View wwtp;
    DM::View storage;
    DM::View pumps;
    DM::View globals;

    QDir SWMMPath;


    std::map<std::string, int> UUIDtoINT;

    void writeSWMMheader(std::fstream &inp);
    void writeSubcatchments(std::fstream &inp);
    void writeJunctions(std::fstream &inp);
    void writeDWF(std::fstream &inp);
    void writeStorage(std::fstream &inp);
    void writeOutfalls(std::fstream &inp);
    void writeConduits(std::fstream &inp);
    void writeXSection(std::fstream &inp);
    void writeWeir(std::fstream &inp);
    void writePumps(std::fstream &inp);
    void writeCoordinates(std::fstream &inp);
    void writeLID_Controlls(std::fstream &inp);
    void writeLID_Usage(std::fstream &inp);
    void writeCurves(std::fstream & inp);


    void writeRainFile();

    void createViewDefinition();

    std::stringstream curves;
    std::string rainfile;

    std::vector<DM::Node*> PointList;


    double Vp;
    double VsurfaceRunoff;
    double VSurfaceStorage;
    double Vwwtp;
    double Vout;
    double TotalImpervious;
    double Impervious_Infiltration;
    double ContinuityError;
    double climateChangeFactor;

    void evalWaterLevelInJunctions();

    std::vector<std::pair <std::string, double> > floodedNodesVolume;

    std::vector<std::pair <std::string, double> > nodeDepthSummery;

    std::vector<std::pair <std::string, double> > linkFlowSummery_capacity;

    std::vector<std::pair <std::string, double> > linkFlowSummery_velocity;

    //Water level in percent
    double water_level_below_0;
    double water_level_below_10;
    double water_level_below_20;

    QFile reportFile;

};

#endif // SWMMWRITEANDREAD_H
