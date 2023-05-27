# Ray Map Editor

## Tiles

A map is a grid of tiles. The map size is configurable, but no larger than 256x256 tiles.
Each tile has a coordinate, where `{0.0}` is in the top left corner. The coordinate may be checked by right clicking on a tile.

Each tile on the map must have a background. Backgrounds are in the left column of the palette.
Background properties are intrinsic so wall types cannot be walked through, door types will open and close, etc.
You don't need to program properties of backgrounds.

Each tile on the may may also have an object. Objects are in the right column of the palette.
Object properties arse also intrinsic so item types will be picked up when touched, enemy types will move around and fight, etc.
Each tile starts with up to one object, though more can be spawned later.
Each object has an ID which *must* be unique, including the IDs of spawned objects.
An object's ID may be checked by right clicking on it.

## Scripts

Scripts define interaction in the map. Each script is comprised of an `IF` and `THEN` part.
`IF` defines what must happen for the script to trigger, i.e. kill some enemies or get an item.
`THEN` defines what happens when the script triggers, i.e. open a door or warp to a location.
Any `IF` may be paired with any other `THEN`.

### IF Operations

These are the conditions which can trigger scripts.

| Operation      | Value | Arguments        | Description                                                                                                                         |
|----------------|-------|------------------|-------------------------------------------------------------------------------------------------------------------------------------|
| SHOOT_OBJS     | 0     | \[IDs\], ORDER   | Triggered when all objects are shot (not killed), may either be in the given order or any order                                     |
| SHOOT_WALLS    | 1     | \[CELLs\], ORDER | Triggered when all walls are shot, may either be in the given order or any order                                                    |
| KILL           | 2     | \[IDs\], ORDER   | Triggered when all enemies are killed, may be in the given order or any order                                                       |
| ENTER          | 3     | CELL, \[IDs\]    | Triggered when the player enters the given cell. If IDs are not empty, the player must have the given items when entering the cell. |
| GET            | 4     | \[IDs\]          | Triggered when all items with given IDs are obtained                                                                                |
| TOUCH          | 5     | ID               | Triggered when item with the given ID is touched (i.e. warp gate)                                                                   |
| BUTTON_PRESSED | 6     | BTN              | Triggered when the button is pressed (useful for tutorial)                                                                          |
| TIME_ELAPSED   | 7     | TIME             | Triggered after the given time elapses from the start of the level                                                                  |

### THEN Operations

These are the actions that occur when a script is triggered

| Operation | Value | Arguments  | Description                                                                                                                                                        |
|-----------|-------|------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| OPEN      | 8     | \[CELLs\]  | Open all doors on the given cells                                                                                                                                  |
| CLOSE     | 9     | \[CELLs\]  | Close all doors on the given cells                                                                                                                                 |
| SPAWN     | 10    | \[SPAWNs\] | Spawn objects of the given types with the given IDs in the given cells. May be an item or enemy. If an object with that ID exists already, it will not be spawned. |
| DESPAWN   | 11    | \[IDs\]    | Immediately remove the given IDs from the map. May be item or enemy.                                                                                               |
| DIALOG    | 12    | TEXT       | Display the text in a dialog window                                                                                                                                |
| WARP      | 13    | CELL       | Warp the player to the given cell                                                                                                                                  |
| WIN       | 14    |            | Finish the level                                                                                                                                                   |

### Script Element Syntax

These are the elements that are used for `IF` and `THEN` arguments.
Arguments, arrays, CELLs, and SPAWNs all have different delimiters to make parsing easier.

| Element   | Syntax             | Notes                                                                                                                                      |
|-----------|--------------------|--------------------------------------------------------------------------------------------------------------------------------------------|
| Arguments | `(a; b; c)`        | Arguments are only used for Operations                                                                                                     |
| Arrays    | `[a, b, c]`        | May be arrays of CELLs, SPAWNs, or IDs                                                                                                     |
| CELL      | `{x. y}`           | Only has x and y components                                                                                                                |
| SPAWN     | `{TYPE- ID- x. y}` | TYPE is any `tileType` (see below). ID, x, and y are integers                                                                              |
| ORDER     | `abc`              | `IN_ORDER` or `ANY_ORDER`                                                                                                                  |
| TEXT      | `abc`              | Not quoted, cannot use the characters `(` or `)`                                                                                           |
| ID        | `0`                | Integer from 0 to 255                                                                                                                      |
| BTN       | `0`                | Integer from 0 to 65535, see [`buttonBit_t`](https://github.com/AEFeinstein/Swadge-IDF-5.0/blob/main/components/hdw-btn/include/hdw-btn.h) |
| TIME      | `0`                | Integer from 0 to 2147483647, in milliseconds                                                                                              |

### Tile Types

These are the objects that can be spawned

| Object           | Notes                                                |
|------------------|------------------------------------------------------|
| `ENEMY_DRAGON`   | Enemy type                                           |
| `ENEMY_SKELETON` | Enemy type                                           |
| `ENEMY_KNIGHT`   | Enemy type                                           |
| `ENEMY_GOLEM`    | Enemy type                                           |
| `OBELISK`        | Static type, no implicit interaction                 |
| `GUN`            | Item type, can be picked up and used to shoot things |

### Script Examples

```
IF SHOOT_OBJS([0, 1, 2, 3]; IN_ORDER)     THEN OPEN([{0.1}, {2.3}])
IF SHOOT_WALLS([{4.5}, {6.7}]; ANY_ORDER) THEN CLOSE([{0.1}, {2.3}])
IF KILL([0, 1, 2, 3]; IN_ORDER)           THEN SPAWN([{ENEMY_DRAGON-98-0.1},{ENEMY_SKELETON-99-2.3}])
IF ENTER({4.5}; [6, 7, 8])                THEN DESPAWN([98, 99])
IF GET([0, 1, 2, 3])                      THEN WIN()
IF TOUCH(7)                               THEN WARP({5.3})
IF BUTTON_PRESSED(512)                    THEN DIALOG(BUTTON PRESSED)
IF TIME_ELAPSED(10000)                    THEN CLOSE([{7.7}, {8.8}])
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
