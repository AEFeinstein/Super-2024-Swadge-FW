# Tomi's Quest Contribution Guide

## The git Process

The git process here is pretty much the same as the normal except you create a branch off of `tomis-quest`, not `main`, and open a pull request to merge back into `tomis-quest` too.

## Adding Enemies

### General Notes

The game uses the `q24_8` integer format for positions, velocities, etc. This format which has 24 bits of integer and 8 bits of decimal. This means the regular integer 256 corresponds to a `q24_8` value of 1. The `q24_8` width of each cell in the map is 1.

The sequence of events in a frame are:

| Step | Function | Description |
| ---- | -------- | ----------- |
| 1    | `drawBackground2d()` | The background is drawn. |
| 2    | `drawForeground2d()` | The foreground is drawn. This _must_ be done immediately after the background so sprites don't 'slip' relative to the layers. |
| 3    | `rayPlayerCheckButtons()` | Buttons are checked and acted on. This can start sword, and jump actions. This also moves the player, **calls `rayEnemyCheckCollision_t`**, checks scripted events, etc. Note that a lot of logic has ended up here and could use a refactoring. |
| 4    | `rayPlayerCheckJoystick()` | Touchpads are checked. This can start shield actions, create bombs, and arrows. |
| 5    | `rayPlayerCheckFloorEffect()` | Floor effects are checked (there currently are none). |
| 6    | `moveRayObjects()` | All non-player objects are moved including bullets and enemies. **`rayEnemyMain_t` and `rayEnemyDropAfterDeath_t` are called here.** |
| 7    | `checkRayCollisions()` | Checks if bullets touch the player, walls, doors, scenery, or enemies. **`rayEnemyGetShot_t` is called here.** Check if player touches items. |
| 8    | `checkScriptTime()` | Timer scripts are checked. |
| 9    | `warpToDestination()`, `rayShowDeathScreen()` | Warp and game over conditions are checked. |

### Enemy Data

Enemies are kind of like Swadge modes in that they have their own struct with function pointers and some common variables. The struct is defined in `mode_ray.h`:

```c
typedef struct rayEnemy
{
    // Common Variables
    rayObjCommon_t c;    ///< Common object properties
    int32_t health;      ///< The enemy's health
    wsg_t* portrait;     ///< Portrait used for scripts
    int32_t iFrameTimer; ///< Timer for invincibility frames
    void* state;         ///< Enemy specific state
    // Function Pointers
    rayEnemyMain_t mainFn;                     ///< The main function for this enemy, handles gameplay logic
    rayEnemyCheckCollision_t collisionFn;      ///< A function called to check if the player collides with an enemy
    rayEnemyGetShot_t getShotFn;               ///< A function called when the enemy is hit with something
    rayEnemyDropAfterDeath_t dropAfterDeathFn; ///< A function called after the enemy dies to drop an item
} rayEnemy_t;
```

Let's break this down in the context of a simple enemy, the Grunt.

First, the enemy gets a `rayMapCellType_t`: `OBJ_ENEMY_GRUNT`. You can pick the next available one.

### Initialization

Then there's an initializer function, `rayInitEnemyGrunt()`, which is called by `rayCreateEnemy()`. Each enemy needs an initializer in that list. The initializer does a few things

* All WSGs are loaded using `loadTexture()`. This is _very important_ because images loaded with that function don't double-load (i.e. two Grunts use the same WSGs) and are automatically freed when the map is destroyed.
    * _Do not_ use `loadWsg()`.
* `wsg_t* portrait` is set to a WSG. This was necessary for any dialog scripts triggered by enemy interaction.
* `int32_t health` is set to a non-zero value to not die.
* `int32_t iFrameTimer` is left as 0 because the enemy isn't invincible yet
* `void* state` is allocated to a `gruntState_t`. Each enemy may have it's own custom internal state tracking things like attack or defense posture, phase of the fight, etc. The Grunt cares about the direction it's traveling and if it got bumped.
* All function pointers are set to their respective functions.

### `rayEnemyMain_t`

The main function is called with the entire game state, `ray_t* ray`, the enemy to run logic for, `rayEnemy_t* enemy`, and the time since last call, `uint32_t elapsedUs`.

This function must return `false` if the enemy survived this loop or `true` if it did not and should be destroyed. Note that if there is a death animation, this should return `true` after the death animation is finished, not when health becomes 0.

This is the main function that governs enemy behavior. It is responsible for the game logic such as:

* Logic:
    * The Grunt uses a timer, `dirTimer`, to pick a new direction to travel every second.
