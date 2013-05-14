#include "swmmreturnperiod.h"


#include <fstream>
#include <QDateTime>

DM_DECLARE_NODE_NAME(SWMMReturnPeriod, Sewer)
SWMMReturnPeriod::SWMMReturnPeriod()
{

    GLOBAL_Counter = 1;

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
    years = 0;

    this->isCombined = false;
    this->addParameter("Folder", DM::STRING, &this->FileName);
    this->addParameter("RainFile", DM::FILENAME, &this->RainFile);
    this->addParameter("ClimateChangeFactor", DM::DOUBLE, & this->climateChangeFactor);
    this->addParameter("combined system", DM::BOOL, &this->isCombined);
    this->addParameter("outputFiles", DM::FILENAME, &this->outputFiles);
    this->addParameter("calculationTimestep", DM::INT, & this->calculationTimestep);

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

void SWMMReturnPeriod::CreateEulerRainFile(double duration, double deltaT, double return_period, double cf, std::string rfile) {

    QDateTime date(QDate(2000,1,1), QTime(0,0));

    QString fileName = QString::fromStdString(rfile);
    std::fstream out;
    out.open(fileName.toAscii(),ios::out);

    std::vector<double> rainseries_tmp;
    int steps = duration/deltaT;
    double D = 0;
    DM::Logger(DM::Debug) << "Rain D U W r";
    for (int s = 0; s < steps; s++) {
        D=D+deltaT;
        double Uapprox = exp(1.4462+0.3396*log(D));
        double Wapprox = -0.4689+2.9227*log(D)-0.0815*pow(log(D),2);



        double rain = (Uapprox+Wapprox*log(return_period))*cf;

        DM::Logger(DM::Debug) << D <<"/"<< Uapprox <<"/"<< Wapprox << "/" << rain;

        rainseries_tmp.push_back(rain);

    }

    std::vector<double> rainseries;
    rainseries.push_back(rainseries_tmp[0]);
    for (uint i = 1; i < rainseries_tmp.size(); i++)
        rainseries.push_back(rainseries_tmp[i] - rainseries_tmp[i-1]);


    //flip first 3rd of the rain
    std::vector<double> rainseries_flipped(rainseries);
    std::vector<double> flipped_values;
    for (int s = 0; s < steps/3; s++) {
        flipped_values.push_back(rainseries[s]);
    }
    std::reverse(flipped_values.begin(), flipped_values.end());

    for (int s = 0; s < flipped_values.size(); s++) rainseries_flipped[s] = flipped_values[s];

    for (int s = 0; s < steps; s++) {
        date = date.addSecs(deltaT*60);
        out << "STA01 " <<date.toString("yyyy M d HH mm").toStdString() << " " << rainseries_flipped[s];
        out << "\n";
    }

    out.close();
}

void SWMMReturnPeriod::writeOutputFiles(DM::System * sys, double rp, SWMMWriteAndRead &swmmreeadfile, std::string swmmuuid, double cf, double id)
{

    std::vector< std::pair<std::string, double > > swmm = swmmreeadfile.getFloodedNodes();

    std::vector<std::string> v_cities = sys->getUUIDs(vcity);
    if (!v_cities.size()) return;

    DM::Component * city = sys->getComponent(v_cities[0]);
    if (!city) {
        DM::Logger(DM::Warning) << "City nor found ";
        return;
    }
    int current_year = city->getAttribute("year")->getDouble();

    std::stringstream fname;
    fname << this->outputFiles  <<  current_year << "_" << rp <<  "_"  << id << ".cvs";

    std::fstream inp;
    double effective_runoff = swmmreeadfile.getVSurfaceRunoff() + swmmreeadfile.getVSurfaceStorage();

    inp.open(fname.str().c_str(),ios::out);
    inp << "ImperviousTotal\t" << swmmreeadfile.getTotalImpervious()<< "\n";
    inp << "ImperviousInfitration\t" << swmmreeadfile.getImperiousInfiltration() / 10000.<< "\n";
    inp << "Vr\t" << effective_runoff << "\n";
    inp << "Vp\t" << swmmreeadfile.getVp()<< "\n";
    inp << "Vwwtp\t" << swmmreeadfile.getVwwtp()<< "\n";
    inp << "Vout\t" << swmmreeadfile.getVout()<< "\n";
    inp << "error\t" << swmmreeadfile.getContinuityError()<< "\n";
    inp << "climatechangefactor\t" <<cf<< "\n";
    inp << "swmmuuid\t"<< swmmuuid << "\n";
    inp << "floodednodes\t" << swmm.size() << "\n";
    inp << "Vstorage" << swmmreeadfile.getVSurfaceStorage()<< "\n";
    inp << "END\t" << swmm.size() << "\n";
    inp << fixed;

    typedef std::pair<std::string, double > rainnode;
    inp << "FLOODSECTION\t" << "\n";
    foreach (rainnode  fn, swmm) {
        inp << fn.first;
        inp << "\t";
        inp << fn.second;
        inp << "\n";
    }
    inp << "END\t" << "\n";

    std::vector< std::pair<std::string, double > > surcharge = swmmreeadfile.getNodeDepthSummery();

    inp << "SURCHARGE" << "\n";

    foreach (rainnode  fn, surcharge) {
        DM::Component * n =  sys->getComponent(fn.first);
        if (!n) continue;
        inp << fn.first;
        inp << "\t";
        inp << n->getAttribute("D")->getDouble();
        inp << "\t";
        inp <<fn.second;
        inp << "\n";
    }

    inp << "END" << "\n";

    inp.close();

}


void SWMMReturnPeriod::run() {

    std::vector<double> return_periods;

    //return_periods.push_back(0.25);
    return_periods.push_back(0.5);
    return_periods.push_back(1);
    return_periods.push_back(2);
    return_periods.push_back(5);
    return_periods.push_back(10);
    /*return_periods.push_back(20);
    return_periods.push_back(30);
    return_periods.push_back(50);
    return_periods.push_back(100);
    return_periods.push_back(200);*/

    std::vector<double> climateChangeFactors;
    climateChangeFactors.push_back(1.0);
    climateChangeFactors.push_back(1.1);
    climateChangeFactors.push_back(1.3);
    climateChangeFactors.push_back(1.5);


    curves.str("");
    city = this->getData("City");

    std::vector<double> cfs;

    foreach (double cf, climateChangeFactors) {
        cfs.push_back(1. + years / 20. * (cf - 1.));
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
        rfile << "/tmp/rain_";
        rfile << nof;

        DM::Logger(DM::Standard) << "return_period " <<  rp;
        DM::Logger(DM::Standard) << "cf_period " <<  cf;
        this->CreateEulerRainFile(30,5,rp,cf,rfile.str());

        SWMMWriteAndRead * swmm;
        swmm = new SWMMWriteAndRead(city,rfile.str(), this->FileName);
        swmm->setupSWMM();
        swmmruns.push_back(swmm);
    }
    DM::Logger(DM::Debug) << "Done with preparing swmm";
#pragma omp parallel for
    for (int nof = 0; nof < numberOfRPs; nof++ ) {
        swmmruns[nof]->runSWMM();

    }
    DM::Logger(DM::Debug) << "Start write output files";
    for (int nof = 0; nof < numberOfRPs; nof++ ) {
        DM::Logger(DM::Debug) << "Start read in  " << nof;
        int index_rp = nof / numberOfcfs;
        int index_cf = nof % numberOfcfs;


        double rp = return_periods[index_rp];
        double cf = cfs[index_cf];
        SWMMWriteAndRead * swmm = swmmruns[nof];
        swmm->readInReportFile();

        writeOutputFiles(city, rp, *swmm, swmm->getSWMMUUID(), cf,climateChangeFactors[index_cf]);

    }
    DM::Logger(DM::Debug) << "Done with swmm";
    for (int nof = 0; nof < numberOfRPs; nof++ ) {
        delete swmmruns[nof];

    }

    swmmruns.clear();

}
