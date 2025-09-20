# Design.md

This file contains the description of miniminer and all components.

miniminer (sic) is a small factory/zachlike style game designed to run within thw capabilities of the swadge.

The game consists of mining out several different types of resources, metals, rock, trees, oil and water to create items to complete various challenges. Once all the challenges are complete, a free build mode is available for players to see how much they can produce to sell in a time limit. Score more by selling more advanced products, and compete with friends with SwadgePass™ functionality.

## Required Hardware

This is the required hardware list for the project.

- TFT Screen
- Arrow buttons
- A, B and Start buttons
- C-Stick
- Accelerometer
- Thermometer
- At least 30kb of NVS space (Re-eval, this is a vibes number)

## Controls

- Arrows: Move the selected tile. Move to the edge of the screen to scroll
- A: Tool 1
- B: Tool 2
- Start: Open menu
  - Save and quit
  - Settings
- Select: Save and quit to main menu
- C-Stick: Select tool
- Shake: Clear map (Only in free play)

## Gameplay Systems

This is the list of systems required to create for miniminer.

### Levels

Each level contains a pre-defined map of tiles, and a set of objective. The tiles determine the play area, and the objectives are provided to motivate players and also act as extended tutorials. Not all features are unlocked in all zones. THe tile layout is going to be constructed from a compressed version of the map to save on space. The size of the map isn't consistent between levels. Each map has a specified width and height and then the tiles are laid into that in order.

Everything inside the level operates on a tick system, similar to Minecraft of Factorio. There's a tick rate (default 10/sec) that determines how fast everything operates. This also allows factories to be standardized, it doesn't matter if a game lags, A base that averages $10 per tick will be the same no matter the device it's running on. Not *really* important due to being a swadge, but maybe if factories become too large it will lag and this will keep it far.

#### Tiles

Each individual square on the grid is a tile, using the tile location on the grid as an ID.

Prototype: 
- Surface type (8 bits): The ground and base, initial speed, if can be moved by player

NVS struct:
- Data (16 bits): Data stored in the tile
  - Resource: Current count (TYPE_MAX val means infinite)
  - Oil: Rate (bits 0-3), amount left (bits 4-15)
  - Machine: Section (bits 0-3), Machine ID (bits 4-13), UNUSED (14-15)
- Power net (8 bits): ID of power net to interact with

Wrapper data:
- Items in tile: Pointer to items located inside a tile. not saved, but quickly simulated when loading.

NVS will not save tiles that are unmodified for storage reasons. Max NVS size is 3 bytes * num tiles, so is entirely dependant on usable play area.

##### Surface Type Details

The surface type determines how the tile looks, like dirt, grass, water, or concrete. It also determines some properties like what can be built on the tile or be extracted from the tile.

Mineable resources have various animation states to provide a visual indication of the remaining resources inside the node. If resource is set to TYPE_MAX, it is infinite and has a special depiction and description to indicate this. When a resource node is depleted, turns into dirt.

Tables 1-4 contain the single byte tile representation to aid in generation of the map. 
- Bits 0-3: Surface Type
- Bit 4: If player adjustable
- Bits 5-6: Special Data
- Bit 7: UNUSED

**Table 1: Surface Types**

| Resource | ID  | Produces | Works with | Bits 5-6 |
| -------- | --- | -------- | ---------- | -------- | 
| Coal | 0 | Rocks, Coal | Miners | Starting resource range (Table 2) |
| Concrete | 1 | N/A | N/A | Style (default, hazard, cracked, stone path) | 
| Copper | 2 | Rocks, Copper ore | Miners | Starting resource range (Table 2) |
| Dirt | 3 | N/A | N/A | Style (dirt, grass, sand, rocky) |
| Iron | 4 | Rocks, Iron ore | Miners | Starting resource range (Table 2) |
| Mixed | 5 | Rocks, Mined ores | Miners | Mixture (Table 3) |
| Oil well | 6 | Oil | Pump | Oil max extraction rate (0-15) |
| Boulder | 7 | Rocks | Miners | Starting resource range (Table 2) |
| Silicon | 8 | Rocks, Silicon Ore | Miners | Starting resource range (Table 2) |
| Walls | 9 | N/A | N/A. Cannot be interacted with | Style (Stone, security, cliff) |
| Water - Moving | A | Water | Pump, Waterwheel | Direction of water | 
| Water - Static | B | Water | Pump, Waterwheel | Style (Blank, Lily pad, Whirlpool, Nessie) | 
| Wood | C | Log | Loggers | Starting resource range (Table 2) |
| UNUSED | D |
| UNUSED | E |
| UNUSED | F |

