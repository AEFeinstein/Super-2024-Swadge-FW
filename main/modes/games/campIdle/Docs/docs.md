# Camp Idle game (Pending theme)

Player is tasked with surviving a length of time in a camp. They can scavenge for items, craft weapons, items and upgrades, and at night they have to defend themselves from a variety of enemies. When the swadge is off, the RTC will allow the player to craft and forage additional resources. Crafting is light. Combat can have odd controls utilizing the swadge's instrument form for 2027.

Endless mode gives you a amount of days survived and time taken, and players can compare their scores with each other.

## Hardware Requirements

- Arrow keys
- Screen
- Selection button
- 2 Slide pads for attacks
- RTC for passive item collection

## Completion checklist

- [ ] Art
  - [ ] UI
    - [ ] Font
    - [ ] Healing indicators
    - [ ] Prog bars
  - [ ] Player
    - [ ] Weapons/Attack anims
    - [ ] Character
    - [ ] Shields
  - [x] Items
    - [x] Food
    - [x] Foraged
    - [x] Crafted
    - [x] Healing
  - [ ] Monsters
    - [ ] Idle
    - [ ] Attack
  - [ ] Camp/Upgrades
- [ ] Sounds
  - [ ] Calm BGM
  - [ ] Frantic BGM
  - [ ] Hit sound
  - [ ] Miss sound
  - [ ] Damage sound
  - [ ] Craft sound
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
  - [ ] Upgrading Camp
  - [ ] Crafting items
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
- [ ] Misc
  - [ ] Trophies
  - [ ] LEDs

### Day

#### Foraging

- Can select specific targets (food, Small, Large leaves, etc.)
- Can select specific areas based on preparedness. Items only appear in specific zones
- Capacity is determined by carry weight and size
- RTC adds a new item chance for every 3 minutes since last viewed. Item chances are burned until either the bag is full or no chances remain

#### Crafting/Upgrading

- Some items require workbenches
- All items require other items to craft aside from those in "foraging"
- Workbenches, weapons and camp upgrades are visual upgrades
- Crafting menu

#### Viewing items

- Grid view with counts
- Detail view with blurb of funny information

### Night

#### Modes

- Set spawns: The spawns for the night are pre-determined
- Endless: Spawns are based on a difficulty value that starts lowe but increases steadily.
  - Endless mode is separate from the main game, and also provides a score to compete against other swadgepass users.

#### Fighting

Monsters come from the left side of the screen. The player is on the right side fo the screen. There's only so much visible depending on the amount of light. Player must use their items while waiting out hte timer.

The UI must display the following items:
- HP/MAX HP
- Stamina/Max Stamina
- HP Drain
- Stamina Gain
- End of night timer
- Qty of healing items/type

#### Monsters

All enemies come in at different speeds. Flying enemies are generally faster. Once in position, they attack after an interval. Flying enemies can only be hit by ground weapons once in attacking range. Ground enemies can be hit slightly before they get to attacking range. When an attack is being prepared, show a progress bar under the enemy.

| Monster | Type | HP  | Show up after |
| ------- | ---- | --- | ------------- |
| Mosquitoes | Air | 10 | Game start |
| Bats | Air | 25 | Swamp |
| Attack Squirrel | Air | 35 | Mountain |
| Hornets | Air | 50 | Jungle |
| Flying skull | Air | 70 | Magical Forest |
| Boars | Ground | 20 | Game Start |
| Slimes | Ground | 30 | Swamp |
| Goblins | Ground | 40 | Mountain |
| Snakes | Ground | 50 | Jungle |
| Fairies | Ground | 90 | Magical forest |

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
| Food stores | 1 | Small pouch | 5 dried grasses | + base regen | 30 food |
| Food stores | 2 | Buried box | 10 cut stone + shovel | + base regen | 70 food |
| Food stores | 3 | Food Barrel | 4 Logs/2 Iron | + Base Regen | 100 food |
| Food stores | 4 | Salted Stores | 10 Salt/4 Logs | + Base Regen | 3 Salt/150 food |
| Food stores | 5 | Magical Stasis | 5 Logs/20 Polished Crystals | Regen * 2 | 2 Polished Crystals/200 food |
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
| Crystal Polisher | 20 Stone/10 Diamond Powder | Polishes magical crystals | 
| Heartmaker | 10 Polished Crystals/10 Cloth/3 Rope | Creates Healing hearts |
| Magic Workbench | 4 Polished Crystals/4 Logs | Allows crafting of magical items |
| Smasher | 10 Rocks/10 Sticks/5 gears | Crushes items | 
| Smelter | 20 Rocks/10 Coal | Melts down ores |
| Stone Cutter | 4 logs/10 Rocks | Allows for creation of Cut Stone |
| Tanning Rack | 10 Sticks/4 Rope | Allows for the creation of pelts |
| Weaver | 20 sticks/4 gears/10 nails | Allows for creation of cloth |
| Workbench | 2 Logs/10 Sticks/2 Resin | Allows complex crafting |

## Materials/Items

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
| Birch Bark | 10 | Small | 2 | Swamp |
| Coal | 50 | Medium | 8 | Mountain |
| Crystal | 255 | Small | 3 | Magical Forest |
| Dried Grasses | 1 | Small | 2 | Forest |
| Iron Ore | N/A | Medium | 10 | Mountain |
| Large Leaves | 2 | Medium | 5 | Forest, Jungle |
| Latex | N/A | Small | 2 | Forest |
| Logs | 50 | Large | 20 | Forest, Jungle, Swamp |
| Resin | N/A | Small | 2 | Forest, Jungle |
| Rocks | N/A | Medium | 8 | All |
| Rock Salt | N/A | Medium | 4 | Mountain |
| Spiderweb | N/A | Small | 1 | Forest, Jungle, Swamp |
| Sticks | 5 | Small | 2 | Forest, Jungle, Swamp |
| Uncured Hides | N/A | Large | 7 | All |
| Vines | N/A | Medium | 5 | Jungle |
| Tar | 20 | Medium | 5 | Swamp |

