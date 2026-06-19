# Camp Idle game (Pending theme)

Player is tasked with surviving a length of time in a camp. They can scavenge for items, craft weapons, items and upgrades, and at night they have to defend themselves from a variety of enemies. When the swadge is off, the RTC will allow the player to craft and forage additional resources. Crafting is light. Combat can have odd controls utilizing the swadge's instrument form for 2027.

Endless mode gives you a amount of days survived and time taken, and players can compare their scores with each other.

## Hardware Requirements

- Arrow keys
- Screen
- Selection button
- 2 Slide pads for attacks
- RTC for passive item collection

## Systems

- [ ] Menus
  - [ ] Custom grid system
  - [ ] Crafting
- [ ] Scavenging
  - [ ] RTC based item scavenging
    - [ ] Targeting specific items or random
    - [ ] Preparedness (Which zones you can go to)
    - [ ] Different zones for different items
  - [ ] Active Scavenge missions
- [ ] Base building
  - [ ] Decorating camp
  - [ ] Upgrading Camp
  - [ ] Crafting items
    - [ ] RTC based crafting
    - [ ] Player crafting
  - [ ] Crafting weapons
- [ ] Night survival
  - [ ] Modes
    - [ ] Main game (Has ending)
    - [ ] Endless (keeps getting harder until you die, one chance each, longer night timers)
      - [ ] Scoring
        - [ ] Swadgepass
  - [ ] Fighting
    - [ ] Weapons
    - [ ] Enemies
    - [ ] Attacking

## Camp Upgrades

There are two types of upgrades, permanent and ones that burn resources. 

### Permanent

Upgrade types/effects
- Bed: Max Stamina
- Hauling: Amount of resources the player can bring back
- Tent: Temperature (Mitigates HP drain)
- Water stores: Max HP

| Type | Tier | Upgrade Name | Required items | Mechanics |
| ---- | ---- | ------------ | -------------- | --------- |
| Bed | 0 | Ground | None | Base stamina only |
| Bed | 1 | Branches | 20 Sticks | Base stamina * 2 |
| Bed | 2 | Straw Mattress | 30 Dried Grasses | Base stamina * 3 |
| Bed | 3 | Pelt Blanket | 15 Pelts/Tanning rack | Base Stamina * 4 |
| Bed | 4 | Cloth Blanket | 20 Cloth | Base Stamina * 5 |
| Bed | 5 | Enchanted Blanket | 20 Cloth/5 Polished Crystals | Base Stamina * 6 | 
| Hauling | 0 | Arms | None | 50 carry weight |
| Hauling | 1 | Pouch | 2 Large Leaves | +20 weight |
| Hauling | 2 | Basket | 30 Dried Grasses | +30 carry weight |
| Hauling | 3 | Backpack | 10 Cloth/2 Ropes | +50 carry weight |
| Hauling | 4 | Sled | 20 Iron/5 logs/5 Ropes | +100 carry weight |
| Hauling | 5 | Golem | 20 Polished Blocks/15 Polished Crystals | Always foraging with 150 carry weight |
| Tent materials | 0 | None | None | HP Drain @ 5/sec |
| Tent materials | 1 | Plant pile | 20 Sticks/10 Large Leaves | HP Drain @ 3/Sec |
| Tent materials | 2 | Rock Walls | Cut Stone | HP Drain @ 1/Sec |
| Tent materials | 3 | Pelt tent | 30 Pelts/5 Rope | HP Drain @ 1/2 Sec |
| Tent materials | 4 | Cloth Tent | 40 Cloth/15 Rope | HP Drain @ 1/5 Sec |
| Tent materials | 5 | Magic infused cloth | 50 Cloth/10 Rope/5 Crystals | No HP Drain |
| Water stores | 0 | None | None | Max HP @ 100 |
| Water stores | 1 | Leaf Cup | 2 Large Leaves | +50 Max HP |
| Water stores | 2 | Bark Pot | 5 Birch Bark | +100 Max HP |
| Water stores | 3 | Waterskin | 10 Rubber/5 Pelt | +100 Max HP |
| Water stores | 4 | Water Barrel | 20 Rubber/5 Logs/10 Iron | +150 Max HP |
| Water stores | 5 | Water condenser | 30 Rubber/5 Polished Crystals | +200 Max HP |

### Requires resources

