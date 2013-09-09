[List of Sewer Modules](Sewer.md)

#InfiltrationTrench

The module auto designs and connects the infiltration system with the catchment. The infiltration systems are designed following the german guideline DWA-A 13. The used equations can be found [here (in german)](http://www.abwdat.de/abw/versickerung/pdf/Regenwasserversickerung_Planung-Dimensionierung.pdf)

##Parameter
|        Name       |          Type          |       Description         | 
|-------------------|------------------------|---------------------------|
| R     | double | design rain in l/s.ha |
| D     | double | duration in min |

##Datastream
|     Identifier    |     Attribute    |      Type             |Access |    Description    |
|-------------------|------------------|-----------------------|-------|-------------------|
| CATCHMENT |                  | COMPONENT   | read  |  |
|                   | roof_area_treated  | double | read | are that should be treated by the infiltration system in m2|
|                   | Impervious | double | read | impervious fraction |
|                   | area | double | read | area in m2 |
|                    |INFILTRATION_SYSTEM | LINK | modify| module reads the existing infiltration systems and adds new ones | 
| INFILTRATION_SYSTEM |                  | COMPONENT   | write  |  |
|                   | treated_area  | double | write | treated area by the infiltration system in m2|
|                   | area  | double | write | area of the infiltration system in m2|
|                   | h  | double | write | height|
|                   | kf  | double | write | conductivity in m/s |

