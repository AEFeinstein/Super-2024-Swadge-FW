//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include "entity.h"
#include "entityManager.h"
#include "tilemap.h"
#include "gameData.h"
#include "hdw-bzr.h"
#include "hdw-btn.h"
#include "esp_random.h"
#include "aabb_utils.h"
#include "trigonometry.h"
#include <esp_log.h>

//==============================================================================
// Constants
//==============================================================================
#define SUBPIXEL_RESOLUTION 4
#define TILE_SIZE_IN_POWERS_OF_2 3
#define TILE_SIZE 8
#define HALF_TILE_SIZE 4
#define DESPAWN_THRESHOLD 64

#define SIGNOF(x) ((x > 0) - (x < 0))
#define TO_TILE_COORDS(x) ((x) >> TILE_SIZE_IN_POWERS_OF_2)
// #define TO_PIXEL_COORDS(x) ((x) >> SUBPIXEL_RESOLUTION)
// #define TO_SUBPIXEL_COORDS(x) ((x) << SUBPIXEL_RESOLUTION)

//==============================================================================
// Look Up Tables
//==============================================================================

#define BOMB_EXPLOSION_TILE_CHECK_OFFSET_LENGTH 74
static const int16_t bombExplosionTileCheckOffsets[74] = {
//   X,  Y
    -1, -3,
     0, -3,
     1, -3,
    -2, -2,
    -1, -2,
     0, -2,
     1, -2,
     2, -2,
    -3, -1,
    -2, -1,
    -1, -1,
     0, -1,
     1, -1,
     2, -1,
     3, -1,
    -3,  0,
    -2,  0,
    -1,  0,
     0,  0,
     1,  0,
     2,  0,
     3,  0,
    -3,  1,
    -2,  1,
    -1,  1,
     0,  1,
     1,  1,
     2,  1,
     3,  1,
    -2,  2,
    -1,  2,
     0,  2,
     1,  2,
     2,  2,
    -1,  3,
     0,  3,
     1,  3
};

#define BALL_SPEED_UP_TABLE_LENGTH 12
#define BALL_SPEED_UP_TABLE_ROW_LENGTH 2

#define BOUNCE_THRESHOLD_LOOKUP_OFFSET 0
#define NEW_SPEED_LOOKUP_OFFSET 1

static const int16_t ballSpeedUps[BALL_SPEED_UP_TABLE_LENGTH * BALL_SPEED_UP_TABLE_ROW_LENGTH] = {
    //bounce     new
    //threshold  speed
      0,         39,
      5,         43,
      4,         47,
      4,         51,
      3,         55,
      3,         59,
      3,         63,
      2,         67,
      20,        71,
      30,        75,
      40,        79,
      50,        80
};

//==============================================================================
// Functions
//==============================================================================
void initializeEntity(entity_t *self, entityManager_t *entityManager, tilemap_t *tilemap, gameData_t *gameData, soundManager_t *soundManager)
{
    self->active = false;
    self->persistent = false;
    self->tilemap = tilemap;
    self->gameData = gameData;
    self->soundManager = soundManager;
    self->homeTileX = 0;
    self->homeTileY = 0;
    self->entityManager = entityManager;
    self->spriteFlipHorizontal = false;
    self->spriteFlipVertical = false;
    self->spriteRotateAngle = 0;
    self->attachedToEntity = NULL;
    self->shouldAdvanceMultiplier = false;
    self->baseSpeed = 0;
    self->bouncesToNextSpeedUp = 5;
    self->speedUpLookupIndex = 0;
    self->maxSpeed = 63;
    self->bouncesOffUnbreakableBlocks = 0;
    self->breakInfiniteLoopBounceThreshold = -1;

    // Fields not explicitly initialized
    // self->type = 0;
    // self->updateFunction = NULL;
    // self->x = 0;
    // self->y = 0;
    // self->xspeed = 0;
    // self->yspeed = 0;
    // self->xMaxSpeed = 0;
    // self->yMaxSpeed = 0;
    // self->xDamping = 0;
    // self->yDamping = 0;
    // self->gravityEnabled = false;
    // self->spriteIndex = 0;
    // self->animationTimer = 0;
    // self->jumpPower = 0;
    // self->visible = false;
    // self->hp = 0;
    // self->invincibilityFrames = 0;
    // self->scoreValue = 0;
    // self->collisionHandler = NULL;
    // self->tileCollisionHandler = NULL;
    // self->overlapTileHandler = NULL;
};

void updatePlayer(entity_t *self)
{
    if(self->gameData->isTouched)
    {
        int32_t xdiff;

        //TODO: tune these values some more!!!

        int32_t touchIntoLevel = (self->gameData->touchX << 2) + 192; // play with this value until center touch moves paddle to center

        //                                    the leftmost coordinate that the originX point of the paddle sprite can occupy
        //                                    |   the rightmost coordinate that the originX point of the paddle sprite can occupy
        touchIntoLevel = CLAMP(touchIntoLevel,608,3872);
        xdiff = self->x - touchIntoLevel;
        xdiff = CLAMP(xdiff, -1024, 1024);
        if (self->x != touchIntoLevel)
        {
            self->xspeed = -xdiff;
        } else {
            self->xspeed = 0;
        }
    } else {
        self->xspeed = 0;
    }

    self->x += self->xspeed;

    if(self->gameData->frameCount % 10 == 0 && self->spriteIndex < SP_PADDLE_2){
        self->spriteIndex++;
    }
};

void updatePlayerVertical(entity_t *self)
{ 
    if(self->gameData->isTouched)
    {
        int32_t ydiff;

        int32_t touchIntoLevel = ((984 - self->gameData->touchY)<< 2) + 128; // play with this value until center touch moves paddle to center

        //                                    the leftmost coordinate that the originX point of the paddle sprite can occupy
        //                                    |   the rightmost coordinate that the originX point of the paddle sprite can occupy
        touchIntoLevel = CLAMP(touchIntoLevel,608,3488);
        ydiff = self->y - touchIntoLevel;
        ydiff = CLAMP(ydiff, -1024, 1024);
        if (self->y != touchIntoLevel)
        {
            self->yspeed = -ydiff;
        } else {
            self->yspeed = 0;
        }
    } else {
        self->yspeed = 0;
    }

    self->y += self->yspeed;

    if(self->gameData->frameCount % 10 == 0 && self->spriteIndex < SP_PADDLE_VERTICAL_2){
        self->spriteIndex++;
    }
};

