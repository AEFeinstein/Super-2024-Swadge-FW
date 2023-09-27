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
void initializeEntityManager(entityManager_t * entityManager, tilemap_t * tilemap, gameData_t * gameData, soundManager_t * soundManager)
{
    loadSprites(entityManager);
    entityManager->entities = calloc(MAX_ENTITIES, sizeof(entity_t));
    
    for(uint8_t i=0; i < MAX_ENTITIES; i++)
    {
        initializeEntity(&(entityManager->entities[i]), entityManager, tilemap, gameData, soundManager);
    }

    entityManager->activeEntities = 0;
    entityManager->tilemap = tilemap;

    
    //entityManager->viewEntity = createPlayer(entityManager, entityManager->tilemap->warps[0].x * 16, entityManager->tilemap->warps[0].y * 16);
    //entityManager->playerEntity = entityManager->viewEntity;
};

void loadSprites(entityManager_t * entityManager)
{
    loadWsg("paddle000.wsg", &(entityManager->sprites[SP_PADDLE_0].wsg), false);
    entityManager->sprites[SP_PADDLE_0].originX=14;
    entityManager->sprites[SP_PADDLE_0].originY=4;
    entityManager->sprites[SP_PADDLE_0].collisionBox.x0 = 0;
    entityManager->sprites[SP_PADDLE_0].collisionBox.x1 = 27;
    entityManager->sprites[SP_PADDLE_0].collisionBox.y0 = 0;
    entityManager->sprites[SP_PADDLE_0].collisionBox.y1 = 7;

    loadWsg("paddle001.wsg", &entityManager->sprites[SP_PADDLE_1].wsg, false);
    entityManager->sprites[SP_PADDLE_1].originX=14;
    entityManager->sprites[SP_PADDLE_1].originY=4;
    entityManager->sprites[SP_PADDLE_1].collisionBox.x0 = 0;
    entityManager->sprites[SP_PADDLE_1].collisionBox.x1 = 27;
    entityManager->sprites[SP_PADDLE_1].collisionBox.y0 = 0;
    entityManager->sprites[SP_PADDLE_1].collisionBox.y1 = 7;

    loadWsg("paddle002.wsg", &entityManager->sprites[SP_PADDLE_2].wsg, false);
    entityManager->sprites[SP_PADDLE_2].originX=14;
    entityManager->sprites[SP_PADDLE_2].originY=4;
    entityManager->sprites[SP_PADDLE_2].collisionBox.x0 = 0;
    entityManager->sprites[SP_PADDLE_2].collisionBox.x1 = 27;
    entityManager->sprites[SP_PADDLE_2].collisionBox.y0 = 0;
    entityManager->sprites[SP_PADDLE_2].collisionBox.y1 = 7;

    loadWsg("paddleVertical000.wsg", &entityManager->sprites[SP_PADDLE_VERTICAL_0].wsg, false);
    entityManager->sprites[SP_PADDLE_VERTICAL_0].originX=4;
    entityManager->sprites[SP_PADDLE_VERTICAL_0].originY=14;
    entityManager->sprites[SP_PADDLE_VERTICAL_0].collisionBox.x0 = 0;
    entityManager->sprites[SP_PADDLE_VERTICAL_0].collisionBox.x1 = 7;
    entityManager->sprites[SP_PADDLE_VERTICAL_0].collisionBox.y0 = 0;
    entityManager->sprites[SP_PADDLE_VERTICAL_0].collisionBox.y1 = 27;

    loadWsg("paddleVertical001.wsg", &entityManager->sprites[SP_PADDLE_VERTICAL_1].wsg, false);
    entityManager->sprites[SP_PADDLE_VERTICAL_1].originX=4;
    entityManager->sprites[SP_PADDLE_VERTICAL_1].originY=14;
    entityManager->sprites[SP_PADDLE_VERTICAL_1].collisionBox.x0 = 0;
    entityManager->sprites[SP_PADDLE_VERTICAL_1].collisionBox.x1 = 7;
    entityManager->sprites[SP_PADDLE_VERTICAL_1].collisionBox.y0 = 0;
    entityManager->sprites[SP_PADDLE_VERTICAL_1].collisionBox.y1 = 27;

    loadWsg("paddleVertical002.wsg", &entityManager->sprites[SP_PADDLE_VERTICAL_2].wsg, false);
    entityManager->sprites[SP_PADDLE_VERTICAL_2].originX=4;
    entityManager->sprites[SP_PADDLE_VERTICAL_2].originY=14;
    entityManager->sprites[SP_PADDLE_VERTICAL_2].collisionBox.x0 = 0;
    entityManager->sprites[SP_PADDLE_VERTICAL_2].collisionBox.x1 = 7;
    entityManager->sprites[SP_PADDLE_VERTICAL_2].collisionBox.y0 = 0;
    entityManager->sprites[SP_PADDLE_VERTICAL_2].collisionBox.y1 = 27;

    loadWsg("ball.wsg", &entityManager->sprites[SP_BALL].wsg, false);
    entityManager->sprites[SP_BALL].originX=4;
    entityManager->sprites[SP_BALL].originY=4;
    entityManager->sprites[SP_BALL].collisionBox.x0 = 0;
    entityManager->sprites[SP_BALL].collisionBox.x1 = 7;
    entityManager->sprites[SP_BALL].collisionBox.y0 = 0;
    entityManager->sprites[SP_BALL].collisionBox.y1 = 7;

    loadWsg("dbmb000.wsg", &entityManager->sprites[SP_BOMB_0].wsg, false);
    entityManager->sprites[SP_BOMB_0].originX=4;
    entityManager->sprites[SP_BOMB_0].originY=4;
    entityManager->sprites[SP_BOMB_0].collisionBox.x0 = 0;
    entityManager->sprites[SP_BOMB_0].collisionBox.x1 = 7;
    entityManager->sprites[SP_BOMB_0].collisionBox.y0 = 0;
    entityManager->sprites[SP_BOMB_0].collisionBox.y1 = 7;

    loadWsg("dbmb001.wsg", &entityManager->sprites[SP_BOMB_1].wsg, false);
    entityManager->sprites[SP_BOMB_1].originX=4;
    entityManager->sprites[SP_BOMB_1].originY=4;
    entityManager->sprites[SP_BOMB_1].collisionBox.x0 = 0;
    entityManager->sprites[SP_BOMB_1].collisionBox.x1 = 7;
    entityManager->sprites[SP_BOMB_1].collisionBox.y0 = 0;
    entityManager->sprites[SP_BOMB_1].collisionBox.y1 = 7;

    loadWsg("dbmb002.wsg", &entityManager->sprites[SP_BOMB_2].wsg, false);
    entityManager->sprites[SP_BOMB_2].originX=4;
    entityManager->sprites[SP_BOMB_2].originY=4;
    entityManager->sprites[SP_BOMB_2].collisionBox.x0 = 0;
    entityManager->sprites[SP_BOMB_2].collisionBox.x1 = 7;
    entityManager->sprites[SP_BOMB_2].collisionBox.y0 = 0;
    entityManager->sprites[SP_BOMB_2].collisionBox.y1 = 7;

    loadWsg("boom000.wsg", &entityManager->sprites[SP_EXPLOSION_0].wsg, false);
    entityManager->sprites[SP_EXPLOSION_0].originX=20;
    entityManager->sprites[SP_EXPLOSION_0].originY=20;
    entityManager->sprites[SP_EXPLOSION_0].collisionBox.x0 = 0;
    entityManager->sprites[SP_EXPLOSION_0].collisionBox.x1 = 39;
    entityManager->sprites[SP_EXPLOSION_0].collisionBox.y0 = 0;
    entityManager->sprites[SP_EXPLOSION_0].collisionBox.y1 = 39;

    loadWsg("boom001.wsg", &entityManager->sprites[SP_EXPLOSION_1].wsg, false);
    entityManager->sprites[SP_EXPLOSION_1].originX=20;
    entityManager->sprites[SP_EXPLOSION_1].originY=20;
    entityManager->sprites[SP_EXPLOSION_1].collisionBox.x0 = 0;
    entityManager->sprites[SP_EXPLOSION_1].collisionBox.x1 = 39;
    entityManager->sprites[SP_EXPLOSION_1].collisionBox.y0 = 0;
    entityManager->sprites[SP_EXPLOSION_1].collisionBox.y1 = 39;

    loadWsg("boom002.wsg", &entityManager->sprites[SP_EXPLOSION_2].wsg, false);
    entityManager->sprites[SP_EXPLOSION_2].originX=20;
    entityManager->sprites[SP_EXPLOSION_2].originY=20;
    entityManager->sprites[SP_EXPLOSION_2].collisionBox.x0 = 0;
    entityManager->sprites[SP_EXPLOSION_2].collisionBox.x1 = 39;
    entityManager->sprites[SP_EXPLOSION_2].collisionBox.y0 = 0;
    entityManager->sprites[SP_EXPLOSION_2].collisionBox.y1 = 39;

    loadWsg("boom003.wsg", &entityManager->sprites[SP_EXPLOSION_3].wsg, false);
    entityManager->sprites[SP_EXPLOSION_3].originX=20;
    entityManager->sprites[SP_EXPLOSION_3].originY=20;
    entityManager->sprites[SP_EXPLOSION_3].collisionBox.x0 = 0;
    entityManager->sprites[SP_EXPLOSION_3].collisionBox.x1 = 39;
    entityManager->sprites[SP_EXPLOSION_3].collisionBox.y0 = 0;
    entityManager->sprites[SP_EXPLOSION_3].collisionBox.y1 = 39;
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

void deactivateAllEntities(entityManager_t * entityManager, bool excludePlayer, bool respawn){
    for(uint8_t i=0; i < MAX_ENTITIES; i++)
    {
        entity_t* currentEntity = &(entityManager->entities[i]);
        if(!currentEntity->active){
            continue;
        }
        
        destroyEntity(currentEntity, respawn);
 
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
            drawWsg(&(entityManager->sprites[currentEntity.spriteIndex].wsg), (currentEntity.x >> SUBPIXEL_RESOLUTION) - entityManager->sprites[currentEntity.spriteIndex].originX - entityManager->tilemap->mapOffsetX, (currentEntity.y >> SUBPIXEL_RESOLUTION)  - entityManager->tilemap->mapOffsetY - entityManager->sprites[currentEntity.spriteIndex].originY, currentEntity.spriteFlipHorizontal, currentEntity.spriteFlipVertical, currentEntity.spriteRotateAngle);
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
        if(entityIndex >= MAX_ENTITIES)
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
    if(entityManager->activeEntities == MAX_ENTITIES){
        return NULL;
    }

    entity_t *createdEntity;

    switch(objectIndex){
        case ENTITY_PLAYER_PADDLE_BOTTOM:
            createdEntity = createPlayer(entityManager, x, y);
            break;
        case ENTITY_PLAYER_PADDLE_TOP:
            createdEntity = createPlayerPaddleTop(entityManager, x, y);
            break;
        case ENTITY_PLAYER_PADDLE_LEFT:
            createdEntity = createPlayerPaddleLeft(entityManager, x, y);
            break;
        case ENTITY_PLAYER_PADDLE_RIGHT:
            createdEntity = createPlayerPaddleRight(entityManager, x, y);
            break;
        case ENTITY_PLAYER_BALL:
            createdEntity = createBall(entityManager, x, y);
            break;
        case ENTITY_PLAYER_BOMB:
            createdEntity = createBomb(entityManager, x, y);
            break;
        case ENTITY_PLAYER_BOMB_EXPLOSION:
            createdEntity = createExplosion(entityManager, x, y);
            break;
        default:
            createdEntity = NULL;
    }

    if(createdEntity != NULL) {
        entityManager->activeEntities++;
    }

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
    entity->jumpPower = 0;
    entity->spriteFlipVertical = false;
    entity->spriteRotateAngle = 0;
    entity->hp = 1;
    entity->animationTimer = 0;

    entity->type = ENTITY_PLAYER_PADDLE_BOTTOM;
    entity->spriteIndex = SP_PADDLE_0;
    entity->updateFunction = &updatePlayer;
    entity->collisionHandler = &playerCollisionHandler;
    entity->tileCollisionHandler = &playerTileCollisionHandler;
    entity->overlapTileHandler = &playerOverlapTileHandler;
    return entity;
}

entity_t* createPlayerPaddleTop(entityManager_t * entityManager, uint16_t x, uint16_t y)
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
    entity->jumpPower = 0;
    entity->spriteFlipVertical = true;
    entity->spriteRotateAngle = 0;
    entity->hp = 1;
    entity->animationTimer = 0; //Used as a cooldown for shooting square wave balls

    entity->type = ENTITY_PLAYER_PADDLE_TOP;
    entity->spriteIndex = SP_PADDLE_0;
    entity->updateFunction = &updatePlayer;
    entity->collisionHandler = &playerCollisionHandler;
    entity->tileCollisionHandler = &playerTileCollisionHandler;
    entity->overlapTileHandler = &playerOverlapTileHandler;
    return entity;
}

entity_t* createPlayerPaddleLeft(entityManager_t * entityManager, uint16_t x, uint16_t y)
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
    entity->jumpPower = 0;
    entity->spriteFlipHorizontal = true;
    entity->spriteFlipVertical = false;
    entity->spriteRotateAngle = 0;
    entity->hp = 1;
    entity->animationTimer = 0; //Used as a cooldown for shooting square wave balls

    entity->type = ENTITY_PLAYER_PADDLE_LEFT;
    entity->spriteIndex = SP_PADDLE_VERTICAL_0;
    entity->updateFunction = &updatePlayerVertical;
    entity->collisionHandler = &playerCollisionHandler;
    entity->tileCollisionHandler = &playerTileCollisionHandler;
    entity->overlapTileHandler = &playerOverlapTileHandler;
    return entity;
}

