//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include <esp_log.h>
#include <string.h>

#include <esp_heap_caps.h>

#include "mgEntityManager.h"
#include "esp_random.h"
#include "palette.h"

#include "cnfs.h"
#include "fs_wsg.h"
#include "mega_pulse_ex_typedef.h"

//==============================================================================
// Constants
//==============================================================================

//==============================================================================
// Functions
//==============================================================================
void mg_initializeEntityManager(mgEntityManager_t* entityManager, mgWsgManager_t* wsgManager, mgTilemap_t* tilemap, mgGameData_t* gameData,
                                mgSoundManager_t* soundManager)
{
    entityManager->wsgManager = wsgManager;
    entityManager->entities = heap_caps_calloc(MAX_ENTITIES, sizeof(mgEntity_t), MALLOC_CAP_8BIT);

    for (uint8_t i = 0; i < MAX_ENTITIES; i++)
    {
        mg_initializeEntity(&(entityManager->entities[i]), entityManager, tilemap, gameData, soundManager);
    }

    entityManager->activeEntities = 0;
    entityManager->tilemap        = tilemap;

    // entityManager->viewEntity = mg_createPlayer(entityManager, entityManager->tilemap->warps[0].x * 16,
    // entityManager->tilemap->warps[0].y * 16);
    entityManager->playerEntity = entityManager->viewEntity;
}

void mg_updateEntities(mgEntityManager_t* entityManager)
{
    for (uint8_t i = 0; i < MAX_ENTITIES; i++)
    {
        if (entityManager->entities[i].active)
        {
            entityManager->entities[i].updateFunction(&(entityManager->entities[i]));

            if (&(entityManager->entities[i]) == entityManager->viewEntity)
            {
                mg_viewFollowEntity(entityManager->tilemap, &(entityManager->entities[i]));
            }
        }
    }
}

void mg_deactivateAllEntities(mgEntityManager_t* entityManager, bool excludePlayer)
{
    for (uint8_t i = 0; i < MAX_ENTITIES; i++)
    {
        mgEntity_t* currentEntity = &(entityManager->entities[i]);

        currentEntity->active = false;

        if (currentEntity->spawnData != NULL)
        {
            currentEntity->spawnData->spawnedEntity = NULL;
            currentEntity->spawnData->spawnable = currentEntity->spawnData->respawnable;
        }

        if (excludePlayer && currentEntity == entityManager->playerEntity)
        {
            currentEntity->active = true;
        }
    }
}

void mg_drawEntities(mgEntityManager_t* entityManager)
{
    for (uint8_t i = 0; i < MAX_ENTITIES; i++)
    {
        mgEntity_t currentEntity = entityManager->entities[i];

        if (currentEntity.active && currentEntity.visible)
        {
            currentEntity.drawHandler(&currentEntity);
        }
    }
}

mgEntity_t* mg_findInactiveEntity(mgEntityManager_t* entityManager)
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

void mg_viewFollowEntity(mgTilemap_t* tilemap, mgEntity_t* entity)
{
    int16_t moveViewByX = TO_PIXEL_COORDS(entity->x);
    int16_t moveViewByY = (entity->y > 63616) ? 0 : TO_PIXEL_COORDS(entity->y);

    int16_t centerOfViewX = tilemap->mapOffsetX + 140;
    int16_t centerOfViewY = tilemap->mapOffsetY + 120;

    // if(centerOfViewX != moveViewByX) {
    moveViewByX -= centerOfViewX;
    //}

    // if(centerOfViewY != moveViewByY) {
    moveViewByY -= centerOfViewY;
    //}

    // if(moveViewByX && moveViewByY){
    mg_scrollTileMap(tilemap, moveViewByX, moveViewByY);
    //}
}

mgEntity_t* mg_createEntity(mgEntityManager_t* entityManager, uint8_t objectIndex, uint16_t x, uint16_t y)
{
    // if(entityManager->activeEntities == MAX_ENTITIES){
    //     return NULL;
    // }

    mgEntity_t* createdEntity;

    switch (objectIndex)
    {
        case ENTITY_PLAYER:
            createdEntity = mg_createPlayer(entityManager, x, y);
            break;
        case mgEntity_tEST:
            createdEntity = createTestObject(entityManager, x, y);
            break;
        case ENTITY_SCROLL_LOCK_LEFT:
            createdEntity = createScrollLockLeft(entityManager, x, y);
            break;
        case ENTITY_SCROLL_LOCK_RIGHT:
            createdEntity = createScrollLockRight(entityManager, x, y);
            break;
        case ENTITY_SCROLL_LOCK_UP:
            createdEntity = createScrollLockUp(entityManager, x, y);
            break;
        case ENTITY_SCROLL_LOCK_DOWN:
            createdEntity = createScrollLockDown(entityManager, x, y);
            break;
        case ENTITY_SCROLL_UNLOCK:
            createdEntity = createScrollUnlock(entityManager, x, y);
            break;
        case ENTITY_HIT_BLOCK:
            createdEntity = createHitBlock(entityManager, x, y);
            break;
        case ENTITY_POWERUP:
            createdEntity = createPowerUp(entityManager, x, y);
            break;
        case ENTITY_WARP:
            createdEntity = createWarp(entityManager, x, y);
            break;
        case ENTITY_DUST_BUNNY:
            createdEntity = createDustBunny(entityManager, x, y);
            break;
        case ENTITY_WASP:
            createdEntity = createWasp(entityManager, x, y);
            break;
        case ENTITY_BUSH_2:
            createdEntity = createEnemyBushL2(entityManager, x, y);
            break;
        case ENTITY_BUSH_3:
            createdEntity = createEnemyBushL3(entityManager, x, y);
            break;
        case ENTITY_DUST_BUNNY_2:
            createdEntity = createDustBunnyL2(entityManager, x, y);
            break;
        case ENTITY_DUST_BUNNY_3:
            createdEntity = createDustBunnyL3(entityManager, x, y);
            break;
        case ENTITY_WASP_2:
            createdEntity = createWaspL2(entityManager, x, y);
            break;
        case ENTITY_WASP_3:
            createdEntity = createWaspL3(entityManager, x, y);
            break;
        case ENTITY_BGCOL_BLUE:
            createdEntity = createBgColBlue(entityManager, x, y);
            break;
        case ENTITY_BGCOL_YELLOW:
            createdEntity = createBgColYellow(entityManager, x, y);
            break;
        case ENTITY_BGCOL_ORANGE:
            createdEntity = createBgColOrange(entityManager, x, y);
            break;
        case ENTITY_BGCOL_PURPLE:
            createdEntity = createBgColPurple(entityManager, x, y);
            break;
        case ENTITY_BGCOL_DARK_PURPLE:
            createdEntity = createBgColDarkPurple(entityManager, x, y);
            break;
        case ENTITY_BGCOL_BLACK:
            createdEntity = createBgColBlack(entityManager, x, y);
            break;
        case ENTITY_BGCOL_NEUTRAL_GREEN:
            createdEntity = createBgColNeutralGreen(entityManager, x, y);
            break;
        case ENTITY_BGCOL_DARK_RED:
            createdEntity = createBgColNeutralDarkRed(entityManager, x, y);
            break;
        case ENTITY_BGCOL_DARK_GREEN:
            createdEntity = createBgColNeutralDarkGreen(entityManager, x, y);
            break;
        case ENTITY_1UP:
            createdEntity = create1up(entityManager, x, y);
            break;
        case ENTITY_WAVE_BALL:
            createdEntity = createWaveBall(entityManager, x, y);
            break;
        case ENTITY_CHECKPOINT:
            createdEntity = createCheckpoint(entityManager, x, y);
            break;
        case ENTITY_BGM_STOP:
            createdEntity = createBgmStop(entityManager, x, y);
            break;
        case ENTITY_BGM_CHANGE_1:
            createdEntity = createBgmChange1(entityManager, x, y);
            break;
        case ENTITY_BGM_CHANGE_2:
            createdEntity = createBgmChange2(entityManager, x, y);
            break;
        case ENTITY_BGM_CHANGE_3:
            createdEntity = createBgmChange3(entityManager, x, y);
            break;
        case ENTITY_BGM_CHANGE_4:
            createdEntity = createBgmChange4(entityManager, x, y);
            break;
        case ENTITY_BGM_CHANGE_5:
            createdEntity = createBgmChange5(entityManager, x, y);
            break;
        case ENTITY_WARP_ENTRANCE_WALL:
            createdEntity = createWarpEntranceWall(entityManager, x, y);
            break;
        case ENTITY_WARP_ENTRANCE_FLOOR:
            createdEntity = createWarpEntranceFloor(entityManager, x, y);
            break;
        default:
            createdEntity = NULL;
    }

    // if(createdEntity != NULL) {
    //     entityManager->activeEntities++;
    // }

    return createdEntity;
}