void updateBall(entity_t *self)
{ 
    if(self->attachedToEntity != NULL){
        //Ball is caught
        switch(self->attachedToEntity->type){
            case(ENTITY_PLAYER_PADDLE_BOTTOM):
                self->x = self->attachedToEntity->x;
                self->y = self->attachedToEntity->y-((self->entityManager->sprites[self->spriteIndex].originY + self->entityManager->sprites[self->attachedToEntity->spriteIndex].originY) << SUBPIXEL_RESOLUTION);

                if(
                    self->gameData->btnState & PB_UP
                    &&
                    !(self->gameData->prevBtnState & PB_UP)
                )
                {
                    //Launch ball
                    setVelocity(self, 90 - CLAMP((self->attachedToEntity->xspeed)/SUBPIXEL_RESOLUTION,-60,60), self->baseSpeed);
                    self->attachedToEntity = NULL;
                    bzrPlaySfx(&(self->soundManager->launch), BZR_STEREO);
                }
                break;
            case(ENTITY_PLAYER_PADDLE_TOP):
                self->x = self->attachedToEntity->x;
                self->y = self->attachedToEntity->y+((self->entityManager->sprites[self->spriteIndex].originY + self->entityManager->sprites[self->attachedToEntity->spriteIndex].originY) << SUBPIXEL_RESOLUTION);

                if(
                    self->gameData->btnState & PB_UP
                    &&
                    !(self->gameData->prevBtnState & PB_UP)
                )
                {
                    //Launch ball
                    setVelocity(self, 270 + CLAMP((self->attachedToEntity->xspeed)/SUBPIXEL_RESOLUTION,-60,60), self->baseSpeed);
                    self->attachedToEntity = NULL;
                    bzrPlaySfx(&(self->soundManager->launch), BZR_STEREO);
                }
                break;
            case(ENTITY_PLAYER_PADDLE_LEFT):
                self->x = self->attachedToEntity->x+((self->entityManager->sprites[self->spriteIndex].originX + self->entityManager->sprites[self->attachedToEntity->spriteIndex].originX) << SUBPIXEL_RESOLUTION);
                self->y = self->attachedToEntity->y;

                if(
                    self->gameData->btnState & PB_UP
                    &&
                    !(self->gameData->prevBtnState & PB_UP)
                )
                {
                    //Launch ball
                    setVelocity(self, 0 - CLAMP((self->attachedToEntity->yspeed)/SUBPIXEL_RESOLUTION,-60,60), self->baseSpeed);
                    self->attachedToEntity = NULL;
                    bzrPlaySfx(&(self->soundManager->launch), BZR_STEREO);
                }
                break;
            case(ENTITY_PLAYER_PADDLE_RIGHT):
                self->x = self->attachedToEntity->x-((self->entityManager->sprites[self->spriteIndex].originX + self->entityManager->sprites[self->attachedToEntity->spriteIndex].originX) << SUBPIXEL_RESOLUTION);
                self->y = self->attachedToEntity->y;

                if(
                    self->gameData->btnState & PB_UP
                    &&
                    !(self->gameData->prevBtnState & PB_UP)
                )
                {
                    //Launch ball
                    setVelocity(self, 180 - CLAMP(-(self->attachedToEntity->yspeed)/SUBPIXEL_RESOLUTION,-60,60), self->baseSpeed);
                    self->attachedToEntity = NULL;
                    bzrPlaySfx(&(self->soundManager->launch), BZR_STEREO);
                }
                break;
            default:
                break;
        }
    } else {
        //Ball is in play
        moveEntityWithTileCollisions(self);
        detectEntityCollisions(self);

        if(
            self->gameData->btnState & PB_DOWN
            &&
            !(self->gameData->prevBtnState & PB_DOWN)
        )
        {
            if(self->gameData->playerTimeBombsCount < 3){
                //Drop time bomb
                entity_t* createdBomb = createEntity(self->entityManager, ENTITY_PLAYER_TIME_BOMB, self->x >> SUBPIXEL_RESOLUTION, self->y >> SUBPIXEL_RESOLUTION);
                if(createdBomb != NULL){
                    self->gameData->playerTimeBombsCount++;
                    bzrPlaySfx(&(self->soundManager->dropBomb), BZR_LEFT);
                }
            }
        }

        if(
            self->gameData->btnState & PB_RIGHT
            &&
            !(self->gameData->prevBtnState & PB_RIGHT)
        )
        {
            if(!self->gameData->playerRemoteBombPlaced){
                //Drop remote bomb
                entity_t* createdBomb = createEntity(self->entityManager, ENTITY_PLAYER_REMOTE_BOMB, self->x >> SUBPIXEL_RESOLUTION, self->y >> SUBPIXEL_RESOLUTION);
                if(createdBomb != NULL){
                    self->gameData->playerRemoteBombPlaced = true;
                    bzrPlaySfx(&(self->soundManager->dropBomb), BZR_LEFT);
                }
            }
        }

        if(self->gameData->frameCount % 5 == 0) {
            self->spriteIndex = SP_BALL_0 + ((self->spriteIndex + 1) % 3);
        }

        self->spriteFlipHorizontal = (self->xspeed >= 0) ? false : true;

        if(self->gameData->frameCount % 2 == 0) {
            createEntity(self->entityManager, ENTITY_BALL_TRAIL, self->x >> SUBPIXEL_RESOLUTION, self->y >> SUBPIXEL_RESOLUTION);
        }
    }

    detectLostBall(self, true);
};

void updateBallAtStart(entity_t *self){
    //Find a nearby paddle and attach ball to it
    for (uint8_t i = 0; i < MAX_ENTITIES; i++)
    {
        entity_t *checkEntity = &(self->entityManager->entities[i]);
        if (checkEntity->active && checkEntity != self && (checkEntity->type == ENTITY_PLAYER_PADDLE_BOTTOM || checkEntity->type == ENTITY_PLAYER_PADDLE_TOP || checkEntity->type == ENTITY_PLAYER_PADDLE_LEFT || checkEntity->type == ENTITY_PLAYER_PADDLE_RIGHT) )
        {
            if (getTaxiCabDistanceBetweenEntities(self, checkEntity) < 400)
            {
                self->attachedToEntity = checkEntity;
                self->updateFunction = &updateBall;
                self->collisionHandler = &ballCollisionHandler;
            }
        }
    }
}


bool isOutsidePlayfield(entity_t* self){
    return (self->y > 3840 || self->x > 4480);
}

void detectLostBall(entity_t* self, bool respawn){
    if(isOutsidePlayfield(self)) {
        destroyEntity(self, respawn);
        self->gameData->ballsInPlay--;
        
        if(self->gameData->ballsInPlay <= 0){
            self->gameData->changeState = ST_DEAD;
            bzrPlaySfx(&(self->soundManager->die), BZR_STEREO);
        }
    }
}

void updateCaptiveBallNotInPlay(entity_t* self){
    moveEntityWithTileCollisions(self);
    detectEntityCollisions(self);

    if(isOutsidePlayfield(self)) {
        destroyEntity(self, false);
    }
}

void updateCaptiveBallInPlay(entity_t* self){
    moveEntityWithTileCollisions(self);
    detectEntityCollisions(self);
    detectLostBall(self, false);

    if(self->gameData->frameCount % 2 == 0) {
        createEntity(self->entityManager, ENTITY_BALL_TRAIL, self->x >> SUBPIXEL_RESOLUTION, self->y >> SUBPIXEL_RESOLUTION);
    }
}

uint32_t getTaxiCabDistanceBetweenEntities(entity_t* self, entity_t* other){
    return abs(self->x - other->x) + abs(self->y - other->y);
}

void updateTimeBomb(entity_t * self){
    if(self->gameData->frameCount % 5 == 0) {
        self->spriteIndex = SP_BOMB_0 + ((self->spriteIndex + 1) % 3);
    }

    self->animationTimer--;
    
    if(self->animationTimer == 0){
        explodeBomb(self);
        self->gameData->playerTimeBombsCount--;
    }
}

void updateRemoteBomb(entity_t * self){    
    if(self->animationTimer > 0){
        self->animationTimer--;
        return;
    }

    if(self->gameData->frameCount % 5 == 0) {
        self->spriteIndex = SP_BOMB_0 + ((self->spriteIndex + 1) % 3);
    }

    if(
        self->gameData->btnState & PB_RIGHT
        &&
        !(self->gameData->prevBtnState & PB_RIGHT)
    ){
        explodeBomb(self);
        self->gameData->playerRemoteBombPlaced = false;
    }
}

void explodeBomb(entity_t* self){
    uint8_t tx = TO_TILE_COORDS(self->x >> SUBPIXEL_RESOLUTION);
        uint8_t ty = TO_TILE_COORDS(self->y >> SUBPIXEL_RESOLUTION);
        uint8_t ctx, cty;

        for(uint16_t i = 0; i < BOMB_EXPLOSION_TILE_CHECK_OFFSET_LENGTH; i+=2){
            ctx = tx + bombExplosionTileCheckOffsets[i];
            cty = ty + bombExplosionTileCheckOffsets[i+1];
            uint8_t tileId = getTile(self->tilemap, ctx, cty);

            switch(tileId){
                case TILE_BLOCK_1x1_RED ... TILE_UNUSED_127: {
                    scorePoints(self->gameData, 10 * breakBlockTile(self->tilemap, self->gameData, tileId, ctx, cty), 0);
                    break;
                }
                case TILE_BOUNDARY_1 ... TILE_UNUSED_F:{
                    break;
                }
                default: {         
                    break;
                }
            }
        }

        destroyEntity(self, false);
        createEntity(self->entityManager, ENTITY_PLAYER_BOMB_EXPLOSION, self->x >> SUBPIXEL_RESOLUTION, self->y >> SUBPIXEL_RESOLUTION);
        bzrPlaySfx(&(self->soundManager->detonate), BZR_LEFT);
}

void updateExplosion(entity_t * self){
    if(self->gameData->frameCount % 5 == 0) {
        self->spriteIndex++;
    }

    if(self->spriteIndex > SP_EXPLOSION_3){
        destroyEntity(self, false);
    }
}

void updateBallTrail(entity_t * self){
    self->animationTimer--;
    if(self->animationTimer < 1){
        if(self->visible){
            self->spriteIndex++;
            if(self->spriteIndex > SP_BALL_TRAIL_3){
                destroyEntity(self, false);
            }
        } else {
            self->visible = true;
        }
        self->animationTimer = 2;
    }
}

