//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include <esp_log.h>
#include <string.h>

#include "paEntityManager.h"
#include "esp_random.h"
#include "palette.h"
#include "soundFuncs.h"

#include "cnfs.h"
#include "fs_wsg.h"

//==============================================================================
// Constants
//==============================================================================
#define SUBPIXEL_RESOLUTION 4
#define PA_TO_TILECOORDS(x) ((x) >> PA_TILE_SIZE_IN_POWERS_OF_2)

//==============================================================================
// Functions
//==============================================================================
void pa_initializeEntityManager(paEntityManager_t* entityManager, paWsgManager_t* wsgManager, paTilemap_t* tilemap,
                                paGameData_t* gameData, paSoundManager_t* soundManager)
{
    entityManager->wsgManager = wsgManager;
    entityManager->entities   = calloc(MAX_ENTITIES, sizeof(paEntity_t));

    for (uint8_t i = 0; i < MAX_ENTITIES; i++)
    {
        pa_initializeEntity(&(entityManager->entities[i]), entityManager, tilemap, gameData, soundManager);
    }

    entityManager->activeEntities = 0;
    entityManager->tilemap        = tilemap;
    entityManager->gameData       = gameData;
    entityManager->soundManager   = soundManager;

    // entityManager->viewEntity = pa_createPlayer(entityManager, entityManager->tilemap->warps[0].x * 16,
    // entityManager->tilemap->warps[0].y * 16);
    entityManager->playerEntity = entityManager->viewEntity;

    // entityManager->activeEnemies = 0;
    // entityManager->maxEnemies = 3;
}

void pa_updateEntities(paEntityManager_t* entityManager)
{
    for (uint8_t i = 0; i < MAX_ENTITIES; i++)
    {
        if (entityManager->entities[i].active)
        {
            entityManager->entities[i].updateFunction(&(entityManager->entities[i]));

            /*if (&(entityManager->entities[i]) == entityManager->viewEntity)
            {
                pa_viewFollowEntity(entityManager->tilemap, &(entityManager->entities[i]));
            }*/
        }
    }
}

void pa_deactivateAllEntities(paEntityManager_t* entityManager, bool excludePlayer)
{
    for (uint8_t i = 0; i < MAX_ENTITIES; i++)
    {
        paEntity_t* currentEntity = &(entityManager->entities[i]);
        currentEntity->active     = false;

        // clear out invisible block tiles that are placed for every Break Block object
        // if(currentEntity->type == PA_ENTITY_BREAK_BLOCK){
        //     pa_setTile(currentEntity->tilemap, PA_TO_TILECOORDS(currentEntity->x >> SUBPIXEL_RESOLUTION),
        //     PA_TO_TILECOORDS(currentEntity->y >> SUBPIXEL_RESOLUTION), PA_TILE_EMPTY);
        // }

        if (currentEntity->type == ENTITY_HIT_BLOCK && currentEntity->jumpPower == PA_TILE_SPAWN_BLOCK_0)
        {
            entityManager->gameData->remainingEnemies--;
        }

        if (excludePlayer && currentEntity == entityManager->playerEntity)
        {
            currentEntity->active = true;
        }
    }

    entityManager->activeEntities = 0;
    entityManager->aggroEnemies   = 0;
}

void pa_drawEntities(paEntityManager_t* entityManager)
{
    for (uint8_t i = 0; i < MAX_ENTITIES; i++)
    {
        paEntity_t currentEntity = entityManager->entities[i];

        if (currentEntity.active && currentEntity.visible)
        {
            drawWsg(entityManager->wsgManager->sprites[currentEntity.spriteIndex].wsg,
                    (currentEntity.x >> SUBPIXEL_RESOLUTION)
                        - entityManager->wsgManager->sprites[currentEntity.spriteIndex].originX
                        - entityManager->tilemap->mapOffsetX,
                    (currentEntity.y >> SUBPIXEL_RESOLUTION) - entityManager->tilemap->mapOffsetY
                        - entityManager->wsgManager->sprites[currentEntity.spriteIndex].originY,
                    currentEntity.spriteFlipHorizontal, currentEntity.spriteFlipVertical, 0);
        }
    }
}

