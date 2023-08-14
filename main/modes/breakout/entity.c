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
    self->attachedToEntity = NULL;

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
    // Check if the touch area is touched
    int32_t centerVal, intensityVal, touchIntoLevel,xdiff;
    if(getTouchCentroid(&centerVal, &intensityVal))
    {
        touchIntoLevel = (centerVal << 2) + 128; // play with this value until center touch moves paddle to center

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

    /*if (self->gameData->btnState & PB_B)
    {
        self->xMaxSpeed = 52;
    }
    else
    {
        self->xMaxSpeed = 30;
    }

    if (self->gameData->btnState & PB_LEFT)
    {
        self->xspeed -= (self->falling && self->xspeed < 0) ? (self->xspeed < -24) ? 0 : 2 : 3;

        if (self->xspeed < -self->xMaxSpeed)
        {
            self->xspeed = -self->xMaxSpeed;
        }
    }
    else if (self->gameData->btnState & PB_RIGHT)
    {
        self->xspeed += (self->falling && self->xspeed > 0) ? (self->xspeed > 24) ? 0 : 2 : 3;

        if (self->xspeed > self->xMaxSpeed)
        {
            self->xspeed = self->xMaxSpeed;
        }
    }

    if(!self->gravityEnabled){
        if (self->gameData->btnState & PB_UP)
        {
            self->yspeed -= 8;
            self->falling = true;

            if (self->yspeed < -16)
            {
                self->yspeed = -16;
            }
        }
        else if (self->gameData->btnState & PB_DOWN)
        {
            self->yspeed += 8;
            self->falling = true;

            if (self->yspeed > 32)
            {
                self->yspeed = 32;
            }
        }
    }

    if (self->gameData->btnState & PB_A)
    {
        if (!self->falling && !(self->gameData->prevBtnState & PB_A))
        {
            // initiate jump
            self->jumpPower = 64 + ((abs(self->xspeed) + 16) >> 3);
            self->yspeed = -self->jumpPower;
            self->falling = true;
             //buzzer_play_sfx(&sndJump1);
        }
        else if (self->jumpPower > 0 && self->yspeed < 0)
        {
            // jump dampening
            self->jumpPower -= 2; // 32
            self->yspeed = -self->jumpPower;
            
            if(self->jumpPower > 35 && self->jumpPower < 37){
                 //buzzer_play_sfx(&sndJump2);
            }

            if(self->yspeed > -6 && self->yspeed < -2){
                 //buzzer_play_sfx(&sndJump3);
            }

            if (self->jumpPower < 0)
            {
                self->jumpPower = 0;
            }
        }
    }
    else if (self->falling && self->jumpPower > 0 && self->yspeed < 0)
    {
        // Cut jump short if player lets go of jump button
        self->jumpPower = 0;
        self->yspeed = self->yspeed / 4;
    }

    if(self->invincibilityFrames > 0){
        self->invincibilityFrames--;
        if(self->invincibilityFrames % 2){
            self->visible = !self->visible;
        }

        if(self->invincibilityFrames <= 0){
            self->visible = true;
        }
    }

    if(self->animationTimer > 0){
        self->animationTimer--;
    }*/

    /*
    if (self->hp >2 && self->gameData->btnState & PB_B && !(self->gameData->prevBtnState & PB_B) && self->animationTimer == 0)
    {
        entity_t * createdEntity = createEntity(self->entityManager, ENTITY_WAVE_BALL, self->x >> SUBPIXEL_RESOLUTION, self->y >> SUBPIXEL_RESOLUTION);
        if(createdEntity != NULL){
            createdEntity->xspeed= (self->spriteFlipHorizontal) ? -(128 + abs(self->xspeed) + abs(self->yspeed)):128 + abs(self->xspeed) + abs(self->yspeed);
            createdEntity->homeTileX = 0;
            createdEntity->homeTileY = 0;
             //buzzer_play_sfx(&sndWaveBall);
        }
        self->animationTimer = 30;
    }*/

    if(
        (
            (self->gameData->btnState & PB_START)
            &&
            !(self->gameData->prevBtnState & PB_START)
        )
    ){
        self->gameData->changeState = ST_PAUSE;
    }

    //moveEntityWithTileCollisions(self);
    //dieWhenFallingOffScreen(self);
    //applyGravity(self);
    //applyDamping(self);
    //detectEntityCollisions(self);
    //animatePlayer(self);
    
};

void updateBall(entity_t *self)
{ 
    if(self->attachedToEntity != NULL){
        //Ball is caught
        self->x = self->attachedToEntity->x;
        self->y = self->attachedToEntity->y-((self->entityManager->sprites[self->spriteIndex].originY + self->entityManager->sprites[self->attachedToEntity->spriteIndex].originY) << SUBPIXEL_RESOLUTION);

        if(
            self->gameData->btnState & PB_UP
            &&
            !self->gameData->prevBtnState & PB_UP
        )
        {
            //Launch ball
            self->yspeed = -63;
            //self->xspeed = self->attachedToEntity->xspeed;
            self->attachedToEntity = NULL;
        }
    } else {
        //Ball is in play
        moveEntityWithTileCollisions(self);
        detectEntityCollisions(self);
    }

    if((self->y >> 4) > 240){
        self->gameData->changeState = ST_DEAD;
        destroyEntity(self, true);
        //self->y = 236 << 4;
        //self->yspeed = -24;
    }
};