mgEntity_t* mg_createPlayer(mgEntityManager_t* entityManager, uint16_t x, uint16_t y)
{
    mgEntity_t* entity = mg_findInactiveEntity(entityManager);

    if (entity == NULL)
    {
        return NULL;
    }

    entity->active  = true;
    entity->visible = true;
    entity->x       = TO_SUBPIXEL_COORDS(x);
    entity->y       = TO_SUBPIXEL_COORDS(y);

    entity->xspeed             = 0;
    entity->yspeed             = 0;
    entity->xMaxSpeed          = 30; // 72; Walking
    entity->yMaxSpeed          = 72; // 72;
    entity->xDamping           = 1; // 1;
    entity->yDamping           = 4;
    entity->gravityEnabled     = true;
    entity->gravity            = 4;
    entity->falling            = true;
    entity->jumpPower          = 0;
    entity->canDash            = true;
    entity->spriteFlipVertical = false;
    entity->hp                 = 1;
    entity->animationTimer     = 0; // Used as a cooldown for shooting square wave balls
    entity->shotsFired         = 0;
    entity->shotLimit          = 3;

    entity->type                 = ENTITY_PLAYER;
    entity->spriteIndex          = MG_SP_PLAYER_IDLE;
    entity->state                = MG_MG_ST_NORMAL;
    entity->updateFunction       = &mg_updatePlayer;
    entity->collisionHandler     = &mg_playerCollisionHandler;
    entity->tileCollisionHandler = &mg_playerTileCollisionHandler;
    entity->fallOffTileHandler   = &defaultFallOffTileHandler;
    entity->overlapTileHandler   = &mg_playerOverlapTileHandler;
    entity->drawHandler          = &mg_defaultEntityDrawHandler;
    entity->tileCollider         = &entityTileCollider_1x2;
    return entity;
}

mgEntity_t* createTestObject(mgEntityManager_t* entityManager, uint16_t x, uint16_t y)
{
    mgEntity_t* entity = mg_findInactiveEntity(entityManager);

    if (entity == NULL)
    {
        return NULL;
    }

    entity->active  = true;
    entity->visible = true;
    entity->x       = TO_SUBPIXEL_COORDS(x);
    entity->y       = TO_SUBPIXEL_COORDS(y);

    entity->xspeed               = (x < (entityManager->tilemap->mapOffsetX + 120)) ? 8 : -8;
    entity->yspeed               = 0;
    entity->xMaxSpeed            = 132;
    entity->yMaxSpeed            = 132;
    entity->gravityEnabled       = true;
    entity->gravity              = 4;
    entity->spriteFlipHorizontal = false;
    entity->spriteFlipVertical   = false;
    entity->scoreValue           = 100;

    entity->type                 = mgEntity_tEST;
    entity->spriteIndex          = MG_SP_ENEMY_BASIC;
    entity->updateFunction       = &updateTestObject;
    entity->collisionHandler     = &mg_enemyCollisionHandler;
    entity->tileCollisionHandler = &mg_enemyTileCollisionHandler;
    entity->fallOffTileHandler   = &defaultFallOffTileHandler;
    entity->overlapTileHandler   = &mg_defaultOverlapTileHandler;

    entity->drawHandler          = &mg_defaultEntityDrawHandler;
    return entity;
}

mgEntity_t* createScrollLockLeft(mgEntityManager_t* entityManager, uint16_t x, uint16_t y)
{
    mgEntity_t* entity = mg_findInactiveEntity(entityManager);

    if (entity == NULL)
    {
        return NULL;
    }

    entity->active  = true;
    entity->visible = false;
    entity->x       = TO_SUBPIXEL_COORDS(x);
    entity->y       = TO_SUBPIXEL_COORDS(y);

    entity->type                 = ENTITY_SCROLL_LOCK_LEFT;
    entity->updateFunction       = &updateScrollLockLeft;
    entity->collisionHandler     = &mg_dummyCollisionHandler;
    entity->tileCollisionHandler = &mg_dummyTileCollisionHandler;
    entity->overlapTileHandler   = &mg_defaultOverlapTileHandler;

    entity->drawHandler          = &mg_defaultEntityDrawHandler;
    return entity;
}

