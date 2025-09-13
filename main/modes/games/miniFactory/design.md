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
  - Selector: Allows player to interact with environment
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
  - Storage: Shows current inventory and allows the player to sell
  - Enter planning mode: Free build. Only a few slots
- Select: Prompts save and quit dialog
- Shaking Swadge: Prompts player to reset board

In Menu/Encyclopedia/Shops/Etc:
- Up/Down: Change selection
- Left/B: Go back
- A: Select

## Eras:

- Hand work: Clicking on stuff. Free.
- Wood: Can be made and operated by hand, significantly speeds up crafting and stuff. Cannot be compressed
- Steam: Basic metalworking, uses burning coal or wood to power some machines. Cannot be compressed
- Basic electrical
- Oil
- Advanced electrical

## Overworld map

- Forest with a stream
- Forest
- Iron quarry
- 