void updateBallAtStart(entity_t *self){
    //Find a nearby paddle and attach ball to it
    for (uint8_t i = 0; i < MAX_ENTITIES; i++)
    {
        entity_t *checkEntity = &(self->entityManager->entities[i]);
        if (checkEntity->active && checkEntity != self && checkEntity->type == ENTITY_PLAYER_PADDLE_BOTTOM)
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

/*
void updateHitBlock(entity_t *self)
{
    if(self->homeTileY > self->tilemap->mapHeight){
        destroyEntity(self, false);
        return;
    }
   
    self->x += self->xspeed;
    self->y += self->yspeed;

    self->animationTimer++;
    if (self->animationTimer == 6)
    {
        self->xspeed = -self->xspeed;
        self->yspeed = -self->yspeed;
    }
    if (self->animationTimer > 12)
    {
        uint8_t aboveTile = (self->homeTileY == 0) ? 0 : self->tilemap->map[(self->homeTileY - 1) * self->tilemap->mapWidth + self->homeTileX];
        uint8_t belowTile = (self->homeTileY == (self->tilemap->mapHeight - 1))? 0 : self->tilemap->map[(self->homeTileY + 1) * self->tilemap->mapWidth + self->homeTileX];
        entity_t *createdEntity = NULL;

        switch (aboveTile)
        {
            case TILE_CTNR_COIN:
            case TILE_CTNR_10COIN:
            {
                addCoins(self->gameData, 1);
                scorePoints(self->gameData, 10);
                self->jumpPower = TILE_CONTAINER_2;
                break;
            }
            case TILE_CTNR_POW1:
            {
                createdEntity = createEntity(self->entityManager, ENTITY_POWERUP, (self->homeTileX * TILE_SIZE) + HALF_TILE_SIZE, ((self->homeTileY + ((self->yspeed < 0 && (!isSolid(belowTile) && belowTile != TILE_BOUNCE_BLOCK))?1:-1)) * TILE_SIZE) + HALF_TILE_SIZE);
                createdEntity->homeTileX = 0;
                createdEntity->homeTileY = 0;

                self->jumpPower = TILE_CONTAINER_2;
                break;
            }
            case TILE_WARP_0 ... TILE_WARP_F:
            {
                createdEntity = createEntity(self->entityManager, ENTITY_WARP, (self->homeTileX * TILE_SIZE) + HALF_TILE_SIZE, ((self->homeTileY + ((self->yspeed < 0 && (!isSolid(belowTile) && belowTile != TILE_BOUNCE_BLOCK))?1:-1)) * TILE_SIZE) + HALF_TILE_SIZE);

                createdEntity->homeTileX = self->homeTileX;
                createdEntity->homeTileY = self->homeTileY;

                createdEntity->jumpPower = aboveTile - TILE_WARP_0;
                self->jumpPower = TILE_CONTAINER_2;
                break;
            }
            case TILE_CTNR_1UP:
            {
                if(self->gameData->extraLifeCollected){
                    addCoins(self->gameData, 1);
                    scorePoints(self->gameData, 10);
                } else {
                    createdEntity = createEntity(self->entityManager, ENTITY_1UP, (self->homeTileX * TILE_SIZE) + HALF_TILE_SIZE, ((self->homeTileY + ((self->yspeed < 0 && (!isSolid(belowTile) && belowTile != TILE_BOUNCE_BLOCK))?1:-1)) * TILE_SIZE) + HALF_TILE_SIZE);
                    createdEntity->homeTileX = 0;
                    createdEntity->homeTileY = 0;
                    self->gameData->extraLifeCollected = true;
                }

                self->jumpPower = TILE_CONTAINER_2;
                break;
            }
            default:
            {
                break;
            }

        }

        if(self->jumpPower == TILE_BRICK_BLOCK && (self->yspeed > 0 || self->yDamping == 1) && createdEntity == NULL ) {
            self->jumpPower = TILE_EMPTY;
            scorePoints(self->gameData, 10);
             //buzzer_play_sfx(&sndBreak);
        }

        self->tilemap->map[self->homeTileY * self->tilemap->mapWidth + self->homeTileX] = self->jumpPower;

        destroyEntity(self, false);
    }
};
*/

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

/*
void applyDamping(entity_t *self)
{
    if (!self->falling || !self->gravityEnabled)
    {
        if (self->xspeed > 0)
        {
            self->xspeed -= self->xDamping;

            if (self->xspeed < 0)
            {
                self->xspeed = 0;
            }
        }
        else if (self->xspeed < 0)
        {
            self->xspeed += self->xDamping;

            if (self->xspeed > 0)
            {
                self->xspeed = 0;
            }
        }
    }

    if (self->gravityEnabled)
    {
        return;
    }

    if (self->yspeed > 0)
    {
        self->yspeed -= self->yDamping;

        if (self->yspeed < 0)
        {
            self->yspeed = 0;
        }
    }
    else if (self->yspeed < 0)
    {
        self->yspeed += self->yDamping;

        if (self->yspeed > 0)
        {
            self->yspeed = 0;
        }
    }
}

void applyGravity(entity_t *self)
{
    if (!self->gravityEnabled || !self->falling)
    {
        return;
    }

    self->yspeed += self->gravity;

    if (self->yspeed > self->yMaxSpeed)
    {
        self->yspeed = self->yMaxSpeed;
    }
}

void despawnWhenOffscreen(entity_t *self)
{
    if (
        (self->x >> SUBPIXEL_RESOLUTION) < (self->tilemap->mapOffsetX - DESPAWN_THRESHOLD) ||
        (self->x >> SUBPIXEL_RESOLUTION) > (self->tilemap->mapOffsetX + TILEMAP_DISPLAY_WIDTH_PIXELS + DESPAWN_THRESHOLD)
    )
    {
        destroyEntity(self, true);
    }

    if (self->y > 63616){
        return;
    }

    if (
        (self->y >> SUBPIXEL_RESOLUTION) < (self->tilemap->mapOffsetY - (DESPAWN_THRESHOLD << 2)) ||
        (self->y >> SUBPIXEL_RESOLUTION) > (self->tilemap->mapOffsetY + TILEMAP_DISPLAY_HEIGHT_PIXELS + DESPAWN_THRESHOLD)
    )
    {
        destroyEntity(self, true);
    }
}
*/
void destroyEntity(entity_t *self, bool respawn)
{
    if (respawn && !(self->homeTileX == 0 && self->homeTileY == 0))
    {
        self->tilemap->map[self->homeTileY * self->tilemap->mapWidth + self->homeTileX] = self->type + 128;
    }

    // self->entityManager->activeEntities--;
    self->active = false;
}
/*
void animatePlayer(entity_t *self)
{
    if (self->spriteIndex == SP_PLAYER_WIN || self->spriteIndex == SP_PLAYER_HURT)
    {
        // Win pose has been set; don't change it!
        return;
    }

    if (!self->gravityEnabled){
        self->spriteIndex = SP_PLAYER_CLIMB;
        if(self->yspeed < 0 && self->gameData->frameCount % 10 == 0){
            self->spriteFlipHorizontal = !self->spriteFlipHorizontal;
        }
    }
    else if (self->falling)
    {
        if (self->yspeed < 0)
        {
            // Jumping
            self->spriteIndex = SP_PLAYER_JUMP;
        }
        else
        {
            // Falling
            self->spriteIndex = SP_PLAYER_WALK1;
        }
    }
    else if (self->xspeed != 0)
    {
        if ( ((self->gameData->btnState & PB_LEFT) && self->xspeed < 0) || ((self->gameData->btnState & PB_RIGHT) && self->xspeed > 0))
        {
            // Running
            self->spriteFlipHorizontal = (self->xspeed > 0) ? 0 : 1;

            if(self->gameData->frameCount % (10 - (abs(self->xspeed) >> 3) ) == 0) {
                self->spriteIndex = 1 + ((self->spriteIndex + 1) % 3);
            }
        }
        else
        {
            self->spriteIndex = SP_PLAYER_SLIDE;
        }
    }
    else
    {
        // Standing
        self->spriteIndex = SP_PLAYER_IDLE;
    }
}
*/
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
    /*switch (other->type)
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
        {
            other->xspeed = -other->xspeed;

            if (self->y < other->y || self->yspeed > 0)
            {
                scorePoints(self->gameData, other->scoreValue);

                killEnemy(other);
                 //buzzer_play_sfx(&sndSquish);

                self->yspeed = -180;
                self->jumpPower = 64 + ((abs(self->xspeed) + 16) >> 3);
                self->falling = true;
            }
            else if(self->invincibilityFrames <= 0)
            {
                self->hp--;
                updateLedsHpMeter(self->entityManager, self->gameData);
                self->gameData->comboTimer = 0;
                
                if(!self->gameData->debugMode && self->hp == 0){
                    self->updateFunction = &updateEntityDead;
                    self->type = ENTITY_DEAD;
                    self->xspeed = 0;
                    self->yspeed = -60;
                    self->spriteIndex = SP_PLAYER_HURT;
                    self->gameData->changeState = ST_DEAD;
                    self->gravityEnabled = true;
                    self->falling = true;
                } else {
                    self->xspeed = 0;
                    self->yspeed = 0;
                    self->jumpPower = 0;
                    self->invincibilityFrames = 120;
                     //buzzer_play_sfx(&sndHurt);
                }
            }
       
            break;
        }
        case ENTITY_WARP:{
            //Execute warp
            self->x = (self->tilemap->warps[other->jumpPower].x * TILE_SIZE + HALF_TILE_SIZE) << SUBPIXEL_RESOLUTION;
            self->y = (self->tilemap->warps[other->jumpPower].y * TILE_SIZE + HALF_TILE_SIZE) << SUBPIXEL_RESOLUTION;
            self->falling = true;
            viewFollowEntity(self->tilemap, self->entityManager->playerEntity);

            unlockScrolling(self->tilemap);
            deactivateAllEntities(self->entityManager, true);
            self->tilemap->executeTileSpawnAll = true;
             //buzzer_play_sfx(&sndWarp);
            break;
        }
        case ENTITY_POWERUP:{
            self->hp++;
            if(self->hp > 3){
                self->hp = 3;
            }
            scorePoints(self->gameData, 1000);
             //buzzer_play_sfx(&sndPowerUp);
            updateLedsHpMeter(self->entityManager, self->gameData);
            destroyEntity(other, false);
            break;
        }
        case ENTITY_1UP:{
            self->gameData->lives++;
            scorePoints(self->gameData, 0);
             //buzzer_play_sfx(&snd1up);
            destroyEntity(other, false);
            break;
        }
        case ENTITY_CHECKPOINT: {
            if(!other->xDamping){
                //Get tile above checkpoint
                uint8_t aboveTile = self->tilemap->map[(other->homeTileY - 1) * self->tilemap->mapWidth + other->homeTileX];
                
                if(aboveTile >= TILE_WARP_0 && aboveTile <= TILE_WARP_F) {
                    self->gameData->checkpoint = aboveTile - TILE_WARP_0;
                    other->xDamping = 1;
                     //buzzer_play_sfx(&sndCheckpoint);
                }
            }
            break;
        }
        default:
        {
            break;
        }
    }*/
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
                bzrPlaySfx(&(self->soundManager->hit1));
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
    /*switch (tileId)
    {
    case TILE_CONTAINER_1:
    case TILE_BRICK_BLOCK:
    case TILE_INVISIBLE_CONTAINER:
    case TILE_BOUNCE_BLOCK:
    {
        entity_t *hitBlock = createEntity(self->entityManager, ENTITY_HIT_BLOCK, (tx * TILE_SIZE) + HALF_TILE_SIZE, (ty * TILE_SIZE) + HALF_TILE_SIZE);

        if (hitBlock != NULL)
        {

            setTile(self->tilemap, tx, ty, TILE_INVISIBLE_BLOCK);
            hitBlock->homeTileX = tx;
            hitBlock->homeTileY = ty;
            hitBlock->jumpPower = tileId;
            if (tileId == TILE_BRICK_BLOCK)
            {
                hitBlock->spriteIndex = SP_HITBLOCK_BRICKS;
                 if(abs(self->xspeed) > 51 && self->yspeed <= 0){ 
                    hitBlock->yDamping = 1;
                }
            }

            if (tileId == TILE_BOUNCE_BLOCK){
                hitBlock->spriteIndex = SP_BOUNCE_BLOCK;
            }

            switch (direction)
            {
            case 0:
                hitBlock->xspeed = -24;
                if(tileId == TILE_BOUNCE_BLOCK){
                    self->xspeed = 48;
                }
                break;
            case 1:
                hitBlock->xspeed = 24;
                if(tileId == TILE_BOUNCE_BLOCK){
                    self->xspeed = -48;
                }
                break;
            case 2:
                hitBlock->yspeed = -48;
                if(tileId == TILE_BOUNCE_BLOCK){
                    self->yspeed = 48;
                }
                break;
            case 4:
                hitBlock->yspeed = (tileId == TILE_BRICK_BLOCK) ? 16 : 24;
                if(tileId == TILE_BOUNCE_BLOCK){
                    self->yspeed = -64;
                    if(self->gameData->btnState & PB_A){
                        self->jumpPower = 80 + ((abs(self->xspeed) + 16) >> 3);
                    }
                }
                break;
            default:
                break;
            }

             //buzzer_play_sfx(&sndHit);
        }
        break;
    }
    case TILE_GOAL_100PTS:
    {
        if(direction == 4) {
            scorePoints(self->gameData, 100);
            buzzer_stop();
             //buzzer_play_sfx(&sndLevelClearD);
            self->spriteIndex = SP_PLAYER_WIN;
            self->updateFunction = &updateDummy;
            self->gameData->changeState = ST_LEVEL_CLEAR;
        }
        break;
    }
    case TILE_GOAL_500PTS:
    {
        if(direction == 4) {
            scorePoints(self->gameData, 500);
            buzzer_stop();
             //buzzer_play_sfx(&sndLevelClearC);
            self->spriteIndex = SP_PLAYER_WIN;
            self->updateFunction = &updateDummy;
            self->gameData->changeState = ST_LEVEL_CLEAR;
        }
        break;
    }
    case TILE_GOAL_1000PTS:
    {
        if(direction == 4) {
            scorePoints(self->gameData, 1000);
            buzzer_stop();
             //buzzer_play_sfx(&sndLevelClearB);
            self->spriteIndex = SP_PLAYER_WIN;
            self->updateFunction = &updateDummy;
            self->gameData->changeState = ST_LEVEL_CLEAR;
        }
        break;
    }
    case TILE_GOAL_2000PTS:
    {
        if(direction == 4) {
            scorePoints(self->gameData, 2000);
            buzzer_stop();
             //buzzer_play_sfx(&sndLevelClearA);
            self->spriteIndex = SP_PLAYER_WIN;
            self->updateFunction = &updateDummy;
            self->gameData->changeState = ST_LEVEL_CLEAR;
        }
        break;
    }
    case TILE_GOAL_5000PTS:
    {
        if(direction == 4) {
            scorePoints(self->gameData, 5000);
            buzzer_stop();
             //buzzer_play_sfx(&sndLevelClearS);
            self->spriteIndex = SP_PLAYER_WIN;
            self->updateFunction = &updateDummy;
            self->gameData->changeState = ST_LEVEL_CLEAR;
        }
        break;
    }
    default:
    {
        break;
    }
    }
 */
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
            bzrPlaySfx(&(self->soundManager->hit2));
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

    if(gameData->targetBlocksBroken >= tilemap->totalTargetBlocks){
        gameData->changeState = ST_LEVEL_CLEAR;
    }
};

/*
void dieWhenFallingOffScreen(entity_t *self)
{
    uint16_t deathBoundary = (self->tilemap->mapOffsetY + TILEMAP_DISPLAY_HEIGHT_PIXELS + DESPAWN_THRESHOLD);
    if (
        ((self->y >> SUBPIXEL_RESOLUTION) > deathBoundary) &&
        ((self->y >> SUBPIXEL_RESOLUTION) < deathBoundary + DESPAWN_THRESHOLD))
    {
        self->hp = 0;
        updateLedsHpMeter(self->entityManager, self->gameData);
        self->gameData->changeState = ST_DEAD;
        destroyEntity(self, true);
    }
}*/

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

/*
void updateScrollLockLeft(entity_t *self)
{
    self->tilemap->minMapOffsetX = (self->x >> SUBPIXEL_RESOLUTION) - 8;
    viewFollowEntity(self->entityManager->tilemap, self->entityManager->viewEntity);
    destroyEntity(self, true);
}

void updateScrollLockRight(entity_t *self)
{
    self->tilemap->maxMapOffsetX = (self->x >> SUBPIXEL_RESOLUTION) + 8 - TILEMAP_DISPLAY_WIDTH_PIXELS;
    viewFollowEntity(self->entityManager->tilemap, self->entityManager->viewEntity);
    destroyEntity(self, true);
}

void updateScrollLockUp(entity_t *self)
{
    self->tilemap->minMapOffsetY = (self->y >> SUBPIXEL_RESOLUTION) - 8;
    viewFollowEntity(self->entityManager->tilemap, self->entityManager->viewEntity);
    destroyEntity(self, true);
}

void updateScrollLockDown(entity_t *self)
{
    self->tilemap->maxMapOffsetY = (self->y >> SUBPIXEL_RESOLUTION) + 8 - TILEMAP_DISPLAY_HEIGHT_PIXELS;
    viewFollowEntity(self->entityManager->tilemap, self->entityManager->viewEntity);
    destroyEntity(self, true);
}

void updateScrollUnlock(entity_t *self)
{
    unlockScrolling(self->tilemap);
    viewFollowEntity(self->entityManager->tilemap, self->entityManager->viewEntity);
    destroyEntity(self, true);
}

void updateEntityDead(entity_t *self)
{
    applyGravity(self);
    self->x += self->xspeed;
    self->y += self->yspeed;

    despawnWhenOffscreen(self);
}

void updatePowerUp(entity_t *self)
{
    if(self->gameData->frameCount % 10 == 0) {
        self->spriteIndex = ((self->entityManager->playerEntity->hp < 2) ? SP_GAMING_1 : SP_MUSIC_1) + ((self->spriteIndex + 1) % 3);
    }

    moveEntityWithTileCollisions(self);
    applyGravity(self);
    despawnWhenOffscreen(self);
}

void update1up(entity_t *self)
{
    if(self->gameData->frameCount % 10 == 0) {
        self->spriteIndex = SP_1UP_1 + ((self->spriteIndex + 1) % 3);
    }

    moveEntityWithTileCollisions(self);
    applyGravity(self);
    despawnWhenOffscreen(self);
}

void updateWarp(entity_t *self)
{
    if(self->gameData->frameCount % 10 == 0) {
        self->spriteIndex = SP_WARP_1 + ((self->spriteIndex + 1) % 3);
    }

    //Destroy self and respawn warp container block when offscreen
    if (
        (self->x >> SUBPIXEL_RESOLUTION) < (self->tilemap->mapOffsetX - DESPAWN_THRESHOLD) ||
        (self->x >> SUBPIXEL_RESOLUTION) > (self->tilemap->mapOffsetX + TILEMAP_DISPLAY_WIDTH_PIXELS + DESPAWN_THRESHOLD) ||
        (self->y >> SUBPIXEL_RESOLUTION) < (self->tilemap->mapOffsetY - DESPAWN_THRESHOLD) ||
        (self->y >> SUBPIXEL_RESOLUTION) > (self->tilemap->mapOffsetY + TILEMAP_DISPLAY_HEIGHT_PIXELS + DESPAWN_THRESHOLD))
    {
        //In destroyEntity, this will overflow to the correct value.
        self->type = 128 + TILE_CONTAINER_1;

        destroyEntity(self, true);
    }
}

void updateDustBunny(entity_t *self)
{
    if(!self->falling){
        self->yDamping--;
        if(self->yDamping <= 0){
            bool directionToPlayer = (self->entityManager->playerEntity->x < self->x);
            
            switch(self->xDamping){
                case 0: {
                    self->yspeed = (int32_t)(2 + esp_random() % 3) * -24;
                    self->falling = true;
                    self->xDamping = 1;
                    self->yDamping = (1 + esp_random() % 3) * 9;
                    self->spriteIndex = SP_DUSTBUNNY_JUMP;
                    self->spriteFlipHorizontal = directionToPlayer;
                    break;
                }
                case 1: {
                    self->xDamping = 0;
                    self->yDamping = 30;
                    self->spriteIndex = SP_DUSTBUNNY_CHARGE;
                    self->spriteFlipHorizontal = directionToPlayer;
                    break;
                }
                default:
                    self->xDamping = 0;
                    break;
            }
        }
    }
    
    despawnWhenOffscreen(self);
    moveEntityWithTileCollisions(self);
    applyGravity(self);
    detectEntityCollisions(self);
};

void updateDustBunnyL2(entity_t *self)
{
    if(!self->falling){
        self->yDamping--;
        if(self->yDamping <= 0){
            switch(self->xDamping){
                case 0: {
                    self->xspeed = (1 + esp_random() % 4) * 6 * ((self->spriteFlipHorizontal)?-1:1);
                    self->yspeed = (int32_t)(1 + esp_random() % 4) * -24;
                    self->xDamping = 1;
                    self->yDamping = (esp_random() % 3) * 6;
                    self->spriteIndex = SP_DUSTBUNNY_L2_JUMP;
                    break;
                }
                case 1: {
                    self->xDamping = 0;
                    self->yDamping = 15;
                    self->spriteIndex = SP_DUSTBUNNY_L2_CHARGE;
                    break;
                }
                default:
                    self->xDamping = 0;
                    break;
            }
        }
    }
    
    despawnWhenOffscreen(self);
    moveEntityWithTileCollisions(self);
    applyGravity(self);
    detectEntityCollisions(self);
};

void updateDustBunnyL3(entity_t *self)
{
    if(!self->falling){
        self->yDamping--;
        if(self->yDamping <= 0){
            bool directionToPlayer = (self->entityManager->playerEntity->x < self->x);
            
            switch(self->xDamping){
                case 0: {
                    self->xspeed = (int32_t)(1 + esp_random() % 4) * 6 * ((directionToPlayer)?-1:1);
                    self->yspeed = (int32_t)(1 + esp_random() % 4) * -24;
                    self->xDamping = 1;
                    self->yDamping = (esp_random() % 3) * 30;
                    self->spriteIndex = SP_DUSTBUNNY_L3_JUMP;
                    self->spriteFlipHorizontal = directionToPlayer;
                    break;
                }
                case 1: {
                    self->xDamping = 0;
                    self->yDamping = 30;
                    self->spriteIndex = SP_DUSTBUNNY_L3_CHARGE;
                    self->spriteFlipHorizontal = directionToPlayer;
                    break;
                }
                default:
                    self->xDamping = 0;
                    break;
            }
        }
    }
    
    despawnWhenOffscreen(self);
    moveEntityWithTileCollisions(self);
    applyGravity(self);
    detectEntityCollisions(self);
};

bool dustBunnyTileCollisionHandler(entity_t *self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction){
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
                        self->xDamping = 0;
                        self->xspeed = 0;
                        self->yspeed = 0;
                        self->falling = false;
                        self->yDamping = -1;
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
            self->xspeed = 0;
            self->spriteIndex = SP_DUSTBUNNY_IDLE;
            break;
        default: // Should never hit
            return false;
        }
        // trigger tile collision resolution
        return true;
    }

    return false;
};

bool dustBunnyL2TileCollisionHandler(entity_t *self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction){
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
                        self->xDamping = 0;
                        self->xspeed = 0;
                        self->yspeed = 0;
                        self->falling = false;
                        self->yDamping = -1;
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
            self->spriteFlipHorizontal = false;
            break;
        case 1: // PB_RIGHT
            self->xspeed = -self->xspeed;
            self->spriteFlipHorizontal = true;
            break;
        case 2: // PB_UP
            self->yspeed = 0;
            break;
        case 4: // PB_DOWN
            // Landed on platform
            self->falling = false;
            self->yspeed = 0;
            self->xspeed = 0;
            self->spriteIndex = SP_DUSTBUNNY_L2_IDLE;
            break;
        default: // Should never hit
            return false;
        }
        // trigger tile collision resolution
        return true;
    }

    return false;
};

bool dustBunnyL3TileCollisionHandler(entity_t *self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction){
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
                        self->xDamping = 0;
                        self->xspeed = 0;
                        self->yspeed = 0;
                        self->falling = false;
                        self->yDamping = -1;
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
            self->xspeed = 0;
            self->spriteIndex = SP_DUSTBUNNY_L3_IDLE;
            break;
        default: // Should never hit
            return false;
        }
        // trigger tile collision resolution
        return true;
    }

    return false;
};

void updateWasp(entity_t *self)
{
    switch(self->xDamping){
        case 0:
            if(self->gameData->frameCount % 5 == 0) {
                self->spriteIndex = SP_WASP_1 + ((self->spriteIndex + 1) % 2);
            }
            self->yDamping--;

            if(self->entityManager->playerEntity->y > self->y && self->yDamping < 0 && abs(self->x - self->entityManager->playerEntity->x) < 512) {
                self->xDamping = 1;
                self->gravityEnabled = true;
                self->falling = true;
                self->spriteIndex = SP_WASP_DIVE;
                self->xspeed = 0;
                self->yspeed = 64;
            }
            break;
        case 1:
            if(!self->falling) {
                self->yDamping--;
                if(self->yDamping < 0){
                    self->xDamping = 2;
                    self->gravityEnabled = false;
                    self->falling = false;
                    self->yspeed = -24;
                    self->yDamping = 120;
                }
            }
            break;
        case 2:
            if(self->gameData->frameCount % 2 == 0) {
                self->spriteIndex = SP_WASP_1 + ((self->spriteIndex + 1) % 2);
            }

            self->yDamping--;
            if(self->yDamping <0 || self->y <= ((self->homeTileY * TILE_SIZE + 8) << SUBPIXEL_RESOLUTION )) {
                self->xDamping = 0;
                self->xspeed = (self->spriteFlipHorizontal)? -16 : 16;
                self->yspeed = 0;
                self->yDamping = (1 + esp_random() % 2) * 20;
            }
        default:
            break;
    }
    
    despawnWhenOffscreen(self);
    moveEntityWithTileCollisions(self);
    applyGravity(self);
    detectEntityCollisions(self);
};

void updateWaspL2(entity_t *self)
{
    switch(self->xDamping){
        case 0:
            if(self->gameData->frameCount % 5 == 0) {
                self->spriteIndex = SP_WASP_L2_1 + ((self->spriteIndex) % 2);
            }

            self->yDamping--;
            if(esp_random() % 256 > 240){
                bool directionToPlayer = self->entityManager->playerEntity->x < self->x;
                self->xspeed = directionToPlayer ? -24:24;
                self->spriteFlipHorizontal = directionToPlayer;
            }

            if(self->entityManager->playerEntity->y > self->y && self->yDamping < 0 && abs(self->x - self->entityManager->playerEntity->x) < self->jumpPower) {
                self->xDamping = 1;
                self->gravityEnabled = true;
                self->falling = true;
                self->spriteIndex = SP_WASP_L2_DIVE;
                self->xspeed = 0;
                self->yspeed = 96;
            }
            break;
        case 1:
            if(!self->falling) {
                self->yDamping -= 2;
                if(self->yDamping < 0){
                    self->xDamping = 2;
                    self->gravityEnabled = false;
                    self->falling = false;
                    self->yspeed = -48;
                    self->jumpPower = (1 + esp_random() % 3) * 256;
                    self->yDamping = 80;
                }
            }
            break;
        case 2:
            if(self->gameData->frameCount % 2 == 0) {
                self->spriteIndex = SP_WASP_L2_1 + ((self->spriteIndex) % 2);
            }

            self->yDamping--;
            if(self->yDamping < 0 || self->y <= ((self->homeTileY * TILE_SIZE + 8) << SUBPIXEL_RESOLUTION )) {
                self->xDamping = 0;
                self->xspeed = (self->spriteFlipHorizontal)? -24 : 24;
                self->yspeed = 0;
                self->yDamping = (1 + esp_random() % 2) * 20;
            }
            break;
        default:
            break;
    }
    
    despawnWhenOffscreen(self);
    moveEntityWithTileCollisions(self);
    applyGravity(self);
    detectEntityCollisions(self);
};

void updateWaspL3(entity_t *self)
{
    switch(self->xDamping){
        case 0:
            if(self->gameData->frameCount % 5 == 0) {
                self->spriteIndex = SP_WASP_L3_1 + ((self->spriteIndex + 1) % 2);
            }

            self->yDamping--;
            if(esp_random() % 256 > 192){
                bool directionToPlayer = self->entityManager->playerEntity->x < self->x;
                self->xspeed = directionToPlayer ? -32:32;
                self->spriteFlipHorizontal = directionToPlayer;
            }

            if(self->entityManager->playerEntity->y > self->y && self->yDamping < 0 && abs(self->x - self->entityManager->playerEntity->x) < self->jumpPower) {
                self->xDamping = 1;
                self->gravityEnabled = true;
                self->falling = true;
                self->spriteIndex = SP_WASP_L3_DIVE;
                self->xspeed = 0;
                self->yspeed = 128;
            }
            break;
        case 1:
            if(!self->falling) {
                self->yDamping -= 4;
                if(self->yDamping < 0){
                    self->xDamping = 2;
                    self->gravityEnabled = false;
                    self->falling = false;
                    self->yspeed = -64;
                    self->jumpPower = (1 + esp_random() % 3) * 256;
                    self->yDamping = (2 + esp_random() % 6) * 8;
                }
            }
            break;
        case 2:
            if(self->gameData->frameCount % 2 == 0) {
                self->spriteIndex = SP_WASP_L3_1 + ((self->spriteIndex + 1) % 2);
            }

            self->yDamping--;
            if(self->yDamping < 0 || self->y <= ((self->homeTileY * TILE_SIZE + 8) << SUBPIXEL_RESOLUTION )) {
                self->xDamping = 0;
                self->xspeed = (self->spriteFlipHorizontal)? -32 : 32;
                self->yspeed = 0;
                self->yDamping = (1 + esp_random() % 2) * 20;
            }
            break;
        default:
            break;
    }
    
    despawnWhenOffscreen(self);
    moveEntityWithTileCollisions(self);
    applyGravity(self);
    detectEntityCollisions(self);
};

bool waspTileCollisionHandler(entity_t *self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction){
    switch(tileId){
        case TILE_BOUNCE_BLOCK: {
            self->xDamping = 1;
            self->falling = false;
            self->yDamping = 40;

            switch (direction)
            {
                case 0:
                    self->xspeed = 48;
                    break;
                case 1:
                    self->xspeed = -48;
                    break;
                case 2:
                    self->yspeed = 48;
                    break;
                case 4:
                    self->yspeed = -48;
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
        case 1: // PB_RIGHT
            self->spriteFlipHorizontal = !self->spriteFlipHorizontal;
            self->xspeed = -self->xspeed;
            break;
        case 2: // PB_UP
            self->yspeed = 0;
            break;
        case 4: // PB_DOWN
            // Landed on platform
            self->falling = false;
            self->yspeed = 0;
            self->xspeed = 0;
            self->xDamping = 1;
            self->yDamping = 40;
            
            break;
        default: // Should never hit
            return false;
        }
        // trigger tile collision resolution
        return true;
    }

    return false;
};

void killEnemy(entity_t* target){
    target->homeTileX = 0;
    target->homeTileY = 0;
    target->gravityEnabled = true;
    target->falling = true;
    target->type = ENTITY_DEAD;
    target->spriteFlipVertical = true;
    target->updateFunction = &updateEntityDead;
}

void updateBgCol(entity_t *self)
{
    self->gameData->bgColor = self->xDamping;
    destroyEntity(self, true);
}

void turnAroundAtEdgeOfTileHandler(entity_t *self){
    self->falling = true;
    self->xspeed = -self->xspeed;
    self->yspeed = -self->gravity*4;
}

void updateEnemyBushL3(entity_t* self){
    if(self->gameData->frameCount % 10 == 0) {
        self->spriteFlipHorizontal = !self->spriteFlipHorizontal;
    }

    self->yDamping--;
    if(self->yDamping < 0){
        bool directionToPlayer = (self->entityManager->playerEntity->x < self->x);

        if( (self->xspeed < 0 && directionToPlayer) || (self->xspeed > 0 && !directionToPlayer) ){
            self->xspeed = -self->xspeed;
        } else {
            self->xspeed = (directionToPlayer)? -16: 16;
            self->yspeed = -24;
            self->falling = true;
        }

        self->yDamping = (1 + esp_random() % 7) * 30;
        
    }

    despawnWhenOffscreen(self);
    moveEntityWithTileCollisions(self);
    applyGravity(self);
    detectEntityCollisions(self);
}

void updateCheckpoint(entity_t* self){
    if(self->xDamping){
        if(self->gameData->frameCount % 15 == 0) {
            self->spriteIndex = SP_CHECKPOINT_ACTIVE_1 + ((self->spriteIndex + 1) % 2);
        }
    }

    despawnWhenOffscreen(self);
}
*/
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

/*
void updateBgmChange(entity_t* self){
    self->gameData->changeBgm = self->xDamping;
    destroyEntity(self, true);
}

void updateWaveBall(entity_t* self){
    if(self->gameData->frameCount % 4 == 0) {
        self->spriteIndex = (SP_WAVEBALL_1 + ((self->spriteIndex + 1) % 3));
    }

    if(self->gameData->frameCount % 4 == 0) {
        self->xDamping++;

        switch(self->xDamping){
            case 0:
                break;
            case 1:
                self->yDamping = self->xspeed+2; //((esp_random() % 2)?-16:16);
                self->yspeed = -abs(self->yDamping);
                self->xspeed = 0;
                break;
            case 2:
                self->yspeed = 0;
                self->xspeed = self->yDamping;
                break;
            case 3:
                self->yDamping = self->xspeed+2; //((esp_random() % 2)?-16:16);
                self->yspeed = abs(self->yDamping);
                self->xspeed = 0;
                break;
            case 4:
                self->yspeed = 0;
                self->xspeed = self->yDamping;
                self->xDamping = 0;
                break;
            default:
                break;
        }
    }

    //self->yDamping++;

    moveEntityWithTileCollisions(self);
    despawnWhenOffscreen(self);
}

// bool waveBallTileCollisionHandler(entity_t *self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction){
//     if(self->yspeed == 0){
//         destroyEntity(self, false);
//     }
//     return false;
// }

void waveBallOverlapTileHandler(entity_t *self, uint8_t tileId, uint8_t tx, uint8_t ty){
    if(isSolid(tileId) || tileId == TILE_BOUNCE_BLOCK){
        destroyEntity(self, false);
         //buzzer_play_sfx(&sndHit);
    }
}

void powerUpCollisionHandler(entity_t *self, entity_t *other)
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
            }
            break;
        case ENTITY_HIT_BLOCK:
            self->xspeed = other->xspeed;
            self->yspeed = other->yspeed;
            break;
        default:
        {
            break;
        }
    }
}

void killPlayer(entity_t *self)
{
    self->hp = 0;
    updateLedsHpMeter(self->entityManager, self->gameData);
    
    self->updateFunction = &updateEntityDead;
    self->type = ENTITY_DEAD;
    self->xspeed = 0;
    self->yspeed = -60;
    self->spriteIndex = SP_PLAYER_HURT;
    self->gameData->changeState = ST_DEAD;
    self->falling = true;
}
*/