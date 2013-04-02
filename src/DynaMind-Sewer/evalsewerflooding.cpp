#include "evalsewerflooding.h"
#include <sstream>
#include <fstream>

DM_DECLARE_NODE_NAME(EvalSewerFlooding, Sewer)

EvalSewerFlooding::EvalSewerFlooding()
{

    this->FileName = "";
    this->addParameter("FileName", DM::FILENAME, &this->FileName);
    counter = 0;

    city = DM::View("CITY", DM::COMPONENT, DM::READ);
    city.getAttribute("year");

    flooding_area = DM::View("FLOODING_AREA",DM::COMPONENT,DM::MODIFY);
    flooding_area.getAttribute("return_period");
    flooding_area.getAttribute("id");

    std::vector<DM::View> datastream;
    datastream.push_back(city);
    datastream.push_back(flooding_area);

    this->addData("sys", datastream);

}

void EvalSewerFlooding::run() {

    DM::System * sys = this->getData("sys");

    std::vector<std::string> v_cities = sys->getUUIDs(city);
    if (!v_cities.size()) return;

    int current_year = sys->getComponent(v_cities[0])->getAttribute("year")->getDouble();

    std::stringstream fname;
    fname << this->FileName << "_" << current_year;

    std::fstream inp;
    inp.open(fname.str(),ios::out);

    std::vector<std::string> flooding_uuids = sys->getUUIDs(flooding_area);

    foreach (std::string uuid, flooding_uuids) {
        DM::Component * cmp = sys->getComponent(uuid);
        inp << cmp->getUUID();
        inp << "\t";
        inp << cmp->getAttribute("id")->getDouble();
        inp << "\t";
        inp << cmp->getAttribute("return_period")->getDouble();
        inp << "\n";
    }

    inp.close();

    counter++;
}