paEntity_t* pa_findInactiveEntity(paEntityManager_t* entityManager)
{
    if (entityManager->activeEntities == MAX_ENTITIES)
    {
        return NULL;
    };

    uint8_t entityIndex = 0;

    while (entityManager->entities[entityIndex].active)
    {
        entityIndex++;

        // Extra safeguard to make sure we don't get stuck here
        if (entityIndex > MAX_ENTITIES)
        {
            return NULL;
        }
    }

    return &(entityManager->entities[entityIndex]);
}

void pa_viewFollowEntity(paTilemap_t* tilemap, paEntity_t* entity)
{
    int16_t moveViewByX = (entity->x) >> SUBPIXEL_RESOLUTION;
    int16_t moveViewByY = (entity->y > 63616) ? 0 : (entity->y) >> SUBPIXEL_RESOLUTION;

    int16_t centerOfViewX = tilemap->mapOffsetX + 140;
    int16_t centerOfViewY = tilemap->mapOffsetY + 120;

    // if(centerOfViewX != moveViewByX) {
    moveViewByX -= centerOfViewX;
    //}

    // if(centerOfViewY != moveViewByY) {
    moveViewByY -= centerOfViewY;
    //}

    // if(moveViewByX && moveViewByY){
    pa_scrollTileMap(tilemap, moveViewByX, moveViewByY);
    //}
}

paEntity_t* pa_createEntity(paEntityManager_t* entityManager, uint8_t objectIndex, uint16_t x, uint16_t y)
{
    // if(entityManager->activeEntities == MAX_ENTITIES){
    //     return NULL;
    // }

    paEntity_t* createdEntity;

    switch (objectIndex)
    {
        case ENTITY_PLAYER:
            createdEntity = pa_createPlayer(entityManager, x, y);
            break;
        case PA_ENTITY_CRABDOZER:
            createdEntity = createCrabdozer(entityManager, x, y);
            break;
        case ENTITY_HIT_BLOCK:
            createdEntity = createHitBlock(entityManager, x, y);
            break;
        default:
            createdEntity = NULL;
    }

    // if(createdEntity != NULL) {
    //     entityManager->activeEntities++;
    // }

    return createdEntity;
}

paEntity_t* pa_createPlayer(paEntityManager_t* entityManager, uint16_t x, uint16_t y)
{
    paEntity_t* entity = pa_findInactiveEntity(entityManager);

    if (entity == NULL)
    {
        return NULL;
    }

    entity->active  = true;
    entity->visible = true;
    entity->x       = x << SUBPIXEL_RESOLUTION;
    entity->y       = y << SUBPIXEL_RESOLUTION;

    entity->xspeed             = 0;
    entity->yspeed             = 0;
    entity->xMaxSpeed          = 40; // 72; Walking
    entity->yMaxSpeed          = 64; // 72;
    entity->xDamping           = 2;
    entity->yDamping           = 2;
    entity->gravityEnabled     = false;
    entity->gravity            = 4;
    entity->falling            = false;
    entity->jumpPower          = 0;
    entity->spriteFlipVertical = false;
    entity->hp                 = 1;
    entity->animationTimer     = 0; // Used as a cooldown for shooting square wave balls
    entity->state              = PA_PL_ST_NORMAL;
    entity->stateTimer         = -1;

    entity->type                 = ENTITY_PLAYER;
    entity->spriteIndex          = PA_SP_PLAYER_SOUTH;
    entity->updateFunction       = &pa_updatePlayer;
    entity->collisionHandler     = &pa_playerCollisionHandler;
    entity->tileCollisionHandler = &pa_playerTileCollisionHandler;
    entity->fallOffTileHandler   = &defaultFallOffTileHandler;
    entity->overlapTileHandler   = &pa_playerOverlapTileHandler;
    return entity;
}

