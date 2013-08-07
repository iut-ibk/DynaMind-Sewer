[List of Sewer Modules](Sewer.md)

#LinkElementWithNearestPoint

Links nodes to their nearst neighbour


##Parameter
|        Name       |          Type          |       Description         | 
|-------------------|------------------------|---------------------------|
| Points_to_Link    | string | nodes that are linked |
| Point_Field     | string | field in which the nearest nodes are looked for|
| treshhold | double | nodes can only be linked to nodes within the threshold |
| onSignal | bool | if ture only nodes are considered where _new_ > 0|

##Datastream
|     Identifier    |     Attribute    |      Type             |Access |    Description    |
|-------------------|------------------|-----------------------|-------|-------------------|
| [Points_to_Link] |                  | NODE   | read  |  |
|                   | [Point_Field]   | LINK | write | links to nearest [Point_Field] |
|                   | new  | int | read | optional parameter considered when onSignal is true |
| [Point_Field] |                  | NODE   | read  |  |
|                   | [Points_to_Link]   | LINK | write | links to nearest [Points_to_Link] |

