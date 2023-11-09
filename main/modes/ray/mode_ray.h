#ifndef _MODE_RAY_H_
#define _MODE_RAY_H_

//==============================================================================
// Includes
//==============================================================================

#include "swadge2024.h"
#include "fp_math.h"
#include "starfield.h"
#include "esp_random.h"

//==============================================================================
// Defines
//==============================================================================

/** The number of total maps */
#define NUM_MAPS 6
/** The number of keys per map */
#define NUM_KEYS 3
/** The number of missile pickups per map */
#define MISSILE_UPGRADES_PER_MAP 3

/** The player's starting max health */
#define GAME_START_HEALTH 100
/** How much health is gained for each energy tank found */
#define HEALTH_PER_E_TANK 100
/** The number of energy tank pickups per map */
#define E_TANKS_PER_MAP 1
/** The player's total maximum possible health */
#define MAX_HEALTH_EVER (GAME_START_HEALTH + (NUM_MAPS * E_TANKS_PER_MAP * HEALTH_PER_E_TANK))

/** The maximum number of missiles possible */
#define MAX_MISSILES_EVER 99

/** Microseconds per effect when standing in lava or on heal */
#define US_PER_FLOOR_EFFECT 500000

/** Microseconds to charge the charge beam */
#define CHARGE_TIME_US 1048576

/** The number of bullets tracked at a given point in time */
#define MAX_RAY_BULLETS 32

/** The number of frames in an animation */
#define NUM_ANIM_FRAMES 4

/** The time to swap out and swap in a gun, in microseconds */
#define LOADOUT_TIMER_US (1 << 17)

/** The blink time for pause and dialog items */
#define BLINK_US 500000

/** The number of distinct enemies */
#define NUM_ENEMIES (OBJ_ENEMY_BOSS - OBJ_ENEMY_NORMAL + 1)

/** The time in uS for an enemy to warp in*/
#define E_WARP_TIME 1000000

/**
 * @brief Helper macro to check if a cell is of a given type
 * The type is the top three bits of the type
 *
 * @param cell The cell to check
 * @param type The type to check against (top three bits)
 */
#define CELL_IS_TYPE(cell, type) (((cell) & (0xE0)) == (type))

// Bits used for tile type construction, topmost bit
#define BG  0x00
#define OBJ 0x80
// Types of background, next two top bits
#define META  0x00
#define FLOOR 0x20
#define WALL  0x40
#define DOOR  0x60
// Types of objects, next two top bits
#define ITEM    0x00
#define ENEMY   0x20
#define BULLET  0x40
#define SCENERY 0x60

//==============================================================================
// Enums
//==============================================================================

/**
 * @brief Tile types. The top three bits are metadata, the bottom five bits are
 * a unique number per that metadata
 */
