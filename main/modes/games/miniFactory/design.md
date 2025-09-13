# Design.md

This file contains the description of the miniFactory game and all components.

## Gameplay 

Each level is consists of at least one board where tiles may be placed. A goal is defined, to make a product and move it to a designated tile. The method of creation opf the product is an exercise for the player. Each tile that is unoccupied can contain a machine tile which can be set by the player.

Each tile uses a certain amount of power, time, and money/resources to implement. A secondary goal is to get minimums in each of these, which results in a higher score and more resources to use in the next level.

Smaller components become individual components that can be used on the next scale.

Game operates on a tick system. Each block has a start state that checks if all inputs are present, then a set amount of time needs to pass before it spits out the the result. 5 ticks per second, unless I can't maintain that.

## Tile types

- Machines
  - Belt
  - Smelter
  - AutoCrafter
  - Pipe
  - Generator
  - Miner
- Resource node
  - Iron
  - Copper
  - Coal
  - Oil
  - Aluminum
  - Wood
  - Water
- Input
- Output
- Sell point
- Purchase node

### Machines

- Size
- Orientation
- Power cost
- Material cost
- Money cost
- Time to use
- Input / direction
- Output / direction

## Controls

Boards (Locations/Free build):
- Arrows: Move the selector around
- A: Use current tool
- B: Back
- C Stick: Select tool
  - Selector: Allows player to interact with environment. Open machine inventories, cut trees, mine ore
  - Placer: Add a Machine
  - Deconstructor: Remove a Machine
  - Wire mode: Creates wires between machines
  - Pipe mode: Creates pipes between machines
  - Conveyer mode: Draws belts between two objects. These require tiles free to move
  - Magical compressor: Allows blocks to be shrunk down
  - Craft: Some items can be hand crafted
- Start: Opens the menu
  - Save/quit
  - View encyclopedia: See the stats of each machine
  - Shop: Allows the player to buy items
  - Storage: Shows current inventory and allows the player to sell or craft items
  - Enter planning mode: Free build. Only a few slots
- Select: Prompts save and quit dialog
- Shaking Swadge: Prompts player to reset board

In Menu/Encyclopedia/Shops/Etc:
- Up/Down: Change selection
- Left/B: Go back
- A: Select

## Types

- Smelter
- Crafter
- Extruder
- Crusher
- Mixer
- Sawmill

## Eras:

- Hand tools
- Crude wood (Sawhorse, Workbench, Kiln)
- Advanced wood/Basic metal (Sawmill, Waterwheel, Waterhammer)
- Steam (Smelter, )
- Basic

## Overworld map

- Forest with a stream
- Forest
- Iron quarry
- 