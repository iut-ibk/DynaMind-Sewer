#include "inclinedplane.h"

DM_DECLARE_NODE_NAME(InclinedPlane,Sewer)

InclinedPlane::InclinedPlane()
{
    height = 100;
    width = 100;
    cellsize = 10;
    slope = -3./1000;
    appendToStream = false;

    v_plane = DM::View("Topology", DM::RASTERDATA, DM::WRITE);

    this->addParameter("Height", DM::LONG, &height);
    this->addParameter("Width", DM::LONG, &width);
    this->addParameter("CellSize", DM::DOUBLE, &cellsize);
    this->addParameter("Slope", DM::DOUBLE, &slope);
    this->addParameter("appendToStream", DM::BOOL, &appendToStream);


    std::vector<DM::View> datastream;
    datastream.push_back(v_plane);
    this->addData("city", datastream);
}

void InclinedPlane::init() {


    if (!appendToStream)
        return;
    std::vector<DM::View> datastream;
    datastream.push_back(v_plane);
    datastream.push_back(DM::View("dummy", DM::SUBSYSTEM, DM::READ));
    this->addData("city", datastream);
}

string InclinedPlane::getHelpUrl()
{
    return "https://github.com/iut-ibk/DynaMind-Sewer/blob/master/doc/InclinedPlane.md";
}

void InclinedPlane::run()
{
    DM::RasterData * plane = this->getRasterData("city", v_plane);
    plane->setSize(width, height, cellsize, cellsize, 0, 0);

    for (long x = 0; x < width; x++) {
        for (long y = 0; y <height; y++) {
            plane->setCell(x,y,x*cellsize*slope);
        }
    }
    DM::Logger(DM::Standard) << plane->getCell(0,0);
    DM::Logger(DM::Standard) << plane->getCell(0,height-1);
    DM::Logger(DM::Standard) << plane->getCell(width-1,0);
    DM::Logger(DM::Standard) << plane->getCell(width-1, height-1);
}
