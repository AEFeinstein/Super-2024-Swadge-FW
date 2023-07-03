//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include <esp_log.h>
#include <string.h>

#include "entityManager.h"
#include "esp_random.h"
#include "palette.h"

#include "hdw-spiffs.h"
#include "spiffs_wsg.h"

//==============================================================================
// Constants
//==============================================================================
#define SUBPIXEL_RESOLUTION 4

//==============================================================================
// Functions
//==============================================================================
void initializeEntityManager(entityManager_t * entityManager, tilemap_t * tilemap, gameData_t * gameData)
{
    loadSprites(entityManager);
    entityManager->entities = calloc(MAX_ENTITIES, sizeof(entity_t));
    
    for(uint8_t i=0; i < MAX_ENTITIES; i++)
    {
        initializeEntity(&(entityManager->entities[i]), entityManager, tilemap, gameData);
    }

    entityManager->activeEntities = 0;
    entityManager->tilemap = tilemap;

    
    //entityManager->viewEntity = createPlayer(entityManager, entityManager->tilemap->warps[0].x * 16, entityManager->tilemap->warps[0].y * 16);
    entityManager->playerEntity = entityManager->viewEntity;
};

void loadSprites(entityManager_t * entityManager)
{
    loadWsg("paddle000.wsg", &(entityManager->sprites[SP_PADDLE_0].wsg), false);
    entityManager->sprites[SP_PADDLE_0].originX=12;
    entityManager->sprites[SP_PADDLE_0].originY=4;

    loadWsg("paddle001.wsg", &entityManager->sprites[SP_PADDLE_1].wsg, false);
    entityManager->sprites[SP_PADDLE_1].originX=12;
    entityManager->sprites[SP_PADDLE_1].originY=4;

    loadWsg("paddle002.wsg", &entityManager->sprites[SP_PADDLE_2].wsg, false);
    entityManager->sprites[SP_PADDLE_2].originX=12;
    entityManager->sprites[SP_PADDLE_2].originY=4;

    loadWsg("ball.wsg", &entityManager->sprites[SP_BALL].wsg, false);
    entityManager->sprites[SP_BALL].originX=4;
    entityManager->sprites[SP_BALL].originY=4;

    /*loadWsg("sprite004.wsg", &entityManager->sprites[SP_PLAYER_JUMP]);
    loadWsg("sprite005.wsg", &entityManager->sprites[SP_PLAYER_SLIDE]);
    loadWsg("sprite006.wsg", &entityManager->sprites[SP_PLAYER_HURT]);
    loadWsg("sprite007.wsg", &entityManager->sprites[SP_PLAYER_CLIMB]);
    loadWsg("sprite008.wsg", &entityManager->sprites[SP_PLAYER_WIN]);
    loadWsg("sprite009.wsg", &entityManager->sprites[SP_ENEMY_BASIC]);
    loadWsg("tile066.wsg", &entityManager->sprites[SP_HITBLOCK_CONTAINER]);
    loadWsg("tile034.wsg", &entityManager->sprites[SP_HITBLOCK_BRICKS]);
    loadWsg("sprite012.wsg", &entityManager->sprites[SP_DUSTBUNNY_IDLE]);
    loadWsg("sprite013.wsg", &entityManager->sprites[SP_DUSTBUNNY_CHARGE]);
    loadWsg("sprite014.wsg", &entityManager->sprites[SP_DUSTBUNNY_JUMP]);
    loadWsg("sprite015.wsg", &entityManager->sprites[SP_GAMING_1]);
    loadWsg("sprite016.wsg", &entityManager->sprites[SP_GAMING_2]);
    loadWsg("sprite017.wsg", &entityManager->sprites[SP_GAMING_3]);
    loadWsg("sprite018.wsg", &entityManager->sprites[SP_MUSIC_1]);
    loadWsg("sprite019.wsg", &entityManager->sprites[SP_MUSIC_2]);
    loadWsg("sprite020.wsg", &entityManager->sprites[SP_MUSIC_3]);
    loadWsg("sprite021.wsg", &entityManager->sprites[SP_WARP_1]);
    loadWsg("sprite022.wsg", &entityManager->sprites[SP_WARP_2]);
    loadWsg("sprite023.wsg", &entityManager->sprites[SP_WARP_3]);
    loadWsg("sprite024.wsg", &entityManager->sprites[SP_WASP_1]);
    loadWsg("sprite025.wsg", &entityManager->sprites[SP_WASP_2]);
    loadWsg("sprite026.wsg", &entityManager->sprites[SP_WASP_DIVE]);
    loadWsg("sprite027.wsg", &entityManager->sprites[SP_1UP_1]);
    loadWsg("sprite028.wsg", &entityManager->sprites[SP_1UP_2]);
    loadWsg("sprite029.wsg", &entityManager->sprites[SP_1UP_3]);
    loadWsg("sprite030.wsg", &entityManager->sprites[SP_WAVEBALL_1]);
    loadWsg("sprite031.wsg", &entityManager->sprites[SP_WAVEBALL_2]);
    loadWsg("sprite032.wsg", &entityManager->sprites[SP_WAVEBALL_3]);
    loadWsg("sprite033.wsg", &entityManager->sprites[SP_ENEMY_BUSH_L2]);
    loadWsg("sprite034.wsg", &entityManager->sprites[SP_ENEMY_BUSH_L3]);
    loadWsg("sprite035.wsg", &entityManager->sprites[SP_DUSTBUNNY_L2_IDLE]);
    loadWsg("sprite036.wsg", &entityManager->sprites[SP_DUSTBUNNY_L2_CHARGE]);
    loadWsg("sprite037.wsg", &entityManager->sprites[SP_DUSTBUNNY_L2_JUMP]);
    loadWsg("sprite038.wsg", &entityManager->sprites[SP_DUSTBUNNY_L3_IDLE]);
    loadWsg("sprite039.wsg", &entityManager->sprites[SP_DUSTBUNNY_L3_CHARGE]);
    loadWsg("sprite040.wsg", &entityManager->sprites[SP_DUSTBUNNY_L3_JUMP]);
    loadWsg("sprite041.wsg", &entityManager->sprites[SP_WASP_L2_1]);
    loadWsg("sprite042.wsg", &entityManager->sprites[SP_WASP_L2_2]);
    loadWsg("sprite043.wsg", &entityManager->sprites[SP_WASP_L2_DIVE]);
    loadWsg("sprite044.wsg", &entityManager->sprites[SP_WASP_L3_1]);
    loadWsg("sprite045.wsg", &entityManager->sprites[SP_WASP_L3_2]);
    loadWsg("sprite046.wsg", &entityManager->sprites[SP_WASP_L3_DIVE]);
    loadWsg("sprite047.wsg", &entityManager->sprites[SP_CHECKPOINT_INACTIVE]);
    loadWsg("sprite048.wsg", &entityManager->sprites[SP_CHECKPOINT_ACTIVE_1]);
    loadWsg("sprite049.wsg", &entityManager->sprites[SP_CHECKPOINT_ACTIVE_2]);
    loadWsg("tile039.wsg", &entityManager->sprites[SP_BOUNCE_BLOCK]);*/
};