typedef enum __attribute__((packed))
{
    // Special empty type
    EMPTY = 0, // Equivalent to (BG | META | 0),
               // Special delete tile, only used in map editor
    DELETE = (BG | META | 1),
    // Background tiles, floor
    BG_FLOOR       = (BG | FLOOR | 1),
    BG_FLOOR_WATER = (BG | FLOOR | 2),
    BG_FLOOR_LAVA  = (BG | FLOOR | 3),
    BG_CEILING     = (BG | FLOOR | 4),
    BG_FLOOR_HEAL  = (BG | FLOOR | 5),
    // Walls
    BG_WALL_1 = (BG | WALL | 1),
    BG_WALL_2 = (BG | WALL | 2),
    BG_WALL_3 = (BG | WALL | 3),
    BG_WALL_4 = (BG | WALL | 4),
    BG_WALL_5 = (BG | WALL | 5),
    // Doors
    BG_DOOR          = (BG | DOOR | 1),
    BG_DOOR_CHARGE   = (BG | DOOR | 2),
    BG_DOOR_MISSILE  = (BG | DOOR | 3),
    BG_DOOR_ICE      = (BG | DOOR | 4),
    BG_DOOR_XRAY     = (BG | DOOR | 5),
    BG_DOOR_SCRIPT   = (BG | DOOR | 6),
    BG_DOOR_KEY_A    = (BG | DOOR | 7),
    BG_DOOR_KEY_B    = (BG | DOOR | 8),
    BG_DOOR_KEY_C    = (BG | DOOR | 9),
    BG_DOOR_ARTIFACT = (BG | DOOR | 10),
    // Enemies
    OBJ_ENEMY_START_POINT = (OBJ | ENEMY | 1),
    OBJ_ENEMY_NORMAL      = (OBJ | ENEMY | 2),
    OBJ_ENEMY_STRONG      = (OBJ | ENEMY | 3),
    OBJ_ENEMY_ARMORED     = (OBJ | ENEMY | 4),
    OBJ_ENEMY_FLAMING     = (OBJ | ENEMY | 5),
    OBJ_ENEMY_HIDDEN      = (OBJ | ENEMY | 6),
    OBJ_ENEMY_BOSS        = (OBJ | ENEMY | 7),
    // Power-ups
    OBJ_ITEM_BEAM        = (OBJ | ITEM | 1),
    OBJ_ITEM_CHARGE_BEAM = (OBJ | ITEM | 2),
    OBJ_ITEM_MISSILE     = (OBJ | ITEM | 3),
    OBJ_ITEM_ICE         = (OBJ | ITEM | 4),
    OBJ_ITEM_XRAY        = (OBJ | ITEM | 5),
    OBJ_ITEM_SUIT_WATER  = (OBJ | ITEM | 6),
    OBJ_ITEM_SUIT_LAVA   = (OBJ | ITEM | 7),
    OBJ_ITEM_ENERGY_TANK = (OBJ | ITEM | 8),
    // Permanent non-power-items
    OBJ_ITEM_KEY_A    = (OBJ | ITEM | 9),
    OBJ_ITEM_KEY_B    = (OBJ | ITEM | 10),
    OBJ_ITEM_KEY_C    = (OBJ | ITEM | 11),
    OBJ_ITEM_ARTIFACT = (OBJ | ITEM | 12),
    // Transient items
    OBJ_ITEM_PICKUP_ENERGY  = (OBJ | ITEM | 13),
    OBJ_ITEM_PICKUP_MISSILE = (OBJ | ITEM | 14),
    // Bullets
    OBJ_BULLET_NORMAL    = (OBJ | BULLET | 1),
    OBJ_BULLET_CHARGE    = (OBJ | BULLET | 2),
    OBJ_BULLET_ICE       = (OBJ | BULLET | 3),
    OBJ_BULLET_MISSILE   = (OBJ | BULLET | 4),
    OBJ_BULLET_XRAY      = (OBJ | BULLET | 5),
    OBJ_BULLET_E_NORMAL  = (OBJ | BULLET | 6),
    OBJ_BULLET_E_STRONG  = (OBJ | BULLET | 7),
    OBJ_BULLET_E_ARMOR   = (OBJ | BULLET | 8),
    OBJ_BULLET_E_FLAMING = (OBJ | BULLET | 9),
    OBJ_BULLET_E_HIDDEN  = (OBJ | BULLET | 10),
    // Scenery
    OBJ_SCENERY_TERMINAL = (OBJ | SCENERY | 1),
    OBJ_SCENERY_PORTAL   = (OBJ | SCENERY | 2),
    OBJ_SCENERY_F1       = (OBJ | SCENERY | 3),
    OBJ_SCENERY_F2       = (OBJ | SCENERY | 4),
    OBJ_SCENERY_F3       = (OBJ | SCENERY | 5),
    OBJ_SCENERY_F4       = (OBJ | SCENERY | 6),
    OBJ_SCENERY_F5       = (OBJ | SCENERY | 7),
    OBJ_SCENERY_F6       = (OBJ | SCENERY | 8),
    OBJ_SCENERY_F7       = (OBJ | SCENERY | 9),
} rayMapCellType_t;

/**
 * @brief Possible enemy states
 */
