/**
 * @file
 * @author  Chrisitan Urich <christian.urich@gmail.com>
 * @version 1.0
 * @section LICENSE
 *
 * This file is part of DynaMind
 *
 * Copyright (C) 2011  Christian Urich

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

#include "createinlets.h"
DM_DECLARE_NODE_NAME(CreateInlets, Sewer)

CreateInlets::CreateInlets()
{


    Blocks = DM::View("CITYBLOCK", DM::FACE, DM::READ);
    Inlets = DM::View("INLET", DM::NODE, DM::WRITE);

    Inlets.addAttribute("ID_CATCHMENT");

    std::vector<DM::View> data;
    data.push_back(Blocks);
    data.push_back(Inlets);
    this->addParameter("Size", DM::DOUBLE, &Size);

    offsetX = 0;
    offsetY = 0;
    width = 1000;
    heigth = 1000;

    this->addParameter("offsetX", DM::DOUBLE, &offsetX);
    this->addParameter("offsetY", DM::DOUBLE, &offsetY);
    this->addParameter("width", DM::DOUBLE, &width);
    this->addParameter("heigth", DM::DOUBLE, &heigth);


    this->addData("City", data);

}

void CreateInlets::run() {

    DM::System * city = this->getData("City");

    std::vector<std::string> blocks = city->getUUIDsOfComponentsInView(Blocks);
    foreach (std::string nBlock, blocks) {
        DM::Face * f = city->getFace(nBlock);

        double minx;
        double maxx;
        double miny;
        double maxy;

        std::vector<std::string> points = f->getNodes();
        for (int i = 0; i < points.size(); i++) {
            DM::Node * n = city->getNode(points[i]);
            if (i == 0) {
                minx = n->getX();
                maxx = n->getX();
                miny = n->getY();
                maxy = n->getY();
                continue;
            }
            if (minx > n->getX())
                minx = n->getX();
            if (miny > n->getY())
                miny = n->getY();
            if (maxx < n->getX())
                maxx = n->getX();
            if (maxy < n->getY())
                maxy = n->getY();

        }

        double l = maxx-minx;
        double h = maxy-miny;

        double startX = minx + Size/2;
        double startY = miny + Size/2;


        double buildyear =f->getAttribute("BuildYear")->getDouble();
        if (buildyear < 1900)
            continue;
        while (startY < maxy) {
            while (startX < maxx) {
                if (startX < offsetX+with && startX > offsetX && startY < offsetY+heigth && startY > offsetY)  {
                    DM::Node * n = city->addNode(DM::Node(startX, startY, 0), Inlets);
                    n->addAttribute("BuildYear", buildyear);
                    n->getAttribute("ID_CATCHMENT")->setString(f->getUUID());

                }
                startX+=Size;
            }
            startY+=Size;
            startX = minx + Size/2;
        }




    }


}