entity_t* createPlayerPaddleRight(entityManager_t * entityManager, uint16_t x, uint16_t y)
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
    entity->jumpPower = 0;
    entity->spriteFlipHorizontal = false;
    entity->spriteFlipVertical = false;
    entity->spriteRotateAngle = 0;
    entity->hp = 1;
    entity->animationTimer = 0; //Used as a cooldown for shooting square wave balls

    entity->type = ENTITY_PLAYER_PADDLE_RIGHT;
    entity->spriteIndex = SP_PADDLE_VERTICAL_0;
    entity->updateFunction = &updatePlayerVertical;
    entity->collisionHandler = &playerCollisionHandler;
    entity->tileCollisionHandler = &playerTileCollisionHandler;
    entity->overlapTileHandler = &playerOverlapTileHandler;
    return entity;
}

entity_t* createBall(entityManager_t * entityManager, uint16_t x, uint16_t y)
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
    entity->spriteFlipHorizontal = false;
    entity->spriteFlipVertical = false;
    entity->spriteRotateAngle = 0;
    entity->scoreValue = 100;
    entity->shouldAdvanceMultiplier = false;

    entity->type = ENTITY_PLAYER_BALL;
    entity->spriteIndex = SP_BALL;
    entity->updateFunction = &updateBallAtStart;
    entity->collisionHandler = &dummyCollisionHandler;
    entity->tileCollisionHandler = &ballTileCollisionHandler;
    entity->overlapTileHandler = &ballOverlapTileHandler;

    return entity;
}