**Table 2: Starting Resource Range**

| Value | Range |
| ----- | ----- |
| 0 | Infinite |
| 1 | Low |
| 2 | Med |
| 3 | High |

**Table 3: Mixture**

| Value | Mixture |
| ----- | ------- |
| 0 | Iron/Copper |
| 1 | Coal/Iron |
| 2 | Coal/Copper |
| 3 | Silicon/Copper |

**Table 4: Water Direction**

| Value | Direction |
| ----- | --------- |
| 0 | North |
| 1 | East |
| 2 | South |
| 3 | West |

#### Machines

Machines are anything added to a tile. These are stored into a list and serialized to be saved per level. Each machine has a function such as mining, smelting, crafting, etc. and each function can only be applied to certain items. The

The data for machines is arrayed as follows:

- Machine ID (10 bits): Which machine this is. Max value is 1023, throw warning when over 1000. Some values are reserved.
- Flags (6 bits): Used to determine current state 
  - If machine is locked from the player picking it up.
  - Rotation 
  - Recipe (Combined with type of machine, to reduce to less than 32 recipes)
  - Fuel ticks left (Max 5 bits)
  - Inventory ID (6 bits, can't be picked up if not empty)
- Type: Type of machine. Expected 8 bits, May be reduced if not required

Each machine requires 3 bytes to store, and only placed machines are saved to the NVS. This puts the max Machine NVS size to be 3096 per level.

##### Crafting

When a machine is ready to convert (Turn items into other items) it goes through several stages:
- Check ready. Has power, required items are available inside tile
- Wait certain number of ticks. Only counts if power net is satisfied
- Change items per recipe.

##### Machine details

Each machine has some details that make it unique.

- Type: Common item ID, such as for a furnace
- Name String
- Description
- Power type: What kind of power is required
- Power consumption: How much power is required 
- Power production type: If machine makes power, type
- Power Production: If the machine makes power, amount
- Resources required to craft
- Recipes
- Craft type
  - Forge: Metal -> shaped metal
  - Mine: Resource -> Ore
  - Autcrafter: Items -> Items
  - Smelter: Ore -> metal 
- Pipe connectors: if connected to liquids, the inlets and outlets
- Size/shape: Some machines are up to 16 tiles big.
- Time required to operate: In ticks
- Internal items inventory
- Craft speed modifier

See [Machines](./machines.csv) for a list of valid machines.

##### Special machines

All IDs 0-23 are reserved for special machines. These one have special properties that require being handled differently. Most of these are based on the goals of the level. Most can be used in prototyping to provide resources for testing.

| Machine name | ID  | Function |
| ------------ | --- | -------- |
| Drop-Off, Sell | 0 | Allows a player to sell items to the store |
| Pick-Up | 1 | Zone where resources come from, in prototyping mode can automatically provide items |
| Drop-Off, Goal | 2 | Put items here to accomplish a "Make x resource" goal, in prototyping mode can automatically void items |
| Power lines | 3 | Connects to an external power grid, can be set to provide or consume electricity |
| Pipeline | 4 | Connects to an external pipeline, can be set to produce or consume a piped resource |

#### Nets 

When a machine is attached to a net, it consumes the stated amount from the net. The max number of nets is 255.

Power is required to operate machines automatically, with the exception of solar/wind. Some machines will also provide power. There are converters (generators, motors), but those cause loss. Power nets must be extended to attach to machines. 

| Type | ID  | Units | Example unit | Additional info |
| ---- | --- | ----- | ------------ | --------------- |
| None | 0 | N/A | Crafting table | None |
| Mechanical (Simple) | 1 | Torque | Waterwheel | Used to power simple machines |
| Mechanical (Advanced) | 2 | Torque | Motor | Used to power more advanced machines |
| Steam | 3 | Pressure | Steam Turbine | Must be in a pipe | 
| Oil | 4 | Pressure | Assembler | Must be in a pipe |
| Water | 5 | Pressure | Assembler | Must be in a pipe |
| Electrical | 6 | Power | Solar, wind, generator | Carried by wires | 

Nets are defined by the following data:
- ID: (8 bits), provides the unique net
- value (15 bits, signed): Generation - Consumption. If negative, network shuts down
- Type: (3 bits): What kind of power it is

Nets are not saved to NVS and should be recalculated when loading.

### Items

All items are simple, non-functional bits of data that are moved around the map to generate machines and be sold for money. Every machine has an item form when it's inside the inventory.

- ID: The identified for a type of item. Due to containing Machines, probably 16 bits
- Name
- Description
- Sell price: How much the player gets for exporting a single unit
- Image: The image to display, plus a palette if required

See [Items](./items.csv) for a list of items that exist.

#### Special items

There are a few special items.

| Name | ID  | Usage | 
| ---- | --- | ----- |
| Anchor | 0 | Keeps a machine with an inventory anchored in place. |
| Artifacts | 1 | Valuable item that needs to be transported |
| Gold | 2 | Valuable items that need to be transported |

### Camera

The camera can be panned by moving the cursor to the edge of the screen, up to the edge of the map. There are several zoom levels to facilitate a better understanding of the map and to look around faster.

| Zoom Level | Tile Size on Screen | Depiction |
| ---------- | ------------------- | --------- |
| 5 (Max) | 32 x 32 | Surface type image, machines are animated. Power types are indicated via badges |
| 4 | 16 x 16 | Simplified machine images, no animations, exposed tiles have surface type images |
| 3 | 8 x 8 | Colors only, see chart |
| 2 | 4 x 4 | Colors only, see chart |
| 1 | 2 x 2 | Colors only, see chart |
| 0 (Min) | 1 x 1 | Colors only, see chart |

| Color | Represents |
| ----- | ---------- |
| Dark Blue | Water |
| Dark Gray | Ground |
| Dark Green | Infinite Trees |
| Dark Red | External connection |
| Light Blue | Electrical |
| Light Gray | Concrete |
| Light Green | Trees |
| Light Red | Pipes |
| Orange | Belts |
| Purple | Infinite Ore Veins |
| Pink | Ore Veins |
| Yellow | Machines |

### Tools

A list of tools that can be selected

#### Camera

- Tool 1: Zoom in
- Tool 2: Zoom out

#### Use tile

- Tool 1: Activates tile if if can be activated
- Tool 2: View tile/Machine info

#### Set tile properties

- Tool 1: Open tile/Machine properties
- Tool 2: View tile/Machine info

#### Place/Remove Tiles

- Tool 1 (empty tile): Places selected item
- Tool 1 (full tile): Removes the item in tile
- Tool 2: open inventory to select item

#### Place/Remove Wires

- Tool 1: Attach/remove wire
- Tool 2: View wire network data

### Crafting

- UI
- Pocket crafting

#### Recipes

Most items have a recipe to create them. These recipes tell a machine if it has the correct items and how fast it should be crafted.

Each machine that can use a recipe has an recipe group that they can use.

See [Recipes](./recipes.csv) for a list of groups and available items.

### Rating System

Players are rated on a few metrics:

For normal Levels with a defined goal:
- Cost: Amount required to set up the base from scratch in money. Lower is better
- Footprint: How many tiles the base requires. Lower is better
- Time required: Game time in seconds to achieve a result. Lower is better

For free play mission at the end
- Cost: Amount required to set up the base from scratch in money. Lower is better
- Footprint: How many tiles the base requires. Lower is better
- $/hr: How much is sold per 36000 ticks. Higher is better.

Once numbers are calculated, a final score is created. A baseline for each is coded in for comparison.

### Inventory

- IDs (0-63)
- Inf storage

### Shop

- Sell items from Main Inventory
- Buy items from shop at 50% markup

### Encyclopedia

View all items and machine data in an convenient place.

## Levels

### Mission structure

- Mission select
  - Tutorial
    - 1: Basics
    - 2: Pipes
    - 3: Electricity
  - Mission
    - TODO: Missions
  - Free Play
  - Prototyping

### Tutorial 1: Basics

Does not save solution

### Tutorial 2: Pipes

Does not save solution

### Tutorial 3: Electricity

Does not save solution

### Free Play

A large, premade map with plenty of resources for a player to use. This one has different metrics to allow a player to show off their understanding of the game by selling the best stuff as fast as possible.

### Prototyping

Does not save solution

## Settings

- Autosave frequency
- ticks/second target
- Shake to clear
- Uncapped data mode: Changes stored variables to be larger, only for use in emulator (???)