paEntity_t* createCrabdozer(paEntityManager_t* entityManager, uint16_t x, uint16_t y)
{
    paEntity_t* entity = pa_findInactiveEntity(entityManager);

    if (entity == NULL)
    {
        return NULL;
    }

    entity->active  = true;
    entity->visible = true;
    entity->x       = x << SUBPIXEL_RESOLUTION;
    entity->y       = y << SUBPIXEL_RESOLUTION;

    entity->xspeed               = 0;
    entity->yspeed               = 0;
    entity->xMaxSpeed            = 132;
    entity->yMaxSpeed            = 132;
    entity->gravityEnabled       = false;
    entity->gravity              = 0;
    entity->spriteFlipHorizontal = false;
    entity->spriteFlipVertical   = false;
    entity->scoreValue           = 100;
    entity->stateTimer           = -1;
    entity->tempStateTimer       = -1;
    entity->stateFlag            = false;
    entity->baseSpeed            = entityManager->gameData->enemyInitialSpeed;

    entity->type                 = PA_ENTITY_CRABDOZER;
    entity->spriteIndex          = PA_SP_ENEMY_SOUTH;
    entity->facingDirection      = PA_DIRECTION_NONE;
    entity->state                = PA_EN_ST_NORMAL;
    entity->stateTimer           = 300 + (esp_random() % 600); // Min 5 seconds, max 15 seconds
    entity->updateFunction       = &updateCrabdozer;
    entity->collisionHandler     = &pa_enemyCollisionHandler;
    entity->tileCollisionHandler = &pa_enemyTileCollisionHandler;
    entity->fallOffTileHandler   = &defaultFallOffTileHandler;
    entity->overlapTileHandler   = &pa_defaultOverlapTileHandler;

    return entity;
}

paEntity_t* pa_createBreakBlock(paEntityManager_t* entityManager, uint16_t x, uint16_t y)
{
    paEntity_t* entity = pa_findInactiveEntity(entityManager);

    if (entity == NULL)
    {
        return NULL;
    }

    entity->active  = true;
    entity->visible = true;
    entity->x       = x << SUBPIXEL_RESOLUTION;
    entity->y       = y << SUBPIXEL_RESOLUTION;

    entity->xspeed               = 0;
    entity->yspeed               = 0;
    entity->xMaxSpeed            = 132;
    entity->yMaxSpeed            = 132;
    entity->gravityEnabled       = false;
    entity->gravity              = 0;
    entity->spriteFlipHorizontal = false;
    entity->spriteFlipVertical   = false;
    entity->scoreValue           = 100;
    entity->animationTimer       = 0;
    entity->type                 = PA_ENTITY_BREAK_BLOCK;
    entity->spriteIndex          = PA_SP_BREAK_BLOCK;
    entity->facingDirection      = PA_DIRECTION_NONE;
    entity->updateFunction       = &pa_updateBreakBlock;
    entity->collisionHandler     = &pa_dummyCollisionHandler;
    entity->tileCollisionHandler = &pa_dummyTileCollisionHandler;
    entity->fallOffTileHandler   = &defaultFallOffTileHandler;
    entity->overlapTileHandler   = &pa_defaultOverlapTileHandler;

    pa_setTile(entityManager->tilemap, PA_TO_TILECOORDS(x), PA_TO_TILECOORDS(y), PA_TILE_EMPTY);

    return entity;
}

paEntity_t* pa_createBlockFragment(paEntityManager_t* entityManager, uint16_t x, uint16_t y)
{
    paEntity_t* entity = pa_findInactiveEntity(entityManager);

    if (entity == NULL)
    {
        return NULL;
    }

    entity->active  = true;
    entity->visible = true;
    entity->x       = x << SUBPIXEL_RESOLUTION;
    entity->y       = y << SUBPIXEL_RESOLUTION;

    entity->xspeed               = -64 + (esp_random() % 128);
    entity->yspeed               = -64 + (esp_random() % 128);
    entity->xMaxSpeed            = 132;
    entity->yMaxSpeed            = 132;
    entity->gravityEnabled       = true;
    entity->gravity              = 0;
    entity->spriteFlipHorizontal = false;
    entity->spriteFlipVertical   = false;
    entity->scoreValue           = 100;
    entity->animationTimer       = 0;
    entity->type                 = PA_ENTITY_BLOCK_FRAGMENT;
    entity->spriteIndex          = PA_SP_BLOCK_FRAGMENT;
    entity->facingDirection      = PA_DIRECTION_NONE;
    entity->updateFunction       = &pa_updateBlockFragment;
    entity->collisionHandler     = &pa_dummyCollisionHandler;
    entity->tileCollisionHandler = &pa_dummyTileCollisionHandler;
    entity->fallOffTileHandler   = &defaultFallOffTileHandler;
    entity->overlapTileHandler   = &pa_defaultOverlapTileHandler;

    return entity;
}

