//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include <esp_log.h>
#include <string.h>

#include "paEntityManager.h"
#include "esp_random.h"
#include "palette.h"

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
void pa_initializeEntityManager(paEntityManager_t* entityManager, paTilemap_t* tilemap, paGameData_t* gameData,
                                paSoundManager_t* soundManager)
{
    pa_loadSprites(entityManager);
    entityManager->entities = calloc(MAX_ENTITIES, sizeof(paEntity_t));

    for (uint8_t i = 0; i < MAX_ENTITIES; i++)
    {
        pa_initializeEntity(&(entityManager->entities[i]), entityManager, tilemap, gameData, soundManager);
    }

    entityManager->activeEntities = 0;
    entityManager->tilemap        = tilemap;

    // entityManager->viewEntity = pa_createPlayer(entityManager, entityManager->tilemap->warps[0].x * 16,
    // entityManager->tilemap->warps[0].y * 16);
    entityManager->playerEntity = entityManager->viewEntity;

    entityManager->activeEnemies = 0;
    entityManager->maxEnemies = 3;
}

void pa_loadSprites(paEntityManager_t* entityManager)
{
    loadWsg("pa-000.wsg", &(entityManager->sprites[PA_SP_PLAYER_SOUTH].wsg), false);
    entityManager->sprites[PA_SP_PLAYER_SOUTH].originX         = 8;
    entityManager->sprites[PA_SP_PLAYER_SOUTH].originY         = 16;
    
    loadWsg("pa-001.wsg", &(entityManager->sprites[PA_SP_PLAYER_WALK_SOUTH].wsg), false);
    entityManager->sprites[PA_SP_PLAYER_WALK_SOUTH].originX         = 8;
    entityManager->sprites[PA_SP_PLAYER_WALK_SOUTH].originY         = 16;
    
    loadWsg("pa-002.wsg", &(entityManager->sprites[PA_SP_PLAYER_NORTH].wsg), false);
    entityManager->sprites[PA_SP_PLAYER_NORTH].originX         = 8;
    entityManager->sprites[PA_SP_PLAYER_NORTH].originY         = 16;
    
    loadWsg("pa-003.wsg", &(entityManager->sprites[PA_SP_PLAYER_WALK_NORTH].wsg), false);
    entityManager->sprites[PA_SP_PLAYER_WALK_NORTH].originX         = 8;
    entityManager->sprites[PA_SP_PLAYER_WALK_NORTH].originY         = 16;
    
    loadWsg("pa-004.wsg", &(entityManager->sprites[PA_SP_PLAYER_SIDE].wsg), false);
    entityManager->sprites[PA_SP_PLAYER_SIDE].originX         = 8;
    entityManager->sprites[PA_SP_PLAYER_SIDE].originY         = 16;
    
    loadWsg("pa-006.wsg", &(entityManager->sprites[PA_SP_PLAYER_WALK_SIDE_1].wsg), false);
    entityManager->sprites[PA_SP_PLAYER_WALK_SIDE_1].originX         = 8;
    entityManager->sprites[PA_SP_PLAYER_WALK_SIDE_1].originY         = 16;
    
    loadWsg("pa-005.wsg", &(entityManager->sprites[PA_SP_PLAYER_WALK_SIDE_2].wsg), false);
    entityManager->sprites[PA_SP_PLAYER_WALK_SIDE_2].originX         = 8;
    entityManager->sprites[PA_SP_PLAYER_WALK_SIDE_2].originY         = 16;
   
    loadWsg("pa-007.wsg", &(entityManager->sprites[PA_SP_PLAYER_PUSH_SOUTH_1].wsg), false);
    entityManager->sprites[PA_SP_PLAYER_PUSH_SOUTH_1].originX         = 8;
    entityManager->sprites[PA_SP_PLAYER_PUSH_SOUTH_1].originY         = 16;
    
    loadWsg("pa-008.wsg", &(entityManager->sprites[PA_SP_PLAYER_PUSH_SOUTH_2].wsg), false);
    entityManager->sprites[PA_SP_PLAYER_PUSH_SOUTH_2].originX         = 8;
    entityManager->sprites[PA_SP_PLAYER_PUSH_SOUTH_2].originY         = 16;

    loadWsg("pa-009.wsg", &(entityManager->sprites[PA_SP_PLAYER_PUSH_NORTH_1].wsg), false);
    entityManager->sprites[PA_SP_PLAYER_PUSH_NORTH_1].originX         = 8;
    entityManager->sprites[PA_SP_PLAYER_PUSH_NORTH_1].originY         = 16;

    loadWsg("pa-010.wsg", &(entityManager->sprites[PA_SP_PLAYER_PUSH_NORTH_2].wsg), false);
    entityManager->sprites[PA_SP_PLAYER_PUSH_NORTH_2].originX         = 8;
    entityManager->sprites[PA_SP_PLAYER_PUSH_NORTH_2].originY         = 16;

    loadWsg("pa-011.wsg", &(entityManager->sprites[PA_SP_PLAYER_PUSH_SIDE_1].wsg), false);
    entityManager->sprites[PA_SP_PLAYER_PUSH_SIDE_1].originX         = 8;
    entityManager->sprites[PA_SP_PLAYER_PUSH_SIDE_1].originY         = 16;

    loadWsg("pa-012.wsg", &(entityManager->sprites[PA_SP_PLAYER_PUSH_SIDE_2].wsg), false);
    entityManager->sprites[PA_SP_PLAYER_PUSH_SIDE_2].originX         = 8;
    entityManager->sprites[PA_SP_PLAYER_PUSH_SIDE_2].originY         = 16;

    loadWsg("pa-013.wsg", &(entityManager->sprites[PA_SP_PLAYER_HURT].wsg), false);
    entityManager->sprites[PA_SP_PLAYER_HURT].originX         = 8;
    entityManager->sprites[PA_SP_PLAYER_HURT].originY         = 16;

    loadWsg("pa-014.wsg", &(entityManager->sprites[PA_SP_PLAYER_WIN].wsg), false);
    entityManager->sprites[PA_SP_PLAYER_WIN].originX         = 8;
    entityManager->sprites[PA_SP_PLAYER_WIN].originY         = 16;

    loadWsg("pa-015.wsg", &(entityManager->sprites[PA_SP_PLAYER_ICON].wsg), false);
    entityManager->sprites[PA_SP_PLAYER_ICON].originX         = 8;
    entityManager->sprites[PA_SP_PLAYER_ICON].originY         = 16;
    
    loadWsg("pa-tile-009.wsg", &(entityManager->sprites[PA_SP_BLOCK].wsg), false);
    entityManager->sprites[PA_SP_BLOCK].originX         = 8;
    entityManager->sprites[PA_SP_BLOCK].originY         = 8;

    loadWsg("pa-tile-013.wsg", &(entityManager->sprites[PA_SP_BONUS_BLOCK].wsg), false);
    entityManager->sprites[PA_SP_BONUS_BLOCK].originX         = 8;
    entityManager->sprites[PA_SP_BONUS_BLOCK].originY         = 8;

    loadWsg("pa-en-004.wsg", &(entityManager->sprites[PA_SP_ENEMY_SOUTH].wsg), false);
    entityManager->sprites[PA_SP_ENEMY_SOUTH].originX         = 8;
    entityManager->sprites[PA_SP_ENEMY_SOUTH].originY         = 16;

    loadWsg("pa-en-006.wsg", &(entityManager->sprites[PA_SP_ENEMY_NORTH].wsg), false);
    entityManager->sprites[PA_SP_ENEMY_NORTH].originX         = 8;
    entityManager->sprites[PA_SP_ENEMY_NORTH].originY         = 16;

    loadWsg("pa-en-000.wsg", &(entityManager->sprites[PA_SP_ENEMY_SIDE_1].wsg), false);
    entityManager->sprites[PA_SP_ENEMY_SIDE_1].originX         = 8;
    entityManager->sprites[PA_SP_ENEMY_SIDE_1].originY         = 16;

    loadWsg("pa-en-001.wsg", &(entityManager->sprites[PA_SP_ENEMY_SIDE_2].wsg), false);
    entityManager->sprites[PA_SP_ENEMY_SIDE_2].originX         = 8;
    entityManager->sprites[PA_SP_ENEMY_SIDE_2].originY         = 16;

    loadWsg("pa-en-005.wsg", &(entityManager->sprites[PA_SP_ENEMY_DRILL_SOUTH].wsg), false);
    entityManager->sprites[PA_SP_ENEMY_DRILL_SOUTH].originX         = 8;
    entityManager->sprites[PA_SP_ENEMY_DRILL_SOUTH].originY         = 16;

    loadWsg("pa-en-007.wsg", &(entityManager->sprites[PA_SP_ENEMY_DRILL_NORTH].wsg), false);
    entityManager->sprites[PA_SP_ENEMY_DRILL_NORTH].originX         = 8;
    entityManager->sprites[PA_SP_ENEMY_DRILL_NORTH].originY         = 16;

    loadWsg("pa-en-008.wsg", &(entityManager->sprites[PA_SP_ENEMY_STUN].wsg), false);
    entityManager->sprites[PA_SP_ENEMY_STUN].originX         = 8;
    entityManager->sprites[PA_SP_ENEMY_STUN].originY         = 16;

    loadWsg("pa-en-002.wsg", &(entityManager->sprites[PA_SP_ENEMY_DRILL_SIDE_1].wsg), false);
    entityManager->sprites[PA_SP_ENEMY_DRILL_SIDE_1].originX         = 8;
    entityManager->sprites[PA_SP_ENEMY_DRILL_SIDE_1].originY         = 16;

    loadWsg("pa-en-003.wsg", &(entityManager->sprites[PA_SP_ENEMY_DRILL_SIDE_2].wsg), false);
    entityManager->sprites[PA_SP_ENEMY_DRILL_SIDE_2].originX         = 8;
    entityManager->sprites[PA_SP_ENEMY_DRILL_SIDE_2].originY         = 16;

    loadWsg("break-000.wsg", &(entityManager->sprites[PA_SP_BREAK_BLOCK].wsg), false);
    entityManager->sprites[PA_SP_BREAK_BLOCK].originX         = 8;
    entityManager->sprites[PA_SP_BREAK_BLOCK].originY         = 8;

    loadWsg("break-001.wsg", &(entityManager->sprites[PA_SP_BREAK_BLOCK_1].wsg), false);
    entityManager->sprites[PA_SP_BREAK_BLOCK_1].originX         = 8;
    entityManager->sprites[PA_SP_BREAK_BLOCK_1].originY         = 8;

    loadWsg("break-002.wsg", &(entityManager->sprites[PA_SP_BREAK_BLOCK_2].wsg), false);
    entityManager->sprites[PA_SP_BREAK_BLOCK_2].originX         = 8;
    entityManager->sprites[PA_SP_BREAK_BLOCK_2].originY         = 8;

    loadWsg("break-003.wsg", &(entityManager->sprites[PA_SP_BREAK_BLOCK_3].wsg), false);
    entityManager->sprites[PA_SP_BREAK_BLOCK_3].originX         = 8;
    entityManager->sprites[PA_SP_BREAK_BLOCK_3].originY         = 8;

    loadWsg("blockfragment.wsg", &(entityManager->sprites[PA_SP_BLOCK_FRAGMENT].wsg), false);
    entityManager->sprites[PA_SP_BLOCK_FRAGMENT].originX         = 3;
    entityManager->sprites[PA_SP_BLOCK_FRAGMENT].originY         = 3;
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
        currentEntity->active = false;

        //clear out invisible block tiles that are placed for every Break Block object
        if(currentEntity->type == PA_ENTITY_BREAK_BLOCK){
            pa_setTile(currentEntity->tilemap, PA_TO_TILECOORDS(currentEntity->x >> SUBPIXEL_RESOLUTION), PA_TO_TILECOORDS(currentEntity->y >> SUBPIXEL_RESOLUTION), PA_TILE_EMPTY);            
        }

        if (excludePlayer && currentEntity == entityManager->playerEntity)
        {
            currentEntity->active = true;
        }
    }
}

