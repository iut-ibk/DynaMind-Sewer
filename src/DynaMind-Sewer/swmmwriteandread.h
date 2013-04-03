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
private:

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


    void readInReportFile();
    void runSWMM();

    std::stringstream curves;
    std::string rainfile;

    std::vector<DM::Node*> PointList;


    double Vp;
    double Vr;
    double Vwwtp;

    double climateChangeFactor;

    std::vector<std::pair <std::string, double> > floodedNodes;

    QFile reportFile;


public:
    SWMMWriteAndRead(DM::System * city, std::string rainfile, std::string filename  = "");
    void setRainFile(std::string rainfile);
    void setClimateChangeFactor(int cf);
    void writeSWMMFile();
    void run();
    std::string getSWMMUUID();

    std::vector<std::pair<string, double> > getFloodedNodes();






};

#endif // SWMMWRITEANDREAD_H