* Movement:
    * The enemy's position `rayObjCommon_t` can be adjusted. To ensure the enemy doesn't clip through walls, it's recommended to get the bounding box with `rayGetObjBB()`, adjust he bounding box's position, check if it fits in the map with `rayBoundingBoxFitsInMap()`, and only update the position in `rayObjCommon_t` when successful.
    * Note that some enemies do not move, like Turrets, so this can be ignored in that case!
* Attacking:
    * The Grunt doesn't have an attack. Simply touching it damages the player (more on that later). However, the Turret enemy shoots bullets at the player using `rayCreateBullet()`. See there for an example.
* Animation:
    * The sprite may be swapped and attributes may be adjusted for animation purposes.

This function is not responsible for drawing the enemy. That is done automatically using variables set in `rayEnemy_t.rayObjCommon_t`:

* The sprite used is the current one in `sprite`. This sprite may be swapped for animation purposes.
* The sprite is centered at (`posX`,`posY`)
* `spriteMirrored` can be set to draw the sprite mirrored
* `spriteRotation` can be set to draw the sprite rotated
* `solidColor` is used to draw the sprite as a solid color, but this is used to blink the sprite during invincibility frames and _should not_ be manually adjusted.

### `rayEnemyCheckCollision_t`

This function is called with the entire game state, `ray_t* ray`, the enemy to check, `rayEnemy_t* enemy`, the player to check, `rectangle_t player`, and the player's desired movement, `q24_8* deltaX, q24_8* deltaY`.

Simple collisions can be calculated by getting the enemy's bounding box with `rayGetObjBB()` and checking for an intersection with `rectRectIntersection()`.

The player should only collide if `rayPlayerIsHittable()` returns `true` (i.e. the player is not jumping or invincible).

The collision may also check `rayPlayerIsShielding()`, which returns -1 if not shielding, or (0, 1, 2, 3) for the specific shield type when shielding. Maybe the enemy doesn't care if the shield is up or not.

Upon collision, the player may be damaged with `rayPlayerDecrementHealth()`. This also set's the player's invincibility frames, handles game-over states, etc.

Additionally, the player may be bumped away from the collision. For example:

```c
// Give the player a bump away from the grunt
ray->ps.vel.x = ray->p.posX - enemy->c.posX;
ray->ps.vel.y = ray->p.posY - enemy->c.posY;
fastNormVec(&ray->ps.vel.x, &ray->ps.vel.y);

// Bump for 250ms
ray->ps.bumpTimer = 250000;
```

The player's desired movement is given so that the player can influence enemy position and vice-versa.

* If the enemy is stationary, like a turret, and the player collides with the enemy, `deltaX` and `deltaY` may be set to 0 so that the player doesn't actually move.
* If the enemy is pushable, like a box, `deltaX` or `deltaY` may be added to the enemy's position. In this way, `rayEnemyCheckCollision_t` manages movement similar to `rayEnemyMain_t`.
* If the enemy is pushable, but moving it would clip a wall (i.e. invalid move), then `deltaX` and `deltaY` may be set to 0 so that the player doesn't actually move.

### `rayEnemyGetShot_t`

This function is called with the entire game state, `ray_t* ray`, the enemy which was shot, `rayEnemy_t* enemy`, and the type of bullet, `rayMapCellType_t bullet`. The types of bullets can be found by searching for `OBJ_BULLET_`. Yes, `OBJ_BULLET_SWORD` counts as a bullet because everything was a bullet in Magtroid Pocket.

When an enemy is hit, it should decrement `health` accordingly and likely set `iFrameTimer` to `ENEMY_DEFAULT_IFRAMES_US`. If you want the enemy to be invincible for longer or shorter or not at all, adjust `iFrameTimer`.

An enemy may be bumped (i.e. knocked back) when hit. Stationary enemies like the turret are not bumped. For an example of bumping, see the Grunt's `bumpVel` and `bumpTimer`.

### `rayEnemyDropAfterDeath_t`

This function takes no arguments and returns the `rayMapCellType_t` which is dropped. It should probably be set to `rayEnemyStandardItemDrop` or `NULL` for enemies which drop nothing. See that function as an example if you want custom drop types or rates, like having more difficult enemies drop higher denominations or higher rate health.

## Editing Maps

