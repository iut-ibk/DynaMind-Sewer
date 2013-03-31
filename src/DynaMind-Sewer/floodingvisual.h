/**
 * @file
 * @author  Chrisitan Urich <christian.urich@gmail.com>
 * @version 1.0
 * @section LICENSE
 *
 * This file is part of DynaMind
 *
 * Copyright (C) 2013  Christian Urich

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
#ifndef FLOODINGVISUAL_H
#define FLOODINGVISUAL_H

#include <dm.h>

class DM_HELPER_DLL_EXPORT FloodingVisual : public DM::Module
{
    DM_DECLARE_NODE(FloodingVisual)
    private:
        DM::View view_junctions;
        DM::View view_junction_flooding;
        double radius;
        double scaling;

public:
    FloodingVisual();
    void run();
};

#endif // FLOODINGVISUAL_H
