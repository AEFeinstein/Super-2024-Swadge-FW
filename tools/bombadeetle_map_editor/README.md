# Bombadeetle Map Editor



## Usage

```
usage: bombadeetle_maze_bit_editor.py [-i FILE] [-o [FILE]] [-h] 

options:
    -?, --help          show this help message
    -i, --input         Import map from JSON
    -h, --headless      Do everything from the command line
    -o, --output        Set the name of the export binary

```

### Example

```bash
./bombadeetle_maze_bit_editor.py -i "D:\AwesomeWork\MaGFest\bomb_two.json" -o
```

## Controls

Left click on tiles and objects in the palette on the bottom to select them.
Left click on object's name again to remove it from the map.

Enter arrow count in the text boxes to set the number of arrows per level

Load, save, or 'save as' the current map and scripts with the buttons on the top.

| Key | Action |
| --- | ------ |
| `ctrl` + `o` | Open a map                                   |
| `ctrl` + `n` | Create new map                               |
| `ctrl` + `s` | Save the map                                 |
| `ctrl` + `shift` + `s` | Save the map as a different name   |
| `W`,`A`,`S`,`D` | Set selected Bombadeetle or Shloog facing |

## Tiles

The map is always the same exact size of 12 x 12. You place walls by clicking on the borders of each tile.
The map editor should make sure that you don't overlap entities like bombadeetles, shloogs, goals, holes, or teleporters but always make sure before exporting that the map is valid.

There is no error checking to make sure that the level is winable so manually test your level before commiting it to the game.


## Bombadeetle(BIN) File Format 

16.  Map Name (8 bit)
108. Map Walls/Teleporter/Goal/Hole Layout (8 bit)
    W = 0x01
    S = 0x02
    E = 0x04
    N = 0x08
    Teleporter = 0x10
    Goal       = 0x20
    Hole       = 0x40
108. Bombadeetle Layout - facing
    West  = 0x01
    South = 0x02
    East  = 0x04
    North = 0x08
108. Shloog Layout - facing
    West  = 0x01
    South = 0x02
    East  = 0x04
    North = 0x08
1. Left Arrows Count (8 bit)
1. Up Arrows Count (8 bit)
1. Down Arrows Count (8 bit)
1. Right Arrows Count (8 bit)