The main documentation for the map editor can be found at [the editor's README.md](../../tools/rayMapEditor/README.md). It is a python program that has been tested on all major OSs.

Map files can be found in [the assets directory](../../assets/tomisQuest/maps/).

### Anatomy of a Map

Simply, a map is made of cells, and each cell can have one background type and one foreground type. Backgrounds must be 20px by 20px and have no transparency. Their types are:
* Floors - Passable, non-interactive (except for `BG_FLOOR_HOLE`, which the player can jump over or fall into).
    * This is the first column in the map editor palette
* Walls - Impassible, non-interactive
    * This is the second column in the map editor palette
* Doors - Interactive, may be opened with a slash or respective key.
    * This is the third column in the map editor palette

Foregrounds may be any size (they are drawn centered on their position), may have transparency, and their types are:
* Items - Items are picked up when touched (hearts, stupees, keys, weapons, etc.)
    * This is the fourth column in the map editor palette
* Enemies - Self explanatory. In most cases, enemies will not be hardcoded into the map, but rather spawned with scripts when entering areas. There's a useful helper to build these scripts.
    * This is the fifth column in the map editor palette
* Scenery - A passable, interactive, foreground object (like a signpost, brazier, etc.).
    * This is the sixth column in the map editor palette
    * If you want scenery to be impassible, layer a wall underneath it, just be careful about transparency!

Foreground objects each have an ID which can be interacted with via scripts.

If you right click a cell in the map, the cell coordinates and an ID for the object on that cell will be displayed in the right window for convenience.

### Dungeon Templates

Dungeon templates are automatically generated so that doors and keys are placed in a way that will require backtracking to complete and not soft-lock the player. The rooms the keys are in and the key doors **must not be moved**. You may move the key within the room, have it dropped by an enemy there, place it after a puzzle, etc.

Likewise, the stairs at the beginning and end of the dungeon **should not be moved**. If they are moved, then the scripts for warping the players to and from Gaylordia Field must be modified both in the dungeon and in the field maps.

It is your job to fill in each room with something interesting. Some ideas for rooms include, but are not limited to:

* Combat Rooms with moving enemies
* Bullet hell rooms with stationary turrets (maybe make more turret types first? axis aligned shots? pinwheel shots?)
* Trap rooms (think a windy bridge surrounded by holes)
* A room with breakable pots or treasure (good for dead-ends)
* Permutations using the above
* Block pushing puzzles
* Switch-hitting puzzles
* Nothing at all, just some nice scenery!

It is also your job to open the lone scripted door in the template using the key item of the dungeon. This action is not part of template, but the placement of the scripted door is. The dungeon key items are:

1. Shield
2. Boomerang
3. Doria's Lullaby

Scripts make dungeons interactive and interesting. Full script documentation can be found on [the editor's README.md](../../tools/rayMapEditor/README.md). Sorry for the clunky syntax. Some ideas for scripts are:

* Spawn enemies when entering a room. This is a common script and has a built-in helper.
    1. Press `ctrl+e` or click the "Set E.Triggers" button.
    2. Select the location(s) on the map where entering them will spawn enemies.
    3. Press `ctrl+e` or click the "Set Enemies" button (same button, new text).
    4. Place enemies by selecting them from the palette, then drawing on the map. You can delete enemies using the X from the palette as well.
    5. Press `ctrl+e` or click the "Finish Script" button (same button, new text).
* Give players puzzle hints with `DIALOG` scripts (i.e. attached to signposts with `SHOOT_OBJS` or when enemies are killed with `KILL`).
* `SHOOT_WALLS` or `SHOOT_OBJS` in a specific order to open a door or spawn an item.
* Play a `SONG` or `TURNTABLES` to open a door or spawn an item.
    * Note that these items are acquired later in the game, and that `TURNTABLES` must use the `OBJ_SCENERY_TURNTABLE` object.
* Take some action after `TIME_ELAPSED` (speedrun incentive?)

### Testing Maps

Maps can be injected into the emulator for testing so that you don't have to run through the game each time. It's a good idea to clear any saved player state first

```bash
rm nvs.json
```

Then launch the emulator with the injected file:

```
./swadge_emulator -t --tomi-file=./assets/tomisQuest/maps/dungeon_1.rmd
```

Note that you may want to start with specific items in the inventory, which can be done by searching for the following in source, uncommenting parts, and recompiling:

```c
// Uncomment to start with all items for testing
// ray->p.i.haveEwiOfTime     = true;
// ray->p.i.haveBombs         = true;
// ray->p.i.haveJumpBoots     = true;
// ray->p.i.haveShield        = true;
// ray->p.i.haveBow           = true;
// ray->p.i.haveBoomerang     = true;
// ray->p.i.haveTurntables    = true;
// ray->p.i.haveDoriasLullaby = true;
```