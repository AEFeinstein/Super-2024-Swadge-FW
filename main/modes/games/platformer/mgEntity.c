//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include "mgEntity.h"
#include "mgEntityManager.h"
#include "mgTilemap.h"
#include "mgGameData.h"
#include "soundFuncs.h"
#include "hdw-btn.h"
#include "esp_random.h"
#include "aabb_utils.h"
#include "trigonometry.h"
#include <esp_log.h>
#include "soundFuncs.h"
#include "mega_pulse_ex_typedef.h"
#include "shapes.h"

//==============================================================================
// Constants
//==============================================================================

//==============================================================================
// Functions
//==============================================================================
void mg_initializeEntity(mgEntity_t* self, mgEntityManager_t* entityManager, mgTilemap_t* tilemap,
                         mgGameData_t* gameData, mgSoundManager_t* soundManager)
{
    self->active               = false;
    self->tilemap              = tilemap;
    self->gameData             = gameData;
    self->soundManager         = soundManager;
    self->homeTileX            = 0;
    self->homeTileY            = 0;
    self->gravity              = false;
    self->falling              = false;
    self->entityManager        = entityManager;
    self->fallOffTileHandler   = &defaultFallOffTileHandler;
    self->spriteFlipHorizontal = false;
    self->spriteFlipVertical   = false;
    self->tileCollider         = NULL;
    self->linkedEntity         = NULL;
    self->spawnData            = NULL;

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
}

