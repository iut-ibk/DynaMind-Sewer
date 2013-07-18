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

#include "swmmwriteandread.h"

#include <QDir>
#include <QUuid>
#include <QProcess>
#include <QTextStream>
#include <QSettings>
#include <math.h>
#include <algorithm>
#include <fstream>

using namespace DM;
SWMMWriteAndRead::SWMMWriteAndRead(DM::System * city, std::string rainfile, std::string filename) :
    city(city),
    climateChangeFactor(1),
    rainfile(rainfile),
    setting_timestep(7),
    built_year_considered(false)
{
    GLOBAL_Counter = 1;
    this->createViewDefinition();

    curves.str("");

    QDir tmpPath;
    if (filename.empty()) {
        tmpPath = QDir::tempPath();
    } else {
        tmpPath =QDir(QString::fromStdString(filename));
    }

    QString UUIDPath = QUuid::createUuid().toString().remove("{").remove("}");

    tmpPath.mkdir(UUIDPath);
    this->SWMMPath.setPath(tmpPath.absolutePath() + "/" + UUIDPath);
}

void SWMMWriteAndRead::setRainFile(string rainfile)
{
    this->rainfile = rainfile;
}

void SWMMWriteAndRead::setClimateChangeFactor(int cf)
{
    this->climateChangeFactor = cf;
}

void SWMMWriteAndRead::readInReportFile() {
    Logger(Debug) << "Read inputfile " << this->SWMMPath.absolutePath() + "/" + "swmm.rep";

    QMap<int, std::string> revUUIDtoINT;



    for (std::map<std::string, int>::const_iterator it = UUIDtoINT.begin(); it !=UUIDtoINT.end(); ++it) {
        revUUIDtoINT[it->second] =  it->first;
    }

    this->reportFile.setFileName(this->SWMMPath.absolutePath() + "/" + "swmm.rep");
    this->reportFile.open(QIODevice::ReadOnly);
    QTextStream in(&this->reportFile);

    bool erroraccrued = false;
    QStringList fileContent;

    QString line;

    do {
        line = in.readLine();
        fileContent << line;
    } while (!line.isNull());

    this->reportFile.close();

    //Search for Errors

    Logger(Debug) << "Search errors ";


    foreach (QString l, fileContent) {
        //if (l.contains("WARNING")) Logger(Warning) << l.toStdString();
        if (l.contains("ERROR")) {
            Logger(Error) << l.toStdString(); erroraccrued = true;
        }

    }
    if (erroraccrued) {
        Logger(Error) << "Error in SWMM";
        return;
    }

    //Start Reading Input
    //Flooded Nodes
    bool FloodSection = false;
    bool SectionSurfaceRunoff = false;
    bool SectionOutfall = false;
    bool FlowRouting = false;
    bool NodeDepthSummery = false;
    bool LinkFlowSummary = false;
    double SurfaceRunOff = 0;

    double Vp = 0;
    ContinuityError = 0;
    VSurfaceStorage = 0;
    double Vwwtp = 0;
    double Voutfall = 0;
    Logger(Debug) << "Start reading Sections ";
    int counter = 0;
    for (int i = 0; i < fileContent.size(); i++) {
        line = fileContent[i];
        if (line.contains("Runoff Quantity Continuity") ) {
            SectionSurfaceRunoff = true;
            continue;
        }
        if (line.contains("Flow Routing Continuity") ) {
            FlowRouting = true;
            continue;
        }
        if (line.contains("Outfall Loading Summary") ) {
            SectionOutfall = true;
            continue;
        }
        if (line.contains("Node Flooding Summary") ) {
            FloodSection = true;
            continue;
        }
        if (line.contains("Node Depth Summary") ) {
            NodeDepthSummery = true;
            continue;
        }
        if (line.contains("Link Flow Summary") ) {
            LinkFlowSummary = true;
            continue;
        }
        if (line.contains("Continuity Error") && FlowRouting == true) {
            QStringList data =line.split(QRegExp("\\s+"));
            this->ContinuityError = QString(data[data.size()-1]).toDouble();
            FlowRouting = false;
            continue;
        }

        if (SectionSurfaceRunoff) {
            if (line.contains("Surface Runoff")) {
                //Start extract
                QStringList data =line.split(QRegExp("\\s+"));
                SurfaceRunOff = QString(data[data.size()-2]).toDouble()*10;
            }
        }

        if (SectionSurfaceRunoff) {
            if (line.contains("Final Surface Storage")) {
                //Start extract
                QStringList data =line.split(QRegExp("\\s+"));
                VSurfaceStorage = QString(data[data.size()-2]).toDouble()*10;
            }
        }

        if (SectionOutfall) {
            if (line.contains("NODE")) {
                //Start extract
                QStringList data =line.split(QRegExp("\\s+"));
                for (int j = 0; j < data.size(); j++) {
                    if (data[j].size() == 0) {
                        data.removeAt(j);
                    }
                }
                //Extract Node id
                if (data.size() != 11) {
                    Logger(Error) << "Error in Extraction Outfall Nodes";

                } else {
                    QString id_asstring = data[0];
                    id_asstring.remove("NODE");
                    int id = id_asstring.toInt();
                    DM::Node * p = this->city->getNode(revUUIDtoINT[id]);
                    if (p->getAttribute("WWTP")->getDouble() > 0.01) {
                        p->changeAttribute("OutfallVolume",  QString(data[4]).toDouble());
                        Vwwtp +=QString(data[4]).toDouble();
                    } else {
                        p->changeAttribute("OutfallVolume",  QString(data[4]).toDouble());
                        Voutfall +=QString(data[4]).toDouble();
                    }
                }


            }
        }
        if (FloodSection) {
            if (line.contains("NODE")) {
                //Start extract
                QStringList data =line.split(QRegExp("\\s+"));
                for (int j = 0; j < data.size(); j++) {
                    if (data[j].size() == 0) {
                        data.removeAt(j);
                    }
                }
                //Extract Node id
                if (data.size() != 7) {
                    Logger(Error) << "Error in Extraction Flooded Nodes";

                } else {
                    QString id_asstring = data[0];
                    id_asstring.remove("NODE");
                    int id = id_asstring.toInt();
                    DM::Node * p = this->city->getNode(revUUIDtoINT[id]);
                    p->changeAttribute("flooding_V",  QString(data[5]).toDouble());
                    Vp += QString(data[5]).toDouble();
                    floodedNodesVolume.push_back(std::pair<std::string, double> (p->getUUID(),QString(data[5]).toDouble() ));
                }
            }
        }

        if (NodeDepthSummery) {
            if (line.contains("NODE")) {
                //Start extract
                QStringList data =line.split(QRegExp("\\s+"));
                for (int j = 0; j < data.size(); j++) {
                    if (data[j].size() == 0) {
                        data.removeAt(j);
                    }
                }
                //Extract Node id
                if (data.size() != 7) {
                    Logger(Error) << "Error in Extraction Flooded Nodes";

                } else {
                    QString id_asstring = data[0];
                    id_asstring.remove("NODE");
                    int id = id_asstring.toInt();
                    DM::Node * p = this->city->getNode(revUUIDtoINT[id]);
                    if (!p) {
                        Logger(Warning) << id_asstring.toStdString()  << " doesn't exist : line: " << line.toStdString();
                        continue;
                    }
                    nodeDepthSummery.push_back(std::pair<std::string, double> (p->getUUID(),QString(data[3]).toDouble() ));
                }
            }
        }

        if (LinkFlowSummary) {
            if (line.contains("LINK")) {
                //Start extract
                QStringList data =line.split(QRegExp("\\s+"));
                for (int j = 0; j < data.size(); j++) {
                    if (data[j].size() == 0) {
                        data.removeAt(j);
                    }
                }
                //Extract Node id
                if (data.size() != 8) {
                    Logger(Error) << "Error in Extraction Link Flow Summary";

                } else {
                    QString id_asstring = data[0];
                    id_asstring.remove("LINK");
                    int id = id_asstring.toInt();
                    DM::Edge * p = this->city->getEdge(revUUIDtoINT[id]);
                    if (!p) {
                        Logger(Warning) << id_asstring.toStdString()  << " doesn't exist : line: " << line.toStdString();
                        continue;
                    }
                    linkFlowSummery_capacity.push_back(std::pair<std::string, double> (p->getUUID(),QString(data[7]).toDouble() ));
                    linkFlowSummery_velocity.push_back(std::pair<std::string, double> (p->getUUID(),QString(data[6]).toDouble() ));
                }
            }
        }

        if (counter > 0 && line.contains("************")) {
            FloodSection = false;
            SectionSurfaceRunoff = false;
            SectionOutfall = false;
            NodeDepthSummery = false;
            LinkFlowSummary = false;
            counter = -1;
        }
        counter ++;
    }
    Logger (Standard)  << "Vp " << Vp;
    Logger (Standard)  << "Vr " << SurfaceRunOff;
    Logger (Standard)  << "Vwwtp " << Vwwtp;
    Logger (Standard)  << "Voutfall " << Voutfall;
    Logger (Standard)  << "Continuty Error " << this->ContinuityError;
    Logger (Standard)  << "Average Capacity " << this->getAverageCapacity();


    this->Vp = Vp;
    this->Vwwtp = Vwwtp;
    this->VsurfaceRunoff = SurfaceRunOff;
    this->Vout = Voutfall;

    foreach (std::string s, this->city->getUUIDsOfComponentsInView(globals)) {
        DM::Component * c = this->city->getComponent(s);
        c->addAttribute("Vr", SurfaceRunOff);
        c->addAttribute("Vwwtp", Vwwtp);
        c->addAttribute("Vp", Vp);
        c->getAttribute("SWMM_ID")->setString(QString(this->SWMMPath.path()).toStdString());
    }

    this->evalWaterLevelInJunctions();
    Logger (Standard)  << "Flooded Nodes " << this->getWaterLeveleBelow0();
}

