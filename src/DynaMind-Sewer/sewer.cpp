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

#include "dmcompilersettings.h"
#include "dmnodefactory.h"
#include "dmmoduleregistry.h"
#include "timeareamethod.h"
#include "dmswmm.h"
#include "networkanalysis.h"
#include "directnetwork.h"
#include "generatesewernetwork.h"
#include "extractnetwork.h"
#include "outfallplacement.h"
#include "reconstructparameter.h"
#include "removestrahler.h"
#include "createinlets.h"
#include "inclinedplane.h"
#include "outfallstructure.h"
#include "reducejunctions.h"
#include "infiltrationtrench.h"
#include "linkelementwithnearestpoint.h"
#include "floodingvisual.h"
#include "pipeage.h"
#include "swmmreturnperiod.h"
#include "evalsewerflooding.h"

/*#include "pickstartpoints.h"

#include "infiltrationtrench.h"
#include "selectinfiltration.h"*/

#include <iostream>

using namespace std;

extern "C" void DM_HELPER_DLL_EXPORT  registerModules(DM::ModuleRegistry *registry) {
	registry->addNodeFactory(new DM::NodeFactory<TimeAreaMethod>());
	registry->addNodeFactory(new DM::NodeFactory<DMSWMM>());
	registry->addNodeFactory(new DM::NodeFactory<NetworkAnalysis>());
	registry->addNodeFactory(new DM::NodeFactory<DirectNetwork>());
	registry->addNodeFactory(new NodeFactory<GenerateSewerNetwork>());
	registry->addNodeFactory(new NodeFactory<ExtractNetwork>());
	registry->addNodeFactory(new NodeFactory<OutfallPlacement>());
	registry->addNodeFactory(new NodeFactory<ReconstructParameter>());
	registry->addNodeFactory(new NodeFactory<RemoveStrahler>());
	registry->addNodeFactory(new NodeFactory<CreateInlets>());
	registry->addNodeFactory(new NodeFactory<InclinedPlane>());
	registry->addNodeFactory(new NodeFactory<OutfallStructure>());
	registry->addNodeFactory(new NodeFactory<ReduceJunctions>());
	registry->addNodeFactory(new NodeFactory<InfiltrationTrench>());
	registry->addNodeFactory(new NodeFactory<LinkElementWithNearestPoint>());
	registry->addNodeFactory(new NodeFactory<FloodingVisual>());
	registry->addNodeFactory(new NodeFactory<PipeAge>());
	registry->addNodeFactory(new NodeFactory<SWMMReturnPeriod>());
	registry->addNodeFactory(new NodeFactory<EvalSewerFlooding>());
	/*registry->addNodeFactory(new NodeFactory<PickStartPoints>());


	registry->addNodeFactory(new NodeFactory<SelectInfiltration>());*/

}