void mg_updatePlayer(mgEntity_t* self)
{
    switch(self->state){
        case MG_PL_ST_NORMAL:
        default:
            if (self->gameData->doubleTapBtnTimer > 0){
                self->gameData->doubleTapBtnTimer --;

                if(self->gameData->doubleTapBtnTimer <= 0){
                    self->gameData->doubleTapBtnState = 0;
                }
            }
            
            /*
            if (self->gameData->btnState & PB_B)
            {
                self->xMaxSpeed = 52;
            }
            else
            {
                self->xMaxSpeed = 30;
            }
            */

            if (self->gameData->btnState & PB_LEFT)
            {
                //self->xspeed -= (self->falling && self->xspeed < 0) ? (self->xspeed < -24) ? 0 : 2 : 3;
                self->xspeed -= (self->falling && self->xspeed < 0) ? (self->xspeed < -24) ? 0 : 8 : 8;

                if (self->xspeed < -self->xMaxSpeed)
                {
                    self->xspeed = -self->xMaxSpeed;
                }

                if(!(self->gameData->prevBtnState & PB_LEFT)) {
                    
                    if(!(self->gameData->doubleTapBtnState & PB_LEFT) ) {
                        self->gameData->doubleTapBtnState = PB_LEFT;
                        self->gameData->doubleTapBtnTimer = MG_DOUBLE_TAP_TIMER_FRAMES;
                    } else if (self->canDash) {
                        //Initiate dash
                        /*self->state = MG_PL_ST_DASHING;

                        if(self->falling){
                            self->stateTimer = 20;
                            self->jumpPower = 0;
                            self->yspeed = 0;
                            self->canDash = false;
                        } else {
                            self->stateTimer = 32;
                        }

                        self->gameData->doubleTapBtnState = 0;
                        self->gameData->doubleTapBtnTimer = -1;
                        self->gravity = 0;*/
                    }
                        
                }
            }
            else if (self->gameData->btnState & PB_RIGHT)
            {
                //self->xspeed += (self->falling && self->xspeed > 0) ? (self->xspeed > 24) ? 0 : 2 : 3;
                self->xspeed += (self->falling && self->xspeed > 0) ? (self->xspeed > 24) ? 0 : 8 : 8;

                if (self->xspeed > self->xMaxSpeed)
                {
                    self->xspeed = self->xMaxSpeed;
                }

                if(!(self->gameData->prevBtnState & PB_RIGHT)) {
                    
                    if(!(self->gameData->doubleTapBtnState & PB_RIGHT) ) {
                        self->gameData->doubleTapBtnState = PB_RIGHT;
                        self->gameData->doubleTapBtnTimer = MG_DOUBLE_TAP_TIMER_FRAMES;
                    } else if (self->canDash) {
                        //Initiate dash
                        /*self->state = MG_PL_ST_DASHING;

                        if(self->falling){
                            self->stateTimer = 20;
                            self->jumpPower = 0;
                            self->yspeed = 0;
                            self->canDash = false;
                        } else {
                            self->stateTimer = 32;
                        }

                        self->gameData->doubleTapBtnState = 0;
                        self->gameData->doubleTapBtnTimer = -1;
                        self->gravity = 0;*/
                    }
                        
                }
            }
            break;
        case MG_PL_ST_DASHING:

            if(self->spriteFlipHorizontal) {
                self->xspeed = -64;

                if(!self->falling && !(self->gameData->btnState & PB_LEFT)){
                    self->stateTimer = 0;
                } else if (self->gameData->btnState & PB_RIGHT) {
                    self->spriteFlipHorizontal = false;
                }
            } else {
                self->xspeed = 64;

                if(!self->falling && !(self->gameData->btnState & PB_RIGHT)){
                    self->stateTimer = 0;
                } else if (self->gameData->btnState & PB_LEFT) {
                    self->spriteFlipHorizontal = true;
                }
            }
            
            self->stateTimer--;
            if(self->stateTimer <= 0){
                self->state = MG_PL_ST_NORMAL;
                self->gravity = 4;
            }
            break;
        
        case MG_PL_ST_MIC_DROP:
            if(self->yspeed > 0){
                self->yspeed += 8;
            }

            self->stateTimer--;
            if(self->stateTimer <= 0){
                self->state = MG_PL_ST_NORMAL;
                self->yMaxSpeed = 72;
                self->spriteFlipVertical = false;
            }
            break;
    }
    
    /*
    if (self->gameData->btnState & PB_LEFT)
    {
        self->xspeed = -26;
    }
    else if (self->gameData->btnState & PB_RIGHT)
    {
        self->xspeed = 26;
    }*/

    if (!self->gravityEnabled)
    {
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
            self->jumpPower = 60; //+ ((abs(self->xspeed) + 16) >> 3);
            self->yspeed    = -self->jumpPower;
            self->falling   = true;

            if(self->state == MG_PL_ST_DASHING){
                self->canDash = false;
            }

            soundPlaySfx(&(self->soundManager->sndJump1), BZR_LEFT);
        } else if (mg_canWallJump(self) && !(self->gameData->prevBtnState & PB_A)) {
                self->jumpPower = 60; //+ ((abs(self->xspeed) + 16) >> 3);
                self->xspeed = (self->spriteFlipHorizontal) ? 32 : -32;
                self->yspeed    = -self->jumpPower;
                self->falling   = true;
                
                if(self->state == MG_PL_ST_DASHING){
                    self->state = MG_PL_ST_NORMAL;
                    self->stateTimer = -1;
                    self->gravity = 4;
                }
                
                self->spriteFlipHorizontal = (self->xspeed > 0) ? 0 : 1;
                soundPlaySfx(&(self->soundManager->sndJump1), BZR_LEFT);
        }
        else if (self->jumpPower > 0 && self->yspeed < 0)
        {
            // jump dampening
            self->jumpPower -= 2; // 32
            self->yspeed = -self->jumpPower;

            if (self->jumpPower > 35 && self->jumpPower < 37)
            {
                soundPlaySfx(&(self->soundManager->sndJump2), BZR_LEFT);
            }

            if (self->yspeed > -6 && self->yspeed < -2)
            {
                soundPlaySfx(&(self->soundManager->sndJump3), BZR_LEFT);
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
        self->yspeed    = self->yspeed / 4;
    }

    mg_updateInvincibilityFrames(self);

    if (self->animationTimer > 0)
    {
        self->animationTimer--;

        if(self->animationTimer == 0){
            mg_remapPlayerNotShootWsg(self->tilemap->wsgManager);
        }
    }

    if (self->gameData->btnState & PB_B && !(self->gameData->prevBtnState & PB_B)
        && self->shotsFired < self->shotLimit)
    {
        switch(self->state){
            case MG_PL_ST_NORMAL:
                if(self->falling && self->gameData->btnState & PB_DOWN)
                {
                    self->xspeed = 0;
                    self->yspeed = -32l;
                    self->spriteFlipVertical = true;
                    self->state = MG_PL_ST_MIC_DROP;
                    self->yMaxSpeed = 120;
                    self->stateTimer = 180;
                } 
                else if(self->shotsFired < self->shotLimit)
                {
                    mgEntity_t* createdEntity = mg_createEntity(self->entityManager, ENTITY_WAVE_BALL,
                                                    TO_PIXEL_COORDS(self->x), TO_PIXEL_COORDS(self->y) - 2);
                    if (createdEntity != NULL)
                    {
                        createdEntity->xspeed    = (self->spriteFlipHorizontal) ? -(96 + abs(self->xspeed) /*+ abs(self->yspeed)*/)
                                                                                : 96 + abs(self->xspeed) /*+ abs(self->yspeed)*/;
                        createdEntity->homeTileX = 0;
                        createdEntity->homeTileY = 0;
                        createdEntity->linkedEntity = self;
                        self->shotsFired++;
                        soundPlaySfx(&(self->soundManager->sndWaveBall), BZR_LEFT);
                        mg_remapPlayerShootWsg(self->tilemap->wsgManager);
                    }
                    self->animationTimer = 10;
                }
                break;
            default:
                break;
        }
        
    }

    if (((self->gameData->btnState & PB_START) && !(self->gameData->prevBtnState & PB_START)))
    {
        self->gameData->changeState = MG_ST_PAUSE;
    }

    mg_moveEntityWithTileCollisions3(self);
    dieWhenFallingOffScreen(self);
    applyGravity(self);
    applyDamping(self);
    mg_detectEntityCollisions(self);
    animatePlayer(self);
}

void updateTestObject(mgEntity_t* self)
{
    if (self->gameData->frameCount % 10 == 0)
    {
        self->spriteFlipHorizontal = !self->spriteFlipHorizontal;
    }

    mg_updateInvincibilityFrames(self);

    despawnWhenOffscreen(self);
    mg_moveEntityWithTileCollisions(self);
    applyGravity(self);
    mg_detectEntityCollisions(self);
}

void mg_updateInvincibilityFrames(mgEntity_t * self)
{
    if (self->invincibilityFrames > 0)
    {
        self->invincibilityFrames--;
        if (self->invincibilityFrames & 0b1)
        {
            self->visible = !self->visible;
        }

        if (self->invincibilityFrames <= 0)
        {
            self->visible = true;
        }
    }
}

void updateHitBlock(mgEntity_t* self)
{
    if (self->homeTileY > self->tilemap->mapHeight)
    {
        mg_destroyEntity(self, false);
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
        uint8_t aboveTile         = (self->homeTileY == 0)
                                        ? 0
                                        : self->tilemap->map[(self->homeTileY - 1) * self->tilemap->mapWidth + self->homeTileX];
        uint8_t belowTile         = (self->homeTileY == (self->tilemap->mapHeight - 1))
                                        ? 0
                                        : self->tilemap->map[(self->homeTileY + 1) * self->tilemap->mapWidth + self->homeTileX];
        mgEntity_t* createdEntity = NULL;

        switch (aboveTile)
        {
            case MG_TILE_CTNR_COIN:
            case MG_TILE_CTNR_10COIN:
            {
                addCoins(self->gameData, 1);
                mg_scorePoints(self->gameData, 10);
                self->jumpPower = MG_TILE_CONTAINER_2;
                break;
            }
            case MG_TILE_CTNR_POW1:
            {
                createdEntity = mg_createEntity(
                    self->entityManager, ENTITY_POWERUP, (self->homeTileX * MG_TILESIZE) + MG_HALF_TILESIZE,
                    ((self->homeTileY
                      + ((self->yspeed < 0 && (!mg_isSolid(belowTile) && belowTile != MG_TILE_BOUNCE_BLOCK)) ? 1 : -1))
                     * MG_TILESIZE)
                        + MG_HALF_TILESIZE);
                createdEntity->homeTileX = 0;
                createdEntity->homeTileY = 0;

                self->jumpPower = MG_TILE_CONTAINER_2;
                break;
            }
            case MG_TILE_WARP_0 ... MG_TILE_WARP_F:
            {
                createdEntity = mg_createEntity(
                    self->entityManager, ENTITY_WARP, (self->homeTileX * MG_TILESIZE) + MG_HALF_TILESIZE,
                    ((self->homeTileY
                      + ((self->yspeed < 0 && (!mg_isSolid(belowTile) && belowTile != MG_TILE_BOUNCE_BLOCK)) ? 1 : -1))
                     * MG_TILESIZE)
                        + MG_HALF_TILESIZE);

                createdEntity->homeTileX = self->homeTileX;
                createdEntity->homeTileY = self->homeTileY;

                createdEntity->jumpPower = aboveTile - MG_TILE_WARP_0;
                self->jumpPower          = MG_TILE_CONTAINER_2;
                break;
            }
            case MG_TILE_CTNR_1UP:
            {
                if (self->gameData->extraLifeCollected)
                {
                    addCoins(self->gameData, 1);
                    mg_scorePoints(self->gameData, 10);
                }
                else
                {
                    createdEntity = mg_createEntity(
                        self->entityManager, ENTITY_1UP, (self->homeTileX * MG_TILESIZE) + MG_HALF_TILESIZE,
                        ((self->homeTileY
                          + ((self->yspeed < 0 && (!mg_isSolid(belowTile) && belowTile != MG_TILE_BOUNCE_BLOCK)) ? 1
                                                                                                                : -1))
                         * MG_TILESIZE)
                            + MG_HALF_TILESIZE);
                    createdEntity->homeTileX           = 0;
                    createdEntity->homeTileY           = 0;
                    self->gameData->extraLifeCollected = true;
                }

                self->jumpPower = MG_TILE_CONTAINER_2;
                break;
            }
            default:
            {
                break;
            }
        }

        if (self->jumpPower == MG_TILE_BRICK_BLOCK && (self->yspeed > 0 || self->yDamping == 1) && createdEntity == NULL)
        {
            self->jumpPower = MG_TILE_EMPTY;
            mg_scorePoints(self->gameData, 10);
            soundPlaySfx(&(self->soundManager->sndBreak), BZR_LEFT);
        }

        self->tilemap->map[self->homeTileY * self->tilemap->mapWidth + self->homeTileX] = self->jumpPower;

        mg_destroyEntity(self, false);
    }
}

void mg_moveEntityWithTileCollisions(mgEntity_t* self)
{
    uint16_t newX = self->x;
    uint16_t newY = self->y;
    uint8_t tx    = MG_TO_TILECOORDS(TO_PIXEL_COORDS(self->x));
    uint8_t ty    = MG_TO_TILECOORDS(TO_PIXEL_COORDS(self->y));
    // bool collision = false;

    // Are we inside a block? Push self out of block
    uint8_t t = mg_getTile(self->tilemap, tx, ty);
    self->overlapTileHandler(self, t, tx, ty);

    if (mg_isSolid(t))
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
            int16_t hcof = (((TO_PIXEL_COORDS(self->x)) % MG_TILESIZE) - MG_HALF_TILESIZE);

            // Handle halfway though tile
            uint8_t at = mg_getTile(self->tilemap, tx + SIGNOF(hcof), ty);

            if (mg_isSolid(at))
            {
                // collision = true;
                newX = TO_SUBPIXEL_COORDS((tx + 1) * MG_TILESIZE - MG_HALF_TILESIZE);
            }

            uint8_t newTy = MG_TO_TILECOORDS( TO_PIXEL_COORDS((self->y + self->yspeed))
                                             + SIGNOF(self->yspeed) * MG_HALF_TILESIZE);

            if (newTy != ty)
            {
                uint8_t newVerticalTile = mg_getTile(self->tilemap, tx, newTy);

                if (newVerticalTile > MG_TILE_UNUSED_29 && newVerticalTile < MG_TILE_BG_GOAL_ZONE)
                {
                    if (self->tileCollisionHandler(self, newVerticalTile, tx, newTy, 2 << (self->yspeed > 0)))
                    {
                        newY = TO_SUBPIXEL_COORDS((newTy + ((ty < newTy) ? -1 : 1)) * MG_TILESIZE + MG_HALF_TILESIZE);
                    }
                }
            }
        }

        if (self->xspeed != 0)
        {
            int16_t vcof = ((TO_PIXEL_COORDS(self->y) % MG_TILESIZE) - MG_HALF_TILESIZE);

            // Handle halfway though tile
            uint8_t att = mg_getTile(self->tilemap, tx, ty + SIGNOF(vcof));

            if (mg_isSolid(att))
            {
                // collision = true;
                newY = TO_SUBPIXEL_COORDS((ty + 1) * MG_TILESIZE - MG_HALF_TILESIZE);
            }

            // Handle outside of tile
            uint8_t newTx = MG_TO_TILECOORDS(TO_PIXEL_COORDS((self->x + self->xspeed))
                                             + SIGNOF(self->xspeed) * MG_HALF_TILESIZE);

            if (newTx != tx)
            {
                uint8_t newHorizontalTile = mg_getTile(self->tilemap, newTx, ty);

                if (newHorizontalTile > MG_TILE_UNUSED_29 && newHorizontalTile < MG_TILE_BG_GOAL_ZONE)
                {
                    if (self->tileCollisionHandler(self, newHorizontalTile, newTx, ty, (self->xspeed > 0)))
                    {
                        newX = TO_SUBPIXEL_COORDS((newTx + ((tx < newTx) ? -1 : 1)) * MG_TILESIZE + MG_HALF_TILESIZE);
                    }
                }

                if (!self->falling)
                {
                    uint8_t newBelowTile = mg_getTile(self->tilemap, tx, ty + 1);

                    if ((self->gravityEnabled
                         && !mg_isSolid(newBelowTile)) /*(|| (!self->gravityEnabled && newBelowTile != MG_TILELADDER)*/)
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

void mg_moveEntityWithTileCollisions3(mgEntity_t* self)
{
    uint16_t x = TO_PIXEL_COORDS(self->x);
    uint16_t y = TO_PIXEL_COORDS(self->y);
    int16_t xspeed = TO_PIXEL_COORDS(self->xspeed);
    int16_t yspeed = TO_PIXEL_COORDS(self->yspeed);
    mg_EntityTileCollider_t* tileCollider = self->tileCollider;

    int16_t offX, offY, tempX, tempY, tempTx, tempTy, tempT, newX, newY, onGround;

    newX = 0;
    newY = 0;

    if(self->xspeed > 0){
        const mg_EntityTileCollisionPointList_t* rightEdge = tileCollider->rightEdge;
        for(int i=0; i<rightEdge->size; i++){
            offX = rightEdge->collisionPoints[i].x;
            offY = rightEdge->collisionPoints[i].y;
            tempX = x + offX + xspeed;
            tempY = y + offY;

            tempTx = MG_TO_TILECOORDS(tempX);
            tempTy = MG_TO_TILECOORDS(tempY);

            //drawLine(tempX - self->tilemap->mapOffsetX, tempY  - self->tilemap->mapOffsetY, tempX + xspeed + (SIGNOF(xspeed) * MG_HALF_TILESIZE)  - self->tilemap->mapOffsetX, tempY + yspeed + (SIGNOF(yspeed) * MG_HALF_TILESIZE)  - self->tilemap->mapOffsetY, c500, 0);
            //drawRect((tempTx << MG_TILESIZE_IN_POWERS_OF_2) - self->tilemap->mapOffsetX, (tempTy << MG_TILESIZE_IN_POWERS_OF_2) - self->tilemap->mapOffsetY, (tempTx << MG_TILESIZE_IN_POWERS_OF_2) + MG_TILESIZE - self->tilemap->mapOffsetX, (tempTy << MG_TILESIZE_IN_POWERS_OF_2) + MG_TILESIZE - self->tilemap->mapOffsetY, c500);

            tempT = mg_getTile(self->tilemap, tempTx, tempTy);
            
            if(self->tileCollisionHandler(self, tempT, tempTx, tempTy, 1)){
                newX = ((tempTx) << MG_TILESIZE_IN_POWERS_OF_2) - offX;
            }
        }
    } else if(self->xspeed < 0){
        const mg_EntityTileCollisionPointList_t* leftEdge = tileCollider->leftEdge;
        for(int i=0; i<leftEdge->size; i++){
            offX = leftEdge->collisionPoints[i].x;
            offY = leftEdge->collisionPoints[i].y;
            tempX = x + offX + xspeed;
            tempY = y + offY;

            tempTx = MG_TO_TILECOORDS(tempX);
            tempTy = MG_TO_TILECOORDS(tempY);

            //drawLine(tempX - self->tilemap->mapOffsetX, tempY  - self->tilemap->mapOffsetY, tempX + xspeed + (SIGNOF(xspeed) * MG_HALF_TILESIZE)  - self->tilemap->mapOffsetX, tempY + yspeed + (SIGNOF(yspeed) * MG_HALF_TILESIZE)  - self->tilemap->mapOffsetY, c500, 0);
            //drawRect((tempTx << MG_TILESIZE_IN_POWERS_OF_2) - self->tilemap->mapOffsetX, (tempTy << MG_TILESIZE_IN_POWERS_OF_2) - self->tilemap->mapOffsetY, (tempTx << MG_TILESIZE_IN_POWERS_OF_2) + MG_TILESIZE - self->tilemap->mapOffsetX, (tempTy << MG_TILESIZE_IN_POWERS_OF_2) + MG_TILESIZE - self->tilemap->mapOffsetY, c500);

            tempT = mg_getTile(self->tilemap, tempTx, tempTy);
            
            if(self->tileCollisionHandler(self, tempT, tempTx, tempTy, 0)){
                newX = ((tempTx+1) << MG_TILESIZE_IN_POWERS_OF_2) - offX;
            }
        }
    }

    if(self->yspeed > 0){
        const mg_EntityTileCollisionPointList_t* bottomEdge = tileCollider->bottomEdge;
        for(int i=0; i<bottomEdge->size; i++){
            offX = bottomEdge->collisionPoints[i].x;
            offY = bottomEdge->collisionPoints[i].y;
            tempX = x + offX;
            tempY = y + offY + yspeed;

            tempTx = MG_TO_TILECOORDS(tempX);
            tempTy = MG_TO_TILECOORDS(tempY);

            //drawLine(tempX - self->tilemap->mapOffsetX, tempY  - self->tilemap->mapOffsetY, tempX + xspeed + (SIGNOF(xspeed) * MG_HALF_TILESIZE)  - self->tilemap->mapOffsetX, tempY + yspeed + (SIGNOF(yspeed) * MG_HALF_TILESIZE)  - self->tilemap->mapOffsetY, c500, 0);
            //drawRect((tempTx << MG_TILESIZE_IN_POWERS_OF_2) - self->tilemap->mapOffsetX, (tempTy << MG_TILESIZE_IN_POWERS_OF_2) - self->tilemap->mapOffsetY, (tempTx << MG_TILESIZE_IN_POWERS_OF_2) + MG_TILESIZE - self->tilemap->mapOffsetX, (tempTy << MG_TILESIZE_IN_POWERS_OF_2) + MG_TILESIZE - self->tilemap->mapOffsetY, c500);

            tempT = mg_getTile(self->tilemap, tempTx, tempTy);
            
            if(self->tileCollisionHandler(self, tempT, tempTx, tempTy, 4)){
                newY = ((tempTy) << MG_TILESIZE_IN_POWERS_OF_2) - offY;
                self->falling = false;
            }
        }
    } else if(self->yspeed < 0){
        const mg_EntityTileCollisionPointList_t* topEdge = tileCollider->topEdge;
        for(int i=0; i<topEdge->size; i++){
            offX = topEdge->collisionPoints[i].x;
            offY = topEdge->collisionPoints[i].y;
            tempX = x + offX;
            tempY = y + offY + yspeed;

            tempTx = MG_TO_TILECOORDS(tempX);
            tempTy = MG_TO_TILECOORDS(tempY);

            //drawLine(tempX - self->tilemap->mapOffsetX, tempY  - self->tilemap->mapOffsetY, tempX + xspeed + (SIGNOF(xspeed) * MG_HALF_TILESIZE)  - self->tilemap->mapOffsetX, tempY + yspeed + (SIGNOF(yspeed) * MG_HALF_TILESIZE)  - self->tilemap->mapOffsetY, c500, 0);
            //drawRect((tempTx << MG_TILESIZE_IN_POWERS_OF_2) - self->tilemap->mapOffsetX, (tempTy << MG_TILESIZE_IN_POWERS_OF_2) - self->tilemap->mapOffsetY, (tempTx << MG_TILESIZE_IN_POWERS_OF_2) + MG_TILESIZE - self->tilemap->mapOffsetX, (tempTy << MG_TILESIZE_IN_POWERS_OF_2) + MG_TILESIZE - self->tilemap->mapOffsetY, c500);

            tempT = mg_getTile(self->tilemap, tempTx, tempTy);
            
            if(self->tileCollisionHandler(self, tempT, tempTx, tempTy, 2)){
                newY = ((tempTy+1) << MG_TILESIZE_IN_POWERS_OF_2) - offY;
            }
        }
    } else if (!self->falling) {
        onGround = false;
        const mg_EntityTileCollisionPointList_t* bottomEdge = tileCollider->bottomEdge;

        for(int i=0; i<bottomEdge->size; i++){
            offX = bottomEdge->collisionPoints[i].x;
            offY = bottomEdge->collisionPoints[i].y;
            tempX = x + offX;
            tempY = y + offY + 1;

            tempTx = MG_TO_TILECOORDS(tempX);
            tempTy = MG_TO_TILECOORDS(tempY);

             tempT = mg_getTile(self->tilemap, tempTx, tempTy);
            
            if(self->tileCollisionHandler(self, tempT, tempTx, tempTy, 4)){
                onGround = true;
            }
        }

        self->falling = !onGround;
    }    

    self->x = newX ? TO_SUBPIXEL_COORDS(newX) : self->x+self->xspeed;
    self->y = newY ? TO_SUBPIXEL_COORDS(newY) : self->y+self->yspeed;
}

bool mg_canWallJump(mgEntity_t* self){
    if(!self->falling) {
        return false;
    }

    uint16_t x = TO_PIXEL_COORDS(self->x);
    uint16_t y = TO_PIXEL_COORDS(self->y);
    mg_EntityTileCollider_t* tileCollider = self->tileCollider;

    int16_t offX, offY, tempX, tempY, tempTx, tempTy, tempT;

    if(self->xspeed > 0 && !self->spriteFlipHorizontal){
        const mg_EntityTileCollisionPointList_t* rightEdge = tileCollider->rightEdge;
        for(int i=0; i<rightEdge->size; i++){
            offX = rightEdge->collisionPoints[i].x;
            offY = rightEdge->collisionPoints[i].y;
            tempX = x + offX + 1;
            tempY = y + offY;

            tempTx = MG_TO_TILECOORDS(tempX);
            tempTy = MG_TO_TILECOORDS(tempY);

            //drawLine(tempX - self->tilemap->mapOffsetX, tempY  - self->tilemap->mapOffsetY, tempX + xspeed + (SIGNOF(xspeed) * MG_HALF_TILESIZE)  - self->tilemap->mapOffsetX, tempY + yspeed + (SIGNOF(yspeed) * MG_HALF_TILESIZE)  - self->tilemap->mapOffsetY, c500, 0);
            //drawRect((tempTx << MG_TILESIZE_IN_POWERS_OF_2) - self->tilemap->mapOffsetX, (tempTy << MG_TILESIZE_IN_POWERS_OF_2) - self->tilemap->mapOffsetY, (tempTx << MG_TILESIZE_IN_POWERS_OF_2) + MG_TILESIZE - self->tilemap->mapOffsetX, (tempTy << MG_TILESIZE_IN_POWERS_OF_2) + MG_TILESIZE - self->tilemap->mapOffsetY, c500);

            tempT = mg_getTile(self->tilemap, tempTx, tempTy);
            
            if(self->tileCollisionHandler(self, tempT, tempTx, tempTy, 1)){
                return true;
            }
        }
    } else if(self->xspeed < 0 && self->spriteFlipHorizontal){
        const mg_EntityTileCollisionPointList_t* leftEdge = tileCollider->leftEdge;
        for(int i=0; i<leftEdge->size; i++){
            offX = leftEdge->collisionPoints[i].x;
            offY = leftEdge->collisionPoints[i].y;
            tempX = x + offX - 1;
            tempY = y + offY;

            tempTx = MG_TO_TILECOORDS(tempX);
            tempTy = MG_TO_TILECOORDS(tempY);

            //drawLine(tempX - self->tilemap->mapOffsetX, tempY  - self->tilemap->mapOffsetY, tempX + xspeed + (SIGNOF(xspeed) * MG_HALF_TILESIZE)  - self->tilemap->mapOffsetX, tempY + yspeed + (SIGNOF(yspeed) * MG_HALF_TILESIZE)  - self->tilemap->mapOffsetY, c500, 0);
            //drawRect((tempTx << MG_TILESIZE_IN_POWERS_OF_2) - self->tilemap->mapOffsetX, (tempTy << MG_TILESIZE_IN_POWERS_OF_2) - self->tilemap->mapOffsetY, (tempTx << MG_TILESIZE_IN_POWERS_OF_2) + MG_TILESIZE - self->tilemap->mapOffsetX, (tempTy << MG_TILESIZE_IN_POWERS_OF_2) + MG_TILESIZE - self->tilemap->mapOffsetY, c500);

            tempT = mg_getTile(self->tilemap, tempTx, tempTy);
            
            if(self->tileCollisionHandler(self, tempT, tempTx, tempTy, 0)){
                return true;
            }
        }
    }
    
    return false;
}

void defaultFallOffTileHandler(mgEntity_t* self)
{
    self->falling = true;
}

void applyDamping(mgEntity_t* self)
{
    //if (!self->gravityEnabled)
    {
        if (self->xspeed > 0)
        {
            self->xspeed -= (self->falling) ? 2 : 4;

            if (self->xspeed < 0)
            {
                self->xspeed = 0;
            }
        }
        else if (self->xspeed < 0)
        {
            self->xspeed += (self->falling) ? 2 : 4;

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

void applyGravity(mgEntity_t* self)
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

void despawnWhenOffscreen(mgEntity_t* self)
{
    if (TO_PIXEL_COORDS(self->x) < (self->tilemap->mapOffsetX - DESPAWN_THRESHOLD)
        || TO_PIXEL_COORDS(self->x)
               > (self->tilemap->mapOffsetX + MG_TILEMAP_DISPLAY_WIDTH_PIXELS + DESPAWN_THRESHOLD))
    {
        mg_destroyEntity(self, true);
    }

    if (self->y > 63616)
    {
        return;
    }

    if (TO_PIXEL_COORDS(self->y) < (self->tilemap->mapOffsetY - (DESPAWN_THRESHOLD << 2))
        || TO_PIXEL_COORDS(self->y)
               > (self->tilemap->mapOffsetY + MG_TILEMAP_DISPLAY_HEIGHT_PIXELS + DESPAWN_THRESHOLD))
    {
        mg_destroyEntity(self, true);
    }
}

void mg_destroyEntity(mgEntity_t* self, bool respawn)
{
    /*if (respawn && !(self->homeTileX == 0 && self->homeTileY == 0))
    {
        self->tilemap->map[self->homeTileY * self->tilemap->mapWidth + self->homeTileX] = self->type + 128;
    }*/

    if (respawn && self->spawnData != NULL)
    {
        self->spawnData->spawnedEntity = NULL;
        self->spawnData->spawnable = true;
    }

    // self->entityManager->activeEntities--;
    self->active = false;
}

void animatePlayer(mgEntity_t* self)
{
    if (self->spriteIndex == MG_SP_PLAYER_WIN || self->spriteIndex == MG_SP_PLAYER_HURT)
    {
        // Win pose has been set; don't change it!
        return;
    }

    if (!self->gravityEnabled)
    {
        self->spriteIndex = MG_SP_PLAYER_CLIMB;
        if (self->yspeed < 0 && self->gameData->frameCount % 10 == 0)
        {
            self->spriteFlipHorizontal = !self->spriteFlipHorizontal;
        }
    }
    else if (self->falling)
    {
        if (self->yspeed < 0)
        {
            // Jumping
            if(self->jumpPower > 32) {
                self->spriteIndex = MG_SP_PLAYER_JUMP;
            } else {
                self->spriteIndex = MG_SP_PLAYER_JUMP1;
            }
        }
        else
        {
            // Falling
            if(self->yspeed != self->yMaxSpeed){
                self->spriteIndex = MG_SP_PLAYER_JUMP2;
            } else {
                self->spriteIndex = MG_SP_PLAYER_JUMP4;
            }
        }

        if (((self->gameData->btnState & PB_LEFT) && self->xspeed < 0)
            || ((self->gameData->btnState & PB_RIGHT) && self->xspeed > 0))
        {
            self->spriteFlipHorizontal = (self->xspeed > 0) ? 0 : 1;
        }
    }
    else if (self->xspeed != 0)
    {
        if (((self->gameData->btnState & PB_LEFT) && self->xspeed < 0)
            || ((self->gameData->btnState & PB_RIGHT) && self->xspeed > 0))
        {
            // Running
            self->spriteFlipHorizontal = (self->xspeed > 0) ? 0 : 1;

            if(self->spriteIndex < MG_SP_PLAYER_WALK1 || self->spriteIndex > MG_SP_PLAYER_WALK10)
            {
                self->spriteIndex = MG_SP_PLAYER_WALK1;
            }
            else if (self->gameData->frameCount % (5 /*- (abs(self->xspeed) >> 1)*/) == 0)
            {
                self->spriteIndex++;
                if(self->spriteIndex > MG_SP_PLAYER_WALK10)
                {
                    self->spriteIndex = MG_SP_PLAYER_WALK1;
                }
            }
        }
        else
        {
            self->spriteIndex = MG_SP_PLAYER_SLIDE;
        }
    }
    else
    {
        // Standing
        self->spriteIndex = MG_SP_PLAYER_IDLE;
    }
}

void mg_detectEntityCollisions(mgEntity_t* self)
{
    /*for (uint8_t i = 0; i < MAX_ENTITIES; i++)
    {
        mgEntity_t* checkEntity = &(self->entityManager->entities[i]);
        if (checkEntity->active && checkEntity != self)
        {
            uint32_t dist = abs(self->x - checkEntity->x) + abs(self->y - checkEntity->y);

            if (dist < 200)
            {
                self->collisionHandler(self, checkEntity);
            }
        }
    }*/

    mgSprite_t* selfSprite = &(self->tilemap->wsgManager->sprites[self->spriteIndex]);
    const box_t* selfSpriteBox = selfSprite->hitBox;

    box_t selfBox;
    selfBox.x0 = (self->x >> SUBPIXEL_RESOLUTION) - selfSprite->origin->x + selfSpriteBox->x0;
    selfBox.y0 = (self->y >> SUBPIXEL_RESOLUTION) - selfSprite->origin->y + selfSpriteBox->y0;
    selfBox.x1 = (self->x >> SUBPIXEL_RESOLUTION) - selfSprite->origin->x + selfSpriteBox->x1;
    selfBox.y1 = (self->y >> SUBPIXEL_RESOLUTION) - selfSprite->origin->y + selfSpriteBox->y1;

    mgEntity_t* checkEntity;
    mgSprite_t* checkEntitySprite;
    const box_t* checkEntitySpriteBox;
    box_t checkEntityBox;

    for (uint8_t i = 0; i < MAX_ENTITIES; i++)
    {
        checkEntity = &(self->entityManager->entities[i]);
        if (checkEntity->active && checkEntity != self)
        {
            checkEntitySprite    = &(self->tilemap->wsgManager->sprites[checkEntity->spriteIndex]);
            checkEntitySpriteBox = checkEntitySprite->hitBox;

            checkEntityBox.x0
                = TO_PIXEL_COORDS(checkEntity->x) - checkEntitySprite->origin->x + checkEntitySpriteBox->x0;
            checkEntityBox.y0
                = TO_PIXEL_COORDS(checkEntity->y) - checkEntitySprite->origin->y + checkEntitySpriteBox->y0;
            checkEntityBox.x1
                = TO_PIXEL_COORDS(checkEntity->x) - checkEntitySprite->origin->x + checkEntitySpriteBox->x1;
            checkEntityBox.y1
                = TO_PIXEL_COORDS(checkEntity->y) - checkEntitySprite->origin->y + checkEntitySpriteBox->y1;

            if (boxesCollide(selfBox, checkEntityBox, 0))
            {
                self->collisionHandler(self, checkEntity);
            }
        }
    }
}

void mg_playerCollisionHandler(mgEntity_t* self, mgEntity_t* other)
{
    switch (other->type)
    {
        case mgEntity_tEST:
        case ENTITY_DUST_BUNNY:
        case ENTITY_WASP:
        case ENTITY_BUSH_2:
        case ENTITY_BUSH_3:
        case ENTITY_DUST_BUNNY_2:
        case ENTITY_DUST_BUNNY_3:
        case ENTITY_WASP_2:
        case ENTITY_WASP_3:
        case ENTITY_CHARGIN_SCHMUCK:
        case ENTITY_BOUNCIN_SCHMUCK:
        case ENTITY_TURRET:
        {
            if(self->state == MG_PL_ST_MIC_DROP){
                break;
            }

            other->xspeed = -other->xspeed;

            /*if (self->y < other->y || self->yspeed > 0)
            {
                mg_scorePoints(self->gameData, other->scoreValue);

                killEnemy(other);
                soundPlaySfx(&(self->soundManager->sndSquish), BZR_LEFT);

                self->yspeed    = -180;
                self->jumpPower = 64 + ((abs(self->xspeed) + 16) >> 3);
                self->falling   = true;
            }
            else */if (self->invincibilityFrames <= 0)
            {
                self->hp -= 5;
                mg_updateLedsHpMeter(self->entityManager, self->gameData);
                self->gameData->comboTimer = 0;

                if (!self->gameData->debugMode && self->hp == 0)
                {
                    self->updateFunction        = &updateEntityDead;
                    self->type                  = ENTITY_DEAD;
                    self->xspeed                = 0;
                    self->yspeed                = -60;
                    self->spriteIndex           = MG_SP_PLAYER_HURT;
                    self->gameData->changeState = MG_ST_DEAD;
                    self->gravityEnabled        = true;
                    self->falling               = true;
                }
                else
                {
                    self->xspeed              = 0;
                    self->yspeed              = 0;
                    self->jumpPower           = 0;
                    self->invincibilityFrames = 120;
                    soundPlaySfx(&(self->soundManager->sndHurt), BZR_LEFT);
                }
            }

            break;
        }
        case ENTITY_WARP:
        {
            // Execute warp
            self->x = TO_SUBPIXEL_COORDS(self->tilemap->warps[other->jumpPower].x * MG_TILESIZE + MG_HALF_TILESIZE);
            self->y = TO_SUBPIXEL_COORDS(self->tilemap->warps[other->jumpPower].y * MG_TILESIZE + MG_HALF_TILESIZE);
            self->falling = true;
            mg_viewFollowEntity(self->tilemap, self->entityManager->playerEntity);

            mg_unlockScrolling(self->tilemap);
            mg_deactivateAllEntities(self->entityManager, true);
            self->tilemap->executeTileSpawnAll = true;
            soundPlaySfx(&(self->soundManager->sndWarp), BZR_LEFT);
            break;
        }
        case ENTITY_POWERUP:
        {
            self->hp++;
            if (self->hp > 3)
            {
                self->hp = 3;
            }
            mg_scorePoints(self->gameData, 1000);
            soundPlaySfx(&(self->soundManager->sndPowerUp), BZR_LEFT);
            mg_updateLedsHpMeter(self->entityManager, self->gameData);
            mg_destroyEntity(other, false);
            break;
        }
        case ENTITY_1UP:
        {
            self->gameData->lives++;
            mg_scorePoints(self->gameData, 0);
            soundPlaySfx(&(self->soundManager->snd1up), BZR_LEFT);
            mg_destroyEntity(other, false);
            break;
        }
        case ENTITY_CHECKPOINT:
        {
            if (!other->xDamping)
            {
                // Get tile above checkpoint
                uint8_t aboveTile
                    = self->tilemap->map[(other->homeTileY - 1) * self->tilemap->mapWidth + other->homeTileX];

                if (aboveTile >= MG_TILE_WARP_0 && aboveTile <= MG_TILE_WARP_F)
                {
                    self->gameData->checkpoint = aboveTile - MG_TILE_WARP_0;
                    other->xDamping            = 1;
                    soundPlaySfx(&(self->soundManager->sndCheckpoint), BZR_LEFT);
                }
            }
            break;
        }
        case ENTITY_WARP_ENTRANCE_WALL:
        case ENTITY_WARP_ENTRANCE_FLOOR:
        {   
            if(other->spawnData->linkedEntitySpawn != NULL){
                // Execute warp within the current map

                int32_t warpXoffset = other->x - self->x;
                int32_t warpYoffset = other->y - self->y;

                self->x = TO_SUBPIXEL_COORDS( (other->spawnData->linkedEntitySpawn->tx << SUBPIXEL_RESOLUTION) + other->spawnData->linkedEntitySpawn->xOffsetInPixels ) - warpXoffset;
                self->y = TO_SUBPIXEL_COORDS( (other->spawnData->linkedEntitySpawn->ty << SUBPIXEL_RESOLUTION) + other->spawnData->linkedEntitySpawn->yOffsetInPixels ) - warpYoffset;
                self->falling = true;
                mg_viewFollowEntity(self->tilemap, self->entityManager->playerEntity);

                mg_unlockScrolling(self->tilemap);
                mg_deactivateAllEntities(self->entityManager, true);
                self->tilemap->executeTileSpawnAll = true;
                soundPlaySfx(&(self->soundManager->sndWarp), BZR_LEFT);
            } else {
                // Execute warp to a different map
                uint8_t newLevelIndex = other->spawnData->special2;
                uint8_t modifiedPlayerSpawn_tx = other->spawnData->special3;
                uint8_t modifiedPlayerSpawn_ty = other->spawnData->special4;
                uint8_t modifiedPlayerSpawn_xOffset = other->spawnData->special5;
                uint8_t modifiedPlayerSpawn_yOffset = other->spawnData->special6;

                self->gameData->level = newLevelIndex;
                
                mg_deactivateAllEntities(self->entityManager, true);
                mg_loadMapFromFile(self->tilemap, leveldef[newLevelIndex].filename);
                
                if(self->tilemap->defaultPlayerSpawn != NULL){
                    self->tilemap->defaultPlayerSpawn->tx = modifiedPlayerSpawn_tx;
                    self->tilemap->defaultPlayerSpawn->ty = modifiedPlayerSpawn_ty;
                    self->tilemap->defaultPlayerSpawn->xOffsetInPixels = modifiedPlayerSpawn_xOffset;
                    self->tilemap->defaultPlayerSpawn->yOffsetInPixels = modifiedPlayerSpawn_yOffset;
                }
                
                self->x = TO_SUBPIXEL_COORDS( (modifiedPlayerSpawn_tx * 16) +  modifiedPlayerSpawn_xOffset);
                self->y = TO_SUBPIXEL_COORDS( (modifiedPlayerSpawn_ty * 16) +  modifiedPlayerSpawn_yOffset);

                self->tilemap->executeTileSpawnAll = true;
            }

            break;
        }
        case ENTITY_BOUNCE_PAD:
        {
            self->yspeed = (other->spriteFlipVertical) ? 112 : -112;
            self->falling = true;
            soundPlaySfx(&(self->soundManager->sndHit), BZR_LEFT);
            break;
        }
        case ENTITY_BOUNCE_PAD_DIAGONAL:
        {
            self->xspeed = (other->spriteFlipHorizontal) ? -112 : 112;
            self->yspeed = (other->spriteFlipVertical) ? 112 : -112;
            self->falling = true;
            soundPlaySfx(&(self->soundManager->sndHit), BZR_LEFT);
            break;
        }
        case ENTITY_WAVE_BALL:
        {
            if(other->linkedEntity == self)
            {
                break;
            }

            //TODO: This is a repeat of above code; move to its own function
            if (self->invincibilityFrames <= 0)
            {
                self->hp -= 5;
                mg_updateLedsHpMeter(self->entityManager, self->gameData);
                self->gameData->comboTimer = 0;

                if (!self->gameData->debugMode && self->hp == 0)
                {
                    self->updateFunction        = &updateEntityDead;
                    self->type                  = ENTITY_DEAD;
                    self->xspeed                = 0;
                    self->yspeed                = -60;
                    self->spriteIndex           = MG_SP_PLAYER_HURT;
                    self->gameData->changeState = MG_ST_DEAD;
                    self->gravityEnabled        = true;
                    self->falling               = true;
                }
                else
                {
                    self->xspeed              = 0;
                    self->yspeed              = 0;
                    self->jumpPower           = 0;
                    self->invincibilityFrames = 120;
                    soundPlaySfx(&(self->soundManager->sndHurt), BZR_LEFT);
                }
            }
        }
        default:
        {
            break;
        }
    }
}

void mg_enemyCollisionHandler(mgEntity_t* self, mgEntity_t* other)
{
    switch (other->type)
    {
        case mgEntity_tEST:
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
            if ((self->xspeed > 0 && self->x < other->x) || (self->xspeed < 0 && self->x > other->x))
            {
                self->xspeed               = -self->xspeed;
                self->spriteFlipHorizontal = -self->spriteFlipHorizontal;
            }
            break;
        case ENTITY_HIT_BLOCK:
            self->xspeed = other->xspeed * 2;
            self->yspeed = other->yspeed * 2;
            mg_scorePoints(self->gameData, self->scoreValue);
            soundPlaySfx(&(self->soundManager->sndSquish), BZR_LEFT);
            killEnemy(self);
            break;
        case ENTITY_WAVE_BALL:
            if(other->linkedEntity == self){
                break;
            }

            mg_destroyShot(other);
        
            if(self->invincibilityFrames){
                break;
            }
        
            if(self->hp <= 0) {
                self->xspeed = other->xspeed >> 1;
                self->yspeed = -abs(other->xspeed >> 1);
                mg_scorePoints(self->gameData, self->scoreValue);
                soundPlaySfx(&(self->soundManager->sndBreak), BZR_LEFT);
                killEnemy(self);
                
                break;
            }

            self->hp--;
            self->invincibilityFrames=4;

            break;
        case ENTITY_PLAYER: 
            if(other->state == MG_PL_ST_MIC_DROP){
                if(self->invincibilityFrames){
                    break;
                }

                self->hp -= 8;
                self->invincibilityFrames=4;
        
                if(self->hp <= 0) {
                    self->xspeed = other->xspeed >> 1;
                    self->yspeed = -abs(other->xspeed >> 1);
                    mg_scorePoints(self->gameData, self->scoreValue);
                    soundPlaySfx(&(self->soundManager->sndBreak), BZR_LEFT);
                    killEnemy(self);
                    
                    break;
                }

               
            }
            break;
        default:
        {
            break;
        }
    }
}

void mg_dummyCollisionHandler(mgEntity_t* self, mgEntity_t* other)
{
    return;
}

bool mg_playerTileCollisionHandler(mgEntity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction)
{
    switch (tileId)
    {
        case MG_TILE_CONTAINER_1:
        case MG_TILE_BRICK_BLOCK:
        case MG_TILE_INVISIBLE_CONTAINER:
        case MG_TILE_BOUNCE_BLOCK:
        {
            mgEntity_t* hitBlock
                = mg_createEntity(self->entityManager, ENTITY_HIT_BLOCK, (tx * MG_TILESIZE) + MG_HALF_TILESIZE,
                                  (ty * MG_TILESIZE) + MG_HALF_TILESIZE);

            if (hitBlock != NULL)
            {
                mg_setTile(self->tilemap, tx, ty, MG_TILE_INVISIBLE_BLOCK);
                hitBlock->homeTileX = tx;
                hitBlock->homeTileY = ty;
                hitBlock->jumpPower = tileId;
                if (tileId == MG_TILE_BRICK_BLOCK)
                {
                    hitBlock->spriteIndex = MG_SP_HITBLOCK_BRICKS;
                    if (abs(self->xspeed) > 51 && self->yspeed <= 0)
                    {
                        hitBlock->yDamping = 1;
                    }
                }

                if (tileId == MG_TILE_BOUNCE_BLOCK)
                {
                    hitBlock->spriteIndex = MG_SP_BOUNCE_BLOCK;
                }

                switch (direction)
                {
                    case 0:
                        hitBlock->xspeed = -24;
                        if (tileId == MG_TILE_BOUNCE_BLOCK)
                        {
                            self->xspeed = 48;
                        }
                        break;
                    case 1:
                        hitBlock->xspeed = 24;
                        if (tileId == MG_TILE_BOUNCE_BLOCK)
                        {
                            self->xspeed = -48;
                        }
                        break;
                    case 2:
                        hitBlock->yspeed = -48;
                        if (tileId == MG_TILE_BOUNCE_BLOCK)
                        {
                            self->yspeed = 48;
                        }
                        break;
                    case 4:
                        hitBlock->yspeed = (tileId == MG_TILE_BRICK_BLOCK) ? 16 : 24;
                        if (tileId == MG_TILE_BOUNCE_BLOCK)
                        {
                            self->yspeed = -64;
                            if (self->gameData->btnState & PB_A)
                            {
                                self->jumpPower = 80 + ((abs(self->xspeed) + 16) >> 3);
                            }
                        }
                        break;
                    default:
                        break;
                }

                soundPlaySfx(&(self->soundManager->sndHit), BZR_LEFT);
            }
            break;
        }
        case MG_TILE_GOAL_100PTS:
        {
            if (direction == 4)
            {
                mg_scorePoints(self->gameData, 100);
                soundStop(true);
                soundPlaySfx(&(self->soundManager->sndLevelClearD), BZR_LEFT);
                self->spriteIndex           = MG_SP_PLAYER_WIN;
                self->updateFunction        = &mg_updateDummy;
                self->gameData->changeState = MG_ST_LEVEL_CLEAR;
            }
            break;
        }
        case MG_TILE_GOAL_500PTS:
        {
            if (direction == 4)
            {
                mg_scorePoints(self->gameData, 500);
                soundStop(true);
                soundPlaySfx(&(self->soundManager->sndLevelClearC), BZR_LEFT);
                self->spriteIndex           = MG_SP_PLAYER_WIN;
                self->updateFunction        = &mg_updateDummy;
                self->gameData->changeState = MG_ST_LEVEL_CLEAR;
            }
            break;
        }
        case MG_TILE_GOAL_1000PTS:
        {
            if (direction == 4)
            {
                mg_scorePoints(self->gameData, 1000);
                soundStop(true);
                soundPlaySfx(&(self->soundManager->sndLevelClearB), BZR_LEFT);
                self->spriteIndex           = MG_SP_PLAYER_WIN;
                self->updateFunction        = &mg_updateDummy;
                self->gameData->changeState = MG_ST_LEVEL_CLEAR;
            }
            break;
        }
        case MG_TILE_GOAL_2000PTS:
        {
            if (direction == 4)
            {
                mg_scorePoints(self->gameData, 2000);
                soundStop(true);
                soundPlaySfx(&(self->soundManager->sndLevelClearA), BZR_LEFT);
                self->spriteIndex           = MG_SP_PLAYER_WIN;
                self->updateFunction        = &mg_updateDummy;
                self->gameData->changeState = MG_ST_LEVEL_CLEAR;
            }
            break;
        }
        case MG_TILE_GOAL_5000PTS:
        {
            if (direction == 4)
            {
                mg_scorePoints(self->gameData, 5000);
                soundStop(true);
                soundPlaySfx(&(self->soundManager->sndLevelClearS), BZR_LEFT);
                self->spriteIndex           = MG_SP_PLAYER_WIN;
                self->updateFunction        = &mg_updateDummy;
                self->gameData->changeState = MG_ST_LEVEL_CLEAR;
            }
            break;
        }
        case MG_TILE_COIN_1 ... MG_TILE_COIN_3:
        {
            mg_setTile(self->tilemap, tx, ty, MG_TILE_EMPTY);
            addCoins(self->gameData, 1);
            mg_scorePoints(self->gameData, 50);
            break;
        }
        case MG_TILE_LADDER:
        {
            if (self->gravityEnabled)
            {
                self->gravityEnabled = false;
                self->xspeed         = 0;
            }
            break;
        }
        default:
        {
            break;
        }
    }

    if (mg_isSolid(tileId))
    {
        switch (direction)
        {
            case 0: // LEFT
                self->xspeed = 0;
                break;
            case 1: // RIGHT
                self->xspeed = 0;
                break;
            case 2: // UP
                self->yspeed = 0;
                break;
            case 4: // DOWN
                // Landed on platform
                self->falling = false;
                self->canDash = true;
                self->yspeed  = 0;

                if(self->state == MG_PL_ST_MIC_DROP){
                    self->state = MG_PL_ST_NORMAL;
                    self->yMaxSpeed = 72;
                    self->spriteFlipVertical = false;
                }
                break;
            default: // Should never hit
                return false;
        }
        // trigger tile collision resolution
        return true;
    }

    return false;
}

bool mg_enemyTileCollisionHandler(mgEntity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction)
{
    switch (tileId)
    {
        case MG_TILE_BOUNCE_BLOCK:
        {
            switch (direction)
            {
                case 0:
                    // hitBlock->xspeed = -64;
                    if (tileId == MG_TILE_BOUNCE_BLOCK)
                    {
                        self->xspeed = 48;
                    }
                    break;
                case 1:
                    // hitBlock->xspeed = 64;
                    if (tileId == MG_TILE_BOUNCE_BLOCK)
                    {
                        self->xspeed = -48;
                    }
                    break;
                case 2:
                    // hitBlock->yspeed = -128;
                    if (tileId == MG_TILE_BOUNCE_BLOCK)
                    {
                        self->yspeed = 48;
                    }
                    break;
                case 4:
                    // hitBlock->yspeed = (tileId == MG_TILEBRICK_BLOCK) ? 32 : 64;
                    if (tileId == MG_TILE_BOUNCE_BLOCK)
                    {
                        self->yspeed = -48;
                    }
                    break;
                default:
                    break;
            }
            break;
        }
        default:
        {
            break;
        }
    }

    if (mg_isSolid(tileId))
    {
        switch (direction)
        {
            case 0: // LEFT
                self->xspeed = -self->xspeed;
                break;
            case 1: // RIGHT
                self->xspeed = -self->xspeed;
                break;
            case 2: // UP
                self->yspeed = 0;
                break;
            case 4: // DOWN
                // Landed on platform
                self->falling = false;
                self->yspeed  = 0;
                break;
            default: // Should never hit
                return false;
        }
        // trigger tile collision resolution
        return true;
    }

    return false;
}

bool mg_dummyTileCollisionHandler(mgEntity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction)
{
    return false;
}

void dieWhenFallingOffScreen(mgEntity_t* self)
{
    uint16_t deathBoundary = (self->tilemap->mapOffsetY + MG_TILEMAP_DISPLAY_HEIGHT_PIXELS + DESPAWN_THRESHOLD);
    if ((TO_PIXEL_COORDS(self->y) > deathBoundary)
        && (TO_PIXEL_COORDS(self->y) < deathBoundary + DESPAWN_THRESHOLD))
    {
        self->hp = 0;
        mg_updateLedsHpMeter(self->entityManager, self->gameData);
        self->gameData->changeState = MG_ST_DEAD;
        mg_destroyEntity(self, true);
    }
}

void mg_updateDummy(mgEntity_t* self)
{
    // Do nothing, because that's what dummies do!
}

void updateScrollLockLeft(mgEntity_t* self)
{
    self->tilemap->minMapOffsetX = TO_PIXEL_COORDS(self->x) - 8;
    mg_viewFollowEntity(self->entityManager->tilemap, self->entityManager->viewEntity);
    mg_destroyEntity(self, true);
}

void updateScrollLockRight(mgEntity_t* self)
{
    self->tilemap->maxMapOffsetX = TO_PIXEL_COORDS(self->x) + 8 - MG_TILEMAP_DISPLAY_WIDTH_PIXELS;
    mg_viewFollowEntity(self->entityManager->tilemap, self->entityManager->viewEntity);
    mg_destroyEntity(self, true);
}

void updateScrollLockUp(mgEntity_t* self)
{
    self->tilemap->minMapOffsetY = TO_PIXEL_COORDS(self->y) - 8;
    mg_viewFollowEntity(self->entityManager->tilemap, self->entityManager->viewEntity);
    mg_destroyEntity(self, true);
}

void updateScrollLockDown(mgEntity_t* self)
{
    self->tilemap->maxMapOffsetY = TO_PIXEL_COORDS(self->y) + 8 - MG_TILEMAP_DISPLAY_HEIGHT_PIXELS;
    mg_viewFollowEntity(self->entityManager->tilemap, self->entityManager->viewEntity);
    mg_destroyEntity(self, true);
}

void updateScrollUnlock(mgEntity_t* self)
{
    mg_unlockScrolling(self->tilemap);
    mg_viewFollowEntity(self->entityManager->tilemap, self->entityManager->viewEntity);
    mg_destroyEntity(self, true);
}

void updateEntityDead(mgEntity_t* self)
{
    applyGravity(self);
    self->x += self->xspeed;
    self->y += self->yspeed;

    despawnWhenOffscreen(self);
}

void updatePowerUp(mgEntity_t* self)
{
    if (self->gameData->frameCount % 10 == 0)
    {
        self->spriteIndex
            = ((self->entityManager->playerEntity->hp < 2) ? MG_SP_GAMING_1 : MG_SP_MUSIC_1) + ((self->spriteIndex + 1) % 3);
    }

    mg_moveEntityWithTileCollisions(self);
    applyGravity(self);
    despawnWhenOffscreen(self);
}

void update1up(mgEntity_t* self)
{
    if (self->gameData->frameCount % 10 == 0)
    {
        self->spriteIndex = MG_SP_1UP_1 + ((self->spriteIndex + 1) % 3);
    }

    mg_moveEntityWithTileCollisions(self);
    applyGravity(self);
    despawnWhenOffscreen(self);
}

void updateWarp(mgEntity_t* self)
{
    if (self->gameData->frameCount % 10 == 0)
    {
        self->spriteIndex = MG_SP_WARP_1 + ((self->spriteIndex + 1) % 3);
    }

    // Destroy self and respawn warp container block when offscreen
    if (TO_PIXEL_COORDS(self->x) < (self->tilemap->mapOffsetX - DESPAWN_THRESHOLD)
        || TO_PIXEL_COORDS(self->x)
               > (self->tilemap->mapOffsetX + MG_TILEMAP_DISPLAY_WIDTH_PIXELS + DESPAWN_THRESHOLD)
        || TO_PIXEL_COORDS(self->y) < (self->tilemap->mapOffsetY - DESPAWN_THRESHOLD)
        || TO_PIXEL_COORDS(self->y)
               > (self->tilemap->mapOffsetY + MG_TILEMAP_DISPLAY_HEIGHT_PIXELS + DESPAWN_THRESHOLD))
    {
        // In mg_destroyEntity, this will overflow to the correct value.
        self->type = 128 + MG_TILE_CONTAINER_1;

        mg_destroyEntity(self, true);
    }
}

void updateDustBunny(mgEntity_t* self)
{
    if (!self->falling)
    {
        self->yDamping--;
        if (self->yDamping <= 0)
        {
            bool directionToPlayer = (self->entityManager->playerEntity->x < self->x);

            switch (self->xDamping)
            {
                case 0:
                {
                    self->yspeed               = (int32_t)(2 + esp_random() % 3) * -24;
                    self->falling              = true;
                    self->xDamping             = 1;
                    self->yDamping             = (1 + esp_random() % 3) * 9;
                    self->spriteIndex          = MG_SP_DUSTBUNNY_JUMP;
                    self->spriteFlipHorizontal = directionToPlayer;
                    break;
                }
                case 1:
                {
                    self->xDamping             = 0;
                    self->yDamping             = 30;
                    self->spriteIndex          = MG_SP_DUSTBUNNY_CHARGE;
                    self->spriteFlipHorizontal = directionToPlayer;
                    break;
                }
                default:
                    self->xDamping = 0;
                    break;
            }
        }
    }

    mg_updateInvincibilityFrames(self);
    despawnWhenOffscreen(self);
    mg_moveEntityWithTileCollisions(self);
    applyGravity(self);
    mg_detectEntityCollisions(self);
}

void updateDustBunnyL2(mgEntity_t* self)
{
    if (!self->falling)
    {
        self->yDamping--;
        if (self->yDamping <= 0)
        {
            switch (self->xDamping)
            {
                case 0:
                {
                    self->xspeed      = (1 + esp_random() % 4) * 6 * ((self->spriteFlipHorizontal) ? -1 : 1);
                    self->yspeed      = (int32_t)(1 + esp_random() % 4) * -24;
                    self->xDamping    = 1;
                    self->yDamping    = (esp_random() % 3) * 6;
                    self->spriteIndex = MG_SP_DUSTBUNNY_L2_JUMP;
                    break;
                }
                case 1:
                {
                    self->xDamping    = 0;
                    self->yDamping    = 15;
                    self->spriteIndex = MG_SP_DUSTBUNNY_L2_CHARGE;
                    break;
                }
                default:
                    self->xDamping = 0;
                    break;
            }
        }
    }

    mg_updateInvincibilityFrames(self);
    despawnWhenOffscreen(self);
    mg_moveEntityWithTileCollisions(self);
    applyGravity(self);
    mg_detectEntityCollisions(self);
}

void updateDustBunnyL3(mgEntity_t* self)
{
    if (!self->falling)
    {
        self->yDamping--;
        if (self->yDamping <= 0)
        {
            bool directionToPlayer = (self->entityManager->playerEntity->x < self->x);

            switch (self->xDamping)
            {
                case 0:
                {
                    self->xspeed               = (int32_t)(1 + esp_random() % 4) * 6 * ((directionToPlayer) ? -1 : 1);
                    self->yspeed               = (int32_t)(1 + esp_random() % 4) * -24;
                    self->xDamping             = 1;
                    self->yDamping             = (esp_random() % 3) * 30;
                    self->spriteIndex          = MG_SP_DUSTBUNNY_L3_JUMP;
                    self->spriteFlipHorizontal = directionToPlayer;
                    break;
                }
                case 1:
                {
                    self->xDamping             = 0;
                    self->yDamping             = 30;
                    self->spriteIndex          = MG_SP_DUSTBUNNY_L3_CHARGE;
                    self->spriteFlipHorizontal = directionToPlayer;
                    break;
                }
                default:
                    self->xDamping = 0;
                    break;
            }
        }
    }

    mg_updateInvincibilityFrames(self);
    despawnWhenOffscreen(self);
    mg_moveEntityWithTileCollisions(self);
    applyGravity(self);
    mg_detectEntityCollisions(self);
}

bool dustBunnyTileCollisionHandler(mgEntity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction)
{
    switch (tileId)
    {
        case MG_TILE_BOUNCE_BLOCK:
        {
            switch (direction)
            {
                case 0:
                    // hitBlock->xspeed = -64;
                    if (tileId == MG_TILE_BOUNCE_BLOCK)
                    {
                        self->xspeed = 48;
                    }
                    break;
                case 1:
                    // hitBlock->xspeed = 64;
                    if (tileId == MG_TILE_BOUNCE_BLOCK)
                    {
                        self->xspeed = -48;
                    }
                    break;
                case 2:
                    // hitBlock->yspeed = -128;
                    if (tileId == MG_TILE_BOUNCE_BLOCK)
                    {
                        self->yspeed = 48;
                    }
                    break;
                case 4:
                    // hitBlock->yspeed = (tileId == MG_TILEBRICK_BLOCK) ? 32 : 64;
                    if (tileId == MG_TILE_BOUNCE_BLOCK)
                    {
                        self->xDamping = 0;
                        self->xspeed   = 0;
                        self->yspeed   = 0;
                        self->falling  = false;
                        self->yDamping = -1;
                    }
                    break;
                default:
                    break;
            }
            break;
        }
        default:
        {
            break;
        }
    }

    if (mg_isSolid(tileId))
    {
        switch (direction)
        {
            case 0: // LEFT
                self->xspeed = -self->xspeed;
                break;
            case 1: // RIGHT
                self->xspeed = -self->xspeed;
                break;
            case 2: // UP
                self->yspeed = 0;
                break;
            case 4: // DOWN
                // Landed on platform
                self->falling     = false;
                self->yspeed      = 0;
                self->xspeed      = 0;
                self->spriteIndex = MG_SP_DUSTBUNNY_IDLE;
                break;
            default: // Should never hit
                return false;
        }
        // trigger tile collision resolution
        return true;
    }

    return false;
}

bool dustBunnyL2TileCollisionHandler(mgEntity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction)
{
    switch (tileId)
    {
        case MG_TILE_BOUNCE_BLOCK:
        {
            switch (direction)
            {
                case 0:
                    // hitBlock->xspeed = -64;
                    if (tileId == MG_TILE_BOUNCE_BLOCK)
                    {
                        self->xspeed = 48;
                    }
                    break;
                case 1:
                    // hitBlock->xspeed = 64;
                    if (tileId == MG_TILE_BOUNCE_BLOCK)
                    {
                        self->xspeed = -48;
                    }
                    break;
                case 2:
                    // hitBlock->yspeed = -128;
                    if (tileId == MG_TILE_BOUNCE_BLOCK)
                    {
                        self->yspeed = 48;
                    }
                    break;
                case 4:
                    // hitBlock->yspeed = (tileId == MG_TILEBRICK_BLOCK) ? 32 : 64;
                    if (tileId == MG_TILE_BOUNCE_BLOCK)
                    {
                        self->xDamping = 0;
                        self->xspeed   = 0;
                        self->yspeed   = 0;
                        self->falling  = false;
                        self->yDamping = -1;
                    }
                    break;
                default:
                    break;
            }
            break;
        }
        default:
        {
            break;
        }
    }

    if (mg_isSolid(tileId))
    {
        switch (direction)
        {
            case 0: // LEFT
                self->xspeed               = -self->xspeed;
                self->spriteFlipHorizontal = false;
                break;
            case 1: // RIGHT
                self->xspeed               = -self->xspeed;
                self->spriteFlipHorizontal = true;
                break;
            case 2: // UP
                self->yspeed = 0;
                break;
            case 4: // DOWN
                // Landed on platform
                self->falling     = false;
                self->yspeed      = 0;
                self->xspeed      = 0;
                self->spriteIndex = MG_SP_DUSTBUNNY_L2_IDLE;
                break;
            default: // Should never hit
                return false;
        }
        // trigger tile collision resolution
        return true;
    }

    return false;
}

bool dustBunnyL3TileCollisionHandler(mgEntity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction)
{
    switch (tileId)
    {
        case MG_TILE_BOUNCE_BLOCK:
        {
            switch (direction)
            {
                case 0:
                    // hitBlock->xspeed = -64;
                    if (tileId == MG_TILE_BOUNCE_BLOCK)
                    {
                        self->xspeed = 48;
                    }
                    break;
                case 1:
                    // hitBlock->xspeed = 64;
                    if (tileId == MG_TILE_BOUNCE_BLOCK)
                    {
                        self->xspeed = -48;
                    }
                    break;
                case 2:
                    // hitBlock->yspeed = -128;
                    if (tileId == MG_TILE_BOUNCE_BLOCK)
                    {
                        self->yspeed = 48;
                    }
                    break;
                case 4:
                    // hitBlock->yspeed = (tileId == MG_TILEBRICK_BLOCK) ? 32 : 64;
                    if (tileId == MG_TILE_BOUNCE_BLOCK)
                    {
                        self->xDamping = 0;
                        self->xspeed   = 0;
                        self->yspeed   = 0;
                        self->falling  = false;
                        self->yDamping = -1;
                    }
                    break;
                default:
                    break;
            }
            break;
        }
        default:
        {
            break;
        }
    }

    if (mg_isSolid(tileId))
    {
        switch (direction)
        {
            case 0: // LEFT
                self->xspeed = -self->xspeed;
                break;
            case 1: // RIGHT
                self->xspeed = -self->xspeed;
                break;
            case 2: // UP
                self->yspeed = 0;
                break;
            case 4: // DOWN
                // Landed on platform
                self->falling     = false;
                self->yspeed      = 0;
                self->xspeed      = 0;
                self->spriteIndex = MG_SP_DUSTBUNNY_L3_IDLE;
                break;
            default: // Should never hit
                return false;
        }
        // trigger tile collision resolution
        return true;
    }

    return false;
}

void updateWasp(mgEntity_t* self)
{
    switch (self->xDamping)
    {
        case 0:
            if (self->gameData->frameCount % 5 == 0)
            {
                self->spriteIndex = MG_SP_WASP_1 + ((self->spriteIndex + 1) % 2);
            }
            self->yDamping--;

            if (self->entityManager->playerEntity->y > self->y && self->yDamping < 0
                && abs(self->x - self->entityManager->playerEntity->x) < 512)
            {
                self->xDamping       = 1;
                self->gravityEnabled = true;
                self->falling        = true;
                self->spriteIndex    = MG_SP_WASP_DIVE;
                self->xspeed         = 0;
                self->yspeed         = 64;
            }
            break;
        case 1:
            if (!self->falling)
            {
                self->yDamping--;
                if (self->yDamping < 0)
                {
                    self->xDamping       = 2;
                    self->gravityEnabled = false;
                    self->falling        = false;
                    self->yspeed         = -24;
                    self->yDamping       = 120;
                }
            }
            break;
        case 2:
            if (self->gameData->frameCount % 2 == 0)
            {
                self->spriteIndex = MG_SP_WASP_1 + ((self->spriteIndex + 1) % 2);
            }

            self->yDamping--;
            if (self->yDamping < 0 || self->y <= (TO_SUBPIXEL_COORDS(self->homeTileY * MG_TILESIZE + 8)))
            {
                self->xDamping = 0;
                self->xspeed   = (self->spriteFlipHorizontal) ? -16 : 16;
                self->yspeed   = 0;
                self->yDamping = (1 + esp_random() % 2) * 20;
            }
        default:
            break;
    }

    mg_updateInvincibilityFrames(self);
    despawnWhenOffscreen(self);
    mg_moveEntityWithTileCollisions(self);
    applyGravity(self);
    mg_detectEntityCollisions(self);
}

void updateWaspL2(mgEntity_t* self)
{
    switch (self->xDamping)
    {
        case 0:
            if (self->gameData->frameCount % 5 == 0)
            {
                self->spriteIndex = MG_SP_WASP_L2_1 + ((self->spriteIndex) % 2);
            }

            self->yDamping--;
            if (esp_random() % 256 > 240)
            {
                bool directionToPlayer     = self->entityManager->playerEntity->x < self->x;
                self->xspeed               = directionToPlayer ? -24 : 24;
                self->spriteFlipHorizontal = directionToPlayer;
            }

            if (self->entityManager->playerEntity->y > self->y && self->yDamping < 0
                && abs(self->x - self->entityManager->playerEntity->x) < self->jumpPower)
            {
                self->xDamping       = 1;
                self->gravityEnabled = true;
                self->falling        = true;
                self->spriteIndex    = MG_SP_WASP_L2_DIVE;
                self->xspeed         = 0;
                self->yspeed         = 96;
            }
            break;
        case 1:
            if (!self->falling)
            {
                self->yDamping -= 2;
                if (self->yDamping < 0)
                {
                    self->xDamping       = 2;
                    self->gravityEnabled = false;
                    self->falling        = false;
                    self->yspeed         = -48;
                    self->jumpPower      = (1 + esp_random() % 3) * 256;
                    self->yDamping       = 80;
                }
            }
            break;
        case 2:
            if (self->gameData->frameCount % 2 == 0)
            {
                self->spriteIndex = MG_SP_WASP_L2_1 + ((self->spriteIndex) % 2);
            }

            self->yDamping--;
            if (self->yDamping < 0 || self->y <= (TO_SUBPIXEL_COORDS(self->homeTileY * MG_TILESIZE + 8)))
            {
                self->xDamping = 0;
                self->xspeed   = (self->spriteFlipHorizontal) ? -24 : 24;
                self->yspeed   = 0;
                self->yDamping = (1 + esp_random() % 2) * 20;
            }
            break;
        default:
            break;
    }

    despawnWhenOffscreen(self);
    mg_moveEntityWithTileCollisions(self);
    applyGravity(self);
    mg_detectEntityCollisions(self);
}

void updateWaspL3(mgEntity_t* self)
{
    switch (self->xDamping)
    {
        case 0:
            if (self->gameData->frameCount % 5 == 0)
            {
                self->spriteIndex = MG_SP_WASP_L3_1 + ((self->spriteIndex + 1) % 2);
            }

            self->yDamping--;
            if (esp_random() % 256 > 192)
            {
                bool directionToPlayer     = self->entityManager->playerEntity->x < self->x;
                self->xspeed               = directionToPlayer ? -32 : 32;
                self->spriteFlipHorizontal = directionToPlayer;
            }

            if (self->entityManager->playerEntity->y > self->y && self->yDamping < 0
                && abs(self->x - self->entityManager->playerEntity->x) < self->jumpPower)
            {
                self->xDamping       = 1;
                self->gravityEnabled = true;
                self->falling        = true;
                self->spriteIndex    = MG_SP_WASP_L3_DIVE;
                self->xspeed         = 0;
                self->yspeed         = 128;
            }
            break;
        case 1:
            if (!self->falling)
            {
                self->yDamping -= 4;
                if (self->yDamping < 0)
                {
                    self->xDamping       = 2;
                    self->gravityEnabled = false;
                    self->falling        = false;
                    self->yspeed         = -64;
                    self->jumpPower      = (1 + esp_random() % 3) * 256;
                    self->yDamping       = (2 + esp_random() % 6) * 8;
                }
            }
            break;
        case 2:
            if (self->gameData->frameCount % 2 == 0)
            {
                self->spriteIndex = MG_SP_WASP_L3_1 + ((self->spriteIndex + 1) % 2);
            }

            self->yDamping--;
            if (self->yDamping < 0 || self->y <= (TO_SUBPIXEL_COORDS(self->homeTileY * MG_TILESIZE + 8)))
            {
                self->xDamping = 0;
                self->xspeed   = (self->spriteFlipHorizontal) ? -32 : 32;
                self->yspeed   = 0;
                self->yDamping = (1 + esp_random() % 2) * 20;
            }
            break;
        default:
            break;
    }

    mg_updateInvincibilityFrames(self);
    despawnWhenOffscreen(self);
    mg_moveEntityWithTileCollisions(self);
    applyGravity(self);
    mg_detectEntityCollisions(self);
}

bool waspTileCollisionHandler(mgEntity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction)
{
    switch (tileId)
    {
        case MG_TILE_BOUNCE_BLOCK:
        {
            self->xDamping = 1;
            self->falling  = false;
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
        default:
        {
            break;
        }
    }

    if (mg_isSolid(tileId))
    {
        switch (direction)
        {
            case 0: // LEFT
            case 1: // RIGHT
                self->spriteFlipHorizontal = !self->spriteFlipHorizontal;
                self->xspeed               = -self->xspeed;
                break;
            case 2: // UP
                self->yspeed = 0;
                break;
            case 4: // DOWN
                // Landed on platform
                self->falling  = false;
                self->yspeed   = 0;
                self->xspeed   = 0;
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
}

void killEnemy(mgEntity_t* target)
{
    target->homeTileX          = 0;
    target->homeTileY          = 0;
    target->gravityEnabled     = true;
    target->falling            = true;
    target->type               = ENTITY_DEAD;
    target->spriteFlipVertical = true;
    if(target->spawnData != NULL){
        target->spawnData->spawnedEntity = NULL;
        target->spawnData = NULL;
    }
    target->updateFunction     = &updateEntityDead;
}

void updateBgCol(mgEntity_t* self)
{
    self->gameData->bgColor = self->xDamping;
    mg_destroyEntity(self, true);
}

void turnAroundAtEdgeOfTileHandler(mgEntity_t* self)
{
    self->falling = true;
    self->xspeed  = -self->xspeed;
    self->yspeed  = -self->gravity * 4;
}

void updateEnemyBushL3(mgEntity_t* self)
{
    if (self->gameData->frameCount % 10 == 0)
    {
        self->spriteFlipHorizontal = !self->spriteFlipHorizontal;
    }

    self->yDamping--;
    if (self->yDamping < 0)
    {
        bool directionToPlayer = (self->entityManager->playerEntity->x < self->x);

        if ((self->xspeed < 0 && directionToPlayer) || (self->xspeed > 0 && !directionToPlayer))
        {
            self->xspeed = -self->xspeed;
        }
        else
        {
            self->xspeed  = (directionToPlayer) ? -16 : 16;
            self->yspeed  = -24;
            self->falling = true;
        }

        self->yDamping = (1 + esp_random() % 7) * 30;
    }

    mg_updateInvincibilityFrames(self);
    despawnWhenOffscreen(self);
    mg_moveEntityWithTileCollisions(self);
    applyGravity(self);
    mg_detectEntityCollisions(self);
}

void updateCheckpoint(mgEntity_t* self)
{
    if (self->xDamping)
    {
        if (self->gameData->frameCount % 15 == 0)
        {
            self->spriteIndex = MG_SP_CHECKPOINT_ACTIVE_1 + ((self->spriteIndex + 1) % 2);
        }
    }

    despawnWhenOffscreen(self);
}

void mg_playerOverlapTileHandler(mgEntity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty)
{
    switch (tileId)
    {
        case MG_TILE_COIN_1 ... MG_TILE_COIN_3:
        {
            mg_setTile(self->tilemap, tx, ty, MG_TILE_EMPTY);
            addCoins(self->gameData, 1);
            mg_scorePoints(self->gameData, 50);
            break;
        }
        case MG_TILE_LADDER:
        {
            if (self->gravityEnabled)
            {
                self->gravityEnabled = false;
                self->xspeed         = 0;
            }
            break;
        }
        default:
        {
            break;
        }
    }

    if (!self->gravityEnabled && tileId != MG_TILE_LADDER)
    {
        self->gravityEnabled = true;
        self->falling        = true;
        if (self->yspeed < 0)
        {
            self->yspeed = -32;
        }
    }
}

void mg_defaultOverlapTileHandler(mgEntity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty)
{
    // Nothing to do.
}

void updateBgmChange(mgEntity_t* self)
{
    self->gameData->changeBgm = self->xDamping;
    mg_destroyEntity(self, true);
}

void updateWaveBall(mgEntity_t* self)
{
    if (self->gameData->frameCount % 2 == 0)
    {
        self->spriteIndex = (self->spriteIndex >= MG_SP_WAVEBALL_3) ? MG_SP_WAVEBALL_1 : self->spriteIndex + 1;
        //(MG_SP_WAVEBALL_1 + ((self->spriteIndex + 1) % 3));
    }

    /*if (self->gameData->frameCount % 4 == 0)
    {
        self->xDamping++;

        switch (self->xDamping)
        {
            case 0:
                break;
            case 1:
                self->yDamping = self->xspeed + 2; //((esp_random() % 2)?-16:16);
                self->yspeed   = -abs(self->yDamping);
                self->xspeed   = 0;
                break;
            case 2:
                self->yspeed = 0;
                self->xspeed = self->yDamping;
                break;
            case 3:
                self->yDamping = self->xspeed + 2; //((esp_random() % 2)?-16:16);
                self->yspeed   = abs(self->yDamping);
                self->xspeed   = 0;
                break;
            case 4:
                self->yspeed   = 0;
                self->xspeed   = self->yDamping;
                self->xDamping = 0;
                break;
            default:
                break;
        }
    }*/

    // self->yDamping++;

    //mg_moveEntityWithTileCollisions(self);
    //despawnWhenOffscreen(self);

    self->x += self->xspeed;
    self->y += self->yspeed;

    if (TO_PIXEL_COORDS(self->x) < (self->tilemap->mapOffsetX - DESPAWN_THRESHOLD)
        || TO_PIXEL_COORDS(self->x)
               > (self->tilemap->mapOffsetX + MG_TILEMAP_DISPLAY_WIDTH_PIXELS + DESPAWN_THRESHOLD))
    {
        mg_destroyShot(self);
    }

    if (self->y > 63616)
    {
        return;
    }

    if (TO_PIXEL_COORDS(self->y) < (self->tilemap->mapOffsetY - (DESPAWN_THRESHOLD << 2))
        || TO_PIXEL_COORDS(self->y)
               > (self->tilemap->mapOffsetY + MG_TILEMAP_DISPLAY_HEIGHT_PIXELS + DESPAWN_THRESHOLD))
    {
        mg_destroyShot(self);
    }
}

// bool waveBallTileCollisionHandler(mgEntity_t *self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction){
//     if(self->yspeed == 0){
//         mg_destroyEntity(self, false);
//     }
//     return false;
// }

void waveBallOverlapTileHandler(mgEntity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty)
{
    if (mg_isSolid(tileId) || tileId == MG_TILE_BOUNCE_BLOCK)
    {
        mg_destroyShot(self);
        soundPlaySfx(&(self->soundManager->sndHit), BZR_LEFT);
    }
}

void powerUpCollisionHandler(mgEntity_t* self, mgEntity_t* other)
{
    switch (other->type)
    {
        case mgEntity_tEST:
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
            if ((self->xspeed > 0 && self->x < other->x) || (self->xspeed < 0 && self->x > other->x))
            {
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

void killPlayer(mgEntity_t* self)
{
    self->hp = 0;
    mg_updateLedsHpMeter(self->entityManager, self->gameData);

    self->updateFunction        = &updateEntityDead;
    self->type                  = ENTITY_DEAD;
    self->xspeed                = 0;
    self->yspeed                = -60;
    self->spriteIndex           = MG_SP_PLAYER_HURT;
    self->gameData->changeState = MG_ST_DEAD;
    self->falling               = true;
}

void mg_defaultEntityDrawHandler(mgEntity_t* self)
{
    drawWsg(self->entityManager->wsgManager->sprites[self->spriteIndex].wsg,
            (self->x >> SUBPIXEL_RESOLUTION) - self->entityManager->wsgManager->sprites[self->spriteIndex].origin->x
                - self->entityManager->tilemap->mapOffsetX,
            (self->y >> SUBPIXEL_RESOLUTION) - self->entityManager->tilemap->mapOffsetY
                - self->entityManager->wsgManager->sprites[self->spriteIndex].origin->y,
            self->spriteFlipHorizontal, self->spriteFlipVertical, 0);
}

void mg_destroyShot(mgEntity_t* self){
    if(self->linkedEntity != NULL && self->linkedEntity->active){
        self->linkedEntity->shotsFired--;
    }

    mg_destroyEntity(self, false);
}

void mg_updateTurret(mgEntity_t* self)
{
    switch(self->state){
        case 0:
        default:
            self->stateTimer++;

            if(self->stateTimer > 60){
                self->shotsFired = 0;
                self->stateTimer = 0;

                if(self->entityManager->playerEntity != NULL)
                {
                    self->jumpPower = getAtan2(self->entityManager->playerEntity->y - self->y, self->entityManager->playerEntity->x - self->x);  
                }     

                self->jumpPower = clampAngleTo8way(self->jumpPower);

                self->state = 1;
            }
            break;
        case 1 ... 3:
            self->stateTimer++;

            if(self->stateTimer > 5)
            {
                mgEntity_t* createdEntity = mg_createEntity(self->entityManager, ENTITY_WAVE_BALL,
                                                    TO_PIXEL_COORDS(self->x), TO_PIXEL_COORDS(self->y));
                if (createdEntity != NULL)
                {                                                
                    int16_t sin = getSin1024(self->jumpPower);
                    int16_t cos = getCos1024(self->jumpPower);

                    createdEntity->xspeed = (64 * cos) / 1024;
                    createdEntity->yspeed = (64 * sin) / 1024;

                    createdEntity->linkedEntity = self;
                    self->state++;
                    soundPlaySfx(&(self->soundManager->sndWaveBall), BZR_LEFT);
                }

                if(self->state >= 4)
                {
                    self->state = 0;
                }

                self->stateTimer = 0;
            }
            break;

    }

    mg_updateInvincibilityFrames(self);
    mg_detectEntityCollisions(self);
}

int16_t clampAngleTo8way(int16_t angle)
{
    switch(angle)
    {
        case 0 ... 22:
        default:
            return 0;
        case 23 ... 67:
            return 45;
        case 68 ... 112:
            return 90;
        case 113 ... 157:
            return 135;
        case 158 ... 202:
            return 180;
        case 203 ... 247:
            return 225;
        case 248 ... 292:
            return 270;
        case 293 ... 337:
            return 315;
        case 338 ... 359:
            return 0;
    }
}