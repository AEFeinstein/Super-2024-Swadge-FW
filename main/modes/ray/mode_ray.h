#ifndef _MODE_RAY_H_
#define _MODE_RAY_H_

//==============================================================================
// Includes
//==============================================================================

#include "swadge2024.h"
#include "fp_math.h"

//==============================================================================
// Defines
//==============================================================================

/** The number of total maps */
#define NUM_MAPS 6
/** The number of pickups per map (three missile, three e.tanks) */
#define NUM_PICKUPS_PER_MAP 3

/** The number of bullets tracked at a given point in time */
#define MAX_RAY_BULLETS 32

/** The number of non-walking frames in an animation */
#define NUM_NON_WALK_FRAMES 4
/** The number of walking frames in an animation (non-walking doubled) */
#define NUM_WALK_FRAMES (NUM_NON_WALK_FRAMES * 2)

/** The time to swap out and swap in a gun, in microseconds */
#define LOADOUT_TIMER_US (1 << 18)

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
    // Background tiles
    BG_FLOOR        = (BG | FLOOR | 1),
    BG_FLOOR_WATER  = (BG | FLOOR | 2),
    BG_FLOOR_LAVA   = (BG | FLOOR | 3),
    BG_CEILING      = (BG | FLOOR | 4),
    BG_WALL_1       = (BG | WALL | 1),
    BG_WALL_2       = (BG | WALL | 2),
    BG_WALL_3       = (BG | WALL | 3),
    BG_DOOR         = (BG | DOOR | 1),
    BG_DOOR_CHARGE  = (BG | DOOR | 2),
    BG_DOOR_MISSILE = (BG | DOOR | 3),
    BG_DOOR_ICE     = (BG | DOOR | 4),
    BG_DOOR_XRAY    = (BG | DOOR | 5),
    BG_DOOR_SCRIPT  = (BG | DOOR | 6),
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
    OBJ_ITEM_KEY      = (OBJ | ITEM | 9),
    OBJ_ITEM_ARTIFACT = (OBJ | ITEM | 10),
    // Transient items
    OBJ_ITEM_PICKUP_ENERGY  = (OBJ | ITEM | 11),
    OBJ_ITEM_PICKUP_MISSILE = (OBJ | ITEM | 12),
    // Bullets
    OBJ_BULLET_NORMAL  = (OBJ | BULLET | 13),
    OBJ_BULLET_CHARGE  = (OBJ | BULLET | 14),
    OBJ_BULLET_ICE     = (OBJ | BULLET | 15),
    OBJ_BULLET_MISSILE = (OBJ | BULLET | 16),
    OBJ_BULLET_XRAY    = (OBJ | BULLET | 17),
    // Scenery
    OBJ_SCENERY_TERMINAL = (OBJ | SCENERY | 1),
} rayMapCellType_t;

/**
 * @brief Possible enemy states
 */
typedef enum
{
    E_WALKING,  ///< The enemy is walking
    E_SHOOTING, ///< The enemy is shooting (may move while shooting)
    E_HURT,     ///< The enemy was shot
} rayEnemyState_t;

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

//==============================================================================
// Structs
//==============================================================================

/**
 * @brief A single map cell
 */
typedef struct
{
    rayMapCellType_t type; ///< The type of this cell
    q24_8 doorOpen;        ///< A timer for this cell, if it happens to be a door
} rayMapCell_t;

/**
 * @brief An entire map
 */
typedef struct
{
    uint32_t w;           ///< The width of the map
    uint32_t h;           ///< The height of the map
    rayMapCell_t** tiles; ///< A 2D array of tiles in the map
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
    rayObjCommon_t c;                         ///< Common object properties
    rayEnemyState_t state;                    ///< This enemy's current state
    uint32_t animTimer;                       ///< A timer used for this enemy's animations
    uint32_t animTimerLimit;                  ///< The time at which the texture should switch
    uint32_t animTimerFrame;                  ///< The current animation frame
    wsg_t* walkSprites[NUM_NON_WALK_FRAMES];  ///< The walking sprites for this enemy
    wsg_t* shootSprites[NUM_NON_WALK_FRAMES]; ///< The shooting sprites for this enemy
    wsg_t* hurtSprites[NUM_NON_WALK_FRAMES];  ///< The getting shot sprites for this enemy
} rayEnemy_t;

/**
 * @brief The player's inventory
 * TODO save to disk sometime
 */
typedef struct
{
    // Persistent pick-ups
    int32_t missilesPickUps[NUM_MAPS][NUM_PICKUPS_PER_MAP]; ///< Coordinate list of acquired missile expansions
    int32_t healthPickUps[NUM_MAPS][NUM_PICKUPS_PER_MAP];   ///< Coordinate list of acquired e.tanks
    // Current status
    int32_t health;         ///< The player's current health
    int32_t maxHealth;      ///< The player's current max health
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
    bool artifacts[6]; ///< List of acquired artifacts
} rayInventory_t;

/**
 * @brief The entire game state
 *
 */
typedef struct
{
    rayMap_t map;      ///< The loaded map
    int32_t mapId;     ///< The ID of the current map (TODO)
    int32_t doorTimer; ///< A timer used to open doors

    rayBullet_t bullets[MAX_RAY_BULLETS]; ///< A list of all bullets
    list_t enemies;                       ///< A list of all enemies (moves, can be shot)
    list_t scenery;                       ///< A list of all scenery (doesn't move, can be shot)
    list_t items;                         ///< A list of all items (doesn't move, can be shot)

    q24_8 posX;   ///< The player's X position
    q24_8 posY;   ///< The player's Y position
    q24_8 dirX;   ///< The player's X direction
    q24_8 dirY;   ///< The player's Y direction
    q24_8 planeX; ///< The X camera plane, orthogonal to dir vector
    q24_8 planeY; ///< The Y camera plane, orthogonal to dir vector

    q24_8 wallDistBuffer[TFT_WIDTH]; ///< The distance of each vertical strip of pixels, used for sprite casting

    q24_8 posZ;       ///< The Z position, used for head bobbing
    int32_t bobTimer; ///< A timer used for head bobbing
    int32_t bobCount; ///< A count used to adjust posZ sinusoidally

    uint32_t btnState;           ///< The current button state
    bool isStrafing;             ///< true if the player is strafing, false if not
    rayObjCommon_t* targetedObj; ///< An object that is locked onto to strafe around

    rayInventory_t inventory; ///< All the players items

    rayLoadout_t loadout;       ///< The player's current loadout
    rayLoadout_t nextLoadout;   ///< The player's next loadout, if touched
    int32_t loadoutChangeTimer; ///< A timer used for swapping loadouts
    bool forceLoadoutSwap;      ///< Force the loadout to change without touch input

    namedTexture_t* loadedTextures; ///< A list of loaded textures
    uint8_t* typeToIdxMap;          ///< A map of rayMapCellType_t to respective textures
    wsg_t guns[NUM_LOADOUTS];       ///< Textures for the HUD guns

    rayEnemy_t eTemplates[6]; ///< Enemy type templates, copied when initializing enemies

} ray_t;

//==============================================================================
// Extern variables
//==============================================================================

extern swadgeMode_t rayMode;

#endif