void SWMMWriteAndRead::writeJunctions(std::fstream &inp)
{

    QStringList l;
    std::vector<std::string> names = city->getUUIDsOfComponentsInView(junctions);

    foreach (std::string name, names) {
        QString s = QString::fromStdString(name);
        if (l.indexOf(s) >= 0) {
            Logger(Error) << "Duplicated Entry";
        }
        l.append(s);
    }
    std::vector<int> checkForduplicatedNodes;
    inp<<"[JUNCTIONS]\n";
    inp<<";;Name\t\tElev\tYMAX\tY0\tYsur\tApond\n";
    inp<<";;============================================================================\n";
    foreach(std::string name, names) {
        DM::Node * p = city->getNode(name);
        /*if (p->isInView( wwtp ))
            continue;*/
        if (p->isInView( outfalls ))
            continue;
        if (p->isInView( storage ))
            continue;
        if (p->getAttribute("Storage")->getDouble() == 1)
            continue;
        if (UUIDtoINT.find(p->getUUID()) == UUIDtoINT.end())
            continue;

        int id = UUIDtoINT[p->getUUID()];

        if (std::find(checkForduplicatedNodes.begin(), checkForduplicatedNodes.end(), id) != checkForduplicatedNodes.end())
            continue;

        checkForduplicatedNodes.push_back(id);
        inp << "NODE";
        inp << id;
        inp << "\t";
        inp << "\t";
        //Get Val
        inp << p->getAttribute("Z")->getDouble();

        inp << "\t";
        inp <<  p->getAttribute("D")->getDouble();
        inp << "\t";
        inp << "0";
        inp << "\t";
        inp << "0";
        inp << "\t";
        inp << "100000";
        inp << "\n";

    }


}


