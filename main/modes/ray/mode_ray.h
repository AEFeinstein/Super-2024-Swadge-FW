#ifndef _MODE_RAY_H_
#define _MODE_RAY_H_

#include "swadge2024.h"
#include "fp_math.h"

#define MAX_RAY_BULLETS 32

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

typedef enum __attribute__((packed))
{
    // Special empty type
    EMPTY = (BG | META | 0),
    // Special delete tile
    DELETE = (BG | META | 2),
    // Background tiles
    BG_FLOOR        = (BG | FLOOR | 1),
    BG_FLOOR_WATER  = (BG | FLOOR | 2),
    BG_FLOOR_LAVA   = (BG | FLOOR | 3),
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

typedef struct
{
    rayMapCellType_t type;
    q24_8 doorOpen;
} rayMapCell_t;

typedef struct
{
    uint32_t w;
    uint32_t h;
    rayMapCell_t** tiles;
    // TODO load rules somewhere
} rayMap_t;

typedef struct
{
    char* name;
    wsg_t texture;
} namedTexture_t;

typedef struct
{
    wsg_t* sprite;         ///< The current sprite for this object
    q24_8 posX;            ///< The X position of this object
    q24_8 posY;            ///< The Y position of this object
    q24_8 radius;          ///< The radius of this object
    rayMapCellType_t type; ///< The object's type
    int32_t id;            ///< This object's ID
} rayObjCommon_t;

typedef struct
{
    rayObjCommon_t c; ///< Common object properties
    q24_8 velX;       ///< The X velocity of this bullet
    q24_8 velY;       ///< The Y velocity of this bullet
} rayBullet_t;

typedef struct
{
    rayObjCommon_t c;        ///< Common object properties
    uint8_t walkSprites[4];  ///< The walking sprites for this enemy
    uint8_t shootSprites[4]; ///< The shooting sprites for this enemy
    uint8_t hurtSprites[4];  ///< The getting shot sprites for this enemy
    // TODO enemy state and stuff
} rayEnemy_t;

typedef struct
{
    rayObjCommon_t c; ///< Common object properties
} rayScenery_t;

/** @brief The time to swap out and swap in a gun, in microseconds */
#define LOADOUT_TIMER_US (1 << 18)

/**
 * @brief All the possible loadouts
 */
typedef enum
{
    LO_NORMAL,   ///< Normal loadout
    LO_MISSILE,  ///< Missile loadout
    LO_ICE,      ///< Ice beam loadout
    LO_XRAY,     ///< X-Ray loadout
    NUM_LOADOUTS ///< The number of loadouts
} rayLoadout_t;

typedef struct
{
    rayMap_t map;

    rayBullet_t bullets[MAX_RAY_BULLETS];
    list_t enemies;
    list_t scenery;

    q24_8 posX;
    q24_8 posY;
    q24_8 dirX;
    q24_8 dirY;
    q24_8 planeX;
    q24_8 planeY;
    q24_8 dirAngle;
    q24_8 posZ;
    q24_8 wallDistBuffer[TFT_WIDTH];

    int32_t bobTimer;
    int32_t bobCount;

    uint32_t btnState;

    int32_t doorTimer;

    bool isStrafing;

    rayLoadout_t loadout;
    rayLoadout_t nextLoadout;
    int32_t loadoutChangeTimer;

    namedTexture_t* loadedTextures;
    uint8_t* typeToIdxMap;
    wsg_t guns[NUM_LOADOUTS];

    rayEnemy_t eTemplates[6]; // Six enemy types
} ray_t;

extern swadgeMode_t rayMode;

#endif