typedef enum
{
    E_WALKING_1,  ///< The enemy is walking, half cycle
    E_WALKING_2,  ///< The enemy is walking, the other half
    E_SHOOTING,   ///< The enemy is shooting (may move while shooting)
    E_HURT,       ///< The enemy was shot
    E_BLOCKING,   ///< The enemy is blocking
    E_DEAD,       ///< The enemy is dead
    E_NUM_STATES, ///< The number of enemy states
} rayEnemyState_t;

typedef enum
{
    DOING_NOTHING,
    MOVE_POS_X,
    MOVE_NEG_X,
    MOVE_POS_Y,
    MOVE_NEG_Y,
    MOVE_STRAFE_L,
    MOVE_STRAFE_R,
    MOVE_TOWARDS_PLAYER,
    MOVE_AWAY_PLAYER,
    MOVE_NE,
    MOVE_SE,
    MOVE_SW,
    MOVE_NW,
} rayEnemyBehavior_t;

/**
 * @brief All the possible loadouts
 */
typedef enum
{
    LO_NONE,     ///< No loadout
    LO_NORMAL,   ///< Normal loadout
    LO_MISSILE,  ///< Missile loadout
    LO_ICE,      ///< Ice beam loadout
    LO_XRAY,     ///< X-Ray loadout
    NUM_LOADOUTS ///< The number of loadouts
} rayLoadout_t;

/**
 * @brief All the possible environments
 */
typedef enum
{
    V_BASE,   ///< Sci-fi base / space station
    V_JUNGLE, ///< Jungle with alien ruins
    V_CAVE,   ///< Cave with lava
    NUM_ENVS, ///< The number of environments
} rayEnv_t;

/**
 * @brief All the possible textures per-environments
 */
typedef enum
{
    TX_WALL_1,     ///< Wall 1
    TX_WALL_2,     ///< Wall 2
    TX_WALL_3,     ///< Wall 3
    TX_WALL_4,     ///< Wall 4
    TX_WALL_5,     ///< Wall 5
    TX_FLOOR,      ///< The floor texture
    TX_CEILING,    ///< The ceiling texture
    NUM_ENV_TEXES, ///< The number of per-environment textures
} rayEnvTex_t;

/**
 * @brief All the possible boss states
 */
typedef enum
{
    B_MISSILE,       ///< Weak to missiles
    B_ICE,           ///< Weak to ice
    B_XRAY,          ///< Weak to xray
    B_NORMAL,        ///< Weak to normal
    NUM_BOSS_STATES, ///< The number of boss states
} rayBossState_t;

/**
 * @brief Types of events that trigger scripts
 */
typedef enum
{
    SHOOT_OBJS   = 0, ///< Objects were shot
    KILL         = 1, ///< Enemies were killed
    GET          = 2, ///< Items were gotten
    TOUCH        = 3, ///< Objects were touched
    SHOOT_WALLS  = 4, ///< Walls were shot
    ENTER        = 5, ///< Map cells were entered
    TIME_ELAPSED = 6, ///< Time elapsed
    NUM_IF_OP_TYPES,  ///< The number of IF operation types
} ifOp_t;

/**
 * @brief Types of things that scripts can do
 */
typedef enum
{
    OPEN    = 7,  ///< Open doors
    CLOSE   = 8,  ///< Close doors
    SPAWN   = 9,  ///< Spawn enemies or items
    DESPAWN = 10, ///< Despawn enemies or items
    DIALOG  = 11, ///< Show a dialog box
    WARP    = 12, ///< Warp to a location on a map
    WIN     = 13, ///< Win the game
} thenOp_t;

/**
 * @brief A script argument, whether to AND or OR items in a list
 */
typedef enum
{
    AND = 0, ///< All items in the list must happen for the script to trigger
    OR  = 1, ///< Any item in the list may happen for the script to trigger
} andOr_t;

/**
 * @brief A script argument, whether the items in a list are ordered or not
 */
typedef enum
{
    IN_ORDER  = 0, ///< Items in the list must happen in the specific order
    ANY_ORDER = 1, ///< Items in the list may happen in any order
} order_t;

