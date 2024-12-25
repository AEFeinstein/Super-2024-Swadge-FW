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

    entityManager->playerEntity = entityManager->viewEntity;
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

        if (currentEntity->active == false || (excludePlayer && currentEntity == entityManager->playerEntity))
        {
            continue;
        }

        currentEntity->active = false;

        if (currentEntity->type == ENTITY_HIT_BLOCK)
        {
            entityManager->gameData->remainingBlocks--;

            if (currentEntity->state == PA_TILE_SPAWN_BLOCK_0)
            {
                entityManager->gameData->remainingEnemies--;
            }
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
            currentEntity.drawHandler(&currentEntity);
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

    moveViewByX -= centerOfViewX;
    moveViewByY -= centerOfViewY;
    pa_scrollTileMap(tilemap, moveViewByX, moveViewByY);
}

paEntity_t* pa_createEntity(paEntityManager_t* entityManager, uint8_t objectIndex, uint16_t x, uint16_t y)
{
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
    entity->xMaxSpeed          = 40;
    entity->yMaxSpeed          = 64;
    entity->xDamping           = 2;
    entity->yDamping           = 2;
    entity->gravityEnabled     = false;
    entity->gravity            = 4;
    entity->falling            = false;
    entity->spriteFlipVertical = false;
    entity->animationTimer     = 0;
    entity->state              = PA_PL_ST_NORMAL;
    entity->stateTimer         = -1;

    entity->type                 = ENTITY_PLAYER;
    entity->spriteIndex          = PA_SP_PLAYER_SOUTH;
    entity->updateFunction       = &pa_updatePlayer;
    entity->collisionHandler     = &pa_playerCollisionHandler;
    entity->tileCollisionHandler = &pa_playerTileCollisionHandler;
    entity->overlapTileHandler   = &pa_playerOverlapTileHandler;
    entity->drawHandler          = &pa_defaultEntityDrawHandler;
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
    entity->overlapTileHandler   = &pa_defaultOverlapTileHandler;
    entity->drawHandler          = &pa_defaultEntityDrawHandler;

    return entity;
}

paEntity_t* pa_createBonusItem(paEntityManager_t* entityManager, uint16_t x, uint16_t y)
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
    entity->scoreValue           = (entityManager->gameData->firstBonusItemDispensed) ? 2000 : 1000;
    entity->stateTimer           = -1;
    entity->tempStateTimer       = -1;
    entity->stateFlag            = false;
    entity->baseSpeed            = entityManager->gameData->enemyInitialSpeed;

    entity->type                 = PA_ENTITY_BONUS_ITEM;
    entity->spriteIndex          = PA_SP_HOTDOG;
    entity->facingDirection      = PA_DIRECTION_NONE;
    entity->state                = 0;
    entity->stateTimer           = 0;
    entity->updateFunction       = &pa_updateBonusItem;
    entity->collisionHandler     = &pa_dummyCollisionHandler;
    entity->tileCollisionHandler = &pa_dummyTileCollisionHandler;
    entity->overlapTileHandler   = &pa_defaultOverlapTileHandler;
    entity->drawHandler          = &pa_defaultEntityDrawHandler;
    entity->targetTileX          = 1 + (esp_random() % 14);
    entity->targetTileY          = 1 + (esp_random() % 13);

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
    entity->scoreValue           = 0;
    entity->animationTimer       = 0;
    entity->type                 = PA_ENTITY_BREAK_BLOCK;
    entity->spriteIndex          = PA_SP_BREAK_BLOCK;
    entity->facingDirection      = PA_DIRECTION_NONE;
    entity->updateFunction       = &pa_updateBreakBlock;
    entity->collisionHandler     = &pa_dummyCollisionHandler;
    entity->tileCollisionHandler = &pa_dummyTileCollisionHandler;
    entity->overlapTileHandler   = &pa_defaultOverlapTileHandler;
    entity->drawHandler          = &pa_defaultEntityDrawHandler;
    entity->state                = 0;

    uint8_t tile = pa_getTile(entityManager->tilemap, PA_TO_TILECOORDS(x), PA_TO_TILECOORDS(y));

    if (tile == PA_TILE_BLOCK || tile == PA_TILE_SPAWN_BLOCK_0)
    {
        pa_setTile(entityManager->tilemap, PA_TO_TILECOORDS(x), PA_TO_TILECOORDS(y), PA_TILE_EMPTY);
        entityManager->gameData->remainingBlocks--;
    }

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
    entity->gravity              = 4;
    entity->spriteFlipHorizontal = esp_random() % 2;
    entity->spriteFlipVertical   = esp_random() % 2;
    entity->scoreValue           = 100;
    entity->animationTimer       = 0;
    entity->type                 = PA_ENTITY_BLOCK_FRAGMENT;
    entity->spriteIndex          = PA_SP_BLOCK_FRAGMENT;
    entity->facingDirection      = PA_DIRECTION_NONE;
    entity->updateFunction       = &pa_updateBlockFragment;
    entity->collisionHandler     = &pa_dummyCollisionHandler;
    entity->tileCollisionHandler = &pa_dummyTileCollisionHandler;
    entity->overlapTileHandler   = &pa_defaultOverlapTileHandler;
    entity->drawHandler          = &pa_defaultEntityDrawHandler;
    entity->stateTimer           = 8;

    return entity;
}

