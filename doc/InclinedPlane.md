[List of Sewer Modules](Sewer.md)

#InclinedPlane

This model creates simple an inclined plane. The slope is calculated as 
x * CellSize * Slope


##Parameter
|        Name       |          Type          |       Description         | 
|-------------------|------------------------|---------------------------|
| Height    | LONG | height of the raster data set in cells |
| Width    | LONG | width of the raster data set in cells |
| CellSize    | DOUBLE | cell size |
| Slope    | DOUBLE | dimensionless slope |
| appendToStream    | BOOL | append data new raster data set to existing stream |

##Datastream
|     Identifier    |     Attribute    |      Type             |Access |    Description    |
|-------------------|------------------|-----------------------|-------|-------------------|
| Topology |                  | RASTER DATA   | read  |  |