mgEntity_t* createScrollLockRight(mgEntityManager_t* entityManager, uint16_t x, uint16_t y)
{
    mgEntity_t* entity = mg_findInactiveEntity(entityManager);

    if (entity == NULL)
    {
        return NULL;
    }

    entity->active  = true;
    entity->visible = false;
    entity->x       = TO_SUBPIXEL_COORDS(x);
    entity->y       = TO_SUBPIXEL_COORDS(y);

    entity->type                 = ENTITY_SCROLL_LOCK_RIGHT;
    entity->updateFunction       = &updateScrollLockRight;
    entity->collisionHandler     = &mg_dummyCollisionHandler;
    entity->tileCollisionHandler = &mg_dummyTileCollisionHandler;
    entity->overlapTileHandler   = &mg_defaultOverlapTileHandler;

    entity->drawHandler          = &mg_defaultEntityDrawHandler;
    return entity;
}

mgEntity_t* createScrollLockUp(mgEntityManager_t* entityManager, uint16_t x, uint16_t y)
{
    mgEntity_t* entity = mg_findInactiveEntity(entityManager);

    if (entity == NULL)
    {
        return NULL;
    }

    entity->active  = true;
    entity->visible = false;
    entity->x       = TO_SUBPIXEL_COORDS(x);
    entity->y       = TO_SUBPIXEL_COORDS(y);

    entity->type                 = ENTITY_SCROLL_LOCK_UP;
    entity->updateFunction       = &updateScrollLockUp;
    entity->collisionHandler     = &mg_dummyCollisionHandler;
    entity->tileCollisionHandler = &mg_dummyTileCollisionHandler;
    entity->overlapTileHandler   = &mg_defaultOverlapTileHandler;

    entity->drawHandler          = &mg_defaultEntityDrawHandler;
    return entity;
}

mgEntity_t* createScrollLockDown(mgEntityManager_t* entityManager, uint16_t x, uint16_t y)
{
    mgEntity_t* entity = mg_findInactiveEntity(entityManager);

    if (entity == NULL)
    {
        return NULL;
    }

    entity->active  = true;
    entity->visible = false;
    entity->x       = TO_SUBPIXEL_COORDS(x);
    entity->y       = TO_SUBPIXEL_COORDS(y);

    entity->type                 = ENTITY_SCROLL_LOCK_DOWN;
    entity->updateFunction       = &updateScrollLockDown;
    entity->collisionHandler     = &mg_dummyCollisionHandler;
    entity->tileCollisionHandler = &mg_dummyTileCollisionHandler;
    entity->overlapTileHandler   = &mg_defaultOverlapTileHandler;

    entity->drawHandler          = &mg_defaultEntityDrawHandler;
    return entity;
}

mgEntity_t* createScrollUnlock(mgEntityManager_t* entityManager, uint16_t x, uint16_t y)
{
    mgEntity_t* entity = mg_findInactiveEntity(entityManager);

    if (entity == NULL)
    {
        return NULL;
    }

    entity->active  = true;
    entity->visible = false;
    entity->x       = TO_SUBPIXEL_COORDS(x);
    entity->y       = TO_SUBPIXEL_COORDS(y);

    entity->type                 = ENTITY_SCROLL_UNLOCK;
    entity->updateFunction       = &updateScrollUnlock;
    entity->collisionHandler     = &mg_dummyCollisionHandler;
    entity->tileCollisionHandler = &mg_dummyTileCollisionHandler;
    entity->overlapTileHandler   = &mg_defaultOverlapTileHandler;

    entity->drawHandler          = &mg_defaultEntityDrawHandler;
    return entity;
}

mgEntity_t* createHitBlock(mgEntityManager_t* entityManager, uint16_t x, uint16_t y)
{
    mgEntity_t* entity = mg_findInactiveEntity(entityManager);

    if (entity == NULL)
    {
        return NULL;
    }

    entity->active  = true;
    entity->visible = true;
    entity->x       = TO_SUBPIXEL_COORDS(x);
    entity->y       = TO_SUBPIXEL_COORDS(y);

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
    entity->spriteIndex          = MG_SP_HITBLOCK_CONTAINER;
    entity->animationTimer       = 0;
    entity->updateFunction       = &updateHitBlock;
    entity->collisionHandler     = &mg_dummyCollisionHandler;
    entity->tileCollisionHandler = &mg_dummyTileCollisionHandler;
    entity->overlapTileHandler   = &mg_defaultOverlapTileHandler;

    entity->drawHandler          = &mg_defaultEntityDrawHandler;
    return entity;
}

mgEntity_t* createPowerUp(mgEntityManager_t* entityManager, uint16_t x, uint16_t y)
{
    mgEntity_t* entity = mg_findInactiveEntity(entityManager);

    if (entity == NULL)
    {
        return NULL;
    }

    entity->active  = true;
    entity->visible = true;
    entity->x       = TO_SUBPIXEL_COORDS(x);
    entity->y       = TO_SUBPIXEL_COORDS(y);

    entity->xspeed               = (entityManager->playerEntity->x > entity->x) ? -16 : 16;
    entity->yspeed               = 0;
    entity->xMaxSpeed            = 132;
    entity->yMaxSpeed            = 132;
    entity->gravityEnabled       = true;
    entity->gravity              = 4;
    entity->spriteFlipHorizontal = false;
    entity->spriteFlipVertical   = false;

    entity->type                 = ENTITY_POWERUP;
    entity->spriteIndex          = (entityManager->playerEntity->hp < 2) ? MG_SP_GAMING_1 : MG_SP_MUSIC_1;
    entity->animationTimer       = 0;
    entity->updateFunction       = &updatePowerUp;
    entity->collisionHandler     = &powerUpCollisionHandler;
    entity->tileCollisionHandler = &mg_enemyTileCollisionHandler;
    entity->fallOffTileHandler   = &defaultFallOffTileHandler;
    entity->overlapTileHandler   = &mg_defaultOverlapTileHandler;

    entity->drawHandler          = &mg_defaultEntityDrawHandler;
    return entity;
}

mgEntity_t* createWarp(mgEntityManager_t* entityManager, uint16_t x, uint16_t y)
{
    mgEntity_t* entity = mg_findInactiveEntity(entityManager);

    if (entity == NULL)
    {
        return NULL;
    }

    entity->active  = true;
    entity->visible = true;
    entity->x       = TO_SUBPIXEL_COORDS(x);
    entity->y       = TO_SUBPIXEL_COORDS(y);

    entity->xspeed         = 0;
    entity->yspeed         = 0;
    entity->xMaxSpeed      = 132;
    entity->yMaxSpeed      = 132;
    entity->gravityEnabled = true;
    entity->gravity        = 4;

    entity->spriteFlipVertical = false;

    entity->type                 = ENTITY_WARP;
    entity->spriteIndex          = MG_SP_WARP_1;
    entity->animationTimer       = 0;
    entity->updateFunction       = &updateWarp;
    entity->collisionHandler     = &mg_dummyCollisionHandler;
    entity->tileCollisionHandler = &mg_dummyTileCollisionHandler;
    entity->overlapTileHandler   = &mg_defaultOverlapTileHandler;

    entity->drawHandler          = &mg_defaultEntityDrawHandler;
    return entity;
}

