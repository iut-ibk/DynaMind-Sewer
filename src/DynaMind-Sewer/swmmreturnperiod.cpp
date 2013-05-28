#include "swmmreturnperiod.h"

#include <ctime>
#include <fstream>
#include <QDateTime>
#include <QUuid>
#include <drainagehelper.h>

#ifdef _OPENMP
#include <omp.h>
#endif

DM_DECLARE_NODE_NAME(SWMMReturnPeriod, Sewer)
SWMMReturnPeriod::SWMMReturnPeriod()
{
    srand((unsigned)time(0));

    GLOBAL_Counter = 1;
    unique_name = QUuid::createUuid().toString().toStdString();
    internalTimestep = 0;

    conduit = DM::View("CONDUIT", DM::EDGE, DM::READ);
    conduit.getAttribute("Diameter");

    inlet = DM::View("INLET", DM::NODE, DM::READ);
    inlet.getAttribute("CATCHMENT");


    junctions = DM::View("JUNCTION", DM::NODE, DM::READ);
    junctions.getAttribute("D");
    junctions.addAttribute("flooding_V");
    junctions.addAttribute("FLOODING_AREA");

    endnodes = DM::View("OUTFALL", DM::NODE, DM::READ);
    endnodes.addAttribute("OutfallVolume");

    catchment = DM::View("CATCHMENT", DM::FACE, DM::READ);
    catchment.getAttribute("WasteWater");
    catchment.getAttribute("area");
    catchment.getAttribute("Impervious");

    flooding_area = DM::View("FLOODING_AREA", DM::FACE, DM::READ);
    flooding_area.getAttribute("JUNCTION");
    flooding_area.addAttribute("return_period");

    vcity = DM::View("CITY", DM::FACE, DM::READ);
    vcity.getAttribute("year");

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
    globals.addAttribute("Vwwtp");
    globals.addAttribute("pop_growth");

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
    views.push_back(flooding_area);
    views.push_back(vcity);

    this->FileName = "";
    this->climateChangeFactor = 1;
    this->RainFile = "";
    this->calculationTimestep = 1;
    this->use_linear_cf = true;
    this->CFSamples = 1;
    cfRand = false;
    years = 0;

    this->isCombined = false;
    this->addParameter("Folder", DM::STRING, &this->FileName);
    this->addParameter("RainFile", DM::FILENAME, &this->RainFile);
    this->addParameter("ClimateChangeFactor", DM::DOUBLE, & this->climateChangeFactor);
    this->addParameter("combined system", DM::BOOL, &this->isCombined);
    this->addParameter("outputFiles", DM::FILENAME, &this->outputFiles);
    this->addParameter("calculationTimestep", DM::INT, & this->calculationTimestep);
    this->addParameter("cfRand", DM::BOOL, & this->cfRand);
    this->addParameter("use_linear_cf", DM::BOOL, &this->use_linear_cf);
    this->addParameter("CFSamples", DM::INT, &this->CFSamples);
    counterRain = 0;
    this->addData("City", views);
}

void SWMMReturnPeriod::init() {

    std::vector<DM::View> views;
    views.push_back(conduit);
    views.push_back(inlet);
    views.push_back(junctions);
    views.push_back(endnodes);
    views.push_back(catchment);
    views.push_back(outfalls);
    views.push_back(flooding_area);

    if (isCombined){
        views.push_back(weir);
        views.push_back(wwtp);
        views.push_back(storage);
    }

    views.push_back(globals);

    this->addData("City", views);
}

