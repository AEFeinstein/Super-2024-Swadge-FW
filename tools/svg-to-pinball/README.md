If objects aren't where they're supposed to be, svgelements may not be applying transforms correctly. Use this Inkscape plugin to apply transforms to objects before saving the SVG: https://github.com/Klowner/inkscape-applytransforms

## File Format

1. Number of Groups
1. Number of lines
    * Line objects
1. Number of circles
    * circles objects
1. Number of rectangles
    * rectangles objects
1. Number of flippers
    * flippers objects

### Line
**Byte**|**Size**|**Value**
:-----:|:-----:|:-----:
0|2|ID
2|1|Group ID
3|2|p1.x
5|2|p1.y
7|2|p2.x
9|2|p2.y
11|1|Type
12|1|Push Velocity
13|1|Is Solid

### Circle
**Byte**|**Size**|**Value**
:-----:|:-----:|:-----:
0|2|ID
2|1|Group ID
3|2|x
5|2|y
7|1|radius
8|1|Push Velocity

### Rectangle
**Byte**|**Size**|**Value**
:-----:|:-----:|:-----:
0|2|ID
2|1|Group ID
3|2|x
5|2|y
7|2|width
9|2|height

### Flipper
**Byte**|**Size**|**Value**
:-----:|:-----:|:-----:
0|2|x
2|2|y
4|1|Radius
5|1|Length
6|1|Facing Right