void pa_drawEntities(paEntityManager_t* entityManager)
{
    for (uint8_t i = 0; i < MAX_ENTITIES; i++)
    {
        paEntity_t currentEntity = entityManager->entities[i];

        if (currentEntity.active && currentEntity.visible)
        {
            drawWsg(&(entityManager->sprites[currentEntity.spriteIndex].wsg),
                    (currentEntity.x >> SUBPIXEL_RESOLUTION) - entityManager->sprites[currentEntity.spriteIndex].originX
                        - entityManager->tilemap->mapOffsetX,
                    (currentEntity.y >> SUBPIXEL_RESOLUTION) - entityManager->tilemap->mapOffsetY
                        - entityManager->sprites[currentEntity.spriteIndex].originY,
                    currentEntity.spriteFlipHorizontal, currentEntity.spriteFlipVertical,
                    0);
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
        case PA_ENTITY_TEST:
            createdEntity = createTestObject(entityManager, x, y);
            break;
        case PA_ENTITY_BREAK_BLOCK:
            createdEntity = createScrollLockLeft(entityManager, x, y);
            break;
        case PA_ENTITY_BLOCK_FRAGMENT:
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

    entity->type                 = ENTITY_PLAYER;
    entity->spriteIndex          = PA_SP_PLAYER_SOUTH;
    entity->updateFunction       = &pa_updatePlayer;
    entity->collisionHandler     = &pa_playerCollisionHandler;
    entity->tileCollisionHandler = &pa_playerTileCollisionHandler;
    entity->fallOffTileHandler   = &defaultFallOffTileHandler;
    entity->overlapTileHandler   = &pa_playerOverlapTileHandler;
    return entity;
}

paEntity_t* createTestObject(paEntityManager_t* entityManager, uint16_t x, uint16_t y)
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
    entity->baseSpeed = 15;

    entity->type                 = PA_ENTITY_TEST;
    entity->spriteIndex          = PA_SP_ENEMY_SOUTH;
    entity->facingDirection      = PA_DIRECTION_NONE;
    entity->state                = PA_EN_ST_NORMAL;
    entity->stateTimer           = 300 + (esp_random() % 600); //Min 5 seconds, max 15 seconds
    entity->updateFunction       = &updateTestObject;
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

    pa_setTile(entityManager->tilemap, PA_TO_TILECOORDS(x), PA_TO_TILECOORDS(y), PA_TILE_INVISIBLE_BLOCK);

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

    entity->xspeed               = -64 +(esp_random() % 128);
    entity->yspeed               = -64 +(esp_random() % 128);
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

paEntity_t* createScrollLockLeft(paEntityManager_t* entityManager, uint16_t x, uint16_t y)
{
    paEntity_t* entity = pa_findInactiveEntity(entityManager);

    if (entity == NULL)
    {
        return NULL;
    }

    entity->active  = true;
    entity->visible = false;
    entity->x       = x << SUBPIXEL_RESOLUTION;
    entity->y       = y << SUBPIXEL_RESOLUTION;

    entity->type                 = 0;
    entity->updateFunction       = &updateScrollLockLeft;
    entity->collisionHandler     = &pa_dummyCollisionHandler;
    entity->tileCollisionHandler = &pa_dummyTileCollisionHandler;
    entity->overlapTileHandler   = &pa_defaultOverlapTileHandler;

    return entity;
}

paEntity_t* createScrollLockRight(paEntityManager_t* entityManager, uint16_t x, uint16_t y)
{
    paEntity_t* entity = pa_findInactiveEntity(entityManager);

    if (entity == NULL)
    {
        return NULL;
    }

    entity->active  = true;
    entity->visible = false;
    entity->x       = x << SUBPIXEL_RESOLUTION;
    entity->y       = y << SUBPIXEL_RESOLUTION;

    entity->type                 = 0;
    entity->updateFunction       = &updateScrollLockRight;
    entity->collisionHandler     = &pa_dummyCollisionHandler;
    entity->tileCollisionHandler = &pa_dummyTileCollisionHandler;
    entity->overlapTileHandler   = &pa_defaultOverlapTileHandler;

    return entity;
}

paEntity_t* createScrollLockUp(paEntityManager_t* entityManager, uint16_t x, uint16_t y)
{
    paEntity_t* entity = pa_findInactiveEntity(entityManager);

    if (entity == NULL)
    {
        return NULL;
    }

    entity->active  = true;
    entity->visible = false;
    entity->x       = x << SUBPIXEL_RESOLUTION;
    entity->y       = y << SUBPIXEL_RESOLUTION;

    entity->type                 = ENTITY_SCROLL_LOCK_UP;
    entity->updateFunction       = &updateScrollLockUp;
    entity->collisionHandler     = &pa_dummyCollisionHandler;
    entity->tileCollisionHandler = &pa_dummyTileCollisionHandler;
    entity->overlapTileHandler   = &pa_defaultOverlapTileHandler;

    return entity;
}

paEntity_t* createScrollLockDown(paEntityManager_t* entityManager, uint16_t x, uint16_t y)
{
    paEntity_t* entity = pa_findInactiveEntity(entityManager);

    if (entity == NULL)
    {
        return NULL;
    }

    entity->active  = true;
    entity->visible = false;
    entity->x       = x << SUBPIXEL_RESOLUTION;
    entity->y       = y << SUBPIXEL_RESOLUTION;

    entity->type                 = ENTITY_SCROLL_LOCK_DOWN;
    entity->updateFunction       = &updateScrollLockDown;
    entity->collisionHandler     = &pa_dummyCollisionHandler;
    entity->tileCollisionHandler = &pa_dummyTileCollisionHandler;
    entity->overlapTileHandler   = &pa_defaultOverlapTileHandler;

    return entity;
}

paEntity_t* createScrollUnlock(paEntityManager_t* entityManager, uint16_t x, uint16_t y)
{
    paEntity_t* entity = pa_findInactiveEntity(entityManager);

    if (entity == NULL)
    {
        return NULL;
    }

    entity->active  = true;
    entity->visible = false;
    entity->x       = x << SUBPIXEL_RESOLUTION;
    entity->y       = y << SUBPIXEL_RESOLUTION;

    entity->type                 = ENTITY_SCROLL_UNLOCK;
    entity->updateFunction       = &updateScrollUnlock;
    entity->collisionHandler     = &pa_dummyCollisionHandler;
    entity->tileCollisionHandler = &pa_dummyTileCollisionHandler;
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

    entity->active  = true;
    entity->visible = true;
    entity->x       = x << SUBPIXEL_RESOLUTION;
    entity->y       = y << SUBPIXEL_RESOLUTION;
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

paEntity_t* createPowerUp(paEntityManager_t* entityManager, uint16_t x, uint16_t y)
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

    entity->xspeed               = (entityManager->playerEntity->x > entity->x) ? -16 : 16;
    entity->yspeed               = 0;
    entity->xMaxSpeed            = 132;
    entity->yMaxSpeed            = 132;
    entity->gravityEnabled       = true;
    entity->gravity              = 4;
    entity->spriteFlipHorizontal = false;
    entity->spriteFlipVertical   = false;

    entity->type                 = ENTITY_POWERUP;
    entity->spriteIndex          = (entityManager->playerEntity->hp < 2) ? 0 : 1;
    entity->animationTimer       = 0;
    entity->updateFunction       = &updatePowerUp;
    entity->collisionHandler     = &powerUpCollisionHandler;
    entity->tileCollisionHandler = &pa_enemyTileCollisionHandler;
    entity->fallOffTileHandler   = &defaultFallOffTileHandler;
    entity->overlapTileHandler   = &pa_defaultOverlapTileHandler;

    return entity;
}

paEntity_t* createWarp(paEntityManager_t* entityManager, uint16_t x, uint16_t y)
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
    entity->xMaxSpeed      = 132;
    entity->yMaxSpeed      = 132;
    entity->gravityEnabled = true;
    entity->gravity        = 4;

    entity->spriteFlipVertical = false;

    entity->type                 = ENTITY_WARP;
    entity->spriteIndex          = PA_SP_PLAYER_NORTH;
    entity->animationTimer       = 0;
    entity->updateFunction       = &updateWarp;
    entity->collisionHandler     = &pa_dummyCollisionHandler;
    entity->tileCollisionHandler = &pa_dummyTileCollisionHandler;
    entity->overlapTileHandler   = &pa_defaultOverlapTileHandler;

    return entity;
}

paEntity_t* createDustBunny(paEntityManager_t* entityManager, uint16_t x, uint16_t y)
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
    entity->xDamping             = 0; // This will be repurposed to track state
    entity->yDamping             = 0; // This will be repurposed as a state timer
    entity->gravityEnabled       = true;
    entity->gravity              = 4;
    entity->spriteFlipHorizontal = (x < (entityManager->tilemap->mapOffsetX + 120)) ? true : false;
    entity->spriteFlipVertical   = false;

    entity->scoreValue = 150;

    entity->type                 = ENTITY_DUST_BUNNY;
    entity->spriteIndex          = 0;
    entity->updateFunction       = &updateDustBunny;
    entity->collisionHandler     = &pa_enemyCollisionHandler;
    entity->tileCollisionHandler = &dustBunnyTileCollisionHandler;
    entity->fallOffTileHandler   = &defaultFallOffTileHandler;
    entity->overlapTileHandler   = &pa_defaultOverlapTileHandler;

    return entity;
}