/**
 * @brief A script argument, whether the script repeats after triggering or not
 */
typedef enum
{
    ONCE   = 0, ///< The script only triggers once
    ALWAYS = 1, ///< The script resets after triggering
} repeat_t;

typedef enum
{
    NO_KEY,   ///< Key was not picked up
    KEY,      ///< Key was picked up and not used yet
    OPEN_KEY, ///< Key was both picked up and used
} rayKeyState_t;

/**
 * @brief The different screens that can be displayed
 */
typedef enum
{
    RAY_MENU,         ///< The main menu is being shown
    RAY_GAME,         ///< The game loop is being shown
    RAY_DIALOG,       ///< A dialog box is being shown
    RAY_PAUSE,        ///< The pause menu is being shown
    RAY_WARP_SCREEN,  ///< The warp screen animation is being shown
    RAY_DEATH_SCREEN, ///< The player has died
} rayScreen_t;

/**
 * @brief The different pause screens that can be displayed
 */
typedef enum
{
    RP_LOCAL_MAP,   ///< The map the player is currently in
    RP_WORLD_MAP,   ///< All the maps and how they connect
    RP_NUM_SCREENS, ///< The number of pause screens
} rayPauseScreen_t;

/**
 * @brief Enum for visited tiles
 */
typedef enum __attribute__((packed))
{
    NOT_VISITED,      ///< This map tile has not been visited
    VISITED,          ///< This map tile has been visited
    SCRIPT_DOOR_OPEN, ///< This map tile has been visited and has a permanently-open door
} rayTileState_t;

//==============================================================================
// Structs
//==============================================================================

/**
 * @brief A script argument, coordinates in the map
 */
typedef struct
{
    uint8_t x; ///< The X coordinate
    uint8_t y; ///< The Y coordinate
} rayMapCoordinates_t;

/**
 * @brief A script argument, some object that should be spawned
 */
typedef struct
{
    rayMapCellType_t type;   ///< The type of object to spawn
    rayMapCoordinates_t pos; ///< Where to spawn the object
    uint8_t id;              ///< The ID of the spawned object
} raySpawn_t;

/**
 * @brief A script. This has some condition that triggers it (if) and some event
 * that occurs when it is triggered (then). The trigger and event both have
 * arguments stored in a union
 */
typedef struct
{
    bool isActive;         ///< true if the script is active, false if it is not
    int32_t resetTimerSec; ///< Timer to not re-trigger the script immediately
    ifOp_t ifOp;           ///< The type of condition that triggers the script
    /// A union of arguments for the condition that triggers the script
    union
    {
        uint32_t time; ///< A time in ms
        /// A struct of arguments which is a list of IDs
        struct
        {
            andOr_t andOr;      ///< Whether or not the IDs in the list should be AND'd or OR'd
            order_t order;      ///< Whether or not the order of IDs in the list matters
            repeat_t oneTime;   ///< Whether or not the script triggers once or repeatedly
            uint8_t numIds;     ///< The number of IDs in the list
            uint8_t* ids;       ///< A list of IDs
            bool* idsTriggered; ///< A list of triggered IDs
        } idList;
        /// A struct of arguments which is a list of map cells
        struct
        {
            andOr_t andOr;              ///< Whether or not the cells in the list should be AND'd or OR'd
            order_t order;              ///< Whether or not the order of cells in the list matters
            repeat_t oneTime;           ///< Whether or not the script triggers once or repeatedly
            uint8_t numCells;           ///< The number of cells in the list
            rayMapCoordinates_t* cells; ///< A list of cells
            bool* cellsTriggered;       ///< A list of triggered cells
        } cellList;
    } ifArgs;

    thenOp_t thenOp; ///< The type of event that happens
    /// A union of arguments for the event that the script triggers
    union
    {
        /// A struct of arguments which is a list of cells
        struct
        {
            uint8_t numCells;           ///< The number of cells in the list
            rayMapCoordinates_t* cells; ///< A list of cells
        } cellList;

        /// A struct of arguments which is a list of spawns
        struct
        {
            uint8_t numSpawns;  ///< The number of spawns in the list
            raySpawn_t* spawns; ///< A list of spawns
        } spawnList;

        /// A struct of arguments which is a list of IDs
        struct
        {
            uint8_t numIds; ///< The number of IDs in the list
            uint8_t* ids;   ///< A list of IDs
        } idList;

        char* text; ///< Text to be displayed

        /// A struct of arguments which is a map and cell destination to warp to
        struct
        {
            uint8_t mapId;           ///< The map ID to warp to
            rayMapCoordinates_t pos; ///< The position to warp to
        } warpDest;
    } thenArgs;

} rayScript_t;

