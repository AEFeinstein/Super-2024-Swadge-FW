# Ray Map Editor

## Controls

Left click on tiles and objects in the palette on the left edge to select them.
Left click on the map to place the selected tile or object.

Right click on the map to see that cell's coordinate and optionally that object's ID in the right text box

Middle click on the map to drag the whole map around.

Load, save, or 'save as' the current map and scripts with the buttons on the top.

## Tiles

A map is a grid of tiles. The map size is configurable, but no larger than 255x255 tiles.
Each tile has a coordinate, where `{0.0}` is in the top left corner. The coordinate may be checked by right clicking on a tile.

Each tile on the map must have a background. Backgrounds are in the left column of the palette.
Background properties are intrinsic so wall types cannot be walked through, door types will open and close, etc.
You don't need to program properties of backgrounds.

Each tile on the may may also have an object. Objects are in the right column of the palette.
Object properties arse also intrinsic so item types will be picked up when touched, enemy types will move around and fight, etc.
Each tile starts with up to one object, though more can be spawned later.
Each object has an ID which *must* be unique, including the IDs of spawned objects.
IDs are automatically assigned when placing objects on the map, but must be manually assigned when using them in scripts.
An object's ID may be checked by right clicking on it.

## Scripts

Scripts define interaction in the map. Each script is comprised of an `IF` and `THEN` part.
`IF` defines what must happen for the script to trigger, i.e. kill some enemies or get an item.
`THEN` defines what happens when the script triggers, i.e. open a door or warp to a location.
Any `IF` may be paired with any other `THEN`.

### IF Operations

These are the conditions which can trigger scripts.

| Operation    | Value | Arguments                          | Description                                                                                                                                  |
|--------------|-------|------------------------------------|----------------------------------------------------------------------------------------------------------------------------------------------|
| SHOOT_OBJS   | 0     | AND_OR, \[IDs\], ORDER, ONE_TIME   | Triggered when one or all objects are shot. If all, may either be in the given order or any order. May reset after triggering.               |
| KILL         | 1     | AND_OR, \[IDs\], ORDER, ONE_TIME   | Triggered when one or all all enemies are killed. If all, may be in the given order or any order. May reset after triggering.                |
| GET          | 2     | AND_OR, \[IDs\], ORDER, ONE_TIME   | Triggered when one or all objects with given IDs are obtained. If all, may be in the given order or any order. May reset after triggering.   |
| TOUCH        | 3     | AND_OR, \[IDs\], ORDER, ONE_TIME   | Triggered when one or all objects with the given ID are touched. If all, may be in the given order or any order. May reset after triggering. |
| SHOOT_WALLS  | 4     | AND_OR, \[CELLs\], ORDER, ONE_TIME | Triggered when one or all walls in the given cells are shot. If all, may be in the given order or any order. May reset after triggering.     |
| ENTER        | 5     | AND_OR, \[CELLs\], ORDER, ONE_TIME | Triggered when the player enters one or all of the given cells. If all, may be in the given order or any order. May reset after triggering.  |
| TIME_ELAPSED | 6     | TIME                               | Triggered after the given time, in seconds, elapses from the start of the level.                                                        |

### THEN Operations

These are the actions that occur when a script is triggered

| Operation | Value | Arguments   | Description                                                                                                                               |
|-----------|-------|-------------|-------------------------------------------------------------------------------------------------------------------------------------------|
| OPEN      | 7     | \[CELLs\]   | Open all doors on the given cells                                                                                                         |
| CLOSE     | 8     | \[CELLs\]   | Close all doors on the given cells                                                                                                        |
| SPAWN     | 9     | \[SPAWNs\]  | Spawn objects of the given types with the given IDs in the given cells. If an object with that ID exists already, it will not be spawned. |
| DESPAWN   | 10    | \[IDs\]     | Immediately remove the objects with the given IDs from the map.                                                                           |
| DIALOG    | 11    | TEXT        | Display the text in a dialog window                                                                                                       |
| WARP      | 12    | MAP, CELL   | Warp the player to the given cell                                                                                                         |
| WIN       | 13    |             | Beat the game                                                                                                                             |

### Script Element Syntax

These are the elements that are used for `IF` and `THEN` arguments.
Arguments, arrays, CELLs, and SPAWNs all have different delimiters to make parsing easier.