void SWMMWriteAndRead::writeSubcatchments(std::fstream &inp)
{
    this->TotalImpervious = 0;

    //SUBCATCHMENTS
    //-------------------------//
    inp<<"[SUBCATCHMENTS]\n";
    inp<<";;Name\traingage\tOutlet\tArea\tImperv\tWidth\tSlope\n";
    inp<<";;============================================================================\n";
    //for (int i=0;i<subcatchCount;i++)
    //{
    //	inp<<"  sub"<<i<<"\tRG01"<<"\t\tnode"<<i<<"\t20\t80.0\t100.0\t1.0\t1\n";
    //}

    std::vector<std::string> InletNames = city->getUUIDsOfComponentsInView(inlet);

    foreach(std::string name, InletNames) {
        DM::Node * inlet_attr = city->getNode(name);

        std::string CATCHMENT_ID = inlet_attr->getAttribute("CATCHMENT")->getLink().uuid;

        Logger(Debug) << "CATCHMENT_ID  " << CATCHMENT_ID;
        Component * catchment_attr = city->getComponent(CATCHMENT_ID);
        if (inlet_attr->getAttribute("JUNCTION")->getLinks().size() < 1) {
            continue;
        }
        int id = this->UUIDtoINT[inlet_attr->getAttribute("JUNCTION")->getLink().uuid];
        if (id == 0) {
            continue;
        }

        double area = catchment_attr->getAttribute("area")->getDouble()/10000.;

        if ( area < 0 ) {
            continue;
        }

        std::stringstream area_check;
        area_check << area;
        if (area_check.str() == "nan") {
            continue;
        }


        if (UUIDtoINT[CATCHMENT_ID] == 0) {
            UUIDtoINT[CATCHMENT_ID] = GLOBAL_Counter++;
        } else {
            continue;
        }


        double with = sqrt(area*10000.);
        double gradient = fabs(catchment_attr->getAttribute("Gradient")->getDouble());
        double imp = catchment_attr->getAttribute("Impervious")->getDouble();
        if (gradient > 0.01)
            gradient = 0.01;
        if (gradient < 0.001)
            gradient = 0.001;
        this->TotalImpervious += area*imp;

        inp<<"sub"<<UUIDtoINT[CATCHMENT_ID]<<"\tRG01"<<"\t\tnode"<<id<<"\t" << area << "\t" <<imp*100 << "\t"<< with << "\t"<<gradient*100<<"\t1\n";

    }
    inp<<"[POLYGONS]\n";
    inp<<";;Subcatchment\tX-Coord\tY-Coord\n";
    int counter = 0;
    foreach(std::string name, InletNames) {
        DM::Node * inlet_attr = city->getNode(name);

        std::string CATCHMENT_ID = inlet_attr->getAttribute("CATCHMENT")->getLink().uuid;

        if (UUIDtoINT[CATCHMENT_ID] == 0) {
            continue;
        }

        DM::Face * catchment_attr = city->getFace(CATCHMENT_ID);


        foreach(std::string s,  catchment_attr->getNodes()){
            DM::Node * n = city->getNode(s);
            inp << "sub" << UUIDtoINT[CATCHMENT_ID] <<"\t" << n->getX() << "\t" << n->getY()<< "\n";
        }
        counter++;

    }

    inp<<"\n";
    //-------------------------//

    //SUBAREAS
    //-------------------------//
    inp<<"[SUBAREAS]\n";
    inp<<";;Subcatch\tN_IMP\tN_PERV\tS_IMP\tS_PERV\t%ZER\tRouteTo\n";
    inp<<";;============================================================================\n";
    foreach(std::string name, InletNames) {
        DM::Node * inlet_attr = city->getNode(name);
        std::string CATCHMENT_ID = inlet_attr->getAttribute("CATCHMENT")->getLink().uuid;
        int id = this->UUIDtoINT[inlet_attr->getAttribute("JUNCTION")->getLink().uuid];
        if (id == 0) {
            continue;
        }
        if (UUIDtoINT[CATCHMENT_ID] == 0) {
            continue;
        }
        inp<<"  sub"<<UUIDtoINT[CATCHMENT_ID]<<"\t\t0.015\t0.2\t1.8\t5\t0\tOUTLET\n";
    }
    inp<<"\n";
    //-------------------------//


    //INFILTRATION
    //-------------------------//
    inp<<"[INFILTRATION]\n";
    inp<<";;Subcatch\tMaxRate\tMinRate\tDecay\tDryTime\tMaxInf\n";
    inp<<";;============================================================================\n";
    foreach(std::string name, InletNames) {
        DM::Node * inlet_attr = city->getNode(name);

        std::string CATCHMENT_ID = inlet_attr->getAttribute("CATCHMENT")->getLink().uuid;
        DM::Face * catchment_attr = city->getFace(CATCHMENT_ID);
        if (!catchment_attr)
            continue;

        int id = this->UUIDtoINT[CATCHMENT_ID];
        if (id == 0) {
            continue;
        }
        inp<<"  sub"<<UUIDtoINT[CATCHMENT_ID]<<"\t60\t6.12\t3\t6\t0\n";
    }
    inp<<"\n";

}

void SWMMWriteAndRead::writeDWF(std::fstream &inp) {
    std::vector<std::string> InletNames = city->getUUIDsOfComponentsInView(inlet);
    //DWF Dry Weather Flow
    //-------------------------//

    inp<<"[DWF]\n";
    inp<<";;Node\tItem\tValue\n";
    inp<<";;============================================================================\n";

    foreach(std::string name, InletNames) {
        DM::Node * inlet_attr = city->getNode(name);

        std::string CATCHMENT_ID = inlet_attr->getAttribute("CATCHMENT")->getString();

        if (UUIDtoINT[CATCHMENT_ID] == 0) {
            UUIDtoINT[CATCHMENT_ID] = GLOBAL_Counter++;
        }

        double Q = inlet_attr->getAttribute("WasteWater")->getDouble();
        if (Q > 0.000001) {
            inp<<"NODE"<<UUIDtoINT[CATCHMENT_ID]<<"\tFLOW\t"<<Q<<"\n";
        }
    }
    inp<<"\n";
    //-------------------------//

    //POLLUTANTS
    //-------------------------//
    inp<<"[POLLUTANTS]\n";
    inp<<";;Name\tUnits\tCppt\tCgw\tCii\tKd\tSnow\tCoPollut\tCoFract\n";
    inp<<";;============================================================================\n";
    inp<<"ssolid\tMG/L\t0\t0\t0\t0\tNO\n";
    inp<<"vssolid\tMG/L\t0\t0\t0\t0\tNO\n";
    inp<<"cod\tMG/L\t0\t0\t0\t0\tNO\n";
    inp<<"cods\tMG/L\t0\t0\t0\t0\tNO\n";
    inp<<"nh4\tMG/L\t0\t0\t0\t0\tNO\n";
    inp<<"no3\tMG/L\t0\t0\t0\t0\tNO\n";
}
void SWMMWriteAndRead::writeStorage(std::fstream &inp) {
    //-------------------------//

    //STROGAE
    //-------------------------//
    inp<<"\n";
    inp<<"[STORAGE]\n";
    inp<<";;               Invert   Max.     Init.    Shape      Shape                      Ponded   Evap.\n"  ;
    inp<<";;Name           Elev.    Depth    Depth    Curve      Params                     Area     Frac. \n"  ;
    inp<<";;-------------- -------- -------- -------- ---------- -------- -------- -------- -------- --------\n";
    //\nODE85           93.7286  6.35708  0        FUNCTIONAL 1000     0        22222    1000     0
    std::vector<std::string> storages = this->city->getUUIDsOfComponentsInView(storage);
    foreach(std::string name, storages) {
        std::stringstream storage_id;

        Node * p = this->city->getNode(name);

        int id = this->UUIDtoINT[p->getUUID()];
        storage_id << "NODE" << id;
        inp << "NODE";
        inp << id;
        inp << "\t";
        inp << "\t";


        if (p->getAttribute("type")->getString() == "TABULAR") {
            inp << p->getAttribute("Z")->getDouble();
            inp << "\t";
            inp << p->getAttribute("max_depth")->getDouble();
            inp << "\t";
            inp << "0";
            inp << "\t";
            inp << p->getAttribute("type")->getString();
            inp << "\t";
            inp << storage_id.str();
            inp << "\t";
            inp << 10000;
            inp << "\t";
            inp << 0;
            inp << "\n";
            //Write XSection to CURVES
            std::vector<double> storage_x = p->getAttribute("storage_x")->getDoubleVector();
            std::vector<double> storage_y = p->getAttribute("storage_y")->getDoubleVector();

            for (unsigned int i = 0; i < storage_x.size(); i++) {
                curves << storage_id.str() << "\t";
                if (i == 0)
                    curves << "STORAGE" << "\t";
                else
                    curves  << "\t";
                curves << storage_x[i] << "\t";
                curves << storage_y[i] << "\t";

                curves <<  "\n";
            }
        }

        if (p->getAttribute("type")->getString() == "FUNCTIONAL") {
            inp << p->getAttribute("Z")->getDouble()-( p->getAttribute("D")->getDouble()-2);
            inp << "\t";
            inp <<  p->getAttribute("D")->getDouble()+1;
            inp << "\t";
            inp << "\t";
            inp << "0";
            inp << "\t";
            inp << "FUNCTIONAL";
            inp << "\t";
            inp << "0";
            inp << "\t";
            inp << "0";
            inp << "\t";
            inp << p->getAttribute("StorageA")->getDouble();
            inp << "\t";
            inp << "1000";
            inp << "0";
            inp << "\n";
        }
    }
}