void updateChoIntro(entity_t * self){
    //This entity's state is determined by whether or not it found the ball.
    if(self->attachedToEntity == NULL){
        //Find the first ball and "attach" self to it
        self->attachedToEntity = findFirstEntityOfType(self->entityManager, ENTITY_PLAYER_BALL);
        if (self->attachedToEntity != NULL)
        {
            self->attachedToEntity->visible = false;

            //If we found the ball, walk towards it from the far side of the screen
            self->y = self->attachedToEntity->y;
            self->x = ((self->attachedToEntity->x >> SUBPIXEL_RESOLUTION) > (TFT_WIDTH >> 1)) ? 0 : TFT_WIDTH << SUBPIXEL_RESOLUTION;
            self->xspeed = (self->attachedToEntity->x > self->x) ? 32 : -32;
            self->spriteFlipHorizontal = (self->xspeed > 0) ? false : true;
            return;
        }

        //Couldn't find ball? Destroy self.
        destroyEntity(self, false);
        return;
    }

    self->x += self->xspeed;

    if(self->gameData->frameCount % 5 == 0) {
        self->spriteIndex++;
        if(self->spriteIndex > SP_CHO_WALK_2){
            self->spriteIndex = SP_CHO_WALK_0;
        }
    }

    //Very close to ball? "Become" the ball
    if(getTaxiCabDistanceBetweenEntities(self, self->attachedToEntity) < 32){
        self->attachedToEntity->visible = true;

        destroyEntity(self, false);
        return;
    }
}

entity_t* findFirstEntityOfType(entityManager_t* entityManager, uint8_t type){
    for (uint8_t i = 0; i < MAX_ENTITIES; i++)
    {
        entity_t* checkEntity = &(entityManager->entities[i]);
        if (checkEntity->active && (checkEntity->type == type))
        {
            return checkEntity;
        }
    }

    return NULL;
}

void updateChoLevelClear(entity_t *self){
    if(self->gameData->countdown > 20){
        if(self->gameData->frameCount % 9 == 0) {
            self->spriteIndex = SP_CHO_WIN_0 + (self->gameData->frameCount & 1);
        }
    } else {
        self->spriteIndex = SP_EXPLOSION_0;
        self->updateFunction = &updateExplosion;
    }
    
}

void updateCrawler(entity_t * self){
    if(self->gameData->frameCount % 10 == 0) {
       self->spriteFlipHorizontal = !self->spriteFlipHorizontal;
    }

    if(self->breakInfiniteLoopBounceThreshold > 0){
        self->breakInfiniteLoopBounceThreshold--;
    }

    detectEntityCollisions(self);

    uint8_t tx = TO_TILE_COORDS(self->x >> SUBPIXEL_RESOLUTION);
    uint8_t ty = TO_TILE_COORDS(self->y >> SUBPIXEL_RESOLUTION);
    uint8_t t;

    switch(self->animationTimer){
        //CLOCKWISE
        case CRAWLER_TOP_TO_RIGHT: //On top of a block, going right
            if(((self->x % (TILE_SIZE << SUBPIXEL_RESOLUTION)) - (HALF_TILE_SIZE << SUBPIXEL_RESOLUTION))){
                break;
            }

            t = getTile(self->tilemap, tx, ty+1);
            if(!isSolid(t)){
                crawlerSetMoveState(self, CRAWLER_RIGHT_TO_BOTTOM);
                self->bouncesOffUnbreakableBlocks++;
            } else {
                self->bouncesOffUnbreakableBlocks = 0;
            }

            t = getTile(self->tilemap, tx+1, ty);
            if(isSolid(t)){
                crawlerSetMoveState(self, CRAWLER_LEFT_TO_TOP);
            }

            break;

        case CRAWLER_RIGHT_TO_BOTTOM: //On the right side of a block, going down
            if(((self->y % (TILE_SIZE << SUBPIXEL_RESOLUTION)) - (HALF_TILE_SIZE << SUBPIXEL_RESOLUTION))){
                break;
            }

            t = getTile(self->tilemap, tx-1, ty);
            if(!isSolid(t)){
                crawlerSetMoveState(self, CRAWLER_BOTTOM_TO_LEFT);
                self->bouncesOffUnbreakableBlocks++;
            } else {
                self->bouncesOffUnbreakableBlocks = 0;
            }
            t = getTile(self->tilemap, tx, ty+1);
            if(isSolid(t)){
                crawlerSetMoveState(self, CRAWLER_TOP_TO_RIGHT);
            }

            break;
        
        case CRAWLER_BOTTOM_TO_LEFT: //On the bottom of a block, going left
            if(((self->x % (TILE_SIZE << SUBPIXEL_RESOLUTION)) - (HALF_TILE_SIZE << SUBPIXEL_RESOLUTION))){
                break;
            }

            t = getTile(self->tilemap, tx, ty-1);
            if(!isSolid(t)){
                crawlerSetMoveState(self, CRAWLER_LEFT_TO_TOP);
                self->bouncesOffUnbreakableBlocks++;
            } else {
                self->bouncesOffUnbreakableBlocks = 0;
            }
            t = getTile(self->tilemap, tx-1, ty);
            if(isSolid(t)){
                crawlerSetMoveState(self, CRAWLER_RIGHT_TO_BOTTOM);
            }

            break;
        
        case CRAWLER_LEFT_TO_TOP: //On the left side of a block, going up
            if(((self->y % (TILE_SIZE << SUBPIXEL_RESOLUTION)) - (HALF_TILE_SIZE << SUBPIXEL_RESOLUTION))){
                break;
            }

            t = getTile(self->tilemap, tx+1, ty);
            if(!isSolid(t)){
                crawlerSetMoveState(self, CRAWLER_TOP_TO_RIGHT);
                self->bouncesOffUnbreakableBlocks++;
            } else {
                self->bouncesOffUnbreakableBlocks = 0;
            }
            t = getTile(self->tilemap, tx, ty-1);
            if(isSolid(t)){
                crawlerSetMoveState(self, CRAWLER_BOTTOM_TO_LEFT);
            }

            break;
        
        //COUNTER-CLOCKWISE
        case CRAWLER_TOP_TO_LEFT: //On top of a block, going left
            if(((self->x % (TILE_SIZE << SUBPIXEL_RESOLUTION)) - (HALF_TILE_SIZE << SUBPIXEL_RESOLUTION))){
                break;
            }

            t = getTile(self->tilemap, tx, ty+1);
            if(!isSolid(t)){
                crawlerSetMoveState(self, CRAWLER_LEFT_TO_BOTTOM);
                self->bouncesOffUnbreakableBlocks++;
            } else {
                self->bouncesOffUnbreakableBlocks = 0;
            }
            t = getTile(self->tilemap, tx-1, ty);
            if(isSolid(t)){
                crawlerSetMoveState(self, CRAWLER_RIGHT_TO_TOP);
            }

            break;

        case CRAWLER_LEFT_TO_BOTTOM: //On the left side of a block, going down
            if(((self->y % (TILE_SIZE << SUBPIXEL_RESOLUTION)) - (HALF_TILE_SIZE << SUBPIXEL_RESOLUTION))){
                break;
            }

            t = getTile(self->tilemap, tx+1, ty);
            if(!isSolid(t)){
                crawlerSetMoveState(self, CRAWLER_BOTTOM_TO_RIGHT);
                self->bouncesOffUnbreakableBlocks++;
            } else {
                self->bouncesOffUnbreakableBlocks = 0;
            }
            t = getTile(self->tilemap, tx, ty+1);
            if(isSolid(t)){
                crawlerSetMoveState(self, CRAWLER_TOP_TO_LEFT);
            }

            break;

        case CRAWLER_BOTTOM_TO_RIGHT:  //On the bottom of a block, going right
            if(((self->x % (TILE_SIZE << SUBPIXEL_RESOLUTION)) - (HALF_TILE_SIZE << SUBPIXEL_RESOLUTION))){
                break;
            }

            t = getTile(self->tilemap, tx, ty-1);
            if(!isSolid(t)){
                crawlerSetMoveState(self, CRAWLER_RIGHT_TO_TOP);
                self->bouncesOffUnbreakableBlocks++;
            } else {
                self->bouncesOffUnbreakableBlocks = 0;
            }

            t = getTile(self->tilemap, tx+1, ty);
            if(isSolid(t)){
                crawlerSetMoveState(self, CRAWLER_LEFT_TO_BOTTOM);
            }

            break;    

        case CRAWLER_RIGHT_TO_TOP:  //On the right side of a block, going up
            if(((self->y % (TILE_SIZE << SUBPIXEL_RESOLUTION)) - (HALF_TILE_SIZE << SUBPIXEL_RESOLUTION))){
                break;
            }

            t = getTile(self->tilemap, tx-1, ty);
            if(!isSolid(t)){
                crawlerSetMoveState(self, CRAWLER_TOP_TO_LEFT);
                self->bouncesOffUnbreakableBlocks++;
            } else {
                self->bouncesOffUnbreakableBlocks = 0;
            }

            t = getTile(self->tilemap, tx, ty-1);
            if(isSolid(t)){
                crawlerSetMoveState(self, CRAWLER_BOTTOM_TO_RIGHT);
            }
            break;

        default:
            break;
    }

    if(self->bouncesOffUnbreakableBlocks > 4){
        scorePoints(self->gameData, 100, 1);
        destroyEntity(self, false);
        createEntity(self->entityManager, ENTITY_PLAYER_BOMB_EXPLOSION, self->x >> SUBPIXEL_RESOLUTION, self->y >> SUBPIXEL_RESOLUTION);
        bzrPlaySfx(&(self->soundManager->detonate), BZR_LEFT); 
    }

    if(isOutsidePlayfield(self)) {
        destroyEntity(self, false);
    }
    
    self->x += self->xspeed;
    self->y += self->yspeed;
}

