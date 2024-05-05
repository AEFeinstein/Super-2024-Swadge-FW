# Sokoban Game Mode

Sokoban, unfinished for 2024, attempting to get finished for 2025!

## Gameplay

### Creating a Level
Levels are created with the software [Tiled](https://www.mapeditor.org/). 

Add the provided tilemap. You cannot add your own tiles. You can, but the system will ignore them. Use the provided tilemap. The image data from this map is unimportant, what is important is the 'ID' of all of the various objects and layers, and special custom properties for any of the items. ID's and custom properties are what gets converted into the .bin level data.

There are currently 3 rulesets: CLASSIC, EULER, and LASER.

- **CLASSIC** is traditional [sokoban](https://en.wikipedia.org/wiki/Sokoban). Can only push 1 block, must cover all goal areas with blocks.
- **EULER** is a port of Hunter's most succesful push-block-thinky game, which combines sokabon with [eulerian paths](https://en.wikipedia.org/wiki/Eulerian_path). You can only visit each square once.
- **LASER** is WIP.

In order to indicate which ruleset your level uses...  (currently the array of files has a matching sokoLevelVariants array.)
In order to indicate which theme (sprite set) your level uses.... (currently there is only one theme, and it is the default theme.)

### Adding a Level
Create a level and save the .tsx file into the appropriate assets folder (assets/soko/levels/...). The pre-processer will convert these to a custom .bin file. This works the same way that the image pre-processor does. THe output folder is flat (all folder structure is ignored), so all levels should prefixed by "SK_" to prevent conflicts with other swadge mode files. 

There is an SK_LEVEL_LIST.txt file with one level per line, with following syntax:

> :{id}:filename.bin:

The id's do not need to packed. A 'levelIndices' int array is created which maps the provided id's to a clean loopable array.

*Text is parsed by sokoExtractLevelNamesAndIndeces in soko.c. Importing is done by sokoLoadBinLevel in soko.c.*

### Adding to the Overworld

Overworld is a map with all puzzles in them. 'portal' objects are used to transition into level.
 In the overworld map file, the object has a custom property called 'target_id'. That gets set to the index of of the level. Everything else is handled by the engine.




 