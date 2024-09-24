//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include "entity_bigbug.h"
#include "entityManager_bigbug.h"
#include "gameData_bigbug.h"
#include "soundFuncs.h"
#include "hdw-btn.h"
#include "esp_random.h"
#include "aabb_utils_bigbug.h"
#include "trigonometry.h"
#include <esp_log.h>

//==============================================================================
// Constants
//==============================================================================
// #define SIGNOF(x) ((x > 0) - (x < 0))

//==============================================================================
// Functions
//==============================================================================
void bb_initializeEntity(bb_entity_t* self, bb_entityManager_t* entityManager, bb_gameData_t* gameData,
                         bb_soundManager_t* soundManager)
{
    self->active               = false;
    self->gameData             = gameData;
    self->soundManager         = soundManager;
    self->entityManager        = entityManager;
}

void bb_destroyEntity(bb_entity_t* self, bool respawn)
{
    if(self->data != NULL){
        free(self->data);
    }
    self->entityManager->activeEntities--;
    self->active = false;
}

void bb_updateGarbotnikFlying(bb_entity_t* self, bb_gameData_t* gameData){
    
    vec_t accel = {.x = 0,
                   .y = 0};

    // Update garbotnik's velocity if a button is currently down
    switch (gameData->btnState)
    {
        // up
        case 0b0001:
            accel.y = -50;
            break;
        case 0b1101:
            accel.y = -50;
            break;

        // down
        case 0b0010:
            accel.y = 50;
            break;
        case 0b1110:
            accel.y = 50;
            break;

        // left
        case 0b0100:
            accel.x = -50;
            break;
        case 0b0111:
            accel.x = -50;
            break;

        // right
        case 0b1000:
            accel.x = 50;
            break;
        case 0b1011:
            accel.x = 50;
            break;

        // up,left
        case 0b0101:
            accel.x = -35; // magnitude is sqrt(1/2) * 100000
            accel.y = -35;
            break;

        // up,right
        case 0b1001:
            accel.x = 35; // 35 707 7035
            accel.y = -35;
            break;

        // down,right
        case 0b1010:
            accel.x = 35;
            accel.y = 35;
            break;

        // down,left
        case 0b0110:
            accel.x = -35;
            accel.y = 35;
            break;
        default:
            break;
    }

    // printf("accel x: %d\n", accel.x);
    // printf("elapsed: %d", (int32_t) elapsedUs);
    // printf("offender: %d\n", (int32_t) elapsedUs / 100000);
    // printf("now   x: %d\n", mulVec2d(accel, elapsedUs) / 100000).x);

    ((bb_garbotnikData*)self->data)->accel = divVec2d(mulVec2d(accel, gameData->elapsedUs), 100000);
}