Types and effects
- Air defenses: Slow down/hurt airborne enemies
- Fire: Visibility range / Number of enemies
- Food stores: Stamina regen rate
- Ground Defenses: Slow down/hurt ground enemies

| Type | Tier | Upgrade Name | Required items | Mechanics | Resource |
| ---- | ---- | ------------ | -------------- | --------- | -------- |
| Air defenses | 0 | None | None | No support | None |
| Air defenses | 1 | Spiderwebs | 10 Sticks | Slows down air enemies | 10 spiderwebs/night |
| Air defenses | 2 | Auto Slingshot | 5 Sticks/2 rocks/2 Rubber | Lightly damages incoming enemies | 10 rocks/night |
| Air defenses | 3 | Launched Nets | 10 Sticks/10 Rubber/10 Rope | Greatly slows down enemies | 2 Rope/night |
| Air defenses | 4 | Fireball launchers | 10 gears/5 tar/20 sticks | Mildly damages enemies | 5 Tar/night |
| Air defenses | 5 | Magic Arrows | 20 gears/10 tar/5 Polished Magic crystals | Greatly damages enemies | None |
| Fire | 0 | None | None | No light provided | None |
| Fire | 1 | Small fire | 5 sticks/10 twigs | Provides small amount of light in the center of camp | 10 fuel/night |
| Fire | 2 | Campfire | 10 sticks/10 rocks/1 log | Provides a larger area of visibility | 25 fuel/night |
| Fire | 3 | Bonfire | 10 Logs/20 rocks | Provides a large amount of light, high fuel burn | 100 fuel/night |
| Fire | 4 | Lanterns | 10 Iron/10 beeswax/10 string | Provide mild light, but spread out | 10 wax/night |
| Fire | 5 | Magical Lantern | 10 Iron/10 Polished Magic crystals/10 Diamond Powder | Fully illuminates the camp | N/A |
| Food Stores | 0 | None | None | Base Regen | None |
| Food stores | 1 | Small pouch | 5 dried grasses | + base regen | 3 food |
| Food stores | 2 | Buried box | 10 cut stone + shovel | + base regen | 7 food |
| Food stores | 3 | Food Barrel | 4 Logs/2 Iron | + Base Regen | 10 food |
| Food stores | 4 | Salted Stores | 10 Salt/4 Logs | + Base Regen | 3 Salt/15 food |
| Food stores | 5 | Magical Stasis | 5 Logs/20 Polished Crystals | Regen * 2 | 2 Polished Crystals/20 food |
| Ground defenses | 0 | None | None | No support | None |
| Ground defenses | 1 | Snares | 10 Sticks | Slows down ground enemies | 10 Spiderwebs/night |
| Ground defenses | 2 | Spikes | 20 Logs | Lightly damages ground enemies | 4 Logs |
| Ground defenses | 3 | Stone Walls | 30 Cut Blocks | Greatly slows down ground enemies | 5 Cut blocks |
| Ground defenses | 4 | Pit Traps | 10 Polished Blocks/10 Logs/10 Bamboo + Shovel | Greatly damages enemies | 10 Bamboo |
| Ground defenses | 5 | Spell traps | 20 Polished Crystals | Kills an enemy every 5 seconds | 5 Polished Crystals |

## Crafting stations

Crafting stations
| Station | Required resources | Usage |
| ------- | ------------------ | ----- |
| Crystal Polisher |  | Polishes magical crystals | 
| Heartmaker | 10 Polished Crystals/10 Cloth/3 Rope | Creates Healing hearts |
| Magic Workbench | 4 Polished Crystals/4 Logs | Allows crafting of magical items |
| Smasher | 10 Rocks/10 Sticks/5 gears | Crushes items | 
| Smelter | 20 Rocks/10 Coal | Melts down ores |
| Stone Cutter | 4 logs/10 Rocks | Allows for creation of Cut Stone |
| Tanning Rack | 10 Sticks/4 Rope | Allows for the creation of pelts |
| Weaver | 20 sticks/4 gears/10 nails | Allows for creation of cloth |
| Workbench | 2 Logs/10 Sticks/2 Resin | Allows complex crafting |

## Materials

### Scavenged

Scavenging can be done actively or passively via time passing. Scavenging can be preformed in the following locations:
| Locations | Required equipment |
| --------- | ------------------ |
| Forest | None |
| Jungle | Machete |
| Magical Forest | Fairy Amulet |
| Mountain | Basket |
| Swamp | Boots |

