[List of Sewer Modules](Sewer.md)

#DMSWMM

The model exports a combined sewer or drainage network into a SWMM 5.0 input file, assesses the performance by running SWMM 5.0 and inports the results back into DynaMind.

##Parameter
|        Name       |          Type          |       Description         | 
|-------------------|------------------------|---------------------------|
| Folder   | string | defines the working directory. The module creates for every SMM simulation a new unique folder in the working directory.       |
| RainFile   | filename | select rain file you want to use in the simulation. This parameter is optional.|
| ClimateChangeFactor   | double | the climate change factor is multiplied with the rainfall intensities of the rain|
| use euler   | bool | instead of rain file a synthetic euler rain is used|
| return period   | double | defines the return period of the euler rain|
| combined sewer system  | bool | true for combined sewer systems, false for drainage networks|
| use_linear_cf | bool | true if the module is used in group the climate change factor is increased linear |
| writeResultFile| bool | writes a result files in the working directory |
| climateChangeFactorFromCity| bool | use the climate change factor defined in the CITY component |
|  calculationTimestep| int | if the module is used in a loop, the parameter defines how often a swmm simulation is executed. e.g. a calculationTimestep of 5 means that every 5 steps swmm is executed |
|  consider_build_time| bool | if true only pipes the exist in the current simulation year defined by the attribute year of the CITY component are used in the assessment |
|  deleteSWMM| bool | delete SWMM folder after simulation SWMM run|

##Datastream
|     Identifier    |     Attribute    |      Type             |Access |    Description    |
|-------------------|------------------|-----------------------|-------|-------------------|
| CONDUIT |                  | EDGE   | read  |  |
|                   | Diameter  | double | read | pipe diameter in m|
|                   | capacity  | double | read | utilised peak capacity in %|
|                   | velocity  | double | read | peak velocity in m/s|
|                   | XSECTION  | LINK | read | link to custom cross section|
| INLET |                  | NODE   | read  |  |
|                   | CATCHMENT  | LINK | read | link to catchment |
| JUNCTION |                  | NODE   | read  |  |
|                   | Z  | double |terrain height in m | |
|                   | invert_elevation  | double | invert elevation in m | |
|                   | D  | double | maximum depth of junction (e.g. from ground surface to invert) | |
|                   | flooding_V  | double | write | flooded volume in m3 |
|                   | node_depth  | double | write | depth from invert to peak water level in m|
| OUTFALL |                  | NODE   | read  |  |
| CATCHMENT |                  | FACE   | read  |  |
|                   | area | double | read |area in m2  |
|                   | Gradient | double | read |slope gradient dimensionless   |
|                   | Impervious | double | read |imperviousness dimensionless   |
|                   | WasteWater | double | read | amount of waste water in m3/s|
|                   | INFILTRATION_SYSTEM | LINK | read | link to infiltration systems in catchment|

##Optional Data 
|     Identifier    |     Attribute    |      Type             |Access |    Description    |
|-------------------|------------------|-----------------------|-------|-------------------|
| PUMPS |                  | EDGE   | read  | default pump type PUMP2 is used |
|                   | area | double | read |area in m2  |
|                   | pump_x | double vector | read | pump curve (see SWMM manual) |
|                   | pump_y | double vector | read | |
| INFILTRATION_SYSTEM |                  | COMPONENT   | read  |  |
|                   | h  | double |read |depth in m  |
|                   | kf | double |read |conductivity in m/s  |
|                   | area | double |read |area of the infiltration system in m2 |
|                   | treated_area | double |read | impervious area treated in m2  |
| XSECTION |                  | NODE   | read  |  |
|                   | type | string | read |section name, only "CUSTOM" is supported|
|                   | shape_x | double | read |shape description (see SWMM manual) |
|                   | shape_y | double | read |shape description (see SWMM manual)|

##Additional Data Combined Drainage Network
|     Identifier    |     Attribute    |      Type             |Access |    Description    |
|-------------------|------------------|-----------------------|-------|-------------------|
| WWTP |                  | NODE   | read  |  |
| WEIR |                  | EDGE   | read  |  |
|                   | crest_height | double | height in m  |
|                   | XSECTION  | LINK | read | link to custom cross section|
|                   | discharge_coefficient | double | discharge coefficient  |
|                   | end_coefficient | double | end coefficient |
| STORAGE |                  | NODE   | read  |  |
|                   | Z | double | read |invert elevation in m |
|                   | max_depth | double | read |max depth in m|
|                   | type| string | read |"TABULAR" or "FUNCTIONAL"|
|                   | storage_x| double vector | read |storage curves for type tabular|
|                   | storage_y| double vector | read |storage curves for type tabular|
|                   | StorageA| double | read |storage area for type functional|