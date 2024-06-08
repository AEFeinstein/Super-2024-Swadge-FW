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
#define SIGNOF(x) ((x > 0) - (x < 0))

//==============================================================================
// Functions
//==============================================================================
void bb_initializeEntity(bb_entity_t* self, bb_entityManager_t* entityManager, bb_gameData_t* gameData,
                      bb_soundManager_t* soundManager)
{
    self->active                           = false;
    self->persistent                       = false;
    self->gameData                         = gameData;
    self->soundManager                     = soundManager;
    self->homeTileX                        = 0;
    self->homeTileY                        = 0;
    self->entityManager                    = entityManager;
    self->spriteFlipHorizontal             = false;
    self->spriteFlipVertical               = false;
    self->spriteRotateAngle                = 0;
    self->attachedToEntity                 = NULL;
    self->shouldAdvanceMultiplier          = false;
    self->baseSpeed                        = 0;
    self->bouncesToNextSpeedUp             = 5;
    self->speedUpLookupIndex               = 0;
    self->maxSpeed                         = 63;
    self->bouncesOffUnbreakableBlocks      = 0;
    self->breakInfiniteLoopBounceThreshold = -1;
}

void bb_destroyEntity(bb_entity_t* self, bool respawn)
{
    self->attachedToEntity = NULL;
    self->entityManager->activeEntities--;
    self->active = false;
}