paEntity_t* createWasp(paEntityManager_t* entityManager, uint16_t x, uint16_t y)
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
    entity->spriteIndex          = SP_WASP_1;
    entity->updateFunction       = &updateWasp;
    entity->collisionHandler     = &pa_enemyCollisionHandler;
    entity->tileCollisionHandler = &waspTileCollisionHandler;
    entity->fallOffTileHandler   = &defaultFallOffTileHandler;
    entity->overlapTileHandler   = &pa_defaultOverlapTileHandler;

    return entity;
}

paEntity_t* createEnemyBushL2(paEntityManager_t* entityManager, uint16_t x, uint16_t y)
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
    entity->spriteIndex          = SP_ENEMY_BUSH_L2;
    entity->updateFunction       = &updateTestObject;
    entity->collisionHandler     = &pa_enemyCollisionHandler;
    entity->tileCollisionHandler = &pa_enemyTileCollisionHandler;
    entity->fallOffTileHandler   = &turnAroundAtEdgeOfTileHandler;
    entity->overlapTileHandler   = &pa_defaultOverlapTileHandler;

    return entity;
}

paEntity_t* createEnemyBushL3(paEntityManager_t* entityManager, uint16_t x, uint16_t y)
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
    entity->spriteIndex          = SP_ENEMY_BUSH_L3;
    entity->updateFunction       = &updateEnemyBushL3;
    entity->collisionHandler     = &pa_enemyCollisionHandler;
    entity->tileCollisionHandler = &pa_enemyTileCollisionHandler;
    entity->fallOffTileHandler   = &turnAroundAtEdgeOfTileHandler;
    entity->overlapTileHandler   = &pa_defaultOverlapTileHandler;

    return entity;
}

