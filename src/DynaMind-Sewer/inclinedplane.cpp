#include "inclinedplane.h"

DM_DECLARE_NODE_NAME(InclinedPlane,Sewer)

InclinedPlane::InclinedPlane()
{
    height = 100;
    width = 100;
    cellsize = 10;
    slope = -3./1000;

    v_plane = DM::View("Topology", DM::RASTERDATA, DM::WRITE);

    this->addParameter("Height", DM::LONG, &height);
    this->addParameter("Width", DM::LONG, &width);
    this->addParameter("CellSize", DM::DOUBLE, &cellsize);
    this->addParameter("Slope", DM::DOUBLE, &slope);

    std::vector<DM::View> datastream;
    datastream.push_back(v_plane);
    this->addData("city", datastream);
}

void InclinedPlane::run()
{
   DM::RasterData * plane = this->getRasterData("city", v_plane);
   plane->setSize(width, height, cellsize, cellsize, 0, 0);

   for (long x = 0; x < height; x++) {
       for (long y = 0; y < width; y++) {
           plane->setCell(x,y,x*cellsize*slope);
       }
   }
}
