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

//==============================================================================
// Functions
//==============================================================================
void initializeEntity(entity_t *self, entityManager_t *entityManager, tilemap_t *tilemap, gameData_t *gameData, soundManager_t *soundManager)
{
    self->active = false;
    self->tilemap = tilemap;
    self->gameData = gameData;
    self->soundManager = soundManager;
    self->homeTileX = 0;
    self->homeTileY = 0;
    self->gravity = false;
    self->falling = false;
    self->entityManager = entityManager;
    self->fallOffTileHandler = NULL; //&defaultFallOffTileHandler;
    self->spriteFlipHorizontal = false;
    self->spriteFlipVertical = false;
    self->spriteRotateAngle = 0;
    self->attachedToEntity = NULL;
    self->shouldAdvanceMultiplier = false;

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

        int32_t touchIntoLevel = (self->gameData->touchX << 2) + 128; // play with this value until center touch moves paddle to center

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

    /*
        TODO:
        Move this. Doesn't need to be repeated across every paddle.
    */
    if(
        (
            (self->gameData->btnState & PB_START)
            &&
            !(self->gameData->prevBtnState & PB_START)
        )
    ){
        self->gameData->changeState = ST_PAUSE;
    }
    
};

void updatePlayerVertical(entity_t *self)
{ 
    if(self->gameData->isTouched)
    {
        int32_t ydiff;

        int32_t touchIntoLevel = ((960 - self->gameData->touchY)<< 2) + 160; // play with this value until center touch moves paddle to center

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

    /*
        TODO:
        Move this. Doesn't need to be repeated across every paddle.
    */
    if(
        (
            (self->gameData->btnState & PB_START)
            &&
            !(self->gameData->prevBtnState & PB_START)
        )
    ){
        self->gameData->changeState = ST_PAUSE;
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
                    setVelocity(self, 90 - CLAMP((self->attachedToEntity->xspeed)/SUBPIXEL_RESOLUTION,-60,60), 63);
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
                    setVelocity(self, 270 + CLAMP((self->attachedToEntity->xspeed)/SUBPIXEL_RESOLUTION,-60,60), 63);
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
                    setVelocity(self, 0 - CLAMP((self->attachedToEntity->yspeed)/SUBPIXEL_RESOLUTION,-60,60), 63);
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
                    setVelocity(self, 180 - CLAMP(-(self->attachedToEntity->yspeed)/SUBPIXEL_RESOLUTION,-60,60), 63);
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


        if(self->gameData->bombDetonateCooldown > 0){
            self->gameData->bombDetonateCooldown--;
        }

        if(
            self->gameData->btnState & PB_DOWN
            &&
            !(self->gameData->prevBtnState & PB_DOWN)
        )
        {
            if(self->gameData->playerBombsCount < 3){
                //Drop bomb
                entity_t* createdBomb = createEntity(self->entityManager, ENTITY_PLAYER_BOMB, self->x >> SUBPIXEL_RESOLUTION, self->y >> SUBPIXEL_RESOLUTION);
                if(createdBomb != NULL){
                    if(self->gameData->playerBombsCount == 0){
                        self->gameData->nextBombToDetonate = self->gameData->nextBombSlot;
                    }

                    self->gameData->playerBombs[self->gameData->nextBombSlot] = createdBomb;
                    self->gameData->nextBombSlot = (self->gameData->nextBombSlot + 1) % 3;
                    self->gameData->playerBombsCount++;

                    bzrPlaySfx(&(self->soundManager->dropBomb), BZR_LEFT);
                }
            }
        }
    }

    if(self->y > 3840 || self->x > 4480) {
        self->gameData->changeState = ST_DEAD;
        destroyEntity(self, true);
        bzrPlaySfx(&(self->soundManager->die), BZR_STEREO);
    }
};

void updateBallAtStart(entity_t *self){
    //Find a nearby paddle and attach ball to it
    for (uint8_t i = 0; i < MAX_ENTITIES; i++)
    {
        entity_t *checkEntity = &(self->entityManager->entities[i]);
        if (checkEntity->active && checkEntity != self && (checkEntity->type == ENTITY_PLAYER_PADDLE_BOTTOM || checkEntity->type == ENTITY_PLAYER_PADDLE_TOP || checkEntity->type == ENTITY_PLAYER_PADDLE_LEFT || checkEntity->type == ENTITY_PLAYER_PADDLE_RIGHT) )
        {
            uint32_t dist = abs(self->x - checkEntity->x) + abs(self->y - checkEntity->y);

            if (dist < 400)
            {
                self->attachedToEntity = checkEntity;
                self->updateFunction = &updateBall;
                self->collisionHandler = &ballCollisionHandler;
            }
        }
    }
}

void updateBomb(entity_t * self){
    if(self->gameData->playerBombs[self->gameData->nextBombToDetonate] != self || self->gameData->bombDetonateCooldown > 0){
        return;
    }

    if(self->gameData->frameCount % 5 == 0) {
        self->spriteIndex = SP_BOMB_0 + ((self->spriteIndex + 1) % 2);
    }

    if(
        self->gameData->btnState & PB_UP
        &&
        !(self->gameData->prevBtnState & PB_UP)
    ){
        uint8_t tx = TO_TILE_COORDS(self->x >> SUBPIXEL_RESOLUTION);
        uint8_t ty = TO_TILE_COORDS(self->y >> SUBPIXEL_RESOLUTION);
        uint8_t ctx, cty;

        for(uint16_t i = 0; i < BOMB_EXPLOSION_TILE_CHECK_OFFSET_LENGTH; i+=2){
            ctx = tx + bombExplosionTileCheckOffsets[i];
            cty = ty + bombExplosionTileCheckOffsets[i+1];
            uint8_t tileId = getTile(self->tilemap, ctx, cty);

            switch(tileId){
                case TILE_BLOCK_1x1_RED ... TILE_UNUSED_127: {
                    breakBlockTile(self->tilemap, self->gameData, tileId, ctx, cty);
                    scorePoints(self->gameData, 10, 0);
                    break;
                }
                case TILE_BOUNDARY_1 ... TILE_BOUNDARY_3:{
                    break;
                }
                default: {         
                    break;
                }
            }
        }

        destroyEntity(self, false);
        createEntity(self->entityManager, ENTITY_PLAYER_BOMB_EXPLOSION, self->x >> SUBPIXEL_RESOLUTION, self->y >> SUBPIXEL_RESOLUTION);
        
        self->gameData->nextBombToDetonate = (self->gameData->nextBombToDetonate + 1) % 3;
        self->gameData->playerBombsCount--;
        self->gameData->bombDetonateCooldown = 8;

        bzrPlaySfx(&(self->soundManager->detonate), BZR_LEFT);
    }
}

void updateExplosion(entity_t * self){
    if(self->gameData->frameCount % 5 == 0) {
        self->spriteIndex++;
    }

    if(self->spriteIndex > SP_EXPLOSION_3){
        destroyEntity(self, false);
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

                if (!self->falling)
                {
                    uint8_t newBelowTile = getTile(self->tilemap, tx, ty + 1);

                    if ((self->gravityEnabled && !isSolid(newBelowTile)) )
                    {
                        self->fallOffTileHandler(self);
                    }
                }
            }
        }
    }

    self->x = newX + self->xspeed;
    self->y = newY + self->yspeed;
}