paEntity_t* createDustBunnyL2(paEntityManager_t* entityManager, uint16_t x, uint16_t y)
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
    entity->xDamping             = 0; // This will be repurposed to track state
    entity->yDamping             = 0; // This will be repurposed as a state timer
    entity->gravityEnabled       = true;
    entity->gravity              = 4;
    entity->spriteFlipHorizontal = (x < (entityManager->tilemap->mapOffsetX + 120)) ? false : true;
    entity->spriteFlipVertical   = false;
    entity->scoreValue           = 200;

    entity->type                 = ENTITY_DUST_BUNNY_2;
    entity->spriteIndex          = SP_DUSTBUNNY_L2_IDLE;
    entity->updateFunction       = &updateDustBunnyL2;
    entity->collisionHandler     = &pa_enemyCollisionHandler;
    entity->tileCollisionHandler = &dustBunnyL2TileCollisionHandler;
    entity->fallOffTileHandler   = &defaultFallOffTileHandler;
    entity->overlapTileHandler   = &pa_defaultOverlapTileHandler;

    return entity;
}

paEntity_t* createDustBunnyL3(paEntityManager_t* entityManager, uint16_t x, uint16_t y)
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
    entity->xDamping             = 0; // This will be repurposed to track state
    entity->yDamping             = 0; // This will be repurposed as a state timer
    entity->gravityEnabled       = true;
    entity->gravity              = 4;
    entity->spriteFlipHorizontal = (x < (entityManager->tilemap->mapOffsetX + 120)) ? true : false;
    entity->spriteFlipVertical   = false;
    entity->scoreValue           = 300;

    entity->type                 = ENTITY_DUST_BUNNY_3;
    entity->spriteIndex          = SP_DUSTBUNNY_L3_IDLE;
    entity->updateFunction       = &updateDustBunnyL3;
    entity->collisionHandler     = &pa_enemyCollisionHandler;
    entity->tileCollisionHandler = &dustBunnyL3TileCollisionHandler;
    entity->fallOffTileHandler   = &defaultFallOffTileHandler;
    entity->overlapTileHandler   = &pa_defaultOverlapTileHandler;

    return entity;
}

