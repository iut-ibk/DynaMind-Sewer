[List of Sewer Modules](Sewer.md)

#OutfallStructure

Creates simple outfall structure for combined drainage networks. In SWMM [SWMM 5.0](http://www.epa.gov/nrmrl/wswrd/wq/models/swmm/) outfalls can't be connected to more than one conduit. For some of the network generation algorithms an outfall can have more than one connected conduit. This module provides a simple solution for this problem by creating a simple outfall structure based on the position of the outlet. 


##Datastream
|     Identifier    |     Attribute    |      Type             |Access |    Description    |
|-------------------|------------------|-----------------------|-------|-------------------|
| OUTLET |                  | NODE | read  | |
| OUTFALL |                  | NODE | write  | the outfall is place 15m in x and 15m in y directions from the OUTLET|
| CONDUIT |                  | EDGE | write  | |
|                   | Diameter  | double | write | 4 m  |
|                   | Length  | double | write |  |