void updateEntities(entityManager_t * entityManager)
{
    for(uint8_t i=0; i < MAX_ENTITIES; i++)
    {
        if(entityManager->entities[i].active)
        {
            entityManager->entities[i].updateFunction(&(entityManager->entities[i]));

            if(&(entityManager->entities[i]) == entityManager->viewEntity){
                viewFollowEntity(entityManager->tilemap, &(entityManager->entities[i]));
            }
        }
    }
};

void deactivateAllEntities(entityManager_t * entityManager, bool excludePlayer){
    for(uint8_t i=0; i < MAX_ENTITIES; i++)
    {
        entity_t* currentEntity = &(entityManager->entities[i]);
        
        currentEntity->active = false;

    //TODO: respawn warp container blocks
    /*
        if(currentEntity->type == ENTITY_WARP){
            //In destroyEntity, this will overflow to the correct value.
            currentEntity->type = 128 + TILE_CONTAINER_1;
        }
    */
   
        if(excludePlayer && currentEntity == entityManager->playerEntity){
            currentEntity->active = true;
        }
    }
}

void drawEntities(entityManager_t * entityManager)
{
    for(uint8_t i=0; i < MAX_ENTITIES; i++)
    {
        entity_t currentEntity = entityManager->entities[i];

        if(currentEntity.active && currentEntity.visible)
        {
            drawWsg(&(entityManager->sprites[currentEntity.spriteIndex].wsg), (currentEntity.x >> SUBPIXEL_RESOLUTION) - entityManager->sprites[currentEntity.spriteIndex].originX - entityManager->tilemap->mapOffsetX, (currentEntity.y >> SUBPIXEL_RESOLUTION)  - entityManager->tilemap->mapOffsetY - entityManager->sprites[currentEntity.spriteIndex].originY, currentEntity.spriteFlipHorizontal, currentEntity.spriteFlipVertical, 0);
        }
    }
};

entity_t * findInactiveEntity(entityManager_t * entityManager)
{
    if(entityManager->activeEntities == MAX_ENTITIES)
    {
        return NULL;
    };
    
    uint8_t entityIndex = 0;

    while(entityManager->entities[entityIndex].active){
        entityIndex++;

        //Extra safeguard to make sure we don't get stuck here
        if(entityIndex > MAX_ENTITIES)
        {
            return NULL;
        }
    }

    return &(entityManager->entities[entityIndex]);
}

void viewFollowEntity(tilemap_t * tilemap, entity_t * entity){
    int16_t moveViewByX = (entity->x) >> SUBPIXEL_RESOLUTION;
    int16_t moveViewByY = (entity->y > 63616) ? 0: (entity->y) >> SUBPIXEL_RESOLUTION;

    int16_t centerOfViewX = tilemap->mapOffsetX + 140;
    int16_t centerOfViewY = tilemap->mapOffsetY + 120;

    //if(centerOfViewX != moveViewByX) {
        moveViewByX -= centerOfViewX;
    //}

    //if(centerOfViewY != moveViewByY) {
        moveViewByY -= centerOfViewY;
    //}

    //if(moveViewByX && moveViewByY){
        scrollTileMap(tilemap, moveViewByX, moveViewByY);
    //}
}

entity_t* createEntity(entityManager_t *entityManager, uint8_t objectIndex, uint16_t x, uint16_t y){
    //if(entityManager->activeEntities == MAX_ENTITIES){
    //    return NULL;
    //}

    entity_t *createdEntity;

    switch(objectIndex){
        case ENTITY_PLAYER_PADDLE_BOTTOM:
            createdEntity = createPlayer(entityManager, x, y);
            break;
        /*
        case ENTITY_TEST:
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
            break;*/

        default:
            createdEntity = NULL;
    }

    //if(createdEntity != NULL) {
    //    entityManager->activeEntities++;
    //}

    return createdEntity;
}