paEntity_t* createHitBlock(paEntityManager_t* entityManager, uint16_t x, uint16_t y)
{
    paEntity_t* entity = pa_findInactiveEntity(entityManager);

    if (entity == NULL)
    {
        return NULL;
    }

    entity->active    = true;
    entity->visible   = true;
    entity->x         = x << SUBPIXEL_RESOLUTION;
    entity->y         = y << SUBPIXEL_RESOLUTION;
    entity->homeTileX = PA_TO_TILECOORDS(x);
    entity->homeTileY = PA_TO_TILECOORDS(y);

    entity->xspeed         = 0;
    entity->yspeed         = 0;
    entity->yDamping       = 0;
    entity->xMaxSpeed      = 132;
    entity->yMaxSpeed      = 132;
    entity->gravityEnabled = true;
    entity->gravity        = 4;

    entity->spriteFlipHorizontal = false;
    entity->spriteFlipVertical   = false;

    entity->type                 = ENTITY_HIT_BLOCK;
    entity->spriteIndex          = PA_SP_BLOCK;
    entity->animationTimer       = 0;
    entity->updateFunction       = &updateHitBlock;
    entity->collisionHandler     = &pa_dummyCollisionHandler;
    entity->tileCollisionHandler = &pa_hitBlockTileCollisionHandler;
    entity->overlapTileHandler   = &pa_defaultOverlapTileHandler;

    return entity;
}

void pa_freeEntityManager(paEntityManager_t* self)
{
    free(self->entities);
}

paEntity_t* pa_spawnEnemyFromSpawnBlock(paEntityManager_t* entityManager)
{
    paEntity_t* newEnemy = NULL;

    if (entityManager->gameData->remainingEnemies > 0
        && entityManager->activeEnemies < entityManager->gameData->maxActiveEnemies)
    {
        uint16_t iterations = 0;
        while (newEnemy == NULL && iterations < 2)
        {
            for (uint16_t ty = 1; ty < 14; ty++)
            {
                for (uint16_t tx = 1; tx < 16; tx++)
                {
                    uint8_t t = pa_getTile(entityManager->tilemap, tx, ty);

                    if (t == PA_TILE_SPAWN_BLOCK_0
                        && (iterations > 0 || !(esp_random() % entityManager->gameData->remainingEnemies)))
                    {
                        newEnemy
                            = createCrabdozer(entityManager, (tx << PA_TILE_SIZE_IN_POWERS_OF_2) + PA_HALF_TILE_SIZE,
                                              (ty << PA_TILE_SIZE_IN_POWERS_OF_2) + PA_HALF_TILE_SIZE);

                        if (newEnemy != NULL)
                        {
                            // pa_setTile(entityManager->tilemap, tx, ty, PA_TILE_EMPTY);
                            newEnemy->state      = PA_EN_ST_STUN;
                            newEnemy->stateTimer = 120;
                            /*if(entityManager->activeEnemies == 0 || entityManager->gameData->remainingEnemies == 1){
                                //The first and last enemies are permanently angry
                                newEnemy->stateFlag = true;
                            }*/

                            paEntity_t* newBreakBlock = pa_createBreakBlock(
                                entityManager, (tx << PA_TILE_SIZE_IN_POWERS_OF_2) + PA_HALF_TILE_SIZE,
                                (ty << PA_TILE_SIZE_IN_POWERS_OF_2) + PA_HALF_TILE_SIZE);
                            if (newBreakBlock != NULL)
                            {
                                soundPlaySfx(&(entityManager->soundManager->sndSpawn), 3);
                            }

                            entityManager->activeEnemies++;
                            entityManager->gameData->remainingEnemies--;
                            return newEnemy;
                        }
                    }
                }
            }
            iterations++;
        }
    }

    return newEnemy;
}