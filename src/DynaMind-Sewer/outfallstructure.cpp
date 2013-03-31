/**
 * @file
 * @author  Chrisitan Urich <christian.urich@gmail.com>
 * @version 1.0
 * @section LICENSE
 *
 * This file is part of DynaMind
 *
 * Copyright (C) 2013 Christian Urich

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

#include "outfallstructure.h"

DM_DECLARE_NODE_NAME(OutfallStructure, Sewer)

OutfallStructure::OutfallStructure()
{
    view_outlets = DM::View("OUTLET", DM::NODE, DM::READ);
    view_outfalls = DM::View("OUTFALL", DM::NODE, DM::WRITE);
    view_conduits = DM::View("CONDUIT", DM::EDGE, DM::WRITE);

    std::vector<DM::View> datastream;

    datastream.push_back(view_outlets);
    datastream.push_back(view_outfalls);
    datastream.push_back(view_conduits);

    this->addData("city", datastream);

}

void OutfallStructure::run()
{
    DM::System * city = this->getData("city");

    std::vector<std::string> outlet_uuids = city->getUUIDs(view_outlets);
    foreach (std::string o_uuid, outlet_uuids) {
        DM::Node * n = city->getNode(o_uuid);


        DM::Node offset(15,15,0);

        DM::Node * outfall = city->addNode(*n + offset, view_outfalls);
        outfall->addAttribute("Z",n->getAttribute("Z")->getDouble());
        DM::Edge * e = city->addEdge(n, outfall, view_conduits);
        e->addAttribute("Diameter", 4000);
        e->addAttribute("Length", 15.);

    }
}