Materials stats:
- Fuel: How much item can be burned for
- Location: Where item is located
- Size: Small, Medium, Large, Massive. Player has limited capacity to bring back items
- Weight: Arbitrary units, limits amount player can bring back

Scavenged materials:
| Material | Fuel | Size | Weight | Locations |
| -------- | ---- | ---- | ------ | --------- |
| Bamboo | 1 | Medium | 3 | Jungle |
| Beehive | N/A | Medium | 5 | Forest |
| Birch Bark | 10 | Small | 2 | Swamp
| Coal | 50 | Medium | 8 | Mountain |
| Crystal | 255 | Small | 3 | Magical Forest |
| Dried Grasses | 1 | Small | 2 | Forest |
| Iron Ore | N/A | Medium | 10 | Mountain |
| Large Leaves | 2 | Medium | Forest, Jungle |
| Logs | 50 | Large | 20 | Forest, Jungle, Swamp |
| Resin | N/A | Small | 2 | Forest, Jungle |
| Rocks | N/A | Small | 4 | All |
| Rock Salt | N/A | Medium | 4 | Mountain |
| Rubber | N/A | Small | 2 | Jungle |
| Spiderweb | N/A | Small | 1 | Forest, Jungle, Swamp |
| Sticks | 5 | Small | 2 | Forest, Jungle, Swamp |
| Uncured Hides | N/A | Large | 7 | All |
| Vines | N/A | Medium | 5 | Jungle |
| Tar | 20 | Medium | 5 | Swamp |
| Twigs | 1 | Small | 1 | Forest |

### Crafted materials

Crafted materials
| Material | Made from | Required crafting stations |
| -------- | --------- | -------------------------- |
| Beeswax | 1 Beehive (makes 5) | None | 
| Cloth | 10 cotton | Weaver |
| Cut blocks | 4 Stone | Stone Cutter |
| Diamond Powder | 1 Diamonds | Smasher |
| Gears | 8 Sticks/5 Resin | Workbench |
| Grit | 1 Diamond Powder | Smasher |
| Iron | 1 Iron Ore/1 Coal | Smelter |
| Pelts | 1 Uncured Hide | Tanning Rack |
| Polished Blocks | 1 Cut Stone/1 Grit | Crystal Polisher |
| Polished Crystal | 1 Crystal/1 Grit | Crystal Polisher |
| Rope | 2 Vines | None |
| Salt | 0.25 Rock Salt | Smasher |
| String | Dried grasses | None |

### Foods

| Item | Food value | Location |
| ---- | ---------- | -------- |
| Beans (Canned) |
| Berries |
| Honey |
| Mushrooms |


| Birch bark |
| Cactus |
| Fish |
| Gold |
| Magical Wood |
| Toxic mushrooms |

## Tools

| Tools | Made from | Usage |
| ----- | --------- | ----- |
| Boots | 20 Rubber | Access Swamp |
| Fairy Amulet |  | Access Fairy Forest |
| Rock Hammer | 10 Rocks/3 Sticks | Smelter |
| Shovel | 5 Iron/10 Sticks | Tool |

| Weapon | Weapon type | Usage | Damage Per hit |
| ------ | ----------- | ----- | -------------- |
| Machete | Sharp |
| Pointy Stick | Sharp |
| Knife |
| Sword | 
| Hero's Sword |

| Shield |
| ------ |
| Plank |
| Metal Shield |
| Hero's Shield |

MAYBE
- Knife
- Bow
- Sword
- Magic staff (Fire)
- Magic staff (Ice)
- Shield
- Boomerang

- Torch: 5 sticks/10 Pitch/3 cloth - 10 pitch a night, illuminates the edges pre-magical lantern

## Items

### Healing

| Medical | 1 | Healing Powder |
| Medical | 2 | Bandages |
| Medical | 3 | Healing Poultice |
| Medical | 4 | Potion |
| Medical | 5 | Healing Hearts |

# Misc - Unset

Idle Activities:
- Forage - Provide a zone, continues to work until woken back up 
- Craft - Provide a queue, will work until woken up or run out of queue

## Monsters

- Bats (Keese)
- Slimes (Chus)
- Goblins (Boblins)
- Flying skulls 
- Snakes
- Bears
- Wolves
- Wall masters
- Deku Scrub (Pretends to be a bush)
- Boars

## Characters

- Link Expy
- Zelda Expy