paEntity_t* createWaspL2(paEntityManager_t* entityManager, uint16_t x, uint16_t y)
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
    entity->spriteIndex          = SP_WASP_L2_1;
    entity->updateFunction       = &updateWaspL2;
    entity->collisionHandler     = &pa_enemyCollisionHandler;
    entity->tileCollisionHandler = &waspTileCollisionHandler;
    entity->fallOffTileHandler   = &defaultFallOffTileHandler;
    entity->overlapTileHandler   = &pa_defaultOverlapTileHandler;

    return entity;
}

paEntity_t* createWaspL3(paEntityManager_t* entityManager, uint16_t x, uint16_t y)
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
    entity->spriteIndex          = SP_WASP_L3_1;
    entity->updateFunction       = &updateWaspL3;
    entity->collisionHandler     = &pa_enemyCollisionHandler;
    entity->tileCollisionHandler = &waspTileCollisionHandler;
    entity->fallOffTileHandler   = &defaultFallOffTileHandler;
    entity->overlapTileHandler   = &pa_defaultOverlapTileHandler;

    return entity;
}

paEntity_t* createBgColBlue(paEntityManager_t* entityManager, uint16_t x, uint16_t y)
{
    paEntity_t* entity = pa_findInactiveEntity(entityManager);

    if (entity == NULL)
    {
        return NULL;
    }

    entity->active   = true;
    entity->visible  = false;
    entity->x        = x << SUBPIXEL_RESOLUTION;
    entity->y        = y << SUBPIXEL_RESOLUTION;
    entity->xDamping = c335;

    entity->type                 = ENTITY_BGCOL_BLUE;
    entity->updateFunction       = &updateBgCol;
    entity->collisionHandler     = &pa_dummyCollisionHandler;
    entity->tileCollisionHandler = &pa_dummyTileCollisionHandler;
    entity->overlapTileHandler   = &pa_defaultOverlapTileHandler;

    return entity;
}

