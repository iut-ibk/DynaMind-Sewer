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


#include "removestrahler.h"

DM_DECLARE_NODE_NAME(RemoveStrahler, Sewer)
RemoveStrahler::RemoveStrahler()
{
    this->junctions = DM::View("JUNCTION", DM::NODE, DM::MODIFY);
    this->conduits = DM::View("CONDUIT", DM::EDGE, DM::MODIFY);
    this->conduits.getAttribute("Strahler");
    until = 1;
    this->addParameter("StrahlerNumber", DM::INT,&until);

    std::vector<DM::View> data;
    data.push_back(this->junctions);
    data.push_back(this->conduits);


    this->addData("SEWER", data);


}

void RemoveStrahler::run()
{
    DM::System * sys = this->getData("SEWER");

    std::vector<std::string> condis = sys->getUUIDsOfComponentsInView(conduits);

    foreach (std::string s, condis) {
        DM::Edge * c = sys->getEdge(s);
        if (c->getAttribute("Strahler")->getDouble() < until) {
            sys->removeComponentFromView(c, conduits);
            DM::Node * nstart = sys->getNode(c->getStartpointName());
            sys->removeComponentFromView(nstart, junctions);

        }
    }

}
