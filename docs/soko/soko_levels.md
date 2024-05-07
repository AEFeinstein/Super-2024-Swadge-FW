# Soko Binary File Format

The tools/soko folder contains a pre-processor that converts the [tiled](https://www.mapeditor.org/) tmx map files into a custom binary format (.bin). 

### How The Levels Work
The levels in the game are split into two elements. There are tiles. These are background items, such as 'Floor' or 'Wall'. There must be one at every location on the map (although "EMPTY" is an option).

A collision matrix defined in 'soko_gamerules.c' determines what entities can walk on what tiles.

Entities are stored in their own array, and represent anything that can move, basically. Entities can be (but mostly aren't) located at the same location as other entities. Player, Crate, WarpExternal, LaserEmitter, etc. are entities.

In the tiled editor, there are two layers: an 'Object Layer' called entities and a 'Tiles Layer' layer called tiles. These correspond to the internal structure of the level. There should **not** be more layers than this, as the converter is fragile and poorly written.

Each tile in the tilesheet has an id, and this is used when converting/loading the level to figure out what tile/object is where.

#### Defining the Game Mode
The player entity should contain a 'gamemode' custom property, set to one of the following options:

- SOKO_OVERWORLD
- SOKO_CLASSIC
- SOKO_EULER
- SOKO_LASERBOUNCE

#### Configuring the Overworld
The overworld level is where we will add connections between levels. The structure of the game is flat: the player must return to the overworld after they complete a level. There are not multiple overworlds (zones, world 2-2, etc).

The overworld uses an entity with the class 'warpexternal', with ID 3. This object contains a custom property 'target_id', which corresponds to the level ID value that should get loaded.

#### Setting the Levels
First, save the level tmx file in the appropriate folder in assets.

Then, edit the 'SK_LEVEL_LIST.txt' file.
Add a line for your level with the following syntax:

*:id:filename.bin:*

That's a colon, then a chosen whole-integer id value that doesn't conflict with others. They do not have to be sequenced or defined in order. Then another colon, then the filename. THis will be the same as the .tmx file,except with the .bin extension.

Then another colon at the end of the line.

The level is now ready to be loaded. Add it to the overworld map as described above.

---

### Preprocessor
"soko_tmx_preprocessor.py" scans a directory (recursively) for these files and puts them flat inside the spiffs_image output folder.

Because there is no folder structure on output, sokoban levels should follow a consistent naming scheme to avoid name conflicts.

### Converstion to Binary
tmx files are xml based. Each map should contain a tiles layer (called 'tileset') which gets read as the static tileset, and an objects layer called 'entities'. 

### The Binary Format
The format is a packed sequence of bytes.

First, 3 bytes of header information:
1. Width
2. Height
3. Gamemode

Ignoring entities and compression, the next is a tight "grid" of tile data, left to right, top to bottom, like a book. Each byte is the id of the tile at that position, defined in the 'soko_bin_t' enum in soko.h. The values don't necesarily correspond to the enum values for the tiles or entity structs; but instead the soko_bin_t enum. Which is only used for parsing the file.

#### Entities
As the data is parsed, the position of the last-parsed tile is kept. If the parser encounters a 'SKB_OBJSTART' byte (201), it does not 'advance' the position of the tiles, but instead creates an entity.

Entities are 'SKB_OBJSTART', then a byte defining some number of data pieces, then a 'SKB_OBJEND' byte. Each entity is at least 3 bytes. The position of the entity is the last parsed tile position, and the type of entity is determined by it's second byte. 

If, after the modes design has been finished and all entity type sizes are known, we can remove the 'SKB_OBJEND' byte. For now, we need it.

The rest of the bytes depend on the entity. The WARPEXTERNAL entity has one additional byte, defining the level ID to jump to. Warp internal works the same way. 

All Entity data is stored between bytes 200 and 255.

### Compression
After the file is created, it gets compressed/

> NOTE: this isn't implemented yet. 5/6/2024 - Hunter

The compression scheme is the 'SKB_COMPRESS' byte, followed by some number of times to repeat the previous byte minus 1. 

"Floor, Floor, Floor" could become "Floor, Compress, 1".
"Wall, Wall, Wall, Wall, Wall, Wall, Wall, Wall" would become "Wall, Compress, 6"

Because the data is stores in horizontal rows, this only compresses contiguous horizontal sections of tiles, including when "word wrapped". Regardless, it shouldn't hurt. 

#### Entity Binary Encoding Schemes
*START = 'SKB_OBJSTART', END = 'SKB_OBJEND', and 'SKB_' prefix ignored.*

- START, PLAYER, END
- START, CRATE, END
- START, WARPEXTERNAL, [Target ID], END

*Note: Todo as I re-write the converter in python*