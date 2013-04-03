#ifndef SWMMRETURNPERIOD_H
#define SWMMRETURNPERIOD_H


#include <dmmodule.h>
#include <dm.h>
#include <QDir>
#include <sstream>


class DM_HELPER_DLL_EXPORT SWMMReturnPeriod : public  DM::Module {
    DM_DECLARE_NODE (SWMMReturnPeriod)

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
    DM::View flooding_area;

    DM::View vcity;


    std::string RainFile;
    std::string FileName;
    std::string outputFiles;
    std::vector<DM::Node*> PointList;
    bool isCombined;



    double climateChangeFactor;



    void writeRainFile();
    int years;
    double counterRain;

    std::map<std::string, int> UUIDtoINT;

    std::stringstream curves;
    void createEulerRainFile(double duration, double deltaT, double return_period);

private:
    void writeOutputFiles(DM::System * sys, double rp, std::vector<std::pair<std::string, double> > &  flooding_vec, std::string swmmuuid);
public:
    SWMMReturnPeriod();
    void run();
    void init();
};


#endif // SWMMRETURNPERIOD_H
