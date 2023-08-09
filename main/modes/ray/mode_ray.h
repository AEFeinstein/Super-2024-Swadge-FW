#ifndef _MODE_RAY_H_
#define _MODE_RAY_H_

#include "swadge2024.h"
#include "fp_math.h"

#define MAX_RAY_OBJS 256

typedef enum __attribute__((packed))
{
    EMPTY              = 0,
    BG_FLOOR           = 1,
    BG_WALL            = 2,
    BG_CEILING         = 3,
    BG_DOOR            = 4,
    OBJ_START_POINT    = 5,
    OBJ_ENEMY_DRAGON   = 6,
    OBJ_ENEMY_SKELETON = 7,
    OBJ_ENEMY_KNIGHT   = 8,
    OBJ_ENEMY_GOLEM    = 9,
    OBJ_OBELISK        = 10,
    OBJ_GUN            = 11,
    OBJ_BULLET         = 12,
    OBJ_DELETE         = 13, // Should be last
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
    wsg_t texFloor;
    wsg_t texWall;
    wsg_t texCeiling;
    wsg_t texDoor;

    wsg_t texDragon;
    wsg_t texGolem;
    wsg_t texKnight;
    wsg_t texSkeleton;
    wsg_t texBullet;

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