mgEntity_t* createDustBunny(mgEntityManager_t* entityManager, uint16_t x, uint16_t y)
{
    mgEntity_t* entity = mg_findInactiveEntity(entityManager);

    if (entity == NULL)
    {
        return NULL;
    }

    entity->active  = true;
    entity->visible = true;
    entity->x       = TO_SUBPIXEL_COORDS(x);
    entity->y       = TO_SUBPIXEL_COORDS(y);

    entity->xspeed               = 0;
    entity->yspeed               = 0;
    entity->xMaxSpeed            = 132;
    entity->yMaxSpeed            = 132;
    entity->xDamping             = 0; // This will be repurposed to track state
    entity->yDamping             = 0; // This will be repurposed as a state timer
    entity->gravityEnabled       = true;
    entity->gravity              = 4;
    entity->spriteFlipHorizontal = (x < (entityManager->tilemap->mapOffsetX + 120)) ? true : false;
    entity->spriteFlipVertical   = false;

    entity->scoreValue = 150;

    entity->type                 = ENTITY_DUST_BUNNY;
    entity->spriteIndex          = MG_SP_DUSTBUNNY_IDLE;
    entity->updateFunction       = &updateDustBunny;
    entity->collisionHandler     = &mg_enemyCollisionHandler;
    entity->tileCollisionHandler = &dustBunnyTileCollisionHandler;
    entity->fallOffTileHandler   = &defaultFallOffTileHandler;
    entity->overlapTileHandler   = &mg_defaultOverlapTileHandler;

    entity->drawHandler          = &mg_defaultEntityDrawHandler;
    return entity;
}

mgEntity_t* createWasp(mgEntityManager_t* entityManager, uint16_t x, uint16_t y)
{
    mgEntity_t* entity = mg_findInactiveEntity(entityManager);

    if (entity == NULL)
    {
        return NULL;
    }

    entity->active  = true;
    entity->visible = true;
    entity->x       = TO_SUBPIXEL_COORDS(x);
    entity->y       = TO_SUBPIXEL_COORDS(y);

    entity->yspeed               = 0;
    entity->xMaxSpeed            = 132;
    entity->yMaxSpeed            = 128;
    entity->xDamping             = 0; // This will be repurposed to track state
    entity->yDamping             = 0; // This will be repurposed as a state timer
    entity->gravityEnabled       = false;
    entity->gravity              = 8;
    entity->spriteFlipHorizontal = (x < (entityManager->tilemap->mapOffsetX + 120)) ? false : true;
    entity->spriteFlipVertical   = false;
    entity->scoreValue           = 200;

    entity->xspeed = (entity->spriteFlipHorizontal) ? -16 : 16;

    entity->type                 = ENTITY_WASP;
    entity->spriteIndex          = MG_SP_WASP_1;
    entity->updateFunction       = &updateWasp;
    entity->collisionHandler     = &mg_enemyCollisionHandler;
    entity->tileCollisionHandler = &waspTileCollisionHandler;
    entity->fallOffTileHandler   = &defaultFallOffTileHandler;
    entity->overlapTileHandler   = &mg_defaultOverlapTileHandler;

    entity->drawHandler          = &mg_defaultEntityDrawHandler;
    return entity;
}

mgEntity_t* createEnemyBushL2(mgEntityManager_t* entityManager, uint16_t x, uint16_t y)
{
    mgEntity_t* entity = mg_findInactiveEntity(entityManager);

    if (entity == NULL)
    {
        return NULL;
    }

    entity->active  = true;
    entity->visible = true;
    entity->x       = TO_SUBPIXEL_COORDS(x);
    entity->y       = TO_SUBPIXEL_COORDS(y);

    entity->xspeed               = (x < (entityManager->tilemap->mapOffsetX + 120)) ? 12 : -12;
    entity->yspeed               = 0;
    entity->xMaxSpeed            = 132;
    entity->yMaxSpeed            = 132;
    entity->gravityEnabled       = true;
    entity->gravity              = 4;
    entity->spriteFlipHorizontal = false;
    entity->spriteFlipVertical   = false;
    entity->scoreValue           = 150;

    entity->type                 = ENTITY_BUSH_2;
    entity->spriteIndex          = MG_SP_ENEMY_BUSH_L2;
    entity->updateFunction       = &updateTestObject;
    entity->collisionHandler     = &mg_enemyCollisionHandler;
    entity->tileCollisionHandler = &mg_enemyTileCollisionHandler;
    entity->fallOffTileHandler   = &turnAroundAtEdgeOfTileHandler;
    entity->overlapTileHandler   = &mg_defaultOverlapTileHandler;

    entity->drawHandler          = &mg_defaultEntityDrawHandler;
    return entity;
}

mgEntity_t* createEnemyBushL3(mgEntityManager_t* entityManager, uint16_t x, uint16_t y)
{
    mgEntity_t* entity = mg_findInactiveEntity(entityManager);

    if (entity == NULL)
    {
        return NULL;
    }

    entity->active  = true;
    entity->visible = true;
    entity->x       = TO_SUBPIXEL_COORDS(x);
    entity->y       = TO_SUBPIXEL_COORDS(y);

    entity->xspeed               = (x < (entityManager->tilemap->mapOffsetX + 120)) ? 11 : -11;
    entity->yspeed               = 0;
    entity->xMaxSpeed            = 132;
    entity->yMaxSpeed            = 132;
    entity->gravityEnabled       = true;
    entity->gravity              = 4;
    entity->spriteFlipHorizontal = false;
    entity->spriteFlipVertical   = false;
    entity->scoreValue           = 250;

    entity->yDamping = 20; // This will be repurposed as a state timer

    entity->type                 = ENTITY_BUSH_3;
    entity->spriteIndex          = MG_SP_ENEMY_BUSH_L3;
    entity->updateFunction       = &updateEnemyBushL3;
    entity->collisionHandler     = &mg_enemyCollisionHandler;
    entity->tileCollisionHandler = &mg_enemyTileCollisionHandler;
    entity->fallOffTileHandler   = &turnAroundAtEdgeOfTileHandler;
    entity->overlapTileHandler   = &mg_defaultOverlapTileHandler;

    entity->drawHandler          = &mg_defaultEntityDrawHandler;
    return entity;
}