| Element   | Syntax          | Notes                                                           |
|-----------|-----------------|-----------------------------------------------------------------|
| Arguments | `(a; b; c)`     | Arguments are only used for Operations                          |
| Arrays    | `[a, b, c]`     | May be arrays of CELLs, SPAWNs, or IDs                          |
| CELL      | `{x. y}`        | Only has x and y components                                     |
| ID        | `0`             | Integer from 0 to 255                                           |
| SPAWN     | `{TYPE-ID-x.y}` | `TYPE` is any `tileType` (see below). ID, x, and y are integers |
| AND_OR    | `abc`           | `AND` or `OR`                                                   |
| ORDER     | `abc`           | `IN_ORDER` or `ANY_ORDER`                                       |
| ONE_TIME  | `abc`           | `ONCE` or `ALWAYS`                                              |
| TEXT      | `abc`           | Not quoted, cannot use the characters `(` or `)`                |
| TIME      | `0`             | Integer from 0 to 2147483647, in seconds                        |
| MAP       | `0`             | Integer corresponding to the map, 0 to 5                        |

### Tile Types

These are the objects that can be spawned

| Object                    | Notes                                              |
|---------------------------|----------------------------------------------------|
| `OBJ_ENEMY_NORMAL`        | Enemy type, weak to normal beam                    |
| `OBJ_ENEMY_STRONG`        | Enemy type, weak to charge beam                    |
| `OBJ_ENEMY_ARMORED`       | Enemy type, weak to missile                        |
| `OBJ_ENEMY_FLAMING`       | Enemy type, weak to ice beam                       |
| `OBJ_ENEMY_HIDDEN`        | Enemy type, weak to x-ray beam                     |
| `OBJ_ENEMY_BOSS`          | Boss Enemy                                         |
| `OBJ_ITEM_BEAM`           | Power-up, normal beam                              |
| `OBJ_ITEM_CHARGE_BEAM`    | Power-up, charge beam                              |
| `OBJ_ITEM_MISSILE`        | Power-up, missiles (also missile capacity upgrade) |
| `OBJ_ITEM_ICE`            | Power-up, ice beam                                 |
| `OBJ_ITEM_XRAY`           | Power-up, x-ray visor                              |
| `OBJ_ITEM_SUIT_WATER`     | Power-up, suit, water resistance                   |
| `OBJ_ITEM_SUIT_LAVA`      | Power-up, suit, lava resistance                    |
| `OBJ_ITEM_ENERGY_TANK`    | Power-up, energy tank                              |
| `OBJ_ITEM_KEY_A`          | Access item, key A                                 |
| `OBJ_ITEM_KEY_B`          | Access item, key B                                 |
| `OBJ_ITEM_KEY_C`          | Access item, key C                                 |
| `OBJ_ITEM_ARTIFACT`       | Access item, artifact                              |
| `OBJ_ITEM_PICKUP_ENERGY`  | Pickup, energy                                     |
| `OBJ_ITEM_PICKUP_MISSILE` | Pickup, missiles                                   |
| `OBJ_SCENERY_TERMINAL`    | Scenery, computer terminal                         |

### Script Examples

If three enemies are killed in any order, open a door:
```
IF KILL(AND; [1, 2, 3]; ANY_ORDER; ALWAYS) THEN OPEN([{7.2}])
```
If three objects are shot in a specific order, spawn an energy tank:
```
IF SHOOT_OBJS(AND; [41, 42, 43]; IN_ORDER; ONCE) THEN SPAWN([{OBJ_ITEM_ENERGY_TANK-99-42.45}])
```
When a specific item is gotten, win the game:
```
IF GET (OR; [99]; ANY_ORDER; ONCE) THEN WIN()
```
If an item is touched, then warp to map 3, cell {2.1}:
```
IF TOUCH (OR; [3]; ANY_ORDER; ONCE) THEN WARP(3; {2.1})
```
When four walls are shot in a specific order, kill some enemies:
```
IF SHOOT_WALLS (AND; [{1.1}, {1.2}, {1.3}, {1.4}]; IN_ORDER; ONCE) THEN DESPAWN([5, 6, 7, 8])
```
When a room is entered from either of two ways, close the doors behind the player:
```
IF ENTER (OR; [{42.39}, {42.51}]; ANY_ORDER; ALWAYS) THEN CLOSE([{42.38}, {42.52}])
```
After a minute, tell the player a minute has elapsed:
```
IF TIME_ELAPSED(60000) THEN DIALOG(One minute has elapsed!)
```

## RMD File Format

1. Map Width (8 bit)
1. Map Height (8 bit)
1. The Map (list of cells in row order)
    1. Background for the cell (8 bit)
    1. Object for the cell (8 bit)
        1. Object ID for the cell, if there is an object (8 bit)
1. Number of Scripts (8 bit)
1. Scripts (Variable length, see `rme_script.toBytes()`)