void crawlerSetMoveState(entity_t* self, uint8_t state){
    uint16_t blocksLeft = self->tilemap->totalTargetBlocks - self->gameData->targetBlocksBroken;
    if(blocksLeft < 100){
        self->baseSpeed = 16;
    } else if(blocksLeft < 80) {
        self->baseSpeed = 24;
    } else if(blocksLeft < 60) {
        self->baseSpeed = 32;
    } else if(blocksLeft < 20) {
        self->baseSpeed = 48;
    } else if(blocksLeft < 10) {
        self->baseSpeed = 64;
    }

    switch(state){
        //CLOCKWISE
        case CRAWLER_TOP_TO_RIGHT: //On top of a block, going right
            self->xspeed = self->baseSpeed;
            self->yspeed = 0;
            self->spriteRotateAngle = 0;
            self->spriteIndex = SP_CRAWLER_TOP;
            break;
        case CRAWLER_RIGHT_TO_BOTTOM: //On the right side of a block, going down
            self->yspeed = self->baseSpeed;
            self->xspeed = 0;
            self->spriteRotateAngle = 90;
            self->spriteIndex = SP_CRAWLER_RIGHT;
            break;
        case CRAWLER_BOTTOM_TO_LEFT: //On the bottom of a block, going left
            self->xspeed = -self->baseSpeed;
            self->yspeed = 0;
            self->spriteRotateAngle = 180;
            self->spriteIndex = SP_CRAWLER_BOTTOM;
            break;
        case CRAWLER_LEFT_TO_TOP: //On the left side of a block, going up
            self->yspeed = -self->baseSpeed;
            self->xspeed = 0;
            self->spriteRotateAngle = 270;
            self->spriteIndex = SP_CRAWLER_LEFT;
            break;
        //COUNTER-CLOCKWISE
        case CRAWLER_TOP_TO_LEFT: //On top of a block, going left
            self->xspeed = -self->baseSpeed;
            self->yspeed = 0;
            self->spriteRotateAngle = 0;
            self->spriteIndex = SP_CRAWLER_TOP;
            break;
        case CRAWLER_LEFT_TO_BOTTOM: //On the left side of a block, going down
            self->yspeed = self->baseSpeed;
            self->xspeed = 0;
            self->spriteRotateAngle = 270;
            self->spriteIndex = SP_CRAWLER_LEFT;
            break;
        case CRAWLER_BOTTOM_TO_RIGHT: //On the bottom of a block, going right
            self->xspeed = self->baseSpeed;
            self->yspeed = 0;
            self->spriteRotateAngle = 180;
            self->spriteIndex = SP_CRAWLER_BOTTOM;
            break;
        case CRAWLER_RIGHT_TO_TOP: //On the right side of a block, going up
            self->yspeed = -self->baseSpeed;
            self->xspeed = 0;
            self->spriteRotateAngle = 90;
            self->spriteIndex = SP_CRAWLER_RIGHT;
            break;
        default:
            break;
    }

    self->animationTimer = state;
}

void crawlerInitMoveState(entity_t* self){
    uint8_t tx = TO_TILE_COORDS(self->x >> SUBPIXEL_RESOLUTION);
    uint8_t ty = TO_TILE_COORDS(self->y >> SUBPIXEL_RESOLUTION);
    uint8_t t;

    t = getTile(self->tilemap, tx, ty+1);
    if(isSolid(t)){
        crawlerSetMoveState(self, ((tx + ty) % 2) ? CRAWLER_TOP_TO_RIGHT : CRAWLER_TOP_TO_LEFT);
        return;
    }

    t = getTile(self->tilemap, tx-1, ty);
    if(isSolid(t)){
        crawlerSetMoveState(self, ((tx + ty) % 2) ? CRAWLER_RIGHT_TO_BOTTOM : CRAWLER_RIGHT_TO_TOP);
        return;
    }

    t = getTile(self->tilemap, tx, ty-1);
    if(isSolid(t)){
        crawlerSetMoveState(self, ((tx + ty) % 2) ? CRAWLER_BOTTOM_TO_LEFT : CRAWLER_BOTTOM_TO_RIGHT);
        return;
    }

    t = getTile(self->tilemap, tx+1, ty);
    if(isSolid(t)){
        crawlerSetMoveState(self, ((tx + ty) % 2) ? CRAWLER_LEFT_TO_TOP : CRAWLER_LEFT_TO_BOTTOM);
        return;
    }
}

void moveEntityWithTileCollisions(entity_t *self)
{

    uint16_t newX = self->x;
    uint16_t newY = self->y;
    uint8_t tx = TO_TILE_COORDS(self->x >> SUBPIXEL_RESOLUTION);
    uint8_t ty = TO_TILE_COORDS(self->y >> SUBPIXEL_RESOLUTION);
    // bool collision = false;

    // Are we inside a block? Push self out of block
    uint8_t t = getTile(self->tilemap, tx, ty);
    self->overlapTileHandler(self, t, tx, ty);

    if (isSolid(t))
    {

        if (self->xspeed == 0 && self->yspeed == 0)
        {
            newX += (self->spriteFlipHorizontal) ? 16 : -16;
        }
        else
        {
            if (self->yspeed != 0)
            {
                self->yspeed = -self->yspeed;
            }
            else
            {
                self->xspeed = -self->xspeed;
            }
        }
    }
    else
    {

        if (self->yspeed != 0)
        {
            int16_t hcof = (((self->x >> SUBPIXEL_RESOLUTION) % TILE_SIZE) - HALF_TILE_SIZE);

            // Handle halfway though tile
            uint8_t at = getTile(self->tilemap, tx + SIGNOF(hcof), ty);

            if (isSolid(at))
            {
                // collision = true;
                newX = ((tx + 1) * TILE_SIZE - HALF_TILE_SIZE) << SUBPIXEL_RESOLUTION;
            }

            uint8_t newTy = TO_TILE_COORDS(((self->y + self->yspeed) >> SUBPIXEL_RESOLUTION) + SIGNOF(self->yspeed) * HALF_TILE_SIZE);

            if (newTy != ty)
            {
                uint8_t newVerticalTile = getTile(self->tilemap, tx, newTy);

                //if (newVerticalTile > TILE_UNUSED_29 && newVerticalTile < TILE_BG_GOAL_ZONE)
                {
                    if (self->tileCollisionHandler(self, newVerticalTile, tx, newTy, 2 << (self->yspeed > 0)))
                    {
                        newY = ((newTy + ((ty < newTy) ? -1 : 1)) * TILE_SIZE + HALF_TILE_SIZE) << SUBPIXEL_RESOLUTION;
                    }
                }
            }
        }

        if (self->xspeed != 0)
        {
            int16_t vcof = (((self->y >> SUBPIXEL_RESOLUTION) % TILE_SIZE) - HALF_TILE_SIZE);

            // Handle halfway though tile
            uint8_t att = getTile(self->tilemap, tx, ty + SIGNOF(vcof));

            if (isSolid(att))
            {
                // collision = true;
                newY = ((ty + 1) * TILE_SIZE - HALF_TILE_SIZE) << SUBPIXEL_RESOLUTION;
            }

            // Handle outside of tile
            uint8_t newTx = TO_TILE_COORDS(((self->x + self->xspeed) >> SUBPIXEL_RESOLUTION) + SIGNOF(self->xspeed) * HALF_TILE_SIZE);

            if (newTx != tx)
            {
                uint8_t newHorizontalTile = getTile(self->tilemap, newTx, ty);

                //if (newHorizontalTile > TILE_UNUSED_29 && newHorizontalTile < TILE_BG_GOAL_ZONE)
                {
                    if (self->tileCollisionHandler(self, newHorizontalTile, newTx, ty, (self->xspeed > 0)))
                    {
                        newX = ((newTx + ((tx < newTx) ? -1 : 1)) * TILE_SIZE + HALF_TILE_SIZE) << SUBPIXEL_RESOLUTION;
                    }
                }

                /*if (!self->falling)
                {
                    uint8_t newBelowTile = getTile(self->tilemap, tx, ty + 1);

                    if ((self->gravityEnabled && !isSolid(newBelowTile)) )
                    {
                        self->fallOffTileHandler(self);
                    }
                }*/
            }
        }
    }

    self->x = newX + self->xspeed;
    self->y = newY + self->yspeed;
}