entity_t* createBomb(entityManager_t * entityManager, uint16_t x, uint16_t y)
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
    entity->spriteFlipHorizontal = false;
    entity->spriteFlipVertical = false;
    entity->spriteRotateAngle = 0;
    entity->scoreValue = 100;
    entity->animationTimer = 48;

    entity->type = ENTITY_PLAYER_BOMB;
    entity->spriteIndex = SP_BOMB_0;
    entity->updateFunction = &updateBomb;
    entity->collisionHandler = &dummyCollisionHandler;
    entity->tileCollisionHandler = &dummyTileCollisionHandler;
    entity->overlapTileHandler = &defaultOverlapTileHandler;

    //Entity cannot be respawned from the tilemap
    entity->homeTileX = 0;
    entity->homeTileY = 0;

    return entity;
}

entity_t* createExplosion(entityManager_t * entityManager, uint16_t x, uint16_t y)
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
    entity->spriteFlipHorizontal = false;
    entity->spriteFlipVertical = false;
    entity->spriteRotateAngle = 0;
    entity->scoreValue = 100;

    entity->type = ENTITY_PLAYER_BOMB_EXPLOSION;
    entity->spriteIndex = SP_EXPLOSION_0;
    entity->updateFunction = &updateExplosion;
    entity->collisionHandler = &dummyCollisionHandler;
    entity->tileCollisionHandler = &dummyTileCollisionHandler;
    entity->overlapTileHandler = &defaultOverlapTileHandler;

    //Entity cannot be respawned from the tilemap
    entity->homeTileX = 0;
    entity->homeTileY = 0;

    return entity;
}

void freeEntityManager(entityManager_t * self){
    free(self->entities);
    for(uint8_t i=0; i<SPRITESET_SIZE; i++){
        freeWsg(&(self->sprites[i].wsg));
    }
}