void SWMMWriteAndRead:: writeOutfalls(std::fstream &inp) {
    //OUTFALLS
    //-------------------------//
    inp<<"[OUTFALLS]\n";
    inp<<";;Name	Elev	Stage	Gate\n";
    inp<<";;============================================================================\n";

    std::vector<std::string> OutfallNames = city->getUUIDsOfComponentsInView(outfalls);
    for (unsigned int i = 0; i < OutfallNames.size(); i++ ) {
        DM::Node * p = city->getNode(OutfallNames[i]);
        /*if (UUIDtoINT.find(p->getUUID()) == UUIDtoINT.end())
            continue;*/
        if (UUIDtoINT[p->getUUID()] == 0) {
            UUIDtoINT[p->getUUID()] = GLOBAL_Counter++;
            this->PointList.push_back(p);
        }

        double z = p->getAttribute("Z")->getDouble();
        inp<<"NODE"<<this->UUIDtoINT[p->getUUID()]<<"\t"<< z <<"\tFREE\tNO\n";
    }

}
void SWMMWriteAndRead::writeConduits(std::fstream &inp) {
    inp<<"\n";
    inp<<"[CONDUITS]\n";
    inp<<";;Name	Node1	Node2	Length	N	Z1	Z2\n";
    inp<<";;============================================================================\n";

    std::vector<DM::View> conduits;
    conduits.push_back(conduit);

    foreach(DM::View con, conduits)  {
        std::vector<std::string> ConduitNames = city->getUUIDsOfComponentsInView(con);

        foreach(std::string name, ConduitNames) {
            DM::Edge * link = city->getEdge(name);

            if (UUIDtoINT.find(link->getUUID()) == UUIDtoINT.end())
                continue;


            DM::Node * nStartNode = city->getNode(link->getStartpointName());
            DM::Node * nEndNode = city->getNode(link->getEndpointName());


            double x = nStartNode->getX()  - nEndNode->getX();
            double y = nStartNode->getY() - nEndNode->getY();

            double length = sqrt(x*x +y*y);

            if (UUIDtoINT[nStartNode->getUUID()] == 0) {
                UUIDtoINT[nStartNode->getUUID()] = GLOBAL_Counter++;
            }
            if (UUIDtoINT[nEndNode->getUUID()] == 0) {
                UUIDtoINT[nEndNode->getUUID()] = GLOBAL_Counter++;
            }

            int EndNode = UUIDtoINT[nEndNode->getUUID()];
            int StartNode =  UUIDtoINT[nStartNode->getUUID()];

            if ( EndNode == StartNode) {
                Logger(Warning) << "Start Node is End Node";
                continue;
            }


            if (UUIDtoINT[link->getUUID()] == 0) {
                UUIDtoINT[link->getUUID()] = GLOBAL_Counter++;
            }


            inp<<"LINK"<< UUIDtoINT[link->getUUID()]<<"\tNODE"<<EndNode<<"\tNODE"<<StartNode<<"\t"<<length<<"\t"<<"0.01	" << link->getAttribute("inlet_offset")->getDouble()  <<"\t"	<< link->getAttribute("outlet_offset")->getDouble()  << "\n";

        }
    }

}
void SWMMWriteAndRead::writeLID_Controlls(std::fstream &inp) {

    std::vector<std::string> InletNames = city->getUUIDsOfComponentsInView(inlet);

    inp << "\n";
    inp << "[LID_CONTROLS]" << "\n";
    inp << ";;               Type/Layer Parameters" << "\n";
    inp << ";;-------------- ---------- ----------" << "\n";

    foreach(std::string name, InletNames) {
        DM::Component * inlet_attr;
        inlet_attr = city->getComponent(name);

        std::string CATCHMENT_ID = inlet_attr->getAttribute("CATCHMENT")->getLink().uuid;

        Component * catchment_attr = city->getComponent(CATCHMENT_ID);

        if (!catchment_attr)
            continue;

        int id = this->UUIDtoINT[CATCHMENT_ID];
        if (id == 0) {
            continue;
        }

        double area = catchment_attr->getAttribute("Area")->getDouble();// node->area/10000.;

        int numberOfInfiltrationSystems = catchment_attr->getAttribute("INFILTRATION_SYSTEM")->getLinks().size();

        if (numberOfInfiltrationSystems == 0 || area == 0)
            continue;


        /**  [LID_CONTROLS]
    ;;               Type/Layer Parameters∫
    ;;-------------- ---------- ----------
    Infitration      IT
    Infitration      SURFACE    depth        vegetataive cover       surface roughness       surface slope           5
    Infitration      STORAGE    height        void ratio       conductivity         clogging factor
    Infitration      DRAIN      Drain Coefficient          Crain Exponent        Drain Offset Height          6  */

        std::vector<DM::LinkAttribute> infitration_systems = catchment_attr->getAttribute("INFILTRATION_SYSTEM")->getLinks();
        foreach (LinkAttribute la, infitration_systems) {
            if (UUIDtoINT[la.uuid] == 0) {
                UUIDtoINT[la.uuid] = GLOBAL_Counter++;
            } else
                continue;
            DM::Component * infilt = city->getComponent(la.uuid);

            inp << "Infiltration" << UUIDtoINT[la.uuid] << "\t"  << "IT" <<  "\n";
            inp << "Infiltration" << UUIDtoINT[la.uuid] << "\t"  << "SURFACE" <<    "\t" <<  infilt->getAttribute("h")->getDouble()*1000<<    "\t"     <<   "0.0"  <<    "\t"  <<   "0.1"<<    "\t" <<      0.1 <<    "\t" <<      "5" << "\n";
            inp << "Infiltration" << UUIDtoINT[la.uuid] << "\t"  << "STORAGE" <<    "\t" <<  "200"<<    "\t"     <<   "0.75"  <<    "\t"  <<   infilt->getAttribute("kf")->getDouble() * 1000<<    "\t" <<       "0" << "\n";
            inp << "Infiltration" << UUIDtoINT[la.uuid] << "\t"  << "DRAIN" <<    "\t" <<  "0" <<    "\t"     <<   "0.5"  <<     "\t"<< "0"<< "\t"<< "6" << "\n";
            inp<<"\n";
        }
    }
}
void SWMMWriteAndRead::writeLID_Usage(std::fstream &inp) {




    std::vector<std::string> InletNames = city->getUUIDsOfComponentsInView(inlet);

    inp << "\n";

    inp <<  "[LID_USAGE]"  << "\n";
    inp << ";;Subcatchment   LID Process      Number  Area       Width      InitSatur  FromImprv  ToPerv     Report File"  << "\n";
    inp << ";;-------------- ---------------- ------- ---------- ---------- ---------- ---------- ---------- -----------"  << "\n";
    std::map<int, int> written_infils;
    foreach(std::string name, InletNames) {
        DM::Component * inlet_attr;
        inlet_attr = city->getComponent(name);


        std::string CATCHMENT_ID = inlet_attr->getAttribute("CATCHMENT")->getLink().uuid;

        Component * catchment_attr = city->getComponent(CATCHMENT_ID);

        if (!catchment_attr)
            continue;

        int id = this->UUIDtoINT[CATCHMENT_ID];
        if (id == 0) {
            continue;
        }

        double area = catchment_attr->getAttribute("Area")->getDouble();// node->area/10000.;
        double imp = catchment_attr->getAttribute("Impervious")->getDouble();// node->area/10000.;

        int numberOfInfiltrationSystems = catchment_attr->getAttribute("INFILTRATION_SYSTEM")->getLinks().size();

        if (numberOfInfiltrationSystems == 0 || area == 0)
            continue;


        std::vector<DM::LinkAttribute> infitration_systems = catchment_attr->getAttribute("INFILTRATION_SYSTEM")->getLinks();



        foreach (LinkAttribute la, infitration_systems) {
            DM::Component * infilt = city->getComponent(la.uuid);
            double treated = infilt->getAttribute("treated_area")->getDouble();
            Impervious_Infiltration+=treated;

            if (written_infils[UUIDtoINT[la.uuid]] != 0 ) continue;

            double treated_area = treated  / (area*imp) * 100.;

            treated_area = (treated_area < 100.) ? treated_area : 100.;


            inp << "sub" <<UUIDtoINT[CATCHMENT_ID] << "\t"  << "Infiltration"<< UUIDtoINT[la.uuid]  <<    "\t" <<  1 <<    "\t"     <<   infilt->getAttribute("area")->getDouble()   <<    "\t"  <<   "1"<<    "\t" <<       "0" <<    "\t" <<        treated_area <<    "\t" <<    "0" <<    "\n"; // << "\"report" << UUIDtoINT[CATCHMENT_ID] << ".txt\"" << "\n";
            Logger(Debug) << "Catchment Area " << area;
            Logger(Debug) << "Treated Area " << treated;
            written_infils[UUIDtoINT[la.uuid]] = 1;

        }
    }
}