void destroyEntity(entity_t *self, bool respawn)
{
    if (respawn && !(self->homeTileX == 0 && self->homeTileY == 0))
    {
        self->tilemap->map[self->homeTileY * self->tilemap->mapWidth + self->homeTileX] = self->type + 128;
    }

    self->attachedToEntity = NULL;
    self->entityManager->activeEntities--;
    self->active = false;
}

void detectEntityCollisions(entity_t *self)
{
    sprite_t* selfSprite = &(self->entityManager->sprites[self->spriteIndex]);
    box_t* selfSpriteBox = &(selfSprite->collisionBox);
    
    box_t selfBox;
    selfBox.x0 = (self->x >> SUBPIXEL_RESOLUTION) - selfSprite->originX + selfSpriteBox->x0;
    selfBox.y0 = (self->y >> SUBPIXEL_RESOLUTION) - selfSprite->originY + selfSpriteBox->y0;
    selfBox.x1 = (self->x >> SUBPIXEL_RESOLUTION) - selfSprite->originX + selfSpriteBox->x1;
    selfBox.y1 = (self->y >> SUBPIXEL_RESOLUTION) - selfSprite->originY + selfSpriteBox->y1;

    entity_t *checkEntity;
    sprite_t* checkEntitySprite;
    box_t* checkEntitySpriteBox;          
    box_t checkEntityBox;

    for (uint8_t i = 0; i < MAX_ENTITIES; i++)
    {
        checkEntity = &(self->entityManager->entities[i]);
        if (checkEntity->active && checkEntity != self)
        {
            checkEntitySprite = &(self->entityManager->sprites[checkEntity->spriteIndex]);
            checkEntitySpriteBox = &(checkEntitySprite->collisionBox);
            
            checkEntityBox.x0 = (checkEntity->x >> SUBPIXEL_RESOLUTION) - checkEntitySprite->originX + checkEntitySpriteBox->x0;
            checkEntityBox.y0 = (checkEntity->y >> SUBPIXEL_RESOLUTION) - checkEntitySprite->originY + checkEntitySpriteBox->y0;
            checkEntityBox.x1 = (checkEntity->x >> SUBPIXEL_RESOLUTION) - checkEntitySprite->originX + checkEntitySpriteBox->x1;
            checkEntityBox.y1 = (checkEntity->y >> SUBPIXEL_RESOLUTION) - checkEntitySprite->originY + checkEntitySpriteBox->y1;

            if (boxesCollide(selfBox, checkEntityBox, 0))
            {
                self->collisionHandler(self, checkEntity);
            }
        }
    }
}

void playerCollisionHandler(entity_t *self, entity_t *other)
{
    
}


void crawlerCollisionHandler(entity_t *self, entity_t *other)
{
    switch (other->type)
    {
        case ENTITY_CRAWLER:
        {
            if(self->breakInfiniteLoopBounceThreshold){
                break;
            }

            crawlerSetMoveState(self, (self->animationTimer + 4) % 8);
            self->breakInfiniteLoopBounceThreshold = 30;
        }
        default:
        {
            break;
        }
    }
}


void dummyCollisionHandler(entity_t *self, entity_t *other)
{
    return;
}

void ballCollisionHandler(entity_t *self, entity_t *other)
{
    switch (other->type)
    {
        case ENTITY_PLAYER_PADDLE_BOTTOM:
            if(self->yspeed > 0){
                advanceBallSpeed(self, 1);
                setVelocity(self, 90 + (other->x - self->x)/SUBPIXEL_RESOLUTION, self->baseSpeed);
                bzrPlaySfx(&(self->soundManager->hit2), BZR_LEFT);

                if(self->shouldAdvanceMultiplier){
                    scorePoints(self->gameData, 0, self->gameData->ballsInPlay);
                    self->shouldAdvanceMultiplier = false;
                    other->spriteIndex = SP_PADDLE_0;
                }

                self->bouncesOffUnbreakableBlocks = 0;
            }
            break;
        case ENTITY_PLAYER_PADDLE_TOP:
            if(self->yspeed < 0){
                advanceBallSpeed(self, 1);
                setVelocity(self, 270 + (self->x - other->x)/SUBPIXEL_RESOLUTION, self->baseSpeed);
                bzrPlaySfx(&(self->soundManager->hit2), BZR_LEFT);

                if(self->shouldAdvanceMultiplier){
                    scorePoints(self->gameData, 0, self->gameData->ballsInPlay);
                    self->shouldAdvanceMultiplier = false;
                    other->spriteIndex = SP_PADDLE_0;
                }

                self->bouncesOffUnbreakableBlocks = 0;
            }
            break;
        case ENTITY_PLAYER_PADDLE_LEFT:
            if(self->xspeed < 0){
                advanceBallSpeed(self, 1);
                setVelocity(self, 0 + (other->y - self->y)/SUBPIXEL_RESOLUTION, self->baseSpeed);
                bzrPlaySfx(&(self->soundManager->hit2), BZR_LEFT);

                if(self->shouldAdvanceMultiplier){
                    scorePoints(self->gameData, 0, self->gameData->ballsInPlay);
                    self->shouldAdvanceMultiplier = false;
                    other->spriteIndex = SP_PADDLE_VERTICAL_0;
                }

                self->bouncesOffUnbreakableBlocks = 0;
            }
            break;
        case ENTITY_PLAYER_PADDLE_RIGHT:
            if(self->xspeed > 0){
                advanceBallSpeed(self, 1);
                setVelocity(self, 180 + (self->y - other->y)/SUBPIXEL_RESOLUTION, self->baseSpeed);
                bzrPlaySfx(&(self->soundManager->hit2), BZR_LEFT);

                if(self->shouldAdvanceMultiplier){
                    scorePoints(self->gameData, 0, self->gameData->ballsInPlay);
                    self->shouldAdvanceMultiplier = false;
                    other->spriteIndex = SP_PADDLE_VERTICAL_0;
                }

                self->bouncesOffUnbreakableBlocks = 0;
            }
            break;
        case ENTITY_PLAYER_BOMB_EXPLOSION:
            if(!other->spriteRotateAngle) {
                advanceBallSpeed(self, 3);
            }
            
            setVelocity(self, getAtan2(other->y - self->y, self->x - other->x), self->baseSpeed);
            other->spriteRotateAngle = 1 + (self->x % 359);

            if(self->shouldAdvanceMultiplier){
                scorePoints(self->gameData, 0, self->gameData->ballsInPlay);
                self->shouldAdvanceMultiplier = false;
            }

            self->bouncesOffUnbreakableBlocks = 0;
            break;
        case ENTITY_CRAWLER:
            setVelocity(self, getAtan2(other->y - self->y, self->x - other->x), self->baseSpeed);
            advanceBallSpeed(self, 1);
            bzrPlaySfx(&(self->soundManager->hit3), BZR_LEFT);
            break;
        default:
        {
            break;
        }
    }
}