paEntity_t* createBgColYellow(paEntityManager_t* entityManager, uint16_t x, uint16_t y)
{
    paEntity_t* entity = pa_findInactiveEntity(entityManager);

    if (entity == NULL)
    {
        return NULL;
    }

    entity->active   = true;
    entity->visible  = false;
    entity->x        = x << SUBPIXEL_RESOLUTION;
    entity->y        = y << SUBPIXEL_RESOLUTION;
    entity->xDamping = c542;

    entity->type                 = ENTITY_BGCOL_YELLOW;
    entity->updateFunction       = &updateBgCol;
    entity->collisionHandler     = &pa_dummyCollisionHandler;
    entity->tileCollisionHandler = &pa_dummyTileCollisionHandler;
    entity->overlapTileHandler   = &pa_defaultOverlapTileHandler;

    return entity;
}

paEntity_t* createBgColOrange(paEntityManager_t* entityManager, uint16_t x, uint16_t y)
{
    paEntity_t* entity = pa_findInactiveEntity(entityManager);

    if (entity == NULL)
    {
        return NULL;
    }

    entity->active   = true;
    entity->visible  = false;
    entity->x        = x << SUBPIXEL_RESOLUTION;
    entity->y        = y << SUBPIXEL_RESOLUTION;
    entity->xDamping = c532;

    entity->type                 = ENTITY_BGCOL_ORANGE;
    entity->updateFunction       = &updateBgCol;
    entity->collisionHandler     = &pa_dummyCollisionHandler;
    entity->tileCollisionHandler = &pa_dummyTileCollisionHandler;
    entity->overlapTileHandler   = &pa_defaultOverlapTileHandler;

    return entity;
}

paEntity_t* createBgColPurple(paEntityManager_t* entityManager, uint16_t x, uint16_t y)
{
    paEntity_t* entity = pa_findInactiveEntity(entityManager);

    if (entity == NULL)
    {
        return NULL;
    }

    entity->active   = true;
    entity->visible  = false;
    entity->x        = x << SUBPIXEL_RESOLUTION;
    entity->y        = y << SUBPIXEL_RESOLUTION;
    entity->xDamping = c214;

    entity->type                 = ENTITY_BGCOL_PURPLE;
    entity->updateFunction       = &updateBgCol;
    entity->collisionHandler     = &pa_dummyCollisionHandler;
    entity->tileCollisionHandler = &pa_dummyTileCollisionHandler;
    entity->overlapTileHandler   = &pa_defaultOverlapTileHandler;

    return entity;
}

paEntity_t* createBgColDarkPurple(paEntityManager_t* entityManager, uint16_t x, uint16_t y)
{
    paEntity_t* entity = pa_findInactiveEntity(entityManager);

    if (entity == NULL)
    {
        return NULL;
    }

    entity->active   = true;
    entity->visible  = false;
    entity->x        = x << SUBPIXEL_RESOLUTION;
    entity->y        = y << SUBPIXEL_RESOLUTION;
    entity->xDamping = c103;

    entity->type                 = ENTITY_BGCOL_DARK_PURPLE;
    entity->updateFunction       = &updateBgCol;
    entity->collisionHandler     = &pa_dummyCollisionHandler;
    entity->tileCollisionHandler = &pa_dummyTileCollisionHandler;
    entity->overlapTileHandler   = &pa_defaultOverlapTileHandler;

    return entity;
}

paEntity_t* createBgColBlack(paEntityManager_t* entityManager, uint16_t x, uint16_t y)
{
    paEntity_t* entity = pa_findInactiveEntity(entityManager);

    if (entity == NULL)
    {
        return NULL;
    }

    entity->active   = true;
    entity->visible  = false;
    entity->x        = x << SUBPIXEL_RESOLUTION;
    entity->y        = y << SUBPIXEL_RESOLUTION;
    entity->xDamping = c000;

    entity->type                 = ENTITY_BGCOL_BLACK;
    entity->updateFunction       = &updateBgCol;
    entity->collisionHandler     = &pa_dummyCollisionHandler;
    entity->tileCollisionHandler = &pa_dummyTileCollisionHandler;
    entity->overlapTileHandler   = &pa_defaultOverlapTileHandler;

    return entity;
}

paEntity_t* createBgColNeutralGreen(paEntityManager_t* entityManager, uint16_t x, uint16_t y)
{
    paEntity_t* entity = pa_findInactiveEntity(entityManager);

    if (entity == NULL)
    {
        return NULL;
    }

    entity->active   = true;
    entity->visible  = false;
    entity->x        = x << SUBPIXEL_RESOLUTION;
    entity->y        = y << SUBPIXEL_RESOLUTION;
    entity->xDamping = c133;

    entity->type                 = ENTITY_BGCOL_NEUTRAL_GREEN;
    entity->updateFunction       = &updateBgCol;
    entity->collisionHandler     = &pa_dummyCollisionHandler;
    entity->tileCollisionHandler = &pa_dummyTileCollisionHandler;
    entity->overlapTileHandler   = &pa_defaultOverlapTileHandler;

    return entity;
}

paEntity_t* createBgColNeutralDarkRed(paEntityManager_t* entityManager, uint16_t x, uint16_t y)
{
    paEntity_t* entity = pa_findInactiveEntity(entityManager);

    if (entity == NULL)
    {
        return NULL;
    }

    entity->active   = true;
    entity->visible  = false;
    entity->x        = x << SUBPIXEL_RESOLUTION;
    entity->y        = y << SUBPIXEL_RESOLUTION;
    entity->xDamping = c200;

    entity->type                 = ENTITY_BGCOL_DARK_RED;
    entity->updateFunction       = &updateBgCol;
    entity->collisionHandler     = &pa_dummyCollisionHandler;
    entity->tileCollisionHandler = &pa_dummyTileCollisionHandler;
    entity->overlapTileHandler   = &pa_defaultOverlapTileHandler;

    return entity;
}