void SWMMWriteAndRead::writeCurves(fstream &inp)
{
    inp<<"\n";
    inp<<"[CURVES]\n";
    inp<<";;Name           Type       X-Value    Y-Value   \n";
    inp<<";;-------------- ---------- ---------- ----------\n";

    inp << curves.str();
    inp<<"\n";
}

void SWMMWriteAndRead::writeXSection(std::fstream &inp) {
    std::vector<std::string> OutfallNames = city->getUUIDsOfComponentsInView(weir);

    std::vector<std::string> WWTPNames = city->getUUIDsOfComponentsInView(wwtp);
    //XSection
    inp<<"\n";
    inp<<"[XSECTIONS]\n";
    inp<<";;Link	Type	G1	G2	G3	G4\n";
    inp<<";;==========================================\n";

    std::vector<DM::View> condies;
    condies.push_back(weir);
    condies.push_back(conduit);

    foreach (DM::View condie, condies) {
        std::vector<std::string> ConduitNames = city->getUUIDsOfComponentsInView(condie);
        foreach(std::string name, ConduitNames) {
            std::string linkname = "";
            if (condie.getName() == "CONDUIT")
                linkname = "LINK";
            if (condie.getName() == "WEIR")
                linkname = "WEIR";
            DM::Edge * link = city->getEdge(name);


            DM::Node * nStartNode = city->getNode(link->getStartpointName());
            DM::Node * nEndNode = city->getNode(link->getEndpointName());

            if (UUIDtoINT[nStartNode->getUUID()] == 0) {
                UUIDtoINT[nStartNode->getUUID()] = GLOBAL_Counter++;
            }
            if (UUIDtoINT[nEndNode->getUUID()] == 0) {
                UUIDtoINT[nEndNode->getUUID()] = GLOBAL_Counter++;
            }

            double d = link->getAttribute("Diameter")->getDouble();

            if (UUIDtoINT[link->getUUID()] == 0)
                continue;
            if (link->getAttribute("XSECTION")->getLinks().size() == 0) {
                if (condie.getName().compare(conduit.getName()) == 0)
                    inp << linkname << UUIDtoINT[link->getUUID()] << "\tCIRCULAR\t"<< d <<" \t0\t0\t0\n";
                continue;
            }

            DM::Component * xscetion = city->getComponent(link->getAttribute("XSECTION")->getLink().uuid);

            if (xscetion->getAttribute("type")->getString() != "CUSTOM") {
                inp << linkname << UUIDtoINT[link->getUUID()] << "\t";
                inp << xscetion->getAttribute("type")->getString() << "\t";
                std::vector<double> diameters = xscetion->getAttribute("diameters")->getDoubleVector();
                foreach (double d, diameters)
                    inp << d << "\t";
                inp << "\n";
                continue;
            }
            std::stringstream section_id;
            section_id << "XSECTION" <<  UUIDtoINT[link->getUUID()];
            inp << linkname << UUIDtoINT[link->getUUID()] << "\t";
            inp << xscetion->getAttribute("type")->getString() << "\t";
            inp << d << "\t";
            inp <<  section_id.str() << "\t";
            inp << "0" << "\t";
            inp << "0" << "\t";
            inp << "\n";

            //Write XSection to CURVES
            std::vector<double> shape_x = xscetion->getAttribute("shape_x")->getDoubleVector();
            std::vector<double> shape_y = xscetion->getAttribute("shape_y")->getDoubleVector();

            for (unsigned int i = 0; i < shape_x.size(); i++) {
                curves << section_id.str() << "\t";
                if (i == 0)
                    curves << xscetion->getAttribute("shape_type")->getString() << "\t";
                else
                    curves  << "\t";
                curves << shape_x[i] << "\t";
                curves << shape_y[i] << "\t";

                curves <<  "\n";
            }
        }
    }
    inp<<"\n";
}


