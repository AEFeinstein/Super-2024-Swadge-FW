#ifndef _MG_ENTITYSPAWNDATA_H_
#define _MG_ENTITYSPAWNDATA_H_

//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include <stdbool.h>
#include <mega_pulse_ex_typedef.h>
#include "mgEntity.h"

//==============================================================================
// Constants
//==============================================================================

//==============================================================================
// Enums
//==============================================================================

//==============================================================================
// Structs
//==============================================================================

struct mgEntitySpawnData_t {
    bool spawnable;
    bool respawnable;
    uint8_t type;

    uint8_t tx;
    uint8_t ty;
    uint8_t xOffsetInPixels;
    uint8_t yOffsetInPixels;
    uint8_t flags;
    uint8_t special0;
    uint8_t special1;
    uint8_t special2;
    uint8_t special3;
    uint8_t special4;
    uint8_t special5;
    uint8_t special6;
    uint8_t special7;
    mgEntitySpawnData_t* linkedEntitySpawn;
    mgEntity_t* spawnedEntity;
};

#endif