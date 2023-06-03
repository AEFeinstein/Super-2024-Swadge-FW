#ifndef _MODE_RAY_H_
#define _MODE_RAY_H_

#include "swadge2024.h"

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
    OBJ_DELETE         = 12, // Should be last
} rayMapCellType_t;

typedef struct
{
    uint16_t w;
    uint16_t h;
    rayMapCellType_t** tiles;
    // TODO objects
    // TODO rules
} rayMap_t;

extern swadgeMode_t rayMode;

#endif