paEntity_t* pa_createHotDog(paEntityManager_t* entityManager, uint16_t x, uint16_t y)
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

    entity->xspeed               = -16 + (esp_random() % 32);
    entity->yspeed               = -16 + (esp_random() % 32);
    entity->xMaxSpeed            = 132;
    entity->yMaxSpeed            = 132;
    entity->gravityEnabled       = true;
    entity->gravity              = 4;
    entity->spriteFlipHorizontal = esp_random() % 2;
    entity->spriteFlipVertical   = esp_random() % 2;
    entity->scoreValue           = 100;
    entity->animationTimer       = 0;
    entity->type                 = PA_ENTITY_HOTDOG;
    entity->spriteIndex          = PA_SP_HOTDOG;
    entity->facingDirection      = PA_DIRECTION_NONE;
    entity->updateFunction       = &pa_updateBlockFragment;
    entity->collisionHandler     = &pa_dummyCollisionHandler;
    entity->tileCollisionHandler = &pa_dummyTileCollisionHandler;
    entity->overlapTileHandler   = &pa_defaultOverlapTileHandler;
    entity->drawHandler          = &pa_defaultEntityDrawHandler;
    entity->stateTimer           = 32;

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
    entity->scoreValue           = 0;
    entity->updateFunction       = &updateHitBlock;
    entity->collisionHandler     = &pa_dummyCollisionHandler;
    entity->tileCollisionHandler = &pa_hitBlockTileCollisionHandler;
    entity->overlapTileHandler   = &pa_defaultOverlapTileHandler;
    entity->drawHandler          = &pa_defaultEntityDrawHandler;

    return entity;
}

paEntity_t* pa_createScoreDisplay(paEntityManager_t* entityManager, uint16_t x, uint16_t y)
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

    entity->xspeed         = 0;
    entity->yspeed         = 0;
    entity->gravityEnabled = false;
    entity->gravity        = 0;

    entity->spriteFlipHorizontal = false;
    entity->spriteFlipVertical   = false;

    entity->type                 = PA_ENTITY_SCORE_DISPLAY;
    entity->spriteIndex          = 0;
    entity->animationTimer       = 0;
    entity->scoreValue           = 0;
    entity->stateTimer           = 0;
    entity->updateFunction       = &pa_updateScoreDisplay;
    entity->collisionHandler     = &pa_dummyCollisionHandler;
    entity->tileCollisionHandler = &pa_hitBlockTileCollisionHandler;
    entity->overlapTileHandler   = &pa_defaultOverlapTileHandler;
    entity->drawHandler          = &pa_scoreDisplayDrawHandler;

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
                            newEnemy->state      = PA_EN_ST_STUN;
                            newEnemy->stateTimer = 120;

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