void SWMMWriteAndRead::writeWeir(std::fstream &inp)
{

    inp<<"\n";
    inp<<"[WEIRS]\n";
    inp<<";;               Inlet            Outlet           Weir         Crest      Disch.     Flap End      End \n";
    inp<<";;Name           Node             Node             Type         Height     Coeff.     Gate Con.     Coeff.\n";
    inp<<";;-------------- ---------------- ---------------- ------------ ---------- ---------- ---- -------- ----------\n";
    //LINK984          NODE109          NODE985          TRANSVERSE   0          1.80       NO   0        0
    std::vector<std::string> namesWeir = this->city->getUUIDsOfComponentsInView(weir);
    for (unsigned int i = 0; i < namesWeir.size(); i++ ) {

        DM::Edge * weir = this->city->getEdge(namesWeir[i]);
        DM::Node * startn = this->city->getNode(weir->getStartpointName());
        DM::Node * outfall = this->city->getNode(weir->getEndpointName());

        if (UUIDtoINT[startn->getUUID()] == 0) {
            UUIDtoINT[startn->getUUID()] = GLOBAL_Counter++;
            //this->PointList.push_back(startn);
        }
        if (UUIDtoINT[outfall->getUUID()] == 0) {
            UUIDtoINT[outfall->getUUID()] = GLOBAL_Counter++;
            //this->PointList.push_back(outfall);
        }

        if (UUIDtoINT[weir->getUUID()] == 0) {
            UUIDtoINT[weir->getUUID()] = GLOBAL_Counter++;
        }

        inp<<"WEIR"<<UUIDtoINT[weir->getUUID()]<<"\tNODE"<<UUIDtoINT[startn->getUUID()]<<"\tNODE"<<UUIDtoINT[outfall->getUUID()]<<"\t";
        inp<<"TRANSVERSE" << "\t";
        inp<< weir->getAttribute("crest_height")->getDouble() << "\t";

        inp<< weir->getAttribute("discharge_coefficient")->getDouble() << "\t";
        inp<<"NO" << "\t";
        inp<<"0" << "\t";
        inp<< weir->getAttribute("end_coefficient")->getDouble() << "\t";
        inp << "\n";


    }
}

void SWMMWriteAndRead::writePumps(fstream &inp)
{

    inp<<"\n";
    inp<<"[PUMPS]\n";
    inp<<";;               Inlet            Outlet           Pump             Init.  Startup  Shutoff \n";
    inp<<";;Name           Node             Node             Curve            Status Depth    Depth\n";
    inp<<";;-------------- ---------------- ---------------- ---------------- ------ -------- --------\n";

    std::vector<std::string> namePumps = this->city->getUUIDsOfComponentsInView(pumps);
    for (unsigned int i = 0; i < namePumps.size(); i++ ) {
        DM::Edge * pump = this->city->getEdge(namePumps[i]);
        DM::Node * startn = this->city->getNode(pump->getStartpointName());
        DM::Node * endn = this->city->getNode(pump->getEndpointName());

        if (UUIDtoINT[startn->getUUID()] == 0) {
            UUIDtoINT[startn->getUUID()] = GLOBAL_Counter++;
        }
        if (UUIDtoINT[endn->getUUID()] == 0) {
            UUIDtoINT[endn->getUUID()] = GLOBAL_Counter++;
        }

        if (UUIDtoINT[pump->getUUID()] == 0) {
            UUIDtoINT[pump->getUUID()] = GLOBAL_Counter++;
        }

        std::stringstream pump_id;
        pump_id << "PUMP" << UUIDtoINT[pump->getUUID()];

        inp << pump_id.str()  << "\t";
        inp <<"NODE"<<UUIDtoINT[startn->getUUID()] << "\t";
        inp <<"NODE"<<UUIDtoINT[endn->getUUID()] << "\t";
        inp << pump_id.str() << "\t";
        inp<<"ON" << "\t";
        inp<<"0" << "\t";
        inp<<"0" << "\t" << "\n";

        //Write XSection to PUMPS
        std::vector<double> pump_x = pump->getAttribute("pump_x")->getDoubleVector();
        std::vector<double> pump_y = pump->getAttribute("pump_y")->getDoubleVector();

        for (unsigned int i = 0; i < pump_x.size(); i++) {
            curves << pump_id.str() << "\t";
            if (i == 0)
                curves << "PUMP2" << "\t";
            else
                curves  << "\t";
            curves << pump_x[i] << "\t";
            curves << pump_y[i] << "\t";

            curves <<  "\n";
        }
    }
}


void SWMMWriteAndRead::writeCoordinates(std::fstream &inp)
{
    std::vector<std::string> WWTPs = city->getUUIDsOfComponentsInView(wwtp);
    std::vector<std::string> OutfallNames = city->getUUIDsOfComponentsInView(outfalls);
    //COORDINATES
    //-------------------------//
    inp << "\n";
    inp<<"[COORDINATES]\n";
    inp<<";;Node\tXcoord\tyCoord\n";
    inp<<";;============================================================================\n";


    for (unsigned int i = 0; i < this->PointList.size(); i++ ) {
        DM::Node * node = this->PointList[i];
        double x = node->getX();
        double y = node->getY();

        inp << "NODE" << UUIDtoINT[node->getUUID()] ;
        inp << "\t";
        inp << x;
        inp << "\t";
        inp << y;
        inp << "\n";

    }

    for (unsigned int i = 0; i < OutfallNames.size(); i++ ) {
        DM::Node * node = city->getNode(OutfallNames[i]);
        double x = node->getX();
        double y = node->getY();

        inp << "NODE" << UUIDtoINT[node->getUUID()] ;
        inp << "\t";
        inp << x;
        inp << "\t";
        inp << y;
        inp << "\n";
    }

    /*for ( int i = 0; i < WWTPs.size(); i++ ) {
        DM::Node * node = city->getNode(WWTPs[i]);
        double x = node->getX();
        double y = node->getY();

        int id = i;
        inp << "WWTP" << id ;
        inp << "\t";
        inp << x;
        inp << "\t";
        inp << y;
        inp << "\n";
    }*/

}