void captiveBallCollisionHandler(entity_t *self, entity_t *other)
{
    bool shouldChangeState = false;
    switch (other->type)
    {
        case ENTITY_PLAYER_PADDLE_BOTTOM:
        case ENTITY_PLAYER_PADDLE_TOP:
        case ENTITY_PLAYER_PADDLE_LEFT:
        case ENTITY_PLAYER_PADDLE_RIGHT:
            shouldChangeState = true;
            break;
        case ENTITY_PLAYER_BOMB_EXPLOSION:
            if(!other->spriteRotateAngle) {
                shouldChangeState = true;
            }
            break;
        case ENTITY_CRAWLER:
            setVelocity(self, getAtan2(other->y - self->y, self->x - other->x), self->baseSpeed);
            break;
        default:
        {
            return;
        }
    }

    if(shouldChangeState){
        self->collisionHandler = &ballCollisionHandler;
        self->tileCollisionHandler = &ballTileCollisionHandler;
        self->overlapTileHandler = &ballOverlapTileHandler;
        self->updateFunction = &updateCaptiveBallInPlay;
        self->gameData->ballsInPlay++;
        bzrPlaySfx(&(self->soundManager->launch), BZR_RIGHT);
    }
}

void advanceBallSpeed(entity_t* self, uint16_t factor){
    if(self->speedUpLookupIndex >= BALL_SPEED_UP_TABLE_LENGTH){
        return;
    }

    self->bouncesToNextSpeedUp -= factor;
    if(self->bouncesToNextSpeedUp < 0){
        self->speedUpLookupIndex += BALL_SPEED_UP_TABLE_ROW_LENGTH;
        if(self->speedUpLookupIndex >= BALL_SPEED_UP_TABLE_LENGTH){
            return;
        }

        self->bouncesToNextSpeedUp = ballSpeedUps[(self->speedUpLookupIndex * BALL_SPEED_UP_TABLE_ROW_LENGTH) + BOUNCE_THRESHOLD_LOOKUP_OFFSET];

        self->baseSpeed = ballSpeedUps[(self->speedUpLookupIndex * BALL_SPEED_UP_TABLE_ROW_LENGTH) + NEW_SPEED_LOOKUP_OFFSET];
        if(self->baseSpeed > self->maxSpeed){
            self->baseSpeed = self->maxSpeed;
        }
    }
}

bool playerTileCollisionHandler(entity_t *self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction)
{
    if (isSolid(tileId))
    {
        switch (direction)
        {
        case 0: // PB_LEFT
            self->xspeed = 0;
            break;
        case 1: // PB_RIGHT
            self->xspeed = 0;
            break;
        case 2: // PB_UP
            self->yspeed = 0;
            break;
        case 4: // PB_DOWN
            self->yspeed = 0;
            break;
        default: // Should never hit
            return false;
        }
        // trigger tile collision resolution
        return true;
    }

    return false;
}

/*
bool enemyTileCollisionHandler(entity_t *self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction)
{
    switch(tileId){
        case TILE_BOUNCE_BLOCK: {
            switch (direction)
            {
                case 0:
                    //hitBlock->xspeed = -64;
                    if(tileId == TILE_BOUNCE_BLOCK){
                        self->xspeed = 48;
                    }
                    break;
                case 1:
                    //hitBlock->xspeed = 64;
                    if(tileId == TILE_BOUNCE_BLOCK){
                        self->xspeed = -48;
                    }
                    break;
                case 2:
                    //hitBlock->yspeed = -128;
                    if(tileId == TILE_BOUNCE_BLOCK){
                        self->yspeed = 48;
                    }
                    break;
                case 4:
                    //hitBlock->yspeed = (tileId == TILE_BRICK_BLOCK) ? 32 : 64;
                    if(tileId == TILE_BOUNCE_BLOCK){
                        self->yspeed = -48;
                    }
                    break;
                default:
                    break;
            }
            break;
        }
        default: {
            break;
        }
    }

    if (isSolid(tileId))
    {
        switch (direction)
        {
        case 0: // PB_LEFT
            self->xspeed = -self->xspeed;
            break;
        case 1: // PB_RIGHT
            self->xspeed = -self->xspeed;
            break;
        case 2: // PB_UP
            self->yspeed = 0;
            break;
        case 4: // PB_DOWN
            // Landed on platform
            self->falling = false;
            self->yspeed = 0;
            break;
        default: // Should never hit
            return false;
        }
        // trigger tile collision resolution
        return true;
    }

    return false;
}
*/

bool dummyTileCollisionHandler(entity_t *self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction)
{
    return false;
}

bool ballTileCollisionHandler(entity_t *self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction)
{
     switch(tileId){
        case TILE_BLOCK_1x1_RED ... TILE_BLOCK_1x1_BLACK:
        case TILE_BLOCK_2x1_RED_L ... TILE_BLOCK_2x1_BLACK_R:
        case TILE_BLOCK_2x2_RED_UL ... TILE_BLOCK_2x2_BLACK_UR:
        case TILE_BLOCK_2x2_WHITE_DL ... TILE_BLOCK_2x2_BLACK_DR:
         {
            //breakBlockTile(self->tilemap, self->gameData, tileId, tx, ty);
            bzrPlaySfx(&(self->soundManager->hit1), BZR_LEFT);
            scorePoints(self->gameData, 10 * breakBlockTile(self->tilemap, self->gameData, tileId, tx, ty), (self->shouldAdvanceMultiplier) ? -1 : 0);
            self->shouldAdvanceMultiplier = true;
            self->bouncesOffUnbreakableBlocks = 0;
            break;
        }
        case TILE_BOUNDARY_1 ... TILE_UNUSED_F:{
            bzrPlaySfx(&(self->soundManager->hit3), BZR_LEFT);
            self->bouncesOffUnbreakableBlocks++;
            break;
        }
        default: {         
            break;
        }
    }

    if (isSolid(tileId))
    {
        switch (direction)
        {
        case 0: // LEFT
        case 1: // RIGHT
            if(self->bouncesOffUnbreakableBlocks > self->breakInfiniteLoopBounceThreshold){
                //Subtly change ball bounce angle if the ball has bounced off many unbreakable blocks in a row
                self->xspeed += (1 << SUBPIXEL_RESOLUTION) * SIGNOF(self->xspeed);
                self->bouncesOffUnbreakableBlocks = 0;
            }
            self->xspeed = -self->xspeed;
            break;
        case 2: // UP
        case 4: // DOWN
            if(self->bouncesOffUnbreakableBlocks > self->breakInfiniteLoopBounceThreshold){
                //Subtly change ball bounce angle if the ball has bounced off many unbreakable blocks in a row
                self->yspeed += (1 << SUBPIXEL_RESOLUTION) * SIGNOF(self->yspeed);
                self->bouncesOffUnbreakableBlocks = 0;
            }
            self->yspeed = -self->yspeed;
            break;
        default: // Should never hit
            return false;
        }
        // trigger tile collision resolution
        return true;
    }

    return false;
}

bool captiveBallTileCollisionHandler(entity_t *self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction)
{
    if (isSolid(tileId))
    {
        switch (direction)
        {
        case 0: // PB_LEFT
            self->xspeed = -self->xspeed;
            break;
        case 1: // PB_RIGHT
            self->xspeed = -self->xspeed;
            break;
        case 2: // PB_UP
            self->yspeed = -self->yspeed;
            break;
        case 4: // PB_DOWN
            self->yspeed = -self->yspeed;
            break;
        default: // Should never hit
            return false;
        }
        // trigger tile collision resolution
        return true;
    }

    return false;
}

void ballOverlapTileHandler(entity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty){
    switch(tileId){
        case TILE_BLOCK_1x1_RED ... TILE_BLOCK_1x1_BLACK:
        case TILE_BLOCK_2x1_RED_L ... TILE_BLOCK_2x1_BLACK_R:
        case TILE_BLOCK_2x2_RED_UL ... TILE_BLOCK_2x2_BLACK_UR:
        case TILE_BLOCK_2x2_WHITE_DL ... TILE_BLOCK_2x2_BLACK_DR: {
            bzrPlaySfx(&(self->soundManager->hit1), BZR_LEFT);
            scorePoints(self->gameData, 10 * breakBlockTile(self->tilemap, self->gameData, tileId, tx, ty), (self->shouldAdvanceMultiplier) ? -1 : 0);
            self->shouldAdvanceMultiplier = true;
            self->bouncesOffUnbreakableBlocks = 0;
            break;
        }
        case TILE_BOUNDARY_1 ... TILE_UNUSED_F:{
            bzrPlaySfx(&(self->soundManager->hit3), BZR_LEFT);
            break;
        }
        default: {
            break;
        }
    }
}

