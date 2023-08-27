#ifndef _MODE_RAY_H_
#define _MODE_RAY_H_

#include "swadge2024.h"
#include "fp_math.h"

#define MAX_RAY_OBJS 256

typedef enum __attribute__((packed))
{
    EMPTY                   = 0,
    BG_FLOOR                = 1,
    BG_FLOOR_WATER          = 2,
    BG_FLOOR_LAVA           = 3,
    BG_WALL_1               = 4,
    BG_WALL_2               = 5,
    BG_WALL_3               = 6,
    BG_DOOR                 = 7,
    BG_DOOR_CHARGE          = 8,
    BG_DOOR_MISSILE         = 9,
    BG_DOOR_ICE             = 10,
    BG_DOOR_XRAY            = 11,
    OBJ_START_POINT         = 12,
    OBJ_ENEMY_BEAM          = 13,
    OBJ_ENEMY_CHARGE        = 14,
    OBJ_ENEMY_MISSILE       = 15,
    OBJ_ENEMY_ICE           = 16,
    OBJ_ENEMY_XRAY          = 17,
    OBJ_ITEM_BEAM           = 18,
    OBJ_ITEM_CHARGE_BEAM    = 19,
    OBJ_ITEM_MISSILE        = 20,
    OBJ_ITEM_ICE            = 21,
    OBJ_ITEM_XRAY           = 22,
    OBJ_ITEM_SUIT_WATER     = 23,
    OBJ_ITEM_SUIT_LAVA      = 24,
    OBJ_ITEM_ENERGY_TANK    = 25,
    OBJ_ITEM_KEY            = 26,
    OBJ_ITEM_ARTIFACT       = 27,
    OBJ_ITEM_PICKUP_ENERGY  = 28,
    OBJ_ITEM_PICKUP_MISSILE = 29,
    OBJ_SCENERY_TERMINAL    = 30,
    OBJ_DELETE              = 31, // Should be last
    // This and later values do not exist in the map editor
    BULLET_NORMAL,
    BULLET_CHARGE,
    BULLET_ICE,
    BULLET_MISSILE,
    BULLET_XRAY,
    BG_CEILING,
    NUM_RAY_MAP_CELL_TYPES,
} rayMapCellType_t;

typedef struct
{
    rayMapCellType_t type;
    q24_8 doorOpen;
} rayMapCell_t;

typedef struct
{
    uint16_t w;
    uint16_t h;
    rayMapCell_t** tiles;
    // TODO objects
    // TODO rules
} rayMap_t;

typedef struct
{
    wsg_t* sprite;         ///< The sprite for this object
    int32_t dist;          ///< The distance between the player and this object, used for sorting before casting
    q24_8 posX;            ///< The X position of this object
    q24_8 posY;            ///< The Y position of this object
    q24_8 velX;            ///< The X velocity of this object
    q24_8 velY;            ///< The Y velocity of this object
    q24_8 radius;          ///< The radius of this object
    rayMapCellType_t type; ///< The object's type
    int16_t id;            ///< This object's ID
} rayObj_t;

typedef struct
{
    wsg_t textures[NUM_RAY_MAP_CELL_TYPES];

    rayMap_t map;
    rayObj_t objs[MAX_RAY_OBJS];

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
    int16_t bobCount;

    uint16_t btnState;

    int32_t doorTimer;

    bool isStrafing;
} ray_t;

extern swadgeMode_t rayMode;

wsg_t* getTexture(rayMapCellType_t type);

#endif