void SWMMWriteAndRead::writeSWMMFile() {
    Impervious_Infiltration = 0;
    this->TotalImpervious = 0;

    QString fileName = this->SWMMPath.absolutePath()+ "/"+ "swmm.inp";
    std::fstream inp;
    inp.open(fileName.toAscii(),ios::out);
    writeSWMMheader(inp);
    writeSubcatchments(inp);
    writeLID_Controlls(inp);
    writeLID_Usage(inp);
    writeJunctions(inp);
    writeOutfalls(inp);
    writeStorage(inp);
    writeConduits(inp);
    writeWeir(inp);
    writeXSection(inp);
    writeDWF(inp);
    writeCoordinates(inp);
    writePumps(inp);
    writeCurves(inp);

    inp.close();

}

void SWMMWriteAndRead::setupSWMM()
{
    //Get current year from simulation
    std::vector<std::string> c_uuid = city->getUUIDs(DM::View("CITY", DM::COMPONENT, DM::READ));

    if (c_uuid.size() < 1) {
        DM::Logger(DM::Error) << "City not found";
        return;
    }
    this->currentYear = city->getComponent(c_uuid[0])->getAttribute("year")->getDouble();

    floodedNodesVolume.clear();
    nodeDepthSummery.clear();

    foreach(std::string name , city->getUUIDsOfComponentsInView(conduit))
    {
        DM::Edge * e = city->getEdge(name);
        //If conduit is not added no id is created
        if (this->built_year_considered) {
            if (e->getAttribute("built_year")->getDouble() == 0.0  || e->getAttribute("built_year")->getDouble() > this->currentYear ) {
                continue;
            }
        }

        DM::Node * p1 = city->getNode(e->getStartpointName());
        DM::Node * p2 = city->getNode(e->getEndpointName());

        if (UUIDtoINT[e->getUUID()] == 0) {
            UUIDtoINT[e->getUUID()] = GLOBAL_Counter++;
        }

        if (UUIDtoINT[p1->getUUID()] == 0) {
            UUIDtoINT[p1->getUUID()] = GLOBAL_Counter++;
            this->PointList.push_back(p1);
        }

        if (UUIDtoINT[p2->getUUID()] == 0) {
            UUIDtoINT[p2->getUUID()] = GLOBAL_Counter++;
            this->PointList.push_back(p2);
        }
    }

    this->writeSWMMFile();
    this->writeRainFile();
}

string SWMMWriteAndRead::getSWMMUUIDPath()
{
    return this->SWMMPath.absolutePath().toStdString();
}

std::vector<std::pair<string, double > > SWMMWriteAndRead::getFloodedNodes()
{
    return this->floodedNodesVolume;
}

std::vector<std::pair<string, double> > SWMMWriteAndRead::getNodeDepthSummery()
{
    return this->nodeDepthSummery;
}

std::vector<std::pair<string, double> > SWMMWriteAndRead::getLinkFlowSummeryCapacity()
{

    return this->linkFlowSummery_capacity;
}

std::vector<std::pair<string, double> > SWMMWriteAndRead::getLinkFlowSummeryVelocity()
{
    return this->linkFlowSummery_velocity;
}

double SWMMWriteAndRead::getVp()
{
    return this->Vp;
}

double SWMMWriteAndRead::getVSurfaceRunoff()
{
    return this->VsurfaceRunoff;
}

double SWMMWriteAndRead::getVSurfaceStorage()
{
    return this->VSurfaceStorage;
}

double SWMMWriteAndRead::getVwwtp()
{
    return this->Vwwtp;
}

double SWMMWriteAndRead::getVout()
{
    return this->Vout;
}

double SWMMWriteAndRead::getTotalImpervious()
{
    return this->TotalImpervious;
}

double SWMMWriteAndRead::getContinuityError()
{
    return this->ContinuityError;
}

double SWMMWriteAndRead::getImperiousInfiltration()
{
    return this->Impervious_Infiltration;
}

double SWMMWriteAndRead::getAverageCapacity()
{
    typedef std::pair<std::string, double > rainnode;
    std::vector< std::pair<std::string, double > > valume_capacity = this->getLinkFlowSummeryCapacity();
    double volume_cap = 0;

    foreach (rainnode  fn, valume_capacity) {
        volume_cap+=fn.second;
    }
    return volume_cap/=valume_capacity.size();
}

double SWMMWriteAndRead::getWaterLeveleBelow0()
{
    return this->water_level_below_0;
}

double SWMMWriteAndRead::getWaterLeveleBelow10()
{
    return this->water_level_below_10;
}

double SWMMWriteAndRead::getWaterLeveleBelow20()
{
    return this->water_level_below_20;
}

void SWMMWriteAndRead::setCalculationTimeStep(int timeStep)
{
    this->setting_timestep = timeStep;
}

void SWMMWriteAndRead::setBuildYearConsidered(bool buildyear)
{
    this->built_year_considered = buildyear;
}

SWMMWriteAndRead::~SWMMWriteAndRead()
{


    /*QString dirName = this->SWMMPath.absolutePath();
    QDir dir(dirName);
    bool result;

    if (dir.exists(dirName)) {
        Q_FOREACH(QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden  | QDir::AllDirs | QDir::Files, QDir::DirsFirst)) {
            result = QFile::remove(info.absoluteFilePath());
            if (!result) {
                return;
            }
        }
        result = dir.rmdir(dirName);
    }*/

}

void SWMMWriteAndRead::runSWMM()
{
    this->Vp = 0;
    this->Vwwtp = 0;
    this->VsurfaceRunoff = 0;
    //Find Temp Directory
    QDir tmpPath = QDir::tempPath();
    tmpPath.mkdir("vibeswmm");

    //Path to SWMM
    QSettings settings;
    QString swmmPath = settings.value("SWMM").toString().replace("\\","/");

    QProcess process;
    QStringList argument;
    argument << this->SWMMPath.absolutePath() + "/"+ "swmm.inp" << this->SWMMPath.absolutePath() + "/" + "swmm.rep";
    QString swmm = swmmPath;

#ifdef _WIN32
    process.start(swmm,argument);
#else
    Logger(Debug) << argument.join(" ").toStdString();


    if (!(swmm.contains(".exe") )) process.start(swmm,argument);

    else {
        argument.insert(0, swmm);
        DM::Logger(DM::Standard) << argument.join(" ").toStdString();

        process.start("/usr/local/bin/wine",argument);
    }
#endif

    process.waitForFinished(3000000);
}