int16_t breakBlockTile(tilemap_t *tilemap, gameData_t *gameData, uint8_t tileId, uint8_t tx, uint8_t ty){
    int16_t blockBreakCount = 0;

    switch(tileId){
        case TILE_BLOCK_1x1_RED ... TILE_BLOCK_1x1_STONE: {
           setTile(tilemap, tx, ty, TILE_EMPTY);
           blockBreakCount++;
           break;
        }
        case TILE_BLOCK_2x1_RED_L:
        case TILE_BLOCK_2x1_ORANGE_L:
        case TILE_BLOCK_2x1_YELLOW_L:
        case TILE_BLOCK_2x1_GREEN_L:
        case TILE_BLOCK_2x1_CYAN_L:
        case TILE_BLOCK_2x1_BLUE_L:
        case TILE_BLOCK_2x1_PURPLE_L:
        case TILE_BLOCK_2x1_MAGENTA_L:
        case TILE_BLOCK_2x1_WHITE_L:
        case TILE_BLOCK_2x1_TAN_L:
        case TILE_BLOCK_2x1_BROWN_L:
        case TILE_BLOCK_2x1_BLACK_L:
        case TILE_BLOCK_2x1_STONE_L:
        {
           setTile(tilemap, tx, ty, TILE_EMPTY);
           blockBreakCount++;
           
           if(isBlock(getTile(tilemap, tx+1, ty))){
            setTile(tilemap, tx+1, ty, TILE_EMPTY);
            blockBreakCount++;
           }

           break;
        }
        case TILE_BLOCK_2x1_RED_R:
        case TILE_BLOCK_2x1_ORANGE_R:
        case TILE_BLOCK_2x1_YELLOW_R:
        case TILE_BLOCK_2x1_GREEN_R:
        case TILE_BLOCK_2x1_CYAN_R:
        case TILE_BLOCK_2x1_BLUE_R:
        case TILE_BLOCK_2x1_PURPLE_R:
        case TILE_BLOCK_2x1_MAGENTA_R:
        case TILE_BLOCK_2x1_WHITE_R:
        case TILE_BLOCK_2x1_TAN_R:
        case TILE_BLOCK_2x1_BROWN_R:
        case TILE_BLOCK_2x1_BLACK_R:
        case TILE_BLOCK_2x1_STONE_R:
        {
           setTile(tilemap, tx, ty, TILE_EMPTY);
           blockBreakCount++;

           if(isBlock(getTile(tilemap, tx-1, ty))){
            setTile(tilemap, tx-1, ty, TILE_EMPTY);
            blockBreakCount++;
           }

           break;
        }
        case TILE_BLOCK_2x2_RED_UL:
        case TILE_BLOCK_2x2_ORANGE_UL:
        case TILE_BLOCK_2x2_YELLOW_UL:
        case TILE_BLOCK_2x2_GREEN_UL:
        case TILE_BLOCK_2x2_CYAN_UL:
        case TILE_BLOCK_2x2_BLUE_UL:
        case TILE_BLOCK_2x2_PURPLE_UL:
        case TILE_BLOCK_2x2_MAGENTA_UL:
        case TILE_BLOCK_2x2_WHITE_UL:
        case TILE_BLOCK_2x2_TAN_UL:
        case TILE_BLOCK_2x2_BROWN_UL:
        case TILE_BLOCK_2x2_BLACK_UL:
        case TILE_BLOCK_2x2_STONE_UL:
        {
           setTile(tilemap, tx, ty, TILE_EMPTY);
           blockBreakCount++;
           
           if(isBlock(getTile(tilemap, tx+1, ty))){
            setTile(tilemap, tx+1, ty, TILE_EMPTY);
            blockBreakCount++;
           }

           if(isBlock(getTile(tilemap, tx,ty+1))){
            setTile(tilemap, tx, ty+1, TILE_EMPTY);
            blockBreakCount++;
           }

            if(isBlock(getTile(tilemap, tx+1,ty+1))){
            setTile(tilemap, tx+1, ty+1, TILE_EMPTY);
            blockBreakCount++;
           }

           break;
        }
        case TILE_BLOCK_2x2_RED_UR:
        case TILE_BLOCK_2x2_ORANGE_UR:
        case TILE_BLOCK_2x2_YELLOW_UR:
        case TILE_BLOCK_2x2_GREEN_UR:
        case TILE_BLOCK_2x2_CYAN_UR:
        case TILE_BLOCK_2x2_BLUE_UR:
        case TILE_BLOCK_2x2_PURPLE_UR:
        case TILE_BLOCK_2x2_MAGENTA_UR:
        case TILE_BLOCK_2x2_WHITE_UR:
        case TILE_BLOCK_2x2_TAN_UR:
        case TILE_BLOCK_2x2_BROWN_UR:
        case TILE_BLOCK_2x2_BLACK_UR:
        case TILE_BLOCK_2x2_STONE_UR:
        {
           setTile(tilemap, tx, ty, TILE_EMPTY);
           blockBreakCount++;
           
           if(isBlock(getTile(tilemap, tx-1, ty))){
            setTile(tilemap, tx-1, ty, TILE_EMPTY);
            blockBreakCount++;
           }

           if(isBlock(getTile(tilemap, tx,ty+1))){
            setTile(tilemap, tx, ty+1, TILE_EMPTY);
            blockBreakCount++;
           }

            if(isBlock(getTile(tilemap, tx-1,ty+1))){
            setTile(tilemap, tx-1, ty+1, TILE_EMPTY);
            blockBreakCount++;
           }

           break;
        }
        case TILE_BLOCK_2x2_RED_DL:
        case TILE_BLOCK_2x2_ORANGE_DL:
        case TILE_BLOCK_2x2_YELLOW_DL:
        case TILE_BLOCK_2x2_GREEN_DL:
        case TILE_BLOCK_2x2_CYAN_DL:
        case TILE_BLOCK_2x2_BLUE_DL:
        case TILE_BLOCK_2x2_PURPLE_DL:
        case TILE_BLOCK_2x2_MAGENTA_DL:
        case TILE_BLOCK_2x2_WHITE_DL:
        case TILE_BLOCK_2x2_TAN_DL:
        case TILE_BLOCK_2x2_BROWN_DL:
        case TILE_BLOCK_2x2_BLACK_DL:
        case TILE_BLOCK_2x2_STONE_DL:
        {
           setTile(tilemap, tx, ty, TILE_EMPTY);
           blockBreakCount++;
           
           if(isBlock(getTile(tilemap, tx+1, ty))){
            setTile(tilemap, tx+1, ty, TILE_EMPTY);
            blockBreakCount++;
           }

           if(isBlock(getTile(tilemap, tx,ty-1))){
            setTile(tilemap, tx, ty-1, TILE_EMPTY);
            blockBreakCount++;
           }

           if(isBlock(getTile(tilemap, tx+1,ty-1))){
            setTile(tilemap, tx+1, ty-1, TILE_EMPTY);
            blockBreakCount++;
           }

           break;
        }
        case TILE_BLOCK_2x2_RED_DR:
        case TILE_BLOCK_2x2_ORANGE_DR:
        case TILE_BLOCK_2x2_YELLOW_DR:
        case TILE_BLOCK_2x2_GREEN_DR:
        case TILE_BLOCK_2x2_CYAN_DR:
        case TILE_BLOCK_2x2_BLUE_DR:
        case TILE_BLOCK_2x2_PURPLE_DR:
        case TILE_BLOCK_2x2_MAGENTA_DR:
        case TILE_BLOCK_2x2_WHITE_DR:
        case TILE_BLOCK_2x2_TAN_DR:
        case TILE_BLOCK_2x2_BROWN_DR:
        case TILE_BLOCK_2x2_BLACK_DR:
        case TILE_BLOCK_2x2_STONE_DR:
        {
           setTile(tilemap, tx, ty, TILE_EMPTY);
           blockBreakCount++;
           
           if(isBlock(getTile(tilemap, tx-1, ty))){
            setTile(tilemap, tx-1, ty, TILE_EMPTY);
            blockBreakCount++;
           }

           if(isBlock(getTile(tilemap, tx,ty-1))){
            setTile(tilemap, tx, ty-1, TILE_EMPTY);
            blockBreakCount++;
           }

           if(isBlock(getTile(tilemap, tx-1,ty-1))){
            setTile(tilemap, tx-1, ty-1, TILE_EMPTY);
            blockBreakCount++;
           }

           break;
        }
        
        default: {
            break;
        }
    }

    setLedBreakBlock(gameData, tileId);
    gameData->targetBlocksBroken += blockBreakCount;

    if(gameData->targetBlocksBroken >= tilemap->totalTargetBlocks){
        entity_t* playerBall = findFirstEntityOfType(tilemap->entityManager, ENTITY_PLAYER_BALL);
        if(playerBall != NULL){
            playerBall->updateFunction = &updateChoLevelClear;
            playerBall->entityManager->playerEntity = playerBall;
            deactivateAllEntities(playerBall->entityManager, true, false, false);
        }

        gameData->changeState = ST_LEVEL_CLEAR;
    }

    return blockBreakCount;
};