/**
 * @brief A single map cell
 */
typedef struct
{
    uint16_t closeTimer;     ///< Timer to close the door after opening
    q8_8 doorOpen;           ///< A timer for this cell, if it happens to be a door
    rayMapCellType_t type;   ///< The type of this cell
    int8_t openingDirection; ///< If the door is opening or closing
} rayMapCell_t;

/**
 * @brief An entire map
 */
typedef struct
{
    uint32_t w;                   ///< The width of the map
    uint32_t h;                   ///< The height of the map
    rayMapCell_t** tiles;         ///< A 2D array of tiles in the map
    rayTileState_t* visitedTiles; ///< A 1D array of all the visited tiles in the map, row-order
} rayMap_t;

/**
 * @brief A texture with a name
 */
typedef struct
{
    char* name;    ///< The name of the texture
    wsg_t texture; ///< An image used as a texture
} namedTexture_t;

/**
 * @brief Common data for all objects in a map
 */
typedef struct
{
    wsg_t* sprite;         ///< The current sprite for this object
    q24_8 posX;            ///< The X position of this object
    q24_8 posY;            ///< The Y position of this object
    q24_8 radius;          ///< The radius of this object
    rayMapCellType_t type; ///< The object's type
    int32_t id;            ///< This object's ID
    bool spriteMirrored;   ///< Whether or not the sprite should be drawn mirrored
} rayObjCommon_t;

/**
 * @brief Data for a bullet in the map. It has common data and velocity
 */
typedef struct
{
    rayObjCommon_t c; ///< Common object properties
    q24_8 velX;       ///< The X velocity of this bullet
    q24_8 velY;       ///< The Y velocity of this bullet
} rayBullet_t;

/**
 * @brief Data for an enemy in the map. It has common data, state tracking, and textures
 */
typedef struct
{
    rayObjCommon_t c;                                ///< Common object properties
    int32_t health;                                  ///< The enemy's health
    rayEnemyState_t state;                           ///< This enemy's current state
    rayEnemyBehavior_t behavior;                     ///< What the enemy is currently doing
    int32_t warpTimer;                               ///< A timer for warping in
    rayBossState_t bossState;                        ///< The current boss state, unused for non-boss
    int32_t bossTimer;                               ///< A timer for changing bossState, unused for non-boss
    int32_t behaviorTimer;                           ///< A timer used for this enemy's behaviors
    int32_t shootTimer;                              ///< A timer used for this enemy's shooting
    int32_t blockTimer;                              ///< A timer used for this enemy's blocking
    int32_t invincibleTimer;                         ///< Timer for being invincible
    int32_t freezeTimer;                             ///< Timer for slowdown after ice beam
    int32_t animTimer;                               ///< A timer used for this enemy's animations
    int32_t animTimerLimit;                          ///< The time at which the texture should switch
    int32_t animFrame;                               ///< The current animation frame
    wsg_t (*sprites)[E_NUM_STATES][NUM_ANIM_FRAMES]; ///< All of this enemy's sprites
} rayEnemy_t;

/**
 * @brief The player's inventory
 */