void SWMMReturnPeriod::run() {

    if (years == 0) {
        climateChangeFactors.clear();
        if (!cfRand) {
            climateChangeFactors.push_back(1.0);
            climateChangeFactors.push_back(1.1);
            climateChangeFactors.push_back(1.3);
            climateChangeFactors.push_back(1.5);
        } else {
            for (int i = 0; i < this->CFSamples; i++) {
                climateChangeFactors.push_back( (float)rand()/((float)RAND_MAX/0.5) + 1);
            }
        }
    }

    std::vector<double> return_periods;

    //return_periods.push_back(0.25);
    //return_periods.push_back(0.5);
    return_periods.push_back(1);
    return_periods.push_back(2);
    //return_periods.push_back(5);
    //return_periods.push_back(10);
    /*return_periods.push_back(20);
    return_periods.push_back(30);
    return_periods.push_back(50);
    return_periods.push_back(100);
    return_periods.push_back(200);*/

    city = this->getData("City");

    std::vector<double> cfs;

    foreach (double cf, climateChangeFactors) {
        if (this->use_linear_cf) cfs.push_back(1. + years / 20. * (cf - 1.));
        else cfs.push_back(cf);
    }

    this->years++;

    if (this->internalTimestep == this->calculationTimestep) this->internalTimestep = 0;
    this->internalTimestep++;


    if (this->internalTimestep != 1) {
        return;
    }

    int numberOfRPs = return_periods.size() * climateChangeFactors.size();
    int numberOfcfs = climateChangeFactors.size();


    std::vector<SWMMWriteAndRead*> swmmruns;
    for (int nof = 0; nof < numberOfRPs; nof++ ) {

        int index_rp = nof / numberOfcfs;
        int index_cf = nof % numberOfcfs;

        double rp = return_periods[index_rp];
        double cf = cfs[index_cf];
        std::stringstream rfile;
        rfile << QDir::tempPath().toStdString() << "/";
        rfile << "rain_" << QUuid::createUuid().toString().toStdString();

        DM::Logger(DM::Standard) << "return_period " <<  rp;
        DM::Logger(DM::Standard) << "cf_period " <<  cf;
        DrainageHelper::CreateEulerRainFile(30,5,rp,cf,rfile.str());

        SWMMWriteAndRead * swmm;
        swmm = new SWMMWriteAndRead(city,rfile.str(), this->FileName);
        swmm->setupSWMM();
        swmmruns.push_back(swmm);
    }
    DM::Logger(DM::Debug) << "Done with preparing swmm";
#ifdef _OPENMP
    omp_set_num_threads(8);
    DM::Logger(DM::Standard) << "starting omp with " << omp_get_max_threads() << " threads";
#endif
#pragma omp parallel for
    for (int nof = 0; nof < numberOfRPs; nof++ ) {
        swmmruns[nof]->runSWMM();

    }
    std::vector<std::string>v_cities = city->getUUIDs(vcity);
    DM::Component * c = city->getComponent(v_cities[0]);
    if (!c) {
        DM::Logger(DM::Warning) << "City not found ";
        return;
    }

    int current_year = c->getAttribute("year")->getDouble();

    DM::Logger(DM::Debug) << "Start write output files";
    for (int nof = 0; nof < numberOfRPs; nof++ ) {
        DM::Logger(DM::Debug) << "Start read in  " << nof;
        int index_rp = nof / numberOfcfs;
        int index_cf = nof % numberOfcfs;


        double rp = return_periods[index_rp];
        double cf = cfs[index_cf];
        SWMMWriteAndRead * swmm = swmmruns[nof];
        swmm->readInReportFile();


        std::map<std::string, std::string> additionalParameter;

        additionalParameter["PopulationGrowth"] = QString::number(city->getAttribute("pop_growth")->getDouble()).toStdString();
        additionalParameter["cf_tot\t"] = QString::number(cf).toStdString();
        additionalParameter["climatechangefactor\t"] = QString::number(climateChangeFactors[index_cf]).toStdString();
        additionalParameter["return_period"] = QString::number(rp).toStdString();

        if (!cfRand) {
            std::stringstream fname;
            fname << this->outputFiles << "/"  <<  current_year << "_" << rp <<  "_"  << climateChangeFactors[index_cf] << "_" << this->unique_name  << ".cvs";
            DrainageHelper::WriteOutputFiles(fname.str(), city, *swmm, additionalParameter);
        }
        else {
            std::stringstream fname;
            fname << this->outputFiles << "/"  <<  current_year << "_" << rp <<  "_"  << index_cf << "_" << this->unique_name << ".cvs";
            DrainageHelper::WriteOutputFiles(fname.str(), city,  *swmm, additionalParameter);
        }

    }
    DM::Logger(DM::Debug) << "Done with swmm";
    for (int nof = 0; nof < numberOfRPs; nof++ ) {
        delete swmmruns[nof];

    }
    swmmruns.clear();

}