void defaultFallOffTileHandler(entity_t *self){
    self->falling = true;
}

void destroyEntity(entity_t *self, bool respawn)
{
    if (respawn && !(self->homeTileX == 0 && self->homeTileY == 0))
    {
        self->tilemap->map[self->homeTileY * self->tilemap->mapWidth + self->homeTileX] = self->type + 128;
    }

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

/*
void enemyCollisionHandler(entity_t *self, entity_t *other)
{
    switch (other->type)
    {
        case ENTITY_TEST:
        case ENTITY_DUST_BUNNY:
        case ENTITY_WASP:
        case ENTITY_BUSH_2:
        case ENTITY_BUSH_3:
        case ENTITY_DUST_BUNNY_2:
        case ENTITY_DUST_BUNNY_3:
        case ENTITY_WASP_2:
        case ENTITY_WASP_3:
        case ENTITY_POWERUP:
        case ENTITY_1UP:
            if((self->xspeed > 0 && self->x < other->x) || (self->xspeed < 0 && self->x > other->x)){
                self->xspeed = -self->xspeed;
                self->spriteFlipHorizontal = -self->spriteFlipHorizontal;
            }
            break;
        case ENTITY_HIT_BLOCK:
            self->xspeed = other->xspeed*2;
            self->yspeed = other->yspeed*2;
            scorePoints(self->gameData, self->scoreValue);
             //buzzer_play_sfx(&sndSquish);
            killEnemy(self);
            break;
        case ENTITY_WAVE_BALL:
            self->xspeed = other->xspeed >> 1;
            self->yspeed = -abs(other->xspeed >> 1);
            scorePoints(self->gameData, self->scoreValue);
             //buzzer_play_sfx(&sndBreak);
            killEnemy(self);
            destroyEntity(other, false);
            break;
        default:
        {
            break;
        }
    }
}
*/

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
                setVelocity(self, 90 + (other->x - self->x)/SUBPIXEL_RESOLUTION, 63);
                bzrPlaySfx(&(self->soundManager->hit2), BZR_LEFT);

                if(self->shouldAdvanceMultiplier){
                    scorePoints(self->gameData, 0, 2 );
                    self->shouldAdvanceMultiplier = false;
                    other->spriteIndex = SP_PADDLE_0;
                }
            }
            break;
        case ENTITY_PLAYER_PADDLE_TOP:
            if(self->yspeed < 0){
                setVelocity(self, 270 + (self->x - other->x)/SUBPIXEL_RESOLUTION, 63);
                bzrPlaySfx(&(self->soundManager->hit2), BZR_LEFT);

                if(self->shouldAdvanceMultiplier){
                    scorePoints(self->gameData, 0, 2 );
                    self->shouldAdvanceMultiplier = false;
                    other->spriteIndex = SP_PADDLE_0;
                }
            }
            break;
        case ENTITY_PLAYER_PADDLE_LEFT:
            if(self->xspeed < 0){
                setVelocity(self, 0 + (other->y - self->y)/SUBPIXEL_RESOLUTION, 63);
                bzrPlaySfx(&(self->soundManager->hit2), BZR_LEFT);

                if(self->shouldAdvanceMultiplier){
                    scorePoints(self->gameData, 0, 2 );
                    self->shouldAdvanceMultiplier = false;
                    other->spriteIndex = SP_PADDLE_VERTICAL_0;
                }
            }
            break;
        case ENTITY_PLAYER_PADDLE_RIGHT:
            if(self->xspeed > 0){
                setVelocity(self, 180 + (self->y - other->y)/SUBPIXEL_RESOLUTION, 63);
                bzrPlaySfx(&(self->soundManager->hit2), BZR_LEFT);

                if(self->shouldAdvanceMultiplier){
                    scorePoints(self->gameData, 0, 2 );
                    self->shouldAdvanceMultiplier = false;
                    other->spriteIndex = SP_PADDLE_VERTICAL_0;
                }
            }
            break;
        default:
        {
            break;
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
        case TILE_BLOCK_1x1_RED ... TILE_UNUSED_127: {
            breakBlockTile(self->tilemap, self->gameData, tileId, tx, ty);
            bzrPlaySfx(&(self->soundManager->hit1), BZR_LEFT);
            scorePoints(self->gameData, 10, -1);
            self->shouldAdvanceMultiplier = true;
            break;
        }
        case TILE_BOUNDARY_1 ... TILE_BOUNDARY_3:{
            bzrPlaySfx(&(self->soundManager->hit3), BZR_LEFT);
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
        case TILE_BLOCK_1x1_RED ... TILE_UNUSED_127: {
            breakBlockTile(self->tilemap, self->gameData, tileId, tx, ty);
            bzrPlaySfx(&(self->soundManager->hit1), BZR_LEFT);
            scorePoints(self->gameData, 1, -1);
            break;
        }
        case TILE_BOUNDARY_1 ... TILE_BOUNDARY_3:{
            bzrPlaySfx(&(self->soundManager->hit3), BZR_LEFT);
            break;
        }
        default: {
            break;
        }
    }
}