paEntity_t* createBgColNeutralDarkGreen(paEntityManager_t* entityManager, uint16_t x, uint16_t y)
{
    paEntity_t* entity = pa_findInactiveEntity(entityManager);

    if (entity == NULL)
    {
        return NULL;
    }

    entity->active   = true;
    entity->visible  = false;
    entity->x        = x << SUBPIXEL_RESOLUTION;
    entity->y        = y << SUBPIXEL_RESOLUTION;
    entity->xDamping = c010;

    entity->type                 = ENTITY_BGCOL_DARK_GREEN;
    entity->updateFunction       = &updateBgCol;
    entity->collisionHandler     = &pa_dummyCollisionHandler;
    entity->tileCollisionHandler = &pa_dummyTileCollisionHandler;
    entity->overlapTileHandler   = &pa_defaultOverlapTileHandler;

    return entity;
}

paEntity_t* create1up(paEntityManager_t* entityManager, uint16_t x, uint16_t y)
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

    entity->xspeed               = (entityManager->playerEntity->x > entity->x) ? -16 : 16;
    entity->yspeed               = 0;
    entity->xMaxSpeed            = 132;
    entity->yMaxSpeed            = 132;
    entity->gravityEnabled       = true;
    entity->gravity              = 4;
    entity->spriteFlipHorizontal = false;
    entity->spriteFlipVertical   = false;

    entity->type                 = ENTITY_1UP;
    entity->spriteIndex          = SP_1UP_1;
    entity->animationTimer       = 0;
    entity->updateFunction       = &update1up;
    entity->collisionHandler     = &powerUpCollisionHandler;
    entity->tileCollisionHandler = &pa_enemyTileCollisionHandler;
    entity->fallOffTileHandler   = &defaultFallOffTileHandler;
    entity->overlapTileHandler   = &pa_defaultOverlapTileHandler;

    return entity;
}

paEntity_t* createWaveBall(paEntityManager_t* entityManager, uint16_t x, uint16_t y)
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
    entity->gravity              = 4;
    entity->spriteFlipHorizontal = false;
    entity->spriteFlipVertical   = false;
    entity->yDamping             = 3; // This will be repurposed as a state timer
    entity->xDamping             = 0; // This will be repurposed as a state tracker

    entity->type                 = ENTITY_WAVE_BALL;
    entity->spriteIndex          = SP_WAVEBALL_1;
    entity->animationTimer       = 0;
    entity->updateFunction       = &updateWaveBall;
    entity->collisionHandler     = &pa_dummyCollisionHandler;
    entity->tileCollisionHandler = &pa_dummyTileCollisionHandler;
    entity->fallOffTileHandler   = &defaultFallOffTileHandler;
    entity->overlapTileHandler   = &waveBallOverlapTileHandler;

    return entity;
}

paEntity_t* createCheckpoint(paEntityManager_t* entityManager, uint16_t x, uint16_t y)
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
    entity->gravityEnabled       = true;
    entity->gravity              = 4;
    entity->spriteFlipHorizontal = false;
    entity->spriteFlipVertical   = false;

    entity->xDamping = 0; // State of the checkpoint. 0 = inactive, 1 = active

    entity->type                 = ENTITY_CHECKPOINT;
    entity->spriteIndex          = SP_CHECKPOINT_INACTIVE;
    entity->animationTimer       = 0;
    entity->updateFunction       = &updateCheckpoint;
    entity->collisionHandler     = &pa_dummyCollisionHandler;
    entity->tileCollisionHandler = &pa_dummyTileCollisionHandler;
    entity->overlapTileHandler   = &pa_defaultOverlapTileHandler;

    return entity;
}

void pa_freeEntityManager(paEntityManager_t* self)
{
    free(self->entities);
    for (uint8_t i = 0; i < SPRITESET_SIZE; i++)
    {
        freeWsg(&self->sprites[i]);
    }
}

paEntity_t* createBgmChange1(paEntityManager_t* entityManager, uint16_t x, uint16_t y)
{
    paEntity_t* entity = pa_findInactiveEntity(entityManager);

    if (entity == NULL)
    {
        return NULL;
    }

    entity->active   = true;
    entity->visible  = false;
    entity->x        = x << SUBPIXEL_RESOLUTION;
    entity->y        = y << SUBPIXEL_RESOLUTION;
    entity->xDamping = PA_BGM_MAIN;

    entity->type                 = ENTITY_BGM_CHANGE_1;
    entity->updateFunction       = &updateBgmChange;
    entity->collisionHandler     = &pa_dummyCollisionHandler;
    entity->tileCollisionHandler = &pa_dummyTileCollisionHandler;
    entity->overlapTileHandler   = &pa_defaultOverlapTileHandler;

    return entity;
}