typedef struct
{
    // Persistent pick-ups
    int32_t missilesPickUps[NUM_MAPS][MISSILE_UPGRADES_PER_MAP]; ///< ID list of acquired missile expansions
    int32_t healthPickUps[NUM_MAPS][E_TANKS_PER_MAP];            ///< ID list of acquired e.tanks
    // Current status
    int32_t health;         ///< The player's current health
    int32_t maxHealth;      ///< The player's current max health.
    int32_t numMissiles;    ///< The player's current missile count
    int32_t maxNumMissiles; ///< The player's current max missile count
    // Persistent beam pickups
    bool beamLoadOut;    ///< True if the normal beam was acquired
    bool chargePowerUp;  ///< True if the charge beam was acquired
    bool missileLoadOut; ///< True if a missile was acquired
    bool iceLoadOut;     ///< True if the ice loadout was acquired
    bool xrayLoadOut;    ///< True if the xray loadout was acquired
    // Persistent suit pickups
    bool lavaSuit;  ///< True if the lava suit was acquired
    bool waterSuit; ///< True if the water suit was acquired
    // Key items
    bool artifacts[NUM_MAPS];               ///< List of acquired artifacts
    rayKeyState_t keys[NUM_MAPS][NUM_KEYS]; ///< The number of small keys the player currently has
    int32_t damageMult;                     ///< A damage multiplier
} rayInventory_t;

/**
 * @brief A struct with all the player information saved to NVM
 */
typedef struct
{
    q24_8 posX;                 ///< The player's X position
    q24_8 posY;                 ///< The player's Y position
    q24_8 dirX;                 ///< The player's X direction
    q24_8 dirY;                 ///< The player's Y direction
    rayLoadout_t loadout;       ///< The player's current loadout
    int32_t mapId;              ///< The ID of the current map
    bool mapsVisited[NUM_MAPS]; ///< Booleans for each map visited
    rayInventory_t i;           ///< All the players items
} rayPlayer_t;

/**
 * @brief The entire game state
 *
 */