mgEntity_t* createDustBunnyL2(mgEntityManager_t* entityManager, uint16_t x, uint16_t y)
{
    mgEntity_t* entity = mg_findInactiveEntity(entityManager);

    if (entity == NULL)
    {
        return NULL;
    }

    entity->active  = true;
    entity->visible = true;
    entity->x       = TO_SUBPIXEL_COORDS(x);
    entity->y       = TO_SUBPIXEL_COORDS(y);

    entity->xspeed               = 0;
    entity->yspeed               = 0;
    entity->xMaxSpeed            = 132;
    entity->yMaxSpeed            = 132;
    entity->xDamping             = 0; // This will be repurposed to track state
    entity->yDamping             = 0; // This will be repurposed as a state timer
    entity->gravityEnabled       = true;
    entity->gravity              = 4;
    entity->spriteFlipHorizontal = (x < (entityManager->tilemap->mapOffsetX + 120)) ? false : true;
    entity->spriteFlipVertical   = false;
    entity->scoreValue           = 200;

    entity->type                 = ENTITY_DUST_BUNNY_2;
    entity->spriteIndex          = MG_SP_DUSTBUNNY_L2_IDLE;
    entity->updateFunction       = &updateDustBunnyL2;
    entity->collisionHandler     = &mg_enemyCollisionHandler;
    entity->tileCollisionHandler = &dustBunnyL2TileCollisionHandler;
    entity->fallOffTileHandler   = &defaultFallOffTileHandler;
    entity->overlapTileHandler   = &mg_defaultOverlapTileHandler;

    entity->drawHandler          = &mg_defaultEntityDrawHandler;
    return entity;
}

mgEntity_t* createDustBunnyL3(mgEntityManager_t* entityManager, uint16_t x, uint16_t y)
{
    mgEntity_t* entity = mg_findInactiveEntity(entityManager);

    if (entity == NULL)
    {
        return NULL;
    }

    entity->active  = true;
    entity->visible = true;
    entity->x       = TO_SUBPIXEL_COORDS(x);
    entity->y       = TO_SUBPIXEL_COORDS(y);

    entity->xspeed               = 0;
    entity->yspeed               = 0;
    entity->xMaxSpeed            = 132;
    entity->yMaxSpeed            = 132;
    entity->xDamping             = 0; // This will be repurposed to track state
    entity->yDamping             = 0; // This will be repurposed as a state timer
    entity->gravityEnabled       = true;
    entity->gravity              = 4;
    entity->spriteFlipHorizontal = (x < (entityManager->tilemap->mapOffsetX + 120)) ? true : false;
    entity->spriteFlipVertical   = false;
    entity->scoreValue           = 300;

    entity->type                 = ENTITY_DUST_BUNNY_3;
    entity->spriteIndex          = MG_SP_DUSTBUNNY_L3_IDLE;
    entity->updateFunction       = &updateDustBunnyL3;
    entity->collisionHandler     = &mg_enemyCollisionHandler;
    entity->tileCollisionHandler = &dustBunnyL3TileCollisionHandler;
    entity->fallOffTileHandler   = &defaultFallOffTileHandler;
    entity->overlapTileHandler   = &mg_defaultOverlapTileHandler;

    entity->drawHandler          = &mg_defaultEntityDrawHandler;
    return entity;
}

mgEntity_t* createWaspL2(mgEntityManager_t* entityManager, uint16_t x, uint16_t y)
{
    mgEntity_t* entity = mg_findInactiveEntity(entityManager);

    if (entity == NULL)
    {
        return NULL;
    }

    entity->active  = true;
    entity->visible = true;
    entity->x       = TO_SUBPIXEL_COORDS(x);
    entity->y       = TO_SUBPIXEL_COORDS(y);

    entity->yspeed               = 0;
    entity->xMaxSpeed            = 132;
    entity->yMaxSpeed            = 192;
    entity->xDamping             = 0; // This will be repurposed to track state
    entity->yDamping             = 0; // This will be repurposed as a state timer
    entity->jumpPower            = (1 + esp_random() % 4) * 256;
    entity->gravityEnabled       = false;
    entity->gravity              = 8;
    entity->spriteFlipHorizontal = (x < (entityManager->tilemap->mapOffsetX + 120)) ? false : true;
    entity->spriteFlipVertical   = false;
    entity->falling              = false;
    entity->scoreValue           = 300;

    entity->xspeed = (entity->spriteFlipHorizontal) ? -24 : 24;

    entity->type                 = ENTITY_WASP_2;
    entity->spriteIndex          = MG_SP_WASP_L2_1;
    entity->updateFunction       = &updateWaspL2;
    entity->collisionHandler     = &mg_enemyCollisionHandler;
    entity->tileCollisionHandler = &waspTileCollisionHandler;
    entity->fallOffTileHandler   = &defaultFallOffTileHandler;
    entity->overlapTileHandler   = &mg_defaultOverlapTileHandler;

    entity->drawHandler          = &mg_defaultEntityDrawHandler;
    return entity;
}

mgEntity_t* createWaspL3(mgEntityManager_t* entityManager, uint16_t x, uint16_t y)
{
    mgEntity_t* entity = mg_findInactiveEntity(entityManager);

    if (entity == NULL)
    {
        return NULL;
    }

    entity->active  = true;
    entity->visible = true;
    entity->x       = TO_SUBPIXEL_COORDS(x);
    entity->y       = TO_SUBPIXEL_COORDS(y);

    entity->yspeed               = 0;
    entity->xMaxSpeed            = 132;
    entity->yMaxSpeed            = 256;
    entity->xDamping             = 0; // This will be repurposed to track state
    entity->yDamping             = 0; // This will be repurposed as a state timer
    entity->jumpPower            = (1 + esp_random() % 4) * 256;
    entity->gravityEnabled       = false;
    entity->gravity              = 8;
    entity->spriteFlipHorizontal = (x < (entityManager->tilemap->mapOffsetX + 120)) ? false : true;
    entity->spriteFlipVertical   = false;
    entity->scoreValue           = 400;

    entity->xspeed = (entity->spriteFlipHorizontal) ? -24 : 24;

    entity->type                 = ENTITY_WASP_3;
    entity->spriteIndex          = MG_SP_WASP_L3_1;
    entity->updateFunction       = &updateWaspL3;
    entity->collisionHandler     = &mg_enemyCollisionHandler;
    entity->tileCollisionHandler = &waspTileCollisionHandler;
    entity->fallOffTileHandler   = &defaultFallOffTileHandler;
    entity->overlapTileHandler   = &mg_defaultOverlapTileHandler;

    entity->drawHandler          = &mg_defaultEntityDrawHandler;
    return entity;
}