void breakBlockTile(tilemap_t *tilemap, gameData_t *gameData, uint8_t tileId, uint8_t tx, uint8_t ty){
    switch(tileId){
        case TILE_BLOCK_1x1_RED ... TILE_BLOCK_1x1_BLACK: {
           setTile(tilemap, tx, ty, TILE_EMPTY);
           gameData->targetBlocksBroken++;
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
        {
           setTile(tilemap, tx, ty, TILE_EMPTY);
           gameData->targetBlocksBroken++;
           
           if(isBlock(getTile(tilemap, tx+1, ty))){
            setTile(tilemap, tx+1, ty, TILE_EMPTY);
            gameData->targetBlocksBroken++;
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
        {
           setTile(tilemap, tx, ty, TILE_EMPTY);
           gameData->targetBlocksBroken++;

           if(isBlock(getTile(tilemap, tx-1, ty))){
            setTile(tilemap, tx-1, ty, TILE_EMPTY);
            gameData->targetBlocksBroken++;
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
        {
           setTile(tilemap, tx, ty, TILE_EMPTY);
           gameData->targetBlocksBroken++;
           
           if(isBlock(getTile(tilemap, tx+1, ty))){
            setTile(tilemap, tx+1, ty, TILE_EMPTY);
            gameData->targetBlocksBroken++;
           }

           if(isBlock(getTile(tilemap, tx,ty+1))){
            setTile(tilemap, tx, ty+1, TILE_EMPTY);
            gameData->targetBlocksBroken++;
           }

            if(isBlock(getTile(tilemap, tx+1,ty+1))){
            setTile(tilemap, tx+1, ty+1, TILE_EMPTY);
            gameData->targetBlocksBroken++;
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
        {
           setTile(tilemap, tx, ty, TILE_EMPTY);
           gameData->targetBlocksBroken++;
           
           if(isBlock(getTile(tilemap, tx-1, ty))){
            setTile(tilemap, tx-1, ty, TILE_EMPTY);
            gameData->targetBlocksBroken++;
           }

           if(isBlock(getTile(tilemap, tx,ty+1))){
            setTile(tilemap, tx, ty+1, TILE_EMPTY);
            gameData->targetBlocksBroken++;
           }

            if(isBlock(getTile(tilemap, tx-1,ty+1))){
            setTile(tilemap, tx-1, ty+1, TILE_EMPTY);
            gameData->targetBlocksBroken++;
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
        {
           setTile(tilemap, tx, ty, TILE_EMPTY);
           gameData->targetBlocksBroken++;
           
           if(isBlock(getTile(tilemap, tx+1, ty))){
            setTile(tilemap, tx+1, ty, TILE_EMPTY);
            gameData->targetBlocksBroken++;
           }

           if(isBlock(getTile(tilemap, tx,ty-1))){
            setTile(tilemap, tx, ty-1, TILE_EMPTY);
            gameData->targetBlocksBroken++;
           }

            if(isBlock(getTile(tilemap, tx+1,ty-1))){
            setTile(tilemap, tx+1, ty-1, TILE_EMPTY);
            gameData->targetBlocksBroken++;
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
        {
           setTile(tilemap, tx, ty, TILE_EMPTY);
           gameData->targetBlocksBroken++;
           
           if(isBlock(getTile(tilemap, tx-1, ty))){
            setTile(tilemap, tx-1, ty, TILE_EMPTY);
            gameData->targetBlocksBroken++;
           }

           if(isBlock(getTile(tilemap, tx,ty-1))){
            setTile(tilemap, tx, ty-1, TILE_EMPTY);
            gameData->targetBlocksBroken++;
           }

            if(isBlock(getTile(tilemap, tx-1,ty-1))){
            setTile(tilemap, tx-1, ty-1, TILE_EMPTY);
            gameData->targetBlocksBroken++;
           }

           break;
        }
        
        default: {
            break;
        }
    }

    setLedBreakBlock(gameData, tileId);

    if(gameData->targetBlocksBroken >= tilemap->totalTargetBlocks){
        gameData->changeState = ST_LEVEL_CLEAR;
    }
};

void setLedBreakBlock(gameData_t *gameData, uint8_t tileId){
    uint8_t ledIndex = esp_random() % CONFIG_NUM_LEDS;
    uint16_t nr = 0;
    uint16_t ng = 0;
    uint16_t nb = 0;

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
