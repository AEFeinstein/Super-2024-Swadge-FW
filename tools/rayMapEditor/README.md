# Ray Map Editor

## File Format

1. Map Dimensions (width x height)
1. The Map (row order)
    1. Background for the cell
    1. Object for the cell
    1. Object ID for the cell
1. Number of Scripts
1. Scripts

## Script Definition

### Syntax

Arguments, arrays, and CELLs all have different delimiters to make parsing easier.

| Element   | Syntax                    | Notes                                  |
|-----------|---------------------------|----------------------------------------|
| Arguments | `(a; b; c)`               | Arguments are only used for Operations |
| Arrays    | `[a, b, c]`               | May be arrays of any _other_ type      |
| CELL      | `{x. y}`                  | Only has x and y components            |
| ORDER     | `IN_ORDER` or `ANY_ORDER` |                                        |
| ID        | `0`                       | Just an integer                        |
| TIME      | `0`                       | Just an integer, in milliseconds       |
| BTN       | `0`                       | Just an integer, see `buttonBit_t`     |
| TEXT      | `abc`                     | Not quoted                             |

### IF Operations

| Operation      | Value | Arguments        | Description                                                                                                                                   |
|----------------|-------|------------------|-----------------------------------------------------------------------------------------------------------------------------------------------|
| SHOOT_OBJS     | 0     | \[IDs\], ORDER   | Condition triggered when all objects are shot (not killed), may either be in the given order or any order                                     |
| SHOOT_WALLS    | 1     | \[CELLs\], ORDER | Condition triggered when all walls are shot, may either be in the given order or any order                                                    |
| KILL           | 2     | \[IDs\], ORDER   | Condition triggered when all enemies are killed, may be in the given order or any order                                                       |
| ENTER          | 3     | CELL, \[IDs\]    | Condition triggered when the player enters the given cell. If IDs are not empty, the player must have the given items when entering the cell. |
| GET            | 4     | \[IDs\]          | Condition triggered when all items with given IDs are obtained                                                                                |
| TOUCH          | 5     | ID               | Condition triggered when item with the given ID is touched (i.e. warp gate)                                                                   |
| BUTTON_PRESSED | 6     | BTN              | Condition triggered when the button is pressed (useful for tutorial)                                                                          |
| TIME_ELAPSED   | 7     | TIME             | Condition triggered after the given time elapses from the start of the level                                                                  |

### THEN Operations

| Operation | Value | Arguments      | Description                                                                                    |
|-----------|-------|----------------|------------------------------------------------------------------------------------------------|
| OPEN      | 8     | \[CELLs\]      | Open all doors on the given cells                                                              |
| CLOSE     | 9     | \[CELLs\]      | Close all doors on the given cells                                                             |
| SPAWN     | 10    | CELL, TYPE, ID | Spawn an object of the given type with the given ID in the given cell. May be an item or enemy |
| DESPAWN   | 11    | \[IDs\]        | Immediately remove the given IDs from the map. May be item or enemy                            |
| DIALOG    | 12    | TEXT           | Display the text in a dialog window                                                            |
| WARP      | 13    | CELL           | Warp the player to the given cell                                                              |
| WIN       | 14    |                | Finish the level                                                                               |