mgEntity_t* createBgColBlue(mgEntityManager_t* entityManager, uint16_t x, uint16_t y)
{
    mgEntity_t* entity = mg_findInactiveEntity(entityManager);

    if (entity == NULL)
    {
        return NULL;
    }

    entity->active   = true;
    entity->visible  = false;
    entity->x        = TO_SUBPIXEL_COORDS(x);
    entity->y        = TO_SUBPIXEL_COORDS(y);
    entity->xDamping = c335;

    entity->type                 = ENTITY_BGCOL_BLUE;
    entity->updateFunction       = &updateBgCol;
    entity->collisionHandler     = &mg_dummyCollisionHandler;
    entity->tileCollisionHandler = &mg_dummyTileCollisionHandler;
    entity->overlapTileHandler   = &mg_defaultOverlapTileHandler;

    entity->drawHandler          = &mg_defaultEntityDrawHandler;
    return entity;
}

mgEntity_t* createBgColYellow(mgEntityManager_t* entityManager, uint16_t x, uint16_t y)
{
    mgEntity_t* entity = mg_findInactiveEntity(entityManager);

    if (entity == NULL)
    {
        return NULL;
    }

    entity->active   = true;
    entity->visible  = false;
    entity->x        = TO_SUBPIXEL_COORDS(x);
    entity->y        = TO_SUBPIXEL_COORDS(y);
    entity->xDamping = c542;

    entity->type                 = ENTITY_BGCOL_YELLOW;
    entity->updateFunction       = &updateBgCol;
    entity->collisionHandler     = &mg_dummyCollisionHandler;
    entity->tileCollisionHandler = &mg_dummyTileCollisionHandler;
    entity->overlapTileHandler   = &mg_defaultOverlapTileHandler;

    entity->drawHandler          = &mg_defaultEntityDrawHandler;
    return entity;
}

mgEntity_t* createBgColOrange(mgEntityManager_t* entityManager, uint16_t x, uint16_t y)
{
    mgEntity_t* entity = mg_findInactiveEntity(entityManager);

    if (entity == NULL)
    {
        return NULL;
    }

    entity->active   = true;
    entity->visible  = false;
    entity->x        = TO_SUBPIXEL_COORDS(x);
    entity->y        = TO_SUBPIXEL_COORDS(y);
    entity->xDamping = c532;

    entity->type                 = ENTITY_BGCOL_ORANGE;
    entity->updateFunction       = &updateBgCol;
    entity->collisionHandler     = &mg_dummyCollisionHandler;
    entity->tileCollisionHandler = &mg_dummyTileCollisionHandler;
    entity->overlapTileHandler   = &mg_defaultOverlapTileHandler;

    entity->drawHandler          = &mg_defaultEntityDrawHandler;
    return entity;
}

mgEntity_t* createBgColPurple(mgEntityManager_t* entityManager, uint16_t x, uint16_t y)
{
    mgEntity_t* entity = mg_findInactiveEntity(entityManager);

    if (entity == NULL)
    {
        return NULL;
    }

    entity->active   = true;
    entity->visible  = false;
    entity->x        = TO_SUBPIXEL_COORDS(x);
    entity->y        = TO_SUBPIXEL_COORDS(y);
    entity->xDamping = c214;

    entity->type                 = ENTITY_BGCOL_PURPLE;
    entity->updateFunction       = &updateBgCol;
    entity->collisionHandler     = &mg_dummyCollisionHandler;
    entity->tileCollisionHandler = &mg_dummyTileCollisionHandler;
    entity->overlapTileHandler   = &mg_defaultOverlapTileHandler;

    entity->drawHandler          = &mg_defaultEntityDrawHandler;
    return entity;
}

mgEntity_t* createBgColDarkPurple(mgEntityManager_t* entityManager, uint16_t x, uint16_t y)
{
    mgEntity_t* entity = mg_findInactiveEntity(entityManager);

    if (entity == NULL)
    {
        return NULL;
    }

    entity->active   = true;
    entity->visible  = false;
    entity->x        = TO_SUBPIXEL_COORDS(x);
    entity->y        = TO_SUBPIXEL_COORDS(y);
    entity->xDamping = c103;

    entity->type                 = ENTITY_BGCOL_DARK_PURPLE;
    entity->updateFunction       = &updateBgCol;
    entity->collisionHandler     = &mg_dummyCollisionHandler;
    entity->tileCollisionHandler = &mg_dummyTileCollisionHandler;
    entity->overlapTileHandler   = &mg_defaultOverlapTileHandler;

    entity->drawHandler          = &mg_defaultEntityDrawHandler;
    return entity;
}

mgEntity_t* createBgColBlack(mgEntityManager_t* entityManager, uint16_t x, uint16_t y)
{
    mgEntity_t* entity = mg_findInactiveEntity(entityManager);

    if (entity == NULL)
    {
        return NULL;
    }

    entity->active   = true;
    entity->visible  = false;
    entity->x        = TO_SUBPIXEL_COORDS(x);
    entity->y        = TO_SUBPIXEL_COORDS(y);
    entity->xDamping = c000;

    entity->type                 = ENTITY_BGCOL_BLACK;
    entity->updateFunction       = &updateBgCol;
    entity->collisionHandler     = &mg_dummyCollisionHandler;
    entity->tileCollisionHandler = &mg_dummyTileCollisionHandler;
    entity->overlapTileHandler   = &mg_defaultOverlapTileHandler;

    entity->drawHandler          = &mg_defaultEntityDrawHandler;
    return entity;
}

mgEntity_t* createBgColNeutralGreen(mgEntityManager_t* entityManager, uint16_t x, uint16_t y)
{
    mgEntity_t* entity = mg_findInactiveEntity(entityManager);

    if (entity == NULL)
    {
        return NULL;
    }

    entity->active   = true;
    entity->visible  = false;
    entity->x        = TO_SUBPIXEL_COORDS(x);
    entity->y        = TO_SUBPIXEL_COORDS(y);
    entity->xDamping = c133;

    entity->type                 = ENTITY_BGCOL_NEUTRAL_GREEN;
    entity->updateFunction       = &updateBgCol;
    entity->collisionHandler     = &mg_dummyCollisionHandler;
    entity->tileCollisionHandler = &mg_dummyTileCollisionHandler;
    entity->overlapTileHandler   = &mg_defaultOverlapTileHandler;

    entity->drawHandler          = &mg_defaultEntityDrawHandler;
    return entity;
}

mgEntity_t* createBgColNeutralDarkRed(mgEntityManager_t* entityManager, uint16_t x, uint16_t y)
{
    mgEntity_t* entity = mg_findInactiveEntity(entityManager);

    if (entity == NULL)
    {
        return NULL;
    }

    entity->active   = true;
    entity->visible  = false;
    entity->x        = TO_SUBPIXEL_COORDS(x);
    entity->y        = TO_SUBPIXEL_COORDS(y);
    entity->xDamping = c200;

    entity->type                 = ENTITY_BGCOL_DARK_RED;
    entity->updateFunction       = &updateBgCol;
    entity->collisionHandler     = &mg_dummyCollisionHandler;
    entity->tileCollisionHandler = &mg_dummyTileCollisionHandler;
    entity->overlapTileHandler   = &mg_defaultOverlapTileHandler;

    entity->drawHandler          = &mg_defaultEntityDrawHandler;
    return entity;
}

