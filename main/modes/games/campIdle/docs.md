# Camp Idle game (Pending theme)

Player is tasked with surviving a length of time in a camp. They can scavenge for items, craft weapons, items and upgrades, and at night they have to defend themselves from a variety of enemies. When the swadge is off, the RTC will allow the player to craft and forage additional resources. Crafting is light. Combat can have odd controls utilizing the swadge's instrument form for 2027.

Endless mode gives you a amount of days survived and time taken, and players can compare their scores with each other.

## Hardware Requirements

### Hard
- Screen
- Directional controls (Menus, combat)
- Select button

### Soft
- Musical input for more complex gameplay
- RTC for swadge-off operation (crafting, foraging)
- Back button
- Wireless connections (Swadgepass)

## Systems

- [ ] Menus
  - [ ] Custom grid system
  - [ ] Crafting
- [ ] Scavenging
  - [ ] RTC based item scavenging
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
| Bed | 0 | Ground |
| Bed | 1 | Branches |
| Bed | 2 | Straw Mattress |
| Bed | 3 | Pelt Blanket |
| Bed | 4 | Cloth Blanket |
| Bed | 5 | Enchanted Blanket |
| Hauling | 0 | Arms |
| Hauling | 1 | Pouch |
| Hauling | 2 | Basket |
| Hauling | 3 | Backpack |
| Hauling | 4 | Sled |
| Hauling | 5 | Golem |
| Tent materials | 0 | None |
| Tent materials | 1 | Plant pile |
| Tent materials | 2 | Rock walls |
| Tent materials | 3 | Pelt tent |
| Tent materials | 4 | Cloth Tent |
| Tent materials | 5 | Magic infused cloth |
| Water stores | 0 | None |
| Water stores | 1 | Leaf Cup |
| Water stores | 2 | Bark Pot |
| Water stores | 3 | Waterskin |
| Water stores | 4 | Water Barrel |
| Water stores | 5 | Water condenser |

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
| Air defenses | 5 | Magic Arrows |  | Greatly damages enemies | None |
| Fire | 0 | None | None | No light provided | None |
| Fire | 1 | Small fire | 5 sticks/10 twigs | Provides small amount of light in the center of camp | 10 fuel/night |
| Fire | 2 | Campfire | 10 sticks/10 rocks/1 log | Provides a larger area of visibility | 25 fuel/night |
| Fire | 3 | Bonfire | 10 Logs/20 rocks | Provides a large amount of light, high fuel burn | 100 fuel/night |
| Fire | 4 | Lanterns | 10 Iron/10 beeswax/10 string | Provide mild light, but spread out | 10 wax/night |
| Fire | 5 | Magical Lantern | 10 Iron/10 Polished Magic crystals/10 Diamond Powder | Fully illuminates the camp | N/A |
| Food Stores | 0 | None | None | No additional regen | None |
| Food stores | 1 | Small pouch |
| Food stores | 2 | Buried box |
| Food stores | 3 | Food Barrel |
| Food stores | 4 | Salted Stores |
| Food stores | 5 | Magical Stasis |
| Ground defenses | 0 | None | None | No support | None |
| Ground defenses | 1 | Snares |
| Ground defenses | 2 | Spikes |
| Ground defenses | 3 | Stone Walls |
| Ground defenses | 4 | Pit Traps |
| Ground defenses | 5 | Spell traps |

## Crafting stations

Crafting stations
| Station | Required resources | Usage |
| ------- | ------------------ | ----- |
| Smelter | 20 Rocks/10 Coal | Melts down ores |
| Crystal Polisher |  | Polishes magical crystals | 
| Heartmaker |  | Creates Healing hearts |
| Magic Workbench |  | Allows crafting of magical items |
| Smasher | 10 Rocks/10 Sticks/5 gears | Crushes items |
| Workbench | 2 Logs/10 Sticks/2 Resin | Allows complex crafting |

## Materials

### Scavenged

Ice
Desert
Lava/Volcano
Beach

Scavenging can be done actively or passively via time passing. Scavenging can be preformed in the following locations:
| Locations | Required equipment |
| --------- | ------------------ |
| Forest | None |
| Jungle | Sharp weapon |
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
| Beehive | N/A | Medium | 5 | Forest |
| Coal | 50 | Medium | 8 | Mountain |
| Crystal | 255 | Small | 3 | Magical Forest |
| Dried Grasses | 1 | Small | 2 | Forest |
| Iron Ore | N/A | Medium | 10 | Mountain |
| Logs | 50 | Large | 20 | Forest, Jungle, Swamp |
| Resin | N/A | Small | 2 | Forest, Jungle |
| Rocks | N/A | Small | 4 | All |
| Rubber | N/A | Small | 2 | Jungle |
| Spiderweb | N/A | Small | 1 | Forest, Jungle, Swamp |
| Sticks | 5 | Small | 2 | Forest, Jungle, Swamp |
| Vines | N/A | Medium | 5 | Jungle |
| Tar | 20 | Medium | 5 | Swamp |
| Twigs | 1 | Small | 1 | Forest |

### Crafted materials

Crafted materials
| Material | Made from | Required crafting stations |
| -------- | --------- | -------------------------- |
| Beeswax | 1 Beehive (makes 5) | None | 
| Diamond Powder | 1 Diamonds | Smasher |
| Gears | 8 Sticks/5 Resin | Workbench |
| Grit | 1 Diamond Powder | Smasher |
| Iron | 1 Iron Ore/1 Coal | Smelter |
| Polished Crystal | 1 Crystal/1 Grit | Crystal Polisher |
| Rope | 2 Vines | None |
| String | Dried grasses | None |




| Beans (canned) |
| Berries | Food | Provides energy | N/A | Forest |
| Birch bark |
| Cactus |
| Coal |
| Crystals |
| Fish |
| Gold |
| Honey |
| Magical Wood |
| Mushrooms |
| Pelts |
| Salt |
| Spiderweb |
| Toxic mushrooms |
| Vines |
| Water |

## Tools

| Tools | Made from | Usage | Number of uses | Damage |
| ----- | --------- | ----- | -------------- | ------ |
| Boots | 20 Rubber | Access Swamp | Inf | N/A |
| Fairy Amulet |  | Access Fairy Forest | Inf | N/A |
| Iron Hammer | 10 Iron/3 Sticks | Smelter, Weapon | 100 | xx |
| Rock Hammer | 10 Rocks/3 Sticks | Smelter, weapon | 20 | xx |
| Torch | 3 Sticks/2 Tar | Weapon | 50 | 30 |

| Weapon | Weapon type |
| ------ | ----------- |
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
| Medical | 5 | Hearts |

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

## Characters

- Link Expy
- Zelda Expy