### Crafted materials

Crafted materials
| Material | Made from | Required crafting stations |
| -------- | --------- | -------------------------- |
| Cloth | 10 cotton | Weaver |
| Cut blocks | 4 Stone | Stone Cutter |
| Diamond | 10 Rock | Smasher |
| Diamond Powder | 1 Diamonds | Smasher |
| Gears | 8 Sticks/5 Resin | Workbench |
| Iron | 1 Iron Ore/1 Coal | Smelter |
| Pelts | 1 Uncured Hide | Tanning Rack |
| Polished Blocks | 1 Cut Stone/1 Diamond Powder | Crystal Polisher |
| Polished Crystal | 1 Crystal/1 Diamond Powder | Crystal Polisher |
| Rope | 2 Vines | None |
| Salt | 0.25 Rock Salt | Smasher |
| String | Dried grasses | None |

### Foods

| Food | Food value | Location | Size | Weight | Extra |
| ---- | ---------- | -------- | ---- | ------ | ----- |
| Apples | 5 | Forest | Small | 1 | None |
| Beans (Canned) | 100 | Magical Forest | Small | 4 | None |
| Berries | 5 | Forest | Small | 1 | None |
| Donuts | 10 | Swamp | Small | 2 | None |
| Energy drink | 30 | Swamp | Small | 4 | None |
| Honey | 30 | Forest | Small | 3 | None |
| MRE | 50 | Mountain | Small | 6 | None |
| Mushrooms | 20 | Forest, Jungle | Small | 2 | None |
| Mystery Meat | 30 | Jungle | Medium | 8 | None |
| Pan Pizza | 30 | Mountain | Small | 2 | None |
| Pickles | 30 | Jungle | Small | 5 | None |
| Protein Powder | 10 | Magical Forest | Medium | 6 | None |
| Pudding | 60 | Magical Forest | Small | 3 | None |
| Roast Turkey | 40 | Mountain | Medium | 10 | None |
| Squeezy Peanut Butter | 5 | Jungle | Small | 1 | None |
| String Cheese | 20 | Swamp | Small | 1 | Only available between 2-4 AM |
| Tasteful Noods | 35 | Swamp | Medium | 3 | None |

| Bad Food | Food value | Location | Size | Weight |
| -------- | ---------- | -------- | ---- | ------ |
| Floor Pizza | -10 | Magical Forest | Medium | 3 |
| Furry Juice | -30 | Swamp | Small | 1 |
| I Can't Believe It's Not Margarine | -50 | Jungle | Small | 2 |
| Malort | -50 | Mountain | Medium | 7 |
| Pilk | -30 | Swamp | Medium | 4 |
| Raver Sweat | -20 | Mountain | Small | 1 |
| Squirrel Nuts | -10 | Jungle | Small | 1 |
| Your Parent's Lingering Affection | -1 | Magical Forest | Small | 10 |

### Tools

| Tools | Made from | Usage |
| ----- | --------- | ----- |
| Boots | 20 Rubber | Access Swamp |
| Fairy Amulet | 3 Rocks/3 Iron/1 Rope | Access Fairy Forest |
| Rock Hammer | 10 Rocks/3 Sticks | Smelter |
| Shovel | 5 Iron/10 Sticks | Tool |

### Weapons / Shields

| Weapon | Damage Per hit | Made from | Addt'l effects | Controls | Enemies |
| ------ | -------------- | --------- | -------------- | -------- | ------- |
| Pointy Stick | 1-3 | 3 Sticks | None | Tap on Swipe bar 2 | Ground/Close air | 
| Knife | 3-5 | 5 Rock/2 Sticks | None | Swipe down on Swipe bar 2 | Ground |
| Machete | 10-14 | 5 Iron/2 Sticks | Allows for going into Jungle | Swipe toward enemies on swipe bar 1 | Ground |
| Bow | 10-18 | 7 Sticks/3 String | None | Hold down Swipe bar 2 for 0.5 seconds | Air |
| Sword | 15-20 | 20 Iron/6 Cloth | None | Swipe toward enemies on swipe bar 1 | Ground |
| Hero's Sword | 25-32 | 5 Crystals/20 Iron/10 Cloth | Hits all ground enemies every three seconds | Swipe toward enemies on swipe bar 1 | Ground |
| Mage Bow | 25-32 | 3 Crystals/10 String/5 Sticks | Hits three targets at once | Hold down Swipe bar 2 for 0.5 seconds | Air |

Shield provide a bonus amount of HP called SHP and break when it reaches 0

| Shield | Effect | Made from |
| ------ | ------ | --------- |
| Plank | +50 SHP | 2 Logs |
| Metal Shield | +100 SHP | 10 Iron | 
| Hero's Shield | +250 SHP | 30 Iron/10 Crystals |

### Healing

Use Healing by swiping toward the character on slide bar 1

| Item | Effect | Made from |
| ---- | ------ | --------- |
| Healing Powder | +30 HP | 4 dried grasses | 
| Bandages | +50 HP | 3 Large Leaves/1 Beeswax |
| Healing Poultice | +35 HP | 3 Rock salt/5 Berries/1 Mystery Meat/2 Cloth |
| Potion | +100 HP | 2 Pickles/2 Energy Drinks | 
| Healing Hearts | +75 HP | Every 30s from machine |

## Trophies

- Bad apple spoils the bunch: Add bad food to your stores. (Use bad apple image for icon)