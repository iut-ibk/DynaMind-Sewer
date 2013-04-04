#include "swmmreturnperiod.h"
#include "swmmwriteandread.h"

#include <fstream>
#include <QDateTime>

DM_DECLARE_NODE_NAME(SWMMReturnPeriod, Sewer)
SWMMReturnPeriod::SWMMReturnPeriod()
{

    GLOBAL_Counter = 1;

    conduit = DM::View("CONDUIT", DM::EDGE, DM::READ);
    conduit.getAttribute("Diameter");

    inlet = DM::View("INLET", DM::NODE, DM::READ);
    inlet.getAttribute("CATCHMENT");


    junctions = DM::View("JUNCTION", DM::NODE, DM::READ);
    junctions.getAttribute("D");
    junctions.addAttribute("flooding_V");
    junctions.addAttribute("FLOODING_AREA");

    endnodes = DM::View("OUTFALL", DM::NODE, DM::READ);

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

    years = 0;

    this->isCombined = false;
    this->addParameter("Folder", DM::STRING, &this->FileName);
    this->addParameter("RainFile", DM::FILENAME, &this->RainFile);
    this->addParameter("ClimateChangeFactor", DM::DOUBLE, & this->climateChangeFactor);
    this->addParameter("combined system", DM::BOOL, &this->isCombined);
    this->addParameter("outputFiles", DM::FILENAME, &this->outputFiles);

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

void SWMMReturnPeriod::createEulerRainFile(double duration, double deltaT, double return_period) {

    QDateTime date(QDate(2000,1,1), QTime(0,0));

    QString fileName = "/tmp/rain.dat";
    std::fstream out;
    out.open(fileName.toAscii(),ios::out);

    std::vector<double> rainseries_tmp;
    int steps = duration/deltaT;
    double D = 0;
    DM::Logger(DM::Debug) << "Rain D U W r";
    for (int s = 0; s < steps; s++) {
        D=D+5;
        double Uapprox = exp(1.4462+0.3396*log(D));
        double Wapprox = -0.4689+2.9227*log(D)-0.0815*pow(log(D),2);



        double rain = (Uapprox+Wapprox*log(return_period))*this->climateChangeFactor;

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

void SWMMReturnPeriod::writeOutputFiles(DM::System * sys, double rp, std::vector<std::pair<string, double> > &flooding_vec, std::string swmmuuid)
{

    std::vector<std::string> v_cities = sys->getUUIDs(vcity);
    if (!v_cities.size()) return;

    DM::Component * city = sys->getComponent(v_cities[0]);

    int current_year = city->getAttribute("year")->getDouble();

    std::stringstream fname;
    fname << this->outputFiles  <<  current_year << "_" << rp <<  "_"  << ".cvs";

    std::fstream inp;
    inp.open(fname.str(),ios::out);
    inp << swmmuuid << "\n";
    inp << flooding_vec.size() << "\n";
    inp << fixed;

    typedef std::pair<std::string, double > rainnode;

    foreach (rainnode  fn, flooding_vec) {
        inp << fn.first;
        inp << "\t";
        inp << fn.second;
        inp << "\n";
    }

    inp.close();

}


void SWMMReturnPeriod::run() {

    std::vector<double> return_periods;

    return_periods.push_back(0.25);
    return_periods.push_back(0.5);
    return_periods.push_back(1);
    return_periods.push_back(2);
    return_periods.push_back(5);
    return_periods.push_back(10);
    return_periods.push_back(20);
    return_periods.push_back(30);
    return_periods.push_back(50);
    return_periods.push_back(100);
    return_periods.push_back(200);


    curves.str("");
    city = this->getData("City");

    foreach (double rp, return_periods) {
        double cf = 1. + years / 20. * (this->climateChangeFactor - 1.);
        this->climateChangeFactor = cf;

        DM::Logger(DM::Standard) << "return_period " <<  rp;
        this->createEulerRainFile(120,5,rp);

        SWMMWriteAndRead swmm(city, "/tmp/rain.dat", this->FileName);


        swmm.run();

        std::vector<std::string> c_uuids = city->getUUIDs(this->flooding_area);

        foreach (std::string c_uuid, c_uuids)
            city->getComponent(c_uuid)->addAttribute("return_period", 0.0);

        std::vector<std::pair <std::string, double > > flooded_nodes = swmm.getFloodedNodes();

        typedef std::pair<std::string, double > rainnode;
        foreach (rainnode fn, flooded_nodes ) {
            DM::Component * f_node = city->getComponent(fn.first);
            std::vector<DM::LinkAttribute> links = f_node->getAttribute("FLOODING_AREA")->getLinks();
            if (links.size() == 0)
                DM::Logger(DM::Standard) << "Not connected";
            foreach (DM::LinkAttribute l, links) {
                std::string c_uuid = l.uuid;

                DM::Component * flooded = city->getComponent(c_uuid);

                if (!flooded)
                    continue;
                if (flooded->getAttribute("return_period")->getDouble() < 0.00001) {
                    flooded->addAttribute("return_period",rp);
                }

            }


        }

        DM::Logger(DM::Standard) << "Effected Nodes " <<  flooded_nodes.size();
        writeOutputFiles(city, rp, flooded_nodes, swmm.getSWMMUUID());
    }

    this->years++;

}