mgEntity_t* createBgColNeutralDarkGreen(mgEntityManager_t* entityManager, uint16_t x, uint16_t y)
{
    mgEntity_t* entity = mg_findInactiveEntity(entityManager);

    if (entity == NULL)
    {
        return NULL;
    }

    entity->active   = true;
    entity->visible  = false;
    entity->x        = TO_SUBPIXEL_COORDS(x);
    entity->y        = TO_SUBPIXEL_COORDS(y);
    entity->xDamping = c010;

    entity->type                 = ENTITY_BGCOL_DARK_GREEN;
    entity->updateFunction       = &updateBgCol;
    entity->collisionHandler     = &mg_dummyCollisionHandler;
    entity->tileCollisionHandler = &mg_dummyTileCollisionHandler;
    entity->overlapTileHandler   = &mg_defaultOverlapTileHandler;

    entity->drawHandler          = &mg_defaultEntityDrawHandler;
    return entity;
}

mgEntity_t* create1up(mgEntityManager_t* entityManager, uint16_t x, uint16_t y)
{
    mgEntity_t* entity = mg_findInactiveEntity(entityManager);

    if (entity == NULL)
    {
        return NULL;
    }

    entity->active  = true;
    entity->visible = true;
    entity->x       = TO_SUBPIXEL_COORDS(x);
    entity->y       = TO_SUBPIXEL_COORDS(y);

    entity->xspeed               = (entityManager->playerEntity->x > entity->x) ? -16 : 16;
    entity->yspeed               = 0;
    entity->xMaxSpeed            = 132;
    entity->yMaxSpeed            = 132;
    entity->gravityEnabled       = true;
    entity->gravity              = 4;
    entity->spriteFlipHorizontal = false;
    entity->spriteFlipVertical   = false;

    entity->type                 = ENTITY_1UP;
    entity->spriteIndex          = MG_SP_1UP_1;
    entity->animationTimer       = 0;
    entity->updateFunction       = &update1up;
    entity->collisionHandler     = &powerUpCollisionHandler;
    entity->tileCollisionHandler = &mg_enemyTileCollisionHandler;
    entity->fallOffTileHandler   = &defaultFallOffTileHandler;
    entity->overlapTileHandler   = &mg_defaultOverlapTileHandler;

    entity->drawHandler          = &mg_defaultEntityDrawHandler;
    return entity;
}

mgEntity_t* createWaveBall(mgEntityManager_t* entityManager, uint16_t x, uint16_t y)
{
    mgEntity_t* entity = mg_findInactiveEntity(entityManager);

    if (entity == NULL)
    {
        return NULL;
    }

    entity->active  = true;
    entity->visible = true;
    entity->x       = TO_SUBPIXEL_COORDS(x);
    entity->y       = TO_SUBPIXEL_COORDS(y);

    entity->xspeed               = 0;
    entity->yspeed               = 0;
    entity->xMaxSpeed            = 132;
    entity->yMaxSpeed            = 132;
    entity->gravityEnabled       = false;
    entity->gravity              = 4;
    entity->spriteFlipHorizontal = false;
    entity->spriteFlipVertical   = false;
    entity->yDamping             = 3; // This will be repurposed as a state timer
    entity->xDamping             = 0; // This will be repurposed as a state tracker

    entity->type                 = ENTITY_WAVE_BALL;
    entity->spriteIndex          = MG_SP_WAVEBALL_1;
    entity->animationTimer       = 0;
    entity->updateFunction       = &updateWaveBall;
    entity->collisionHandler     = &mg_dummyCollisionHandler;
    entity->tileCollisionHandler = &mg_dummyTileCollisionHandler;
    entity->fallOffTileHandler   = &defaultFallOffTileHandler;
    entity->overlapTileHandler   = &mg_defaultOverlapTileHandler;

    entity->drawHandler          = &mg_defaultEntityDrawHandler;
    return entity;
}

mgEntity_t* createCheckpoint(mgEntityManager_t* entityManager, uint16_t x, uint16_t y)
{
    mgEntity_t* entity = mg_findInactiveEntity(entityManager);

    if (entity == NULL)
    {
        return NULL;
    }

    entity->active  = true;
    entity->visible = true;
    entity->x       = TO_SUBPIXEL_COORDS(x);
    entity->y       = TO_SUBPIXEL_COORDS(y);

    entity->xspeed               = 0;
    entity->yspeed               = 0;
    entity->xMaxSpeed            = 132;
    entity->yMaxSpeed            = 132;
    entity->gravityEnabled       = true;
    entity->gravity              = 4;
    entity->spriteFlipHorizontal = false;
    entity->spriteFlipVertical   = false;

    entity->xDamping = 0; // State of the checkpoint. 0 = inactive, 1 = active

    entity->type                 = ENTITY_CHECKPOINT;
    entity->spriteIndex          = MG_SP_CHECKPOINT_INACTIVE;
    entity->animationTimer       = 0;
    entity->updateFunction       = &updateCheckpoint;
    entity->collisionHandler     = &mg_dummyCollisionHandler;
    entity->tileCollisionHandler = &mg_dummyTileCollisionHandler;
    entity->overlapTileHandler   = &mg_defaultOverlapTileHandler;

    entity->drawHandler          = &mg_defaultEntityDrawHandler;
    return entity;
}

void mg_freeEntityManager(mgEntityManager_t* self)
{
    heap_caps_free(self->entities);
}

mgEntity_t* createBgmChange1(mgEntityManager_t* entityManager, uint16_t x, uint16_t y)
{
    mgEntity_t* entity = mg_findInactiveEntity(entityManager);

    if (entity == NULL)
    {
        return NULL;
    }

    entity->active   = true;
    entity->visible  = false;
    entity->x        = TO_SUBPIXEL_COORDS(x);
    entity->y        = TO_SUBPIXEL_COORDS(y);
    entity->xDamping = MG_BGM_MAIN;

    entity->type                 = ENTITY_BGM_CHANGE_1;
    entity->updateFunction       = &updateBgmChange;
    entity->collisionHandler     = &mg_dummyCollisionHandler;
    entity->tileCollisionHandler = &mg_dummyTileCollisionHandler;
    entity->overlapTileHandler   = &mg_defaultOverlapTileHandler;

    entity->drawHandler          = &mg_defaultEntityDrawHandler;
    return entity;
}