paEntity_t* createBgmChange2(paEntityManager_t* entityManager, uint16_t x, uint16_t y)
{
    paEntity_t* entity = pa_findInactiveEntity(entityManager);

    if (entity == NULL)
    {
        return NULL;
    }

    entity->active   = true;
    entity->visible  = false;
    entity->x        = x << SUBPIXEL_RESOLUTION;
    entity->y        = y << SUBPIXEL_RESOLUTION;
    entity->xDamping = PA_BGM_ATHLETIC;

    entity->type                 = ENTITY_BGM_CHANGE_2;
    entity->updateFunction       = &updateBgmChange;
    entity->collisionHandler     = &pa_dummyCollisionHandler;
    entity->tileCollisionHandler = &pa_dummyTileCollisionHandler;
    entity->overlapTileHandler   = &pa_defaultOverlapTileHandler;

    return entity;
}

paEntity_t* createBgmChange3(paEntityManager_t* entityManager, uint16_t x, uint16_t y)
{
    paEntity_t* entity = pa_findInactiveEntity(entityManager);

    if (entity == NULL)
    {
        return NULL;
    }

    entity->active   = true;
    entity->visible  = false;
    entity->x        = x << SUBPIXEL_RESOLUTION;
    entity->y        = y << SUBPIXEL_RESOLUTION;
    entity->xDamping = PA_BGM_UNDERGROUND;

    entity->type                 = ENTITY_BGM_CHANGE_3;
    entity->updateFunction       = &updateBgmChange;
    entity->collisionHandler     = &pa_dummyCollisionHandler;
    entity->tileCollisionHandler = &pa_dummyTileCollisionHandler;
    entity->overlapTileHandler   = &pa_defaultOverlapTileHandler;

    return entity;
}

paEntity_t* createBgmChange4(paEntityManager_t* entityManager, uint16_t x, uint16_t y)
{
    paEntity_t* entity = pa_findInactiveEntity(entityManager);

    if (entity == NULL)
    {
        return NULL;
    }

    entity->active   = true;
    entity->visible  = false;
    entity->x        = x << SUBPIXEL_RESOLUTION;
    entity->y        = y << SUBPIXEL_RESOLUTION;
    entity->xDamping = PA_BGM_FORTRESS;

    entity->type                 = ENTITY_BGM_CHANGE_4;
    entity->updateFunction       = &updateBgmChange;
    entity->collisionHandler     = &pa_dummyCollisionHandler;
    entity->tileCollisionHandler = &pa_dummyTileCollisionHandler;
    entity->overlapTileHandler   = &pa_defaultOverlapTileHandler;

    return entity;
}

paEntity_t* createBgmChange5(paEntityManager_t* entityManager, uint16_t x, uint16_t y)
{
    paEntity_t* entity = pa_findInactiveEntity(entityManager);

    if (entity == NULL)
    {
        return NULL;
    }

    entity->active   = true;
    entity->visible  = false;
    entity->x        = x << SUBPIXEL_RESOLUTION;
    entity->y        = y << SUBPIXEL_RESOLUTION;
    entity->xDamping = PA_BGM_NULL;

    entity->type                 = ENTITY_BGM_CHANGE_5;
    entity->updateFunction       = &updateBgmChange;
    entity->collisionHandler     = &pa_dummyCollisionHandler;
    entity->tileCollisionHandler = &pa_dummyTileCollisionHandler;
    entity->overlapTileHandler   = &pa_defaultOverlapTileHandler;

    return entity;
}

paEntity_t* createBgmStop(paEntityManager_t* entityManager, uint16_t x, uint16_t y)
{
    paEntity_t* entity = pa_findInactiveEntity(entityManager);

    if (entity == NULL)
    {
        return NULL;
    }

    entity->active   = true;
    entity->visible  = false;
    entity->x        = x << SUBPIXEL_RESOLUTION;
    entity->y        = y << SUBPIXEL_RESOLUTION;
    entity->xDamping = PA_BGM_NULL;

    entity->type                 = ENTITY_BGM_STOP;
    entity->updateFunction       = &updateBgmChange;
    entity->collisionHandler     = &pa_dummyCollisionHandler;
    entity->tileCollisionHandler = &pa_dummyTileCollisionHandler;
    entity->overlapTileHandler   = &pa_defaultOverlapTileHandler;

    return entity;
}

paEntity_t* pa_spawnEnemyFromSpawnBlock(paEntityManager_t* entityManager){
    paEntity_t* newEnemy = NULL;

    if(entityManager->remainingEnemies > 0 && entityManager->activeEnemies < entityManager->maxEnemies){
        uint16_t iterations = 0;
        while(newEnemy == NULL && iterations < 2){
            for(uint16_t ty = 1; ty < 14; ty++){
                for(uint16_t tx = 1; tx < 16; tx++){
                   
                    uint8_t t = pa_getTile(entityManager->tilemap, tx, ty);

                    if(t == PA_TILE_SPAWN_BLOCK_0 && (iterations > 0 || !(esp_random() % entityManager->remainingEnemies) )){
                        newEnemy = createTestObject(entityManager, (tx << PA_TILE_SIZE_IN_POWERS_OF_2) + PA_HALF_TILE_SIZE, (ty << PA_TILE_SIZE_IN_POWERS_OF_2) + PA_HALF_TILE_SIZE);
                        
                        if(newEnemy != NULL){
                            //pa_setTile(entityManager->tilemap, tx, ty, PA_TILE_EMPTY);
                            newEnemy->state = PA_EN_ST_STUN;
                            newEnemy->stateTimer = 120;
                            pa_createBreakBlock(entityManager, (tx << PA_TILE_SIZE_IN_POWERS_OF_2) + PA_HALF_TILE_SIZE, (ty << PA_TILE_SIZE_IN_POWERS_OF_2) + PA_HALF_TILE_SIZE);
                            entityManager->activeEnemies++;
                            entityManager->remainingEnemies--;
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