void setLedBreakBlock(gameData_t *gameData, uint8_t tileId){
    uint8_t ledIndex = esp_random() % CONFIG_NUM_LEDS;
    int16_t nr = 0;
    int16_t ng = 0;
    int16_t nb = 0;

    switch(tileId){
        case TILE_BLOCK_1x1_RED:
        case TILE_BLOCK_2x1_RED_L:
        case TILE_BLOCK_2x1_RED_R:
        case TILE_BLOCK_2x2_RED_UL:
        case TILE_BLOCK_2x2_RED_UR:
        case TILE_BLOCK_2x2_RED_DL:
        case TILE_BLOCK_2x2_RED_DR: {
            nr = 255;
            break;
        }
        case TILE_BLOCK_1x1_ORANGE:
        case TILE_BLOCK_2x1_ORANGE_L:
        case TILE_BLOCK_2x1_ORANGE_R:
        case TILE_BLOCK_2x2_ORANGE_UL:
        case TILE_BLOCK_2x2_ORANGE_UR:
        case TILE_BLOCK_2x2_ORANGE_DL:
        case TILE_BLOCK_2x2_ORANGE_DR: {
            nr = 255;
            ng = 127;
            break;
        }
        case TILE_BLOCK_1x1_YELLOW:
        case TILE_BLOCK_2x1_YELLOW_L:
        case TILE_BLOCK_2x1_YELLOW_R:
        case TILE_BLOCK_2x2_YELLOW_UL:
        case TILE_BLOCK_2x2_YELLOW_UR:
        case TILE_BLOCK_2x2_YELLOW_DL:
        case TILE_BLOCK_2x2_YELLOW_DR: {
            nr = 255;
            ng = 255;
            break;
        }
        case TILE_BLOCK_1x1_GREEN:
        case TILE_BLOCK_2x1_GREEN_L:
        case TILE_BLOCK_2x1_GREEN_R:
        case TILE_BLOCK_2x2_GREEN_UL:
        case TILE_BLOCK_2x2_GREEN_UR:
        case TILE_BLOCK_2x2_GREEN_DL:
        case TILE_BLOCK_2x2_GREEN_DR: {
            ng = 255;
            break;
        }
        case TILE_BLOCK_1x1_CYAN:
        case TILE_BLOCK_2x1_CYAN_L:
        case TILE_BLOCK_2x1_CYAN_R:
        case TILE_BLOCK_2x2_CYAN_UL:
        case TILE_BLOCK_2x2_CYAN_UR:
        case TILE_BLOCK_2x2_CYAN_DL:
        case TILE_BLOCK_2x2_CYAN_DR: {
            ng = 255;
            nb = 255;
            break;
        }
        case TILE_BLOCK_1x1_BLUE:
        case TILE_BLOCK_2x1_BLUE_L:
        case TILE_BLOCK_2x1_BLUE_R:
        case TILE_BLOCK_2x2_BLUE_UL:
        case TILE_BLOCK_2x2_BLUE_UR:
        case TILE_BLOCK_2x2_BLUE_DL:
        case TILE_BLOCK_2x2_BLUE_DR: {
            nb = 255;
            break;
        }
        case TILE_BLOCK_1x1_PURPLE:
        case TILE_BLOCK_2x1_PURPLE_L:
        case TILE_BLOCK_2x1_PURPLE_R:
        case TILE_BLOCK_2x2_PURPLE_UL:
        case TILE_BLOCK_2x2_PURPLE_UR:
        case TILE_BLOCK_2x2_PURPLE_DL:
        case TILE_BLOCK_2x2_PURPLE_DR: {
            nr = 255;
            nb = 127;
            break;
        }
        case TILE_BLOCK_1x1_MAGENTA:
        case TILE_BLOCK_2x1_MAGENTA_L:
        case TILE_BLOCK_2x1_MAGENTA_R:
        case TILE_BLOCK_2x2_MAGENTA_UL:
        case TILE_BLOCK_2x2_MAGENTA_UR:
        case TILE_BLOCK_2x2_MAGENTA_DL:
        case TILE_BLOCK_2x2_MAGENTA_DR: {
            nr = 255;
            nb = 255;
            break;
        }
        case TILE_BLOCK_1x1_WHITE:
        case TILE_BLOCK_2x1_WHITE_L:
        case TILE_BLOCK_2x1_WHITE_R:
        case TILE_BLOCK_2x2_WHITE_UL:
        case TILE_BLOCK_2x2_WHITE_UR:
        case TILE_BLOCK_2x2_WHITE_DL:
        case TILE_BLOCK_2x2_WHITE_DR: {
            nr = 255;
            ng = 255;
            nb = 255;
            break;
        }
        case TILE_BLOCK_1x1_TAN:
        case TILE_BLOCK_2x1_TAN_L:
        case TILE_BLOCK_2x1_TAN_R:
        case TILE_BLOCK_2x2_TAN_UL:
        case TILE_BLOCK_2x2_TAN_UR:
        case TILE_BLOCK_2x2_TAN_DL:
        case TILE_BLOCK_2x2_TAN_DR: {
            nr = 255;
            ng = 204;
            nb = 103;
            break;
        }
        case TILE_BLOCK_1x1_BROWN:
        case TILE_BLOCK_2x1_BROWN_L:
        case TILE_BLOCK_2x1_BROWN_R:
        case TILE_BLOCK_2x2_BROWN_UL:
        case TILE_BLOCK_2x2_BROWN_UR:
        case TILE_BLOCK_2x2_BROWN_DL:
        case TILE_BLOCK_2x2_BROWN_DR: {
            nr = 153;
            ng = 102;
            nb = 102;
            break;
        }
        case TILE_BLOCK_1x1_BLACK:
        case TILE_BLOCK_2x1_BLACK_L:
        case TILE_BLOCK_2x1_BLACK_R:
        case TILE_BLOCK_2x2_BLACK_UL:
        case TILE_BLOCK_2x2_BLACK_UR:
        case TILE_BLOCK_2x2_BLACK_DL:
        case TILE_BLOCK_2x2_BLACK_DR: {
            nr = 64;
            ng = 64;
            nb = 64;
            break;
        }
        default: {
            break;
        }

        
    }

    nr += gameData->leds[ledIndex].r;
    ng += gameData->leds[ledIndex].g;
    nb += gameData->leds[ledIndex].b;

    gameData->leds[ledIndex].r = CLAMP(nr, 0, 255);
    gameData->leds[ledIndex].g = CLAMP(ng, 0, 255);
    gameData->leds[ledIndex].b = CLAMP(nb, 0, 255);

    setLeds(gameData->leds, CONFIG_NUM_LEDS);
}

void updateDummy(entity_t *self)
{
    // Do nothing, because that's what dummies do!
}

void setVelocity(entity_t *self, int16_t direction, int16_t magnitude){
    while (direction < 0)
    {
        direction += 360;
    }
    while (direction > 359)
    {
        direction -= 360;
    }

    int16_t sin  = getSin1024(direction);
    int16_t cos  = getCos1024(direction);

    self->xspeed = (magnitude * cos) / 1024;
    self->yspeed = -(magnitude * sin) / 1024;
}

void playerOverlapTileHandler(entity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty){
    /*switch(tileId){
        case TILE_COIN_1...TILE_COIN_3:{
            setTile(self->tilemap, tx, ty, TILE_EMPTY);
            addCoins(self->gameData, 1);
            scorePoints(self->gameData, 50);
            break;
        }
        case TILE_LADDER:{
            if(self->gravityEnabled){
                self->gravityEnabled = false;
                self->xspeed = 0;
            }
            break;
        }
        default: {
            break;
        }
    }

    if(!self->gravityEnabled && tileId != TILE_LADDER){
        self->gravityEnabled = true;
        self->falling = true;
        if(self->yspeed < 0){
            self->yspeed = -32;
        }
    }*/
}

void defaultOverlapTileHandler(entity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty){
    //Nothing to do.
}