mgEntity_t* createBgmChange2(mgEntityManager_t* entityManager, uint16_t x, uint16_t y)
{
    mgEntity_t* entity = mg_findInactiveEntity(entityManager);

    if (entity == NULL)
    {
        return NULL;
    }

    entity->active   = true;
    entity->visible  = false;
    entity->x        = TO_SUBPIXEL_COORDS(x);
    entity->y        = TO_SUBPIXEL_COORDS(y);
    entity->xDamping = MG_BGM_ATHLETIC;

    entity->type                 = ENTITY_BGM_CHANGE_2;
    entity->updateFunction       = &updateBgmChange;
    entity->collisionHandler     = &mg_dummyCollisionHandler;
    entity->tileCollisionHandler = &mg_dummyTileCollisionHandler;
    entity->overlapTileHandler   = &mg_defaultOverlapTileHandler;

    entity->drawHandler          = &mg_defaultEntityDrawHandler;
    return entity;
}

mgEntity_t* createBgmChange3(mgEntityManager_t* entityManager, uint16_t x, uint16_t y)
{
    mgEntity_t* entity = mg_findInactiveEntity(entityManager);

    if (entity == NULL)
    {
        return NULL;
    }

    entity->active   = true;
    entity->visible  = false;
    entity->x        = TO_SUBPIXEL_COORDS(x);
    entity->y        = TO_SUBPIXEL_COORDS(y);
    entity->xDamping = MG_BGM_UNDERGROUND;

    entity->type                 = ENTITY_BGM_CHANGE_3;
    entity->updateFunction       = &updateBgmChange;
    entity->collisionHandler     = &mg_dummyCollisionHandler;
    entity->tileCollisionHandler = &mg_dummyTileCollisionHandler;
    entity->overlapTileHandler   = &mg_defaultOverlapTileHandler;

    entity->drawHandler          = &mg_defaultEntityDrawHandler;
    return entity;
}

mgEntity_t* createBgmChange4(mgEntityManager_t* entityManager, uint16_t x, uint16_t y)
{
    mgEntity_t* entity = mg_findInactiveEntity(entityManager);

    if (entity == NULL)
    {
        return NULL;
    }

    entity->active   = true;
    entity->visible  = false;
    entity->x        = TO_SUBPIXEL_COORDS(x);
    entity->y        = TO_SUBPIXEL_COORDS(y);
    entity->xDamping = MG_BGM_FORTRESS;

    entity->type                 = ENTITY_BGM_CHANGE_4;
    entity->updateFunction       = &updateBgmChange;
    entity->collisionHandler     = &mg_dummyCollisionHandler;
    entity->tileCollisionHandler = &mg_dummyTileCollisionHandler;
    entity->overlapTileHandler   = &mg_defaultOverlapTileHandler;

    entity->drawHandler          = &mg_defaultEntityDrawHandler;
    return entity;
}

mgEntity_t* createBgmChange5(mgEntityManager_t* entityManager, uint16_t x, uint16_t y)
{
    mgEntity_t* entity = mg_findInactiveEntity(entityManager);

    if (entity == NULL)
    {
        return NULL;
    }

    entity->active   = true;
    entity->visible  = false;
    entity->x        = TO_SUBPIXEL_COORDS(x);
    entity->y        = TO_SUBPIXEL_COORDS(y);
    entity->xDamping = MG_BGM_NULL;

    entity->type                 = ENTITY_BGM_CHANGE_5;
    entity->updateFunction       = &updateBgmChange;
    entity->collisionHandler     = &mg_dummyCollisionHandler;
    entity->tileCollisionHandler = &mg_dummyTileCollisionHandler;
    entity->overlapTileHandler   = &mg_defaultOverlapTileHandler;

    entity->drawHandler          = &mg_defaultEntityDrawHandler;
    return entity;
}

mgEntity_t* createBgmStop(mgEntityManager_t* entityManager, uint16_t x, uint16_t y)
{
    mgEntity_t* entity = mg_findInactiveEntity(entityManager);

    if (entity == NULL)
    {
        return NULL;
    }

    entity->active   = true;
    entity->visible  = false;
    entity->x        = TO_SUBPIXEL_COORDS(x);
    entity->y        = TO_SUBPIXEL_COORDS(y);
    entity->xDamping = MG_BGM_NULL;

    entity->type                 = ENTITY_BGM_STOP;
    entity->updateFunction       = &updateBgmChange;
    entity->collisionHandler     = &mg_dummyCollisionHandler;
    entity->tileCollisionHandler = &mg_dummyTileCollisionHandler;
    entity->overlapTileHandler   = &mg_defaultOverlapTileHandler;

    entity->drawHandler          = &mg_defaultEntityDrawHandler;
    return entity;
}

mgEntity_t* createWarpEntranceWall(mgEntityManager_t* entityManager, uint16_t x, uint16_t y){
    mgEntity_t* entity = mg_findInactiveEntity(entityManager);

    if (entity == NULL)
    {
        return NULL;
    }

    entity->active   = true;
    entity->visible  = false;
    entity->x        = TO_SUBPIXEL_COORDS(x);
    entity->y        = TO_SUBPIXEL_COORDS(y);
    entity->type     = ENTITY_WARP_ENTRANCE_WALL;
    entity->updateFunction       = &mg_updateDummy;
    entity->spriteIndex = MG_SP_INVISIBLE_WARP_FLOOR;
    entity->collisionHandler     = &mg_dummyCollisionHandler;
    entity->tileCollisionHandler = &mg_dummyTileCollisionHandler;
    entity->overlapTileHandler   = &mg_defaultOverlapTileHandler;
    entity->drawHandler          = &mg_defaultEntityDrawHandler;

    return entity;
}

mgEntity_t* createWarpEntranceFloor(mgEntityManager_t* entityManager, uint16_t x, uint16_t y){
    mgEntity_t* entity = mg_findInactiveEntity(entityManager);

    if (entity == NULL)
    {
        return NULL;
    }

    entity->active   = true;
    entity->visible  = false;
    entity->x        = TO_SUBPIXEL_COORDS(x);
    entity->y        = TO_SUBPIXEL_COORDS(y);
    entity->type     = ENTITY_WARP_ENTRANCE_FLOOR;
    entity->updateFunction       = &mg_updateDummy;
    entity->spriteIndex = MG_SP_INVISIBLE_WARP_FLOOR;
    entity->collisionHandler     = &mg_dummyCollisionHandler;
    entity->tileCollisionHandler = &mg_dummyTileCollisionHandler;
    entity->overlapTileHandler   = &mg_defaultOverlapTileHandler;
    entity->drawHandler          = &mg_defaultEntityDrawHandler;

    return entity;
}