void SWMMWriteAndRead::writeRainFile() {
    QFile r_in;
    r_in.setFileName(QString::fromStdString(this->rainfile));
    r_in.open(QIODevice::ReadOnly);
    QTextStream in(&r_in);

    QString line;
    QString fileName = this->SWMMPath.absolutePath()+ "/"+ "rain.dat";
    std::fstream out;
    out.open(fileName.toAscii(),ios::out);

    do {
        line = in.readLine();
        QStringList splitline = line.split(QRegExp("\\s+"));
        if (splitline.size() > 2) {
            for(int i = 0; i < splitline.size() - 1;i++)
                out << QString(splitline[i]).toStdString() << " ";
            double r = QString(splitline[splitline.size()-1]).toDouble();
            out << r*(this->climateChangeFactor) << "\n";
        }

    } while (!line.isNull());

    r_in.close();
    out.close();

}

void SWMMWriteAndRead::createViewDefinition()
{
    conduit = DM::View("CONDUIT", DM::EDGE, DM::READ);
    conduit.getAttribute("Diameter");

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
}

void SWMMWriteAndRead::evalWaterLevelInJunctions()
{
    water_level_below_0 = 0;
    water_level_below_10 = 0;
    water_level_below_20 = 0;
    //Filter Nodes that area acutally flooded
    typedef std::pair<std::string, double > rainnode;
    std::vector< std::pair<std::string, double > > flooded = this->getFloodedNodes();

    foreach (rainnode f, flooded) {
        if (f.second > 0.0) water_level_below_0++;

    }

    std::vector< std::pair<std::string, double > > surcharge = this->getNodeDepthSummery();
    foreach (rainnode  fn, surcharge) {
        DM::Component * n =  this->city->getComponent(fn.first);
        if (fn.second < 0.009) continue;
        if (!n) continue;
        double D = n->getAttribute("D")->getDouble();
        if (D - fn.second < 0.10) water_level_below_10++;
        if (D - fn.second < 0.20) water_level_below_20++;
    }

    water_level_below_0/=surcharge.size();
    water_level_below_10/=surcharge.size();
    water_level_below_20/=surcharge.size();
}

void SWMMWriteAndRead::writeSWMMheader(std::fstream &inp)
{
    inp<<"[TITLE]\n";
    inp<<"VIBe\n\n";
    //-------------------------//

    //OPTIONS
    //-------------------------//
    inp<<"[OPTIONS]\n";
    //inp<<"FLOW_UNITS LPS\n";
    //inp<<"FLOW_ROUTING DYNWAVE\n";
    //inp<<"START_DATE 1/1/2000\n";
    //inp<<"START_TIME 00:00\n";
    //inp<<"END_DATE 1/1/2000\n";
    //inp<<"END_TIME 12:00\n";
    //inp<<"WET_STEP 00:01:00\n";
    //inp<<"DRY_STEP 01:00:00\n";
    //inp<<"ROUTING_STEP 00:05:00\n";
    //inp<<"VARIABLE_STEP 0.99\n";
    //inp<<"REPORT_STEP  00:01:00\n";
    //inp<<"REPORT_START_DATE 1/1/2000\n";
    //inp<<"REPORT_START_TIME 00:00\n\n";

    inp<<"FLOW_UNITS\t\tLPS\n";
    inp<<"INFILTRATION\t\tHORTON\n";
    inp<<"FLOW_ROUTING\t\tDYNWAVE\n";
    //inp<<"START_DATE\t\t7/1/2008\n";
    inp<<"START_DATE\t\t1/1/2000\n";
    inp<<"START_TIME\t\t00:00\n";
    //inp<<"END_DATE\t\t7/31/2008\n";
    inp<<"END_DATE\t\t1/1/2000\n";
    inp<<"END_TIME\t\t12:00\n";
    inp<<"REPORT_START_DATE\t1/1/2000\n";
    inp<<"REPORT_START_TIME\t00:00\n";


    inp<<"SWEEP_START\t\t01/01\n";
    inp<<"SWEEP_END\t\t12/31\n";
    inp<<"DRY_DAYS\t\t0\n";
    inp<<"REPORT_STEP\t\t00:05:00\n";
    inp<<"WET_STEP\t\t00:01:00\n";
    inp<<"DRY_STEP\t\t00:01:00\n";

    if (setting_timestep < 10)  inp<<"ROUTING_STEP\t\t0:00:0"<<QString::number(setting_timestep).toStdString()<<"\n";
    else  inp<<"ROUTING_STEP\t\t0:00:"<<QString::number(setting_timestep).toStdString()<<"\n";

    inp<<"ALLOW_PONDING\t\tNO\n";
    inp<<"INERTIAL_DAMPING\tPARTIAL\n";
    inp<<"VARIABLE_STEP\t\t0.75\n";
    inp<<"LENGTHENING_STEP\t10\n";
    inp<<"MIN_SURFAREA\t\t0\n";
    inp<<"NORMAL_FLOW_LIMITED\tBOTH\n";
    inp<<"SKIP_STEADY_STATE\tNO\n";
    inp<<"IGNORE_RAINFALL\t\tNO\n";
    inp<<"FORCE_MAIN_EQUATION\tH-W\n";
    inp<<"LINK_OFFSETS\t\tDEPTH\n\n";
    //-------------------------//

    //REPORT
    //-------------------------//
    inp<<"[REPORT]\n";
    inp<<"INPUT NO\n";
    inp<<"CONTINUITY YES\n";
    inp<<"FLOWSTATS YES\n";
    inp<<"CONTROLS NO\n";
    inp<<"SUBCATCHMENTS NONE\n";
    inp<<"NODES NONE\n";
    inp<<"LINKS NONE\n\n";
    //-------------------------//

    //RAINFILE
    //-------------------------//
    inp<<"[RAINGAGES]\n";
    inp<<";;Name\tFormat\tInterval\tSCF\tDataSource\tSourceName\tunits\n";
    inp<<";;============================================================================\n";
    inp<<"RG01\tVOLUME\t0:05\t1.0\tFILE\t";
    //inp<< "rain.dat";
    inp<< this->SWMMPath.absolutePath().toStdString() + "/" + "rain.dat";
    //mag mich nicht
    //inp<<rainFile.toAscii();

    inp<<"\tSTA01 MM\n";
    inp<<"\n";
    //-------------------------//

}