typedef struct
{
    rayScreen_t screen;           ///< The current screen being shown
    rayPauseScreen_t pauseScreen; ///< The current pause screen being shown

    menu_t* menu;                    ///< The main menu
    menuLogbookRenderer_t* renderer; ///< Renderer for the menu
    bool wasReset;                   ///< Flag to return to the main menu after wiping NVM

    rayMap_t map;      ///< The loaded map
    int32_t doorTimer; ///< A timer used to open doors

    rayPlayer_t p; ///< All the player's state, loaded from NVM

    int32_t warpDestMapId; ///< The ID of the current map
    q24_8 warpDestPosX;    ///< The player's X position
    q24_8 warpDestPosY;    ///< The player's Y position
    int32_t warpTimerUs;   ///< Timer to display warp screen

    rayBullet_t bullets[MAX_RAY_BULLETS]; ///< A list of all bullets
    list_t enemies;                       ///< A list of all enemies (moves, can be shot)
    list_t scenery;                       ///< A list of all scenery (doesn't move, can be shot)
    list_t items;                         ///< A list of all items (doesn't move, can be shot)

    q24_8 planeX; ///< The X camera plane, orthogonal to dir vector
    q24_8 planeY; ///< The Y camera plane, orthogonal to dir vector

    q24_8 wallDistBuffer[TFT_WIDTH]; ///< The distance of each vertical strip of pixels, used for sprite casting

    q24_8 posZ;       ///< The Z position, used for head bobbing
    int32_t bobTimer; ///< A timer used for head bobbing
    int32_t bobCount; ///< A count used to adjust posZ sinusoidally

    uint32_t btnState;           ///< The current button state
    bool isStrafing;             ///< true if the player is strafing, false if not
    rayObjCommon_t* targetedObj; ///< An object that is locked onto to strafe around

    rayLoadout_t nextLoadout;   ///< The player's next loadout, if touched
    int32_t loadoutChangeTimer; ///< A timer used for swapping loadouts
    bool forceLoadoutSwap;      ///< Force the loadout to change without touch input

    int32_t floorEffectTimer;   ///< Timer for effects when standing on a tile
    int32_t chargeTimer;        ///< Timer to charge shots
    int32_t playerShotCooldown; ///< Cooldown timer between shots
    int32_t gunShakeTimer;      ///< Timer to shake the gun when charged
    int32_t gunShakeX;          ///< Offset to draw gun at when shaking
    bool gunShakeL;             ///< true if the gun is shaking to the left, false otherwise

    namedTexture_t* loadedTextures;                             ///< A list of loaded textures
    uint8_t* typeToIdxMap;                                      ///< A map of rayMapCellType_t to respective textures
    wsg_t envTex[NUM_ENVS][NUM_ENV_TEXES];                      ///< The environment textures
    wsg_t guns[NUM_LOADOUTS];                                   ///< Textures for the HUD guns
    wsg_t portrait;                                             ///< A portrait used for text dialogs
    wsg_t missileHUDicon;                                       ///< A missile icon drawn in the HUD
    wsg_t enemyTex[NUM_ENEMIES][E_NUM_STATES][NUM_ANIM_FRAMES]; ///< The enemy textures
    wsg_t hiddenXRTex[E_NUM_STATES][NUM_ANIM_FRAMES];           ///< The textures for X-Ray hidden enemies
    wsg_t bossTex[NUM_BOSS_STATES - 1][E_NUM_STATES][NUM_ANIM_FRAMES]; ///< The textures for the boss

    font_t ibm;     ///< A font to draw the HUD
    font_t logbook; ///< A font to draw the menu

    const char* dialogText;     ///< A pointer to the current dialog text
    const char* nextDialogText; ///< A pointer to the next dialog text, if it doesn't fit in one box
    wsg_t* dialogPortrait;      ///< A portrait to draw above the dialog text

    int32_t btnLockoutUs; ///< A timer to block buttons when first showing a dialog
    int32_t blinkTimer;   ///< A timer to blink things on the pause menu
    bool blink;           ///< Boolean for two draw states on the pause menu

    list_t scripts[NUM_IF_OP_TYPES]; ///< An array of lists of scripts
    uint32_t scriptTimer;            ///< A microsecond timer to check for time based scripts
    uint32_t secondsSinceStart;      ///< The number of seconds since this map was loaded

    starfield_t starfield; ///< Starfield used for warp animation

    song_t songs[NUM_MAPS + 1]; ///< Per-map background music, plus a boss theme
    song_t sfx_door_open;       ///< SFX when a door opens
    song_t sfx_e_damage;        ///< SFX when an enemy takes damage
    song_t sfx_e_freeze;        ///< SFX when an enemy is frozen
    song_t sfx_p_charge;        ///< SFX when the charge beam is shot
    song_t sfx_p_damage;        ///< SFX when the player takes damage
    song_t sfx_p_shoot;         ///< SFX when the a normal beam is shot
    song_t sfx_e_block;         ///< SFX when an enemy blocks a shot
    song_t sfx_e_dead;          ///< SFX when an enemy dies
    song_t sfx_item_get;        ///< SFX when an item is obtained
    song_t sfx_p_charge_start;  ///< SFX when the charge beam starts to charge
    song_t sfx_p_missile;       ///< SFX when a missile is shot
    song_t sfx_p_ice;           ///< SFX when the ice beam is shot
    song_t sfx_p_xray;          ///< SFX when th xray beam is shot
    song_t sfx_warp;            ///< SFX when the player warps

    int32_t pRotationTimer; ///< timer for player rotation

    int32_t itemRotateTimer; ///< A timer to 'rotate' items by scaling the X direction
    int32_t itemRotateDeg;   ///< The number of degrees all items are 'rotated' by
    bool itemRotateMirror;   ///< If items should be drawn mirrored
} ray_t;

//==============================================================================
// Extern variables
//==============================================================================

extern swadgeMode_t rayMode;

extern const char* const rayMapNames[];
extern const paletteColor_t rayMapColors[];
extern const char RAY_NVS_KEY[];
extern const char* const RAY_NVS_VISITED_KEYS[];
extern const char MAGTROID_UNLOCK_KEY[];

//==============================================================================
// Functions
//==============================================================================

void rayFreeCurrentState(ray_t* ray);
void rayStartGame(void);

#endif