entity_t* createPlayer(entityManager_t * entityManager, uint16_t x, uint16_t y)
{
    entity_t * entity = findInactiveEntity(entityManager);

    if(entity == NULL) {
        return NULL;
    }

    entity->active = true;
    entity->visible = true;
    entity->x = x << SUBPIXEL_RESOLUTION;
    entity->y = y << SUBPIXEL_RESOLUTION;

    entity->xspeed = 0;
    entity->yspeed = 0;
    entity->xMaxSpeed = 40; //72; Walking
    entity->yMaxSpeed = 64; //72;
    entity->xDamping = 1;
    entity->yDamping = 4;
    entity->gravityEnabled = false;
    entity->gravity = 4;
    entity->falling = false;
    entity->jumpPower = 0;
    entity->spriteFlipVertical = false;
    entity->hp = 1;
    entity->animationTimer = 0; //Used as a cooldown for shooting square wave balls

    //entity->type = ENTITY_PLAYER;
    entity->spriteIndex = SP_PADDLE_0;
    entity->updateFunction = &updatePlayer;
    entity->collisionHandler = &playerCollisionHandler;
    entity->tileCollisionHandler = &playerTileCollisionHandler;
    entity->fallOffTileHandler = &defaultFallOffTileHandler;
    entity->overlapTileHandler = &playerOverlapTileHandler;
    return entity;
}
/*
entity_t* createTestObject(entityManager_t * entityManager, uint16_t x, uint16_t y)
{
    entity_t * entity = findInactiveEntity(entityManager);

    if(entity == NULL) {
        return NULL;
    }

    entity->active = true;
    entity->visible = true;
    entity->x = x << SUBPIXEL_RESOLUTION;
    entity->y = y << SUBPIXEL_RESOLUTION;
    
    entity->xspeed = (x < (entityManager->tilemap->mapOffsetX + 120)) ? 8 : -8;
    entity->yspeed = 0;
    entity->xMaxSpeed = 132;
    entity->yMaxSpeed = 132;
    entity->gravityEnabled = true;
    entity->gravity = 4;
    entity->spriteFlipHorizontal = false;
    entity->spriteFlipVertical = false;
    entity->scoreValue = 100;

    //entity->type = ENTITY_TEST;
    entity->spriteIndex = SP_ENEMY_BASIC;
    entity->updateFunction = &updateTestObject;
    entity->collisionHandler = &enemyCollisionHandler;
    entity->tileCollisionHandler = &enemyTileCollisionHandler;
    entity->fallOffTileHandler = &defaultFallOffTileHandler;
    entity->overlapTileHandler = &defaultOverlapTileHandler;

    return entity;
}

entity_t* createScrollLockLeft(entityManager_t * entityManager, uint16_t x, uint16_t y)
{
    entity_t * entity = findInactiveEntity(entityManager);

    if(entity == NULL) {
        return NULL;
    }

    entity->active = true;
    entity->visible = false;
    entity->x = x << SUBPIXEL_RESOLUTION;
    entity->y = y << SUBPIXEL_RESOLUTION;
    
    //entity->type = ENTITY_SCROLL_LOCK_LEFT;
    entity->updateFunction = &updateScrollLockLeft;
    entity->collisionHandler = &dummyCollisionHandler;
    entity->tileCollisionHandler = &dummyTileCollisionHandler;
    entity->overlapTileHandler = &defaultOverlapTileHandler;
    
    return entity;
}

entity_t* createScrollLockRight(entityManager_t * entityManager, uint16_t x, uint16_t y)
{
    entity_t * entity = findInactiveEntity(entityManager);

    if(entity == NULL) {
        return NULL;
    }

    entity->active = true;
    entity->visible = false;
    entity->x = x << SUBPIXEL_RESOLUTION;
    entity->y = y << SUBPIXEL_RESOLUTION;
    
    //entity->type = ENTITY_SCROLL_LOCK_RIGHT;
    entity->updateFunction = &updateScrollLockRight;
    entity->collisionHandler = &dummyCollisionHandler;
    entity->tileCollisionHandler = &dummyTileCollisionHandler;
    entity->overlapTileHandler = &defaultOverlapTileHandler;

    return entity;
}

entity_t* createScrollLockUp(entityManager_t * entityManager, uint16_t x, uint16_t y)
{
    entity_t * entity = findInactiveEntity(entityManager);

    if(entity == NULL) {
        return NULL;
    }

    entity->active = true;
    entity->visible = false;
    entity->x = x << SUBPIXEL_RESOLUTION;
    entity->y = y << SUBPIXEL_RESOLUTION;
    
    //entity->type = ENTITY_SCROLL_LOCK_UP;
    entity->updateFunction = &updateScrollLockUp;
    entity->collisionHandler = &dummyCollisionHandler;
    entity->tileCollisionHandler = &dummyTileCollisionHandler;
    entity->overlapTileHandler = &defaultOverlapTileHandler;

    return entity;
}

entity_t* createScrollLockDown(entityManager_t * entityManager, uint16_t x, uint16_t y)
{
    entity_t * entity = findInactiveEntity(entityManager);

    if(entity == NULL) {
        return NULL;
    }

    entity->active = true;
    entity->visible = false;
    entity->x = x << SUBPIXEL_RESOLUTION;
    entity->y = y << SUBPIXEL_RESOLUTION;
    
    //entity->type = ENTITY_SCROLL_LOCK_DOWN;
    entity->updateFunction = &updateScrollLockDown;
    entity->collisionHandler = &dummyCollisionHandler;
    entity->tileCollisionHandler = &dummyTileCollisionHandler;
    entity->overlapTileHandler = &defaultOverlapTileHandler;

    return entity;
}

entity_t* createScrollUnlock(entityManager_t * entityManager, uint16_t x, uint16_t y)
{
    entity_t * entity = findInactiveEntity(entityManager);

    if(entity == NULL) {
        return NULL;
    }

    entity->active = true;
    entity->visible = false;
    entity->x = x << SUBPIXEL_RESOLUTION;
    entity->y = y << SUBPIXEL_RESOLUTION;
    
    //entity->type = ENTITY_SCROLL_UNLOCK;
    entity->updateFunction = &updateScrollUnlock;
    entity->collisionHandler = &dummyCollisionHandler;
    entity->tileCollisionHandler = &dummyTileCollisionHandler;
    entity->overlapTileHandler = &defaultOverlapTileHandler;

    return entity;
}

entity_t* createHitBlock(entityManager_t * entityManager, uint16_t x, uint16_t y)
{
    entity_t * entity = findInactiveEntity(entityManager);

    if(entity == NULL) {
        return NULL;
    }

    entity->active = true;
    entity->visible = true;
    entity->x = x << SUBPIXEL_RESOLUTION;
    entity->y = y << SUBPIXEL_RESOLUTION;
    
    entity->xspeed = 0;
    entity->yspeed = 0;
    entity->yDamping = 0;
    entity->xMaxSpeed = 132;
    entity->yMaxSpeed = 132;
    entity->gravityEnabled = true;
    entity->gravity = 4;

    entity->spriteFlipHorizontal = false;
    entity->spriteFlipVertical = false;

    //entity->type = ENTITY_HIT_BLOCK;
    entity->spriteIndex = SP_HITBLOCK_CONTAINER;
    entity->animationTimer = 0;
    entity->updateFunction = &updateHitBlock;
    entity->collisionHandler = &dummyCollisionHandler;
    entity->tileCollisionHandler = &dummyTileCollisionHandler;
    entity->overlapTileHandler = &defaultOverlapTileHandler;

    return entity;
}

entity_t* createPowerUp(entityManager_t * entityManager, uint16_t x, uint16_t y){
    entity_t * entity = findInactiveEntity(entityManager);

    if(entity == NULL) {
        return NULL;
    }

    entity->active = true;
    entity->visible = true;
    entity->x = x << SUBPIXEL_RESOLUTION;
    entity->y = y << SUBPIXEL_RESOLUTION;
    
    entity->xspeed = (entityManager->playerEntity->x > entity->x)? -16: 16;
    entity->yspeed = 0;
    entity->xMaxSpeed = 132;
    entity->yMaxSpeed = 132;
    entity->gravityEnabled = true;
    entity->gravity = 4;
    entity->spriteFlipHorizontal = false;
    entity->spriteFlipVertical = false;

    //entity->type = ENTITY_POWERUP;
    entity->spriteIndex = (entityManager->playerEntity->hp < 2) ? SP_GAMING_1 : SP_MUSIC_1;
    entity->animationTimer = 0;
    entity->updateFunction = &updatePowerUp;
    entity->collisionHandler = &powerUpCollisionHandler;
    entity->tileCollisionHandler = &enemyTileCollisionHandler;
    entity->fallOffTileHandler = &defaultFallOffTileHandler;
    entity->overlapTileHandler = &defaultOverlapTileHandler;

    return entity;
};

entity_t* createWarp(entityManager_t * entityManager, uint16_t x, uint16_t y){
    entity_t * entity = findInactiveEntity(entityManager);

    if(entity == NULL) {
        return NULL;
    }

    entity->active = true;
    entity->visible = true;
    entity->x = x << SUBPIXEL_RESOLUTION;
    entity->y = y << SUBPIXEL_RESOLUTION;
    
    entity->xspeed = 0;
    entity->yspeed = 0;
    entity->xMaxSpeed = 132;
    entity->yMaxSpeed = 132;
    entity->gravityEnabled = true;
    entity->gravity = 4;

    entity->spriteFlipVertical = false;

    //entity->type = ENTITY_WARP;
    entity->spriteIndex = SP_WARP_1;
    entity->animationTimer = 0;
    entity->updateFunction = &updateWarp;
    entity->collisionHandler = &dummyCollisionHandler;
    entity->tileCollisionHandler = &dummyTileCollisionHandler;
    entity->overlapTileHandler = &defaultOverlapTileHandler;

    return entity;
};

entity_t* createDustBunny(entityManager_t * entityManager, uint16_t x, uint16_t y)
{
    entity_t * entity = findInactiveEntity(entityManager);

    if(entity == NULL) {
        return NULL;
    }

    entity->active = true;
    entity->visible = true;
    entity->x = x << SUBPIXEL_RESOLUTION;
    entity->y = y << SUBPIXEL_RESOLUTION;
    
    entity->xspeed = 0;
    entity->yspeed = 0;
    entity->xMaxSpeed = 132;
    entity->yMaxSpeed = 132;
    entity->xDamping = 0; //This will be repurposed to track state
    entity->yDamping = 0; //This will be repurposed as a state timer
    entity->gravityEnabled = true;
    entity->gravity = 4;
    entity->spriteFlipHorizontal = (x < (entityManager->tilemap->mapOffsetX + 120)) ? true : false;
    entity->spriteFlipVertical = false;

    entity->scoreValue = 150;

    //entity->type = ENTITY_DUST_BUNNY;
    entity->spriteIndex = SP_DUSTBUNNY_IDLE;
    entity->updateFunction = &updateDustBunny;
    entity->collisionHandler = &enemyCollisionHandler;
    entity->tileCollisionHandler = &dustBunnyTileCollisionHandler;
    entity->fallOffTileHandler = &defaultFallOffTileHandler;
    entity->overlapTileHandler = &defaultOverlapTileHandler;

    return entity;
}

entity_t* createWasp(entityManager_t * entityManager, uint16_t x, uint16_t y)
{
    entity_t * entity = findInactiveEntity(entityManager);

    if(entity == NULL) {
        return NULL;
    }

    entity->active = true;
    entity->visible = true;
    entity->x = x << SUBPIXEL_RESOLUTION;
    entity->y = y << SUBPIXEL_RESOLUTION;

    entity->yspeed = 0;
    entity->xMaxSpeed = 132;
    entity->yMaxSpeed = 128;
    entity->xDamping = 0; //This will be repurposed to track state
    entity->yDamping = 0; //This will be repurposed as a state timer
    entity->gravityEnabled = false;
    entity->gravity = 8;
    entity->spriteFlipHorizontal = (x < (entityManager->tilemap->mapOffsetX + 120)) ? false : true;
    entity->spriteFlipVertical = false;
    entity->scoreValue = 200;
        
    entity->xspeed = (entity->spriteFlipHorizontal)? -16 : 16;

    //entity->type = ENTITY_WASP;
    entity->spriteIndex = SP_WASP_1;
    entity->updateFunction = &updateWasp;
    entity->collisionHandler = &enemyCollisionHandler;
    entity->tileCollisionHandler = &waspTileCollisionHandler;
    entity->fallOffTileHandler = &defaultFallOffTileHandler;
    entity->overlapTileHandler = &defaultOverlapTileHandler;

    return entity;
}

entity_t* createEnemyBushL2(entityManager_t * entityManager, uint16_t x, uint16_t y)
{
    entity_t * entity = findInactiveEntity(entityManager);

    if(entity == NULL) {
        return NULL;
    }

    entity->active = true;
    entity->visible = true;
    entity->x = x << SUBPIXEL_RESOLUTION;
    entity->y = y << SUBPIXEL_RESOLUTION;
    
    entity->xspeed = (x < (entityManager->tilemap->mapOffsetX + 120)) ? 12 : -12;
    entity->yspeed = 0;
    entity->xMaxSpeed = 132;
    entity->yMaxSpeed = 132;
    entity->gravityEnabled = true;
    entity->gravity = 4;
    entity->spriteFlipHorizontal = false;
    entity->spriteFlipVertical = false;
    entity->scoreValue = 150;

    //entity->type = ENTITY_BUSH_2;
    entity->spriteIndex = SP_ENEMY_BUSH_L2;
    entity->updateFunction = &updateTestObject;
    entity->collisionHandler = &enemyCollisionHandler;
    entity->tileCollisionHandler = &enemyTileCollisionHandler;
    entity->fallOffTileHandler = &turnAroundAtEdgeOfTileHandler;
    entity->overlapTileHandler = &defaultOverlapTileHandler;

    return entity;
}

entity_t* createEnemyBushL3(entityManager_t * entityManager, uint16_t x, uint16_t y)
{
    entity_t * entity = findInactiveEntity(entityManager);

    if(entity == NULL) {
        return NULL;
    }

    entity->active = true;
    entity->visible = true;
    entity->x = x << SUBPIXEL_RESOLUTION;
    entity->y = y << SUBPIXEL_RESOLUTION;
    
    entity->xspeed = (x < (entityManager->tilemap->mapOffsetX + 120)) ? 11 : -11;
    entity->yspeed = 0;
    entity->xMaxSpeed = 132;
    entity->yMaxSpeed = 132;
    entity->gravityEnabled = true;
    entity->gravity = 4;
    entity->spriteFlipHorizontal = false;
    entity->spriteFlipVertical = false;
    entity->scoreValue = 250;

    entity->yDamping = 20; //This will be repurposed as a state timer

    //entity->type = ENTITY_BUSH_3;
    entity->spriteIndex = SP_ENEMY_BUSH_L3;
    entity->updateFunction = &updateEnemyBushL3;
    entity->collisionHandler = &enemyCollisionHandler;
    entity->tileCollisionHandler = &enemyTileCollisionHandler;
    entity->fallOffTileHandler = &turnAroundAtEdgeOfTileHandler;
    entity->overlapTileHandler = &defaultOverlapTileHandler;

    return entity;
}

entity_t* createDustBunnyL2(entityManager_t * entityManager, uint16_t x, uint16_t y)
{
    entity_t * entity = findInactiveEntity(entityManager);

    if(entity == NULL) {
        return NULL;
    }

    entity->active = true;
    entity->visible = true;
    entity->x = x << SUBPIXEL_RESOLUTION;
    entity->y = y << SUBPIXEL_RESOLUTION;
    
    entity->xspeed = 0;
    entity->yspeed = 0;
    entity->xMaxSpeed = 132;
    entity->yMaxSpeed = 132;
    entity->xDamping = 0; //This will be repurposed to track state
    entity->yDamping = 0; //This will be repurposed as a state timer
    entity->gravityEnabled = true;
    entity->gravity = 4;
    entity->spriteFlipHorizontal = (x < (entityManager->tilemap->mapOffsetX + 120)) ? false : true;
    entity->spriteFlipVertical = false;
    entity->scoreValue = 200;

    //entity->type = ENTITY_DUST_BUNNY_2;
    entity->spriteIndex = SP_DUSTBUNNY_L2_IDLE;
    entity->updateFunction = &updateDustBunnyL2;
    entity->collisionHandler = &enemyCollisionHandler;
    entity->tileCollisionHandler = &dustBunnyL2TileCollisionHandler;
    entity->fallOffTileHandler = &defaultFallOffTileHandler;
    entity->overlapTileHandler = &defaultOverlapTileHandler;

    return entity;
}

entity_t* createDustBunnyL3(entityManager_t * entityManager, uint16_t x, uint16_t y)
{
    entity_t * entity = findInactiveEntity(entityManager);

    if(entity == NULL) {
        return NULL;
    }

    entity->active = true;
    entity->visible = true;
    entity->x = x << SUBPIXEL_RESOLUTION;
    entity->y = y << SUBPIXEL_RESOLUTION;
    
    entity->xspeed = 0;
    entity->yspeed = 0;
    entity->xMaxSpeed = 132;
    entity->yMaxSpeed = 132;
    entity->xDamping = 0; //This will be repurposed to track state
    entity->yDamping = 0; //This will be repurposed as a state timer
    entity->gravityEnabled = true;
    entity->gravity = 4;
    entity->spriteFlipHorizontal = (x < (entityManager->tilemap->mapOffsetX + 120)) ? true : false;
    entity->spriteFlipVertical = false;
    entity->scoreValue = 300;

    //entity->type = ENTITY_DUST_BUNNY_3;
    entity->spriteIndex = SP_DUSTBUNNY_L3_IDLE;
    entity->updateFunction = &updateDustBunnyL3;
    entity->collisionHandler = &enemyCollisionHandler;
    entity->tileCollisionHandler = &dustBunnyL3TileCollisionHandler;
    entity->fallOffTileHandler = &defaultFallOffTileHandler;
    entity->overlapTileHandler = &defaultOverlapTileHandler;

    return entity;
}

entity_t* createWaspL2(entityManager_t * entityManager, uint16_t x, uint16_t y)
{
    entity_t * entity = findInactiveEntity(entityManager);

    if(entity == NULL) {
        return NULL;
    }

    entity->active = true;
    entity->visible = true;
    entity->x = x << SUBPIXEL_RESOLUTION;
    entity->y = y << SUBPIXEL_RESOLUTION;

    entity->yspeed = 0;
    entity->xMaxSpeed = 132;
    entity->yMaxSpeed = 192;
    entity->xDamping = 0; //This will be repurposed to track state
    entity->yDamping = 0; //This will be repurposed as a state timer
    entity->jumpPower = (1 + esp_random() % 4) * 256;
    entity->gravityEnabled = false;
    entity->gravity = 8;
    entity->spriteFlipHorizontal = (x < (entityManager->tilemap->mapOffsetX + 120)) ? false : true;
    entity->spriteFlipVertical = false;
    entity->falling = false;
    entity->scoreValue = 300;
    
    entity->xspeed = (entity->spriteFlipHorizontal)? -24 : 24;

    //entity->type = ENTITY_WASP_2;
    entity->spriteIndex = SP_WASP_L2_1;
    entity->updateFunction = &updateWaspL2;
    entity->collisionHandler = &enemyCollisionHandler;
    entity->tileCollisionHandler = &waspTileCollisionHandler;
    entity->fallOffTileHandler = &defaultFallOffTileHandler;
    entity->overlapTileHandler = &defaultOverlapTileHandler;

    return entity;
}

entity_t* createWaspL3(entityManager_t * entityManager, uint16_t x, uint16_t y)
{
    entity_t * entity = findInactiveEntity(entityManager);

    if(entity == NULL) {
        return NULL;
    }

    entity->active = true;
    entity->visible = true;
    entity->x = x << SUBPIXEL_RESOLUTION;
    entity->y = y << SUBPIXEL_RESOLUTION;

    entity->yspeed = 0;
    entity->xMaxSpeed = 132;
    entity->yMaxSpeed = 256;
    entity->xDamping = 0; //This will be repurposed to track state
    entity->yDamping = 0; //This will be repurposed as a state timer
    entity->jumpPower = (1 + esp_random() % 4) * 256;
    entity->gravityEnabled = false;
    entity->gravity = 8;
    entity->spriteFlipHorizontal = (x < (entityManager->tilemap->mapOffsetX + 120)) ? false : true;
    entity->spriteFlipVertical = false;
    entity->scoreValue = 400;
        
    entity->xspeed = (entity->spriteFlipHorizontal)? -24 : 24;

    //entity->type = ENTITY_WASP_3;
    entity->spriteIndex = SP_WASP_L3_1;
    entity->updateFunction = &updateWaspL3;
    entity->collisionHandler = &enemyCollisionHandler;
    entity->tileCollisionHandler = &waspTileCollisionHandler;
    entity->fallOffTileHandler = &defaultFallOffTileHandler;
    entity->overlapTileHandler = &defaultOverlapTileHandler;

    return entity;
}

entity_t* createBgColBlue(entityManager_t * entityManager, uint16_t x, uint16_t y)
{
    entity_t * entity = findInactiveEntity(entityManager);

    if(entity == NULL) {
        return NULL;
    }

    entity->active = true;
    entity->visible = false;
    entity->x = x << SUBPIXEL_RESOLUTION;
    entity->y = y << SUBPIXEL_RESOLUTION;
    entity->xDamping = c335;
    
    //entity->type = ENTITY_BGCOL_BLUE;
    entity->updateFunction = &updateBgCol;
    entity->collisionHandler = &dummyCollisionHandler;
    entity->tileCollisionHandler = &dummyTileCollisionHandler;
    entity->overlapTileHandler = &defaultOverlapTileHandler;

    return entity;
}

entity_t* createBgColYellow(entityManager_t * entityManager, uint16_t x, uint16_t y)
{
    entity_t * entity = findInactiveEntity(entityManager);

    if(entity == NULL) {
        return NULL;
    }

    entity->active = true;
    entity->visible = false;
    entity->x = x << SUBPIXEL_RESOLUTION;
    entity->y = y << SUBPIXEL_RESOLUTION;
    entity->xDamping = c542;
    
    //entity->type = ENTITY_BGCOL_YELLOW;
    entity->updateFunction = &updateBgCol;
    entity->collisionHandler = &dummyCollisionHandler;
    entity->tileCollisionHandler = &dummyTileCollisionHandler;
    entity->overlapTileHandler = &defaultOverlapTileHandler;

    return entity;
}

entity_t* createBgColOrange(entityManager_t * entityManager, uint16_t x, uint16_t y)
{
    entity_t * entity = findInactiveEntity(entityManager);

    if(entity == NULL) {
        return NULL;
    }

    entity->active = true;
    entity->visible = false;
    entity->x = x << SUBPIXEL_RESOLUTION;
    entity->y = y << SUBPIXEL_RESOLUTION;
    entity->xDamping = c532;

    //entity->type = ENTITY_BGCOL_ORANGE;
    entity->updateFunction = &updateBgCol;
    entity->collisionHandler = &dummyCollisionHandler;
    entity->tileCollisionHandler = &dummyTileCollisionHandler;
    entity->overlapTileHandler = &defaultOverlapTileHandler;

    return entity;
}

entity_t* createBgColPurple(entityManager_t * entityManager, uint16_t x, uint16_t y)
{
    entity_t * entity = findInactiveEntity(entityManager);

    if(entity == NULL) {
        return NULL;
    }

    entity->active = true;
    entity->visible = false;
    entity->x = x << SUBPIXEL_RESOLUTION;
    entity->y = y << SUBPIXEL_RESOLUTION;
    entity->xDamping = c214;
    
    //entity->type = ENTITY_BGCOL_PURPLE;
    entity->updateFunction = &updateBgCol;
    entity->collisionHandler = &dummyCollisionHandler;
    entity->tileCollisionHandler = &dummyTileCollisionHandler;
    entity->overlapTileHandler = &defaultOverlapTileHandler;

    return entity;
}

entity_t* createBgColDarkPurple(entityManager_t * entityManager, uint16_t x, uint16_t y)
{
    entity_t * entity = findInactiveEntity(entityManager);

    if(entity == NULL) {
        return NULL;
    }

    entity->active = true;
    entity->visible = false;
    entity->x = x << SUBPIXEL_RESOLUTION;
    entity->y = y << SUBPIXEL_RESOLUTION;
    entity->xDamping = c103;

    //entity->type = ENTITY_BGCOL_DARK_PURPLE;
    entity->updateFunction = &updateBgCol;
    entity->collisionHandler = &dummyCollisionHandler;
    entity->tileCollisionHandler = &dummyTileCollisionHandler;
    entity->overlapTileHandler = &defaultOverlapTileHandler;

    return entity;
}

entity_t* createBgColBlack(entityManager_t * entityManager, uint16_t x, uint16_t y)
{
    entity_t * entity = findInactiveEntity(entityManager);

    if(entity == NULL) {
        return NULL;
    }

    entity->active = true;
    entity->visible = false;
    entity->x = x << SUBPIXEL_RESOLUTION;
    entity->y = y << SUBPIXEL_RESOLUTION;
    entity->xDamping = c000;


    //entity->type = ENTITY_BGCOL_BLACK;
    entity->updateFunction = &updateBgCol;
    entity->collisionHandler = &dummyCollisionHandler;
    entity->tileCollisionHandler = &dummyTileCollisionHandler;
    entity->overlapTileHandler = &defaultOverlapTileHandler;

    return entity;
}

entity_t* createBgColNeutralGreen(entityManager_t * entityManager, uint16_t x, uint16_t y)
{
    entity_t * entity = findInactiveEntity(entityManager);

    if(entity == NULL) {
        return NULL;
    }

    entity->active = true;
    entity->visible = false;
    entity->x = x << SUBPIXEL_RESOLUTION;
    entity->y = y << SUBPIXEL_RESOLUTION;
    entity->xDamping = c133;

    //entity->type = ENTITY_BGCOL_NEUTRAL_GREEN;
    entity->updateFunction = &updateBgCol;
    entity->collisionHandler = &dummyCollisionHandler;
    entity->tileCollisionHandler = &dummyTileCollisionHandler;
    entity->overlapTileHandler = &defaultOverlapTileHandler;

    return entity;
}

entity_t* createBgColNeutralDarkRed(entityManager_t * entityManager, uint16_t x, uint16_t y)
{
    entity_t * entity = findInactiveEntity(entityManager);

    if(entity == NULL) {
        return NULL;
    }

    entity->active = true;
    entity->visible = false;
    entity->x = x << SUBPIXEL_RESOLUTION;
    entity->y = y << SUBPIXEL_RESOLUTION;
    entity->xDamping = c200;
    
    //entity->type = ENTITY_BGCOL_DARK_RED;
    entity->updateFunction = &updateBgCol;
    entity->collisionHandler = &dummyCollisionHandler;
    entity->tileCollisionHandler = &dummyTileCollisionHandler;
    entity->overlapTileHandler = &defaultOverlapTileHandler;

    return entity;
}

entity_t* createBgColNeutralDarkGreen(entityManager_t * entityManager, uint16_t x, uint16_t y)
{
    entity_t * entity = findInactiveEntity(entityManager);

    if(entity == NULL) {
        return NULL;
    }

    entity->active = true;
    entity->visible = false;
    entity->x = x << SUBPIXEL_RESOLUTION;
    entity->y = y << SUBPIXEL_RESOLUTION;
     entity->xDamping = c010;

    //entity->type = ENTITY_BGCOL_DARK_GREEN;
    entity->updateFunction = &updateBgCol;
    entity->collisionHandler = &dummyCollisionHandler;
    entity->tileCollisionHandler = &dummyTileCollisionHandler;
    entity->overlapTileHandler = &defaultOverlapTileHandler;

    return entity;
}

entity_t* create1up(entityManager_t * entityManager, uint16_t x, uint16_t y){
    entity_t * entity = findInactiveEntity(entityManager);

    if(entity == NULL) {
        return NULL;
    }

    entity->active = true;
    entity->visible = true;
    entity->x = x << SUBPIXEL_RESOLUTION;
    entity->y = y << SUBPIXEL_RESOLUTION;
    
    entity->xspeed = (entityManager->playerEntity->x > entity->x)? -16: 16;
    entity->yspeed = 0;
    entity->xMaxSpeed = 132;
    entity->yMaxSpeed = 132;
    entity->gravityEnabled = true;
    entity->gravity = 4;
    entity->spriteFlipHorizontal = false;
    entity->spriteFlipVertical = false;

    //entity->type = ENTITY_1UP;
    entity->spriteIndex = SP_1UP_1;
    entity->animationTimer = 0;
    entity->updateFunction = &update1up;
    entity->collisionHandler = &powerUpCollisionHandler;
    entity->tileCollisionHandler = &enemyTileCollisionHandler;
    entity->fallOffTileHandler = &defaultFallOffTileHandler;
    entity->overlapTileHandler = &defaultOverlapTileHandler;

    return entity;
};

entity_t* createWaveBall(entityManager_t * entityManager, uint16_t x, uint16_t y){
    entity_t * entity = findInactiveEntity(entityManager);

    if(entity == NULL) {
        return NULL;
    }

    entity->active = true;
    entity->visible = true;
    entity->x = x << SUBPIXEL_RESOLUTION;
    entity->y = y << SUBPIXEL_RESOLUTION;
    
    entity->xspeed = 0;
    entity->yspeed = 0;
    entity->xMaxSpeed = 132;
    entity->yMaxSpeed = 132;
    entity->gravityEnabled = false;
    entity->gravity = 4;
    entity->spriteFlipHorizontal = false;
    entity->spriteFlipVertical = false;
    entity->yDamping = 3; //This will be repurposed as a state timer
    entity->xDamping = 0; //This will be repurposed as a state tracker

    //entity->type = ENTITY_WAVE_BALL;
    entity->spriteIndex = SP_WAVEBALL_1;
    entity->animationTimer = 0;
    entity->updateFunction = &updateWaveBall;
    entity->collisionHandler = &dummyCollisionHandler;
    entity->tileCollisionHandler = &dummyTileCollisionHandler;
    entity->fallOffTileHandler = &defaultFallOffTileHandler;
    entity->overlapTileHandler = &waveBallOverlapTileHandler;

    return entity;
};

entity_t* createCheckpoint(entityManager_t * entityManager, uint16_t x, uint16_t y){
    entity_t * entity = findInactiveEntity(entityManager);

    if(entity == NULL) {
        return NULL;
    }

    entity->active = true;
    entity->visible = true;
    entity->x = x << SUBPIXEL_RESOLUTION;
    entity->y = y << SUBPIXEL_RESOLUTION;
    
    entity->xspeed = 0;
    entity->yspeed = 0;
    entity->xMaxSpeed = 132;
    entity->yMaxSpeed = 132;
    entity->gravityEnabled = true;
    entity->gravity = 4;
    entity->spriteFlipHorizontal = false;
    entity->spriteFlipVertical = false;

    entity->xDamping = 0; //State of the checkpoint. 0 = inactive, 1 = active

    //entity->type = ENTITY_CHECKPOINT;
    entity->spriteIndex = SP_CHECKPOINT_INACTIVE;
    entity->animationTimer = 0;
    entity->updateFunction = &updateCheckpoint;
    entity->collisionHandler = &dummyCollisionHandler;
    entity->tileCollisionHandler = &dummyTileCollisionHandler;
    entity->overlapTileHandler = &defaultOverlapTileHandler;

    return entity;
};
*/
void freeEntityManager(entityManager_t * self){
    free(self->entities);
    for(uint8_t i=0; i<SPRITESET_SIZE; i++){
        freeWsg(&(self->sprites[i].wsg));
    }
}
/*
entity_t* createBgmChange1(entityManager_t * entityManager, uint16_t x, uint16_t y)
{
    entity_t * entity = findInactiveEntity(entityManager);

    if(entity == NULL) {
        return NULL;
    }

    entity->active = true;
    entity->visible = false;
    entity->x = x << SUBPIXEL_RESOLUTION;
    entity->y = y << SUBPIXEL_RESOLUTION;
    entity->xDamping = BGM_MAIN;
    
    //entity->type = ENTITY_BGM_CHANGE_1;
    entity->updateFunction = &updateBgmChange;
    entity->collisionHandler = &dummyCollisionHandler;
    entity->tileCollisionHandler = &dummyTileCollisionHandler;
    entity->overlapTileHandler = &defaultOverlapTileHandler;

    return entity;
}

entity_t* createBgmChange2(entityManager_t * entityManager, uint16_t x, uint16_t y)
{
    entity_t * entity = findInactiveEntity(entityManager);

    if(entity == NULL) {
        return NULL;
    }

    entity->active = true;
    entity->visible = false;
    entity->x = x << SUBPIXEL_RESOLUTION;
    entity->y = y << SUBPIXEL_RESOLUTION;
    entity->xDamping = BGM_ATHLETIC;
    
    //entity->type = ENTITY_BGM_CHANGE_2;
    entity->updateFunction = &updateBgmChange;
    entity->collisionHandler = &dummyCollisionHandler;
    entity->tileCollisionHandler = &dummyTileCollisionHandler;
    entity->overlapTileHandler = &defaultOverlapTileHandler;

    return entity;
}

entity_t* createBgmChange3(entityManager_t * entityManager, uint16_t x, uint16_t y)
{
    entity_t * entity = findInactiveEntity(entityManager);

    if(entity == NULL) {
        return NULL;
    }

    entity->active = true;
    entity->visible = false;
    entity->x = x << SUBPIXEL_RESOLUTION;
    entity->y = y << SUBPIXEL_RESOLUTION;
    entity->xDamping = BGM_UNDERGROUND;
    
    //entity->type = ENTITY_BGM_CHANGE_3;
    entity->updateFunction = &updateBgmChange;
    entity->collisionHandler = &dummyCollisionHandler;
    entity->tileCollisionHandler = &dummyTileCollisionHandler;
    entity->overlapTileHandler = &defaultOverlapTileHandler;

    return entity;
}

entity_t* createBgmChange4(entityManager_t * entityManager, uint16_t x, uint16_t y)
{
    entity_t * entity = findInactiveEntity(entityManager);

    if(entity == NULL) {
        return NULL;
    }

    entity->active = true;
    entity->visible = false;
    entity->x = x << SUBPIXEL_RESOLUTION;
    entity->y = y << SUBPIXEL_RESOLUTION;
    entity->xDamping = BGM_FORTRESS;
    
    //entity->type = ENTITY_BGM_CHANGE_4;
    entity->updateFunction = &updateBgmChange;
    entity->collisionHandler = &dummyCollisionHandler;
    entity->tileCollisionHandler = &dummyTileCollisionHandler;
    entity->overlapTileHandler = &defaultOverlapTileHandler;

    return entity;
}

entity_t* createBgmChange5(entityManager_t * entityManager, uint16_t x, uint16_t y)
{
    entity_t * entity = findInactiveEntity(entityManager);

    if(entity == NULL) {
        return NULL;
    }

    entity->active = true;
    entity->visible = false;
    entity->x = x << SUBPIXEL_RESOLUTION;
    entity->y = y << SUBPIXEL_RESOLUTION;
    entity->xDamping = BGM_NULL;
    
    //entity->type = ENTITY_BGM_CHANGE_5;
    entity->updateFunction = &updateBgmChange;
    entity->collisionHandler = &dummyCollisionHandler;
    entity->tileCollisionHandler = &dummyTileCollisionHandler;
    entity->overlapTileHandler = &defaultOverlapTileHandler;

    return entity;
}

entity_t* createBgmStop(entityManager_t * entityManager, uint16_t x, uint16_t y)
{
    entity_t * entity = findInactiveEntity(entityManager);

    if(entity == NULL) {
        return NULL;
    }

    entity->active = true;
    entity->visible = false;
    entity->x = x << SUBPIXEL_RESOLUTION;
    entity->y = y << SUBPIXEL_RESOLUTION;
    entity->xDamping = BGM_NULL;
    
    //entity->type = ENTITY_BGM_STOP;
    entity->updateFunction = &updateBgmChange;
    entity->collisionHandler = &dummyCollisionHandler;
    entity->tileCollisionHandler = &dummyTileCollisionHandler;
    entity->overlapTileHandler = &defaultOverlapTileHandler;

    return entity;
}
*/