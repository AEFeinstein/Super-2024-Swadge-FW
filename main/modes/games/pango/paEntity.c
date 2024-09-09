//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include "paEntity.h"
#include "paEntityManager.h"
#include "paTilemap.h"
#include "paGameData.h"
#include "soundFuncs.h"
#include "hdw-btn.h"
#include "esp_random.h"
//#include "aabb_utils.h"
#include "trigonometry.h"
#include <esp_log.h>
#include "soundFuncs.h"

//==============================================================================
// Constants
//==============================================================================
#define SUBPIXEL_RESOLUTION        4
#define PA_TILE_SIZE_IN_POWERS_OF_2 4
#define PA_TILE_SIZE                16
#define PA_HALF_TILESIZE           8
#define DESPAWN_THRESHOLD          64

#define SIGNOF(x)           ((x > 0) - (x < 0))
#define PA_TO_TILECOORDS(x) ((x) >> PA_TILE_SIZE_IN_POWERS_OF_2)
#define PA_GET_TAXICAB_DISTANCE(x1, y1, x2, y2) (abs(x1 - x2) + abs(y1 - y2))
// #define TO_PIXEL_COORDS(x) ((x) >> SUBPIXEL_RESOLUTION)
// #define TO_SUBPIXEL_COORDS(x) ((x) << SUBPIXEL_RESOLUTION)

//==============================================================================
// Functions
//==============================================================================
void pa_initializeEntity(paEntity_t* self, paEntityManager_t* entityManager, paTilemap_t* tilemap,
                         paGameData_t* gameData, paSoundManager_t* soundManager)
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
    self->facingDirection = PA_DIRECTION_SOUTH;
    self->stateTimer = -1;
    self->tempStateTimer = -1;
    self->baseSpeed = 0;
    self->stateFlag = false;

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

void pa_updatePlayer(paEntity_t* self)
{
    switch(self->state){
        case PA_PL_ST_NORMAL:
        default:
        {
            if (self->gameData->btnState & PB_LEFT)
            {
                self->xspeed -= 4;

                if (self->xspeed < -16)
                {
                    self->xspeed = -16;
                }
            }
            else if (self->gameData->btnState & PB_RIGHT)
            {
                self->xspeed += 4;

                if (self->xspeed > 16)
                {
                    self->xspeed = 16;
                }
            }

            if (self->gameData->btnState & PB_UP)
            {
                self->yspeed -= 4;

                if (self->yspeed < -16)
                {
                    self->yspeed = -16;
                }
            }
            else if (self->gameData->btnState & PB_DOWN)
            {
                self->yspeed += 4;

                if (self->yspeed > 16)
                {
                    self->yspeed = 16;
                }
            }

            if (self->animationTimer > 0)
            {
                self->animationTimer--;
            }

            if (((self->gameData->btnState & PB_START) && !(self->gameData->prevBtnState & PB_START)))
            {
                self->gameData->changeState = PA_ST_PAUSE;
            }

        /*
            if(self->xspeed){
                self->targetTileX = PA_TO_TILECOORDS(self->x >> SUBPIXEL_RESOLUTION) + SIGNOF(self->xspeed);

                if(!self->yspeed){
                    self->targetTileY = PA_TO_TILECOORDS(self->y >> SUBPIXEL_RESOLUTION);
                }
            }

            if(self->yspeed){
                self->targetTileY = PA_TO_TILECOORDS(self->y >> SUBPIXEL_RESOLUTION) + SIGNOF(self->yspeed);

                if(!self->xspeed){
                    self->targetTileX = PA_TO_TILECOORDS(self->x >> SUBPIXEL_RESOLUTION);
                }
            }
        */

            switch(self->facingDirection){
                case PA_DIRECTION_WEST:
                    self->targetTileX = PA_TO_TILECOORDS(self->x >> SUBPIXEL_RESOLUTION) - 1;
                    self->targetTileY = PA_TO_TILECOORDS(self->y >> SUBPIXEL_RESOLUTION);
                    break;
                case PA_DIRECTION_EAST:
                    self->targetTileX = PA_TO_TILECOORDS(self->x >> SUBPIXEL_RESOLUTION) + 1;
                    self->targetTileY = PA_TO_TILECOORDS(self->y >> SUBPIXEL_RESOLUTION);
                    break;
                case PA_DIRECTION_NORTH:
                    self->targetTileX = PA_TO_TILECOORDS(self->x >> SUBPIXEL_RESOLUTION);
                    self->targetTileY = PA_TO_TILECOORDS(self->y >> SUBPIXEL_RESOLUTION) - 1;
                    break;
                case PA_DIRECTION_SOUTH:
                default:
                    self->targetTileX = PA_TO_TILECOORDS(self->x >> SUBPIXEL_RESOLUTION);
                    self->targetTileY = PA_TO_TILECOORDS(self->y >> SUBPIXEL_RESOLUTION) + 1;
                    break;
            }

            if (self->gameData->btnState & PB_A && !(self->gameData->prevBtnState & PB_A))
            {
                uint8_t t = pa_getTile(self->tilemap, self->targetTileX, self->targetTileY);
                if(t == PA_TILE_BLOCK || t  == PA_TILE_SPAWN_BLOCK_0 || t == PA_TILE_BONUS_BLOCK_0){
                    paEntity_t* newHitBlock = createHitBlock(self->entityManager, (self->targetTileX << SUBPIXEL_RESOLUTION) + PA_HALF_TILESIZE, (self->targetTileY << SUBPIXEL_RESOLUTION) + PA_HALF_TILESIZE);
                    
                    if(newHitBlock != NULL){
                        pa_setTile(self->tilemap, self->targetTileX, self->targetTileY, PA_TILE_EMPTY);
                        newHitBlock->jumpPower = t;
                        switch(self->facingDirection){
                            case PA_DIRECTION_WEST:
                                newHitBlock->xspeed = -64;
                                break;
                            case PA_DIRECTION_EAST:
                                newHitBlock->xspeed = 64;
                                break;
                            case PA_DIRECTION_NORTH:
                                newHitBlock->yspeed = -64;
                                break;
                            case PA_DIRECTION_SOUTH:
                            default:
                                newHitBlock->yspeed = 64;
                                break;
                        }
                        soundPlaySfx(&(self->soundManager->sndSquish), BZR_LEFT);
                    }
                }

                self->state = PA_PL_ST_PUSHING;
                self->stateTimer = 8;

                switch(self->facingDirection){
                    case PA_DIRECTION_WEST:
                        self->spriteIndex = PA_SP_PLAYER_PUSH_SIDE_1;
                        break;
                    case PA_DIRECTION_EAST:
                        self->spriteIndex = PA_SP_PLAYER_PUSH_SIDE_1;
                        break;
                    case PA_DIRECTION_NORTH:
                        self->spriteIndex = PA_SP_PLAYER_PUSH_NORTH_1;
                        break;
                    case PA_DIRECTION_SOUTH:
                    default:
                        self->spriteIndex = PA_SP_PLAYER_PUSH_SOUTH_1;
                        break;
                }

                break;
            }
            animatePlayer(self);
            break;
        }
        case PA_PL_ST_PUSHING:
        {
            self->stateTimer--;

            if(self->stateTimer < 0){
                self->state = PA_PL_ST_NORMAL;
                break;
            }

            if(self->stateTimer == 2){
                self->spriteIndex++;
            }

            break;
        }
    }
    
    pa_moveEntityWithTileCollisions(self);
    applyDamping(self);
    pa_detectEntityCollisions(self);
}

void updateCrabdozer(paEntity_t* self)
{
    switch(self->state){
        case PA_EN_ST_STUN: 
            self->stateTimer--;
            if(self->stateTimer < 0){
                self->facingDirection = PA_DIRECTION_NONE;
                
                /*if(self->stateFlag){
                    self->state = PA_EN_ST_AGGRESSIVE;
                    self->stateTimer = 32767; //effectively always aggressive
                    self->entityManager->aggroEnemies++;
                } else*/ {
                    self->state = PA_EN_ST_NORMAL;
                    self->stateTimer = (300 + esp_random() % 600); //Min 5 seconds, max 15 seconds
                }     
            } else {
                if (self->gameData->frameCount % ((self->stateTimer >> 1)+1) == 0)
                {
                    self->spriteIndex = PA_SP_ENEMY_STUN;
                    self->spriteFlipHorizontal = !self->spriteFlipHorizontal;
                }
            }

            pa_detectEntityCollisions(self);
            break;
        case PA_EN_ST_NORMAL:
        case PA_EN_ST_AGGRESSIVE: 
        case PA_EN_ST_RUNAWAY: {
            
            self->stateTimer--;
            if(self->stateTimer < 0 || self->entityManager->aggroEnemies < self->gameData->minAggroEnemies){
                if(self->state == PA_EN_ST_RUNAWAY){
                    killEnemy(self);
                    break;
                } else if(self->state == PA_EN_ST_NORMAL && (self->entityManager->aggroEnemies < self->gameData->maxAggroEnemies)){
                    self->state = PA_EN_ST_AGGRESSIVE;
                    self->entityManager->aggroEnemies++;
                    self->baseSpeed+=2;
                    self->stateTimer = (300 + esp_random() % 300); //Min 5 seconds, max 10 seconds
                } else if (self->state == PA_EN_ST_AGGRESSIVE){
                    self->state = PA_EN_ST_NORMAL;
                    self->entityManager->aggroEnemies--;
                    self->baseSpeed-=2;
                    self->stateTimer = (300 + esp_random() % 300); //Min 5 seconds, max 10 seconds
                }
            }

            if(self->state != PA_EN_ST_RUNAWAY && self->entityManager->activeEnemies == 1 && self->gameData->remainingEnemies == 0){
                self->state = PA_EN_ST_RUNAWAY;
                self->entityManager->aggroEnemies = 1;
                self->baseSpeed=20;
                self->stateTimer = 480; //8 seconds

                self->targetTileX = (esp_random() % 2) ? 1 : 15;
                self->targetTileY = (esp_random() % 2) ? 1 : 13;
            }

            uint8_t tx = PA_TO_TILECOORDS(self->x >> SUBPIXEL_RESOLUTION);
            uint8_t ty = PA_TO_TILECOORDS(self->y >> SUBPIXEL_RESOLUTION);

            uint8_t t1, t2, t3 = 0;
            uint8_t distT1, distT2, distT3;

            if(self->state != PA_EN_ST_RUNAWAY) {
                self->targetTileX = PA_TO_TILECOORDS(self->entityManager->playerEntity->x >> SUBPIXEL_RESOLUTION);
                self->targetTileY = PA_TO_TILECOORDS(self->entityManager->playerEntity->y >> SUBPIXEL_RESOLUTION);
            }

            int16_t hcof = (((self->x >> SUBPIXEL_RESOLUTION) % PA_TILE_SIZE) - PA_HALF_TILESIZE);
            int16_t vcof = (((self->y >> SUBPIXEL_RESOLUTION) % PA_TILE_SIZE) - PA_HALF_TILESIZE);

            bool doAgression = (self->state == PA_EN_ST_AGGRESSIVE) /*? esp_random() % 2 : false*/;

            switch(self->facingDirection){
                case PA_DIRECTION_WEST:
                    if(hcof) {
                        break;
                    }
                    
                    t1 = pa_getTile(self->tilemap, tx-1, ty);
                    t2 = pa_getTile(self->tilemap, tx, ty-1);
                    t3 = pa_getTile(self->tilemap, tx, ty+1);

                    distT1 = PA_GET_TAXICAB_DISTANCE(tx-1, ty, self->targetTileX, self->targetTileY);
                    distT2 = PA_GET_TAXICAB_DISTANCE(tx, ty-1, self->targetTileX, self->targetTileY);
                    distT3 = PA_GET_TAXICAB_DISTANCE(tx, ty+1, self->targetTileX, self->targetTileY);

                    if((!t2 || doAgression) && distT2 < distT1 && (t3 || distT2 < distT3)) {                        
                        if(doAgression && t2 == PA_TILE_BLOCK){
                            pa_enemyBreakBlock(self, PA_DIRECTION_NORTH, self->baseSpeed >> 1, tx, ty);
                        } else {
                            pa_enemyChangeDirection(self, PA_DIRECTION_NORTH, self->baseSpeed);
                        }
                        break;
                    } 

                    if((!t3 || doAgression) && distT3 < distT1) {                        
                        if(doAgression && t3 == PA_TILE_BLOCK){
                            pa_enemyBreakBlock(self, PA_DIRECTION_SOUTH, self->baseSpeed >> 1, tx, ty);
                        } else {
                            pa_enemyChangeDirection(self, PA_DIRECTION_SOUTH, self->baseSpeed);
                        }
                        break;
                    }

                    if (t1) {
                        if(doAgression && t1 == PA_TILE_BLOCK){
                            pa_enemyBreakBlock(self, PA_DIRECTION_WEST, self->baseSpeed >> 1, tx, ty);
                            break;
                        }

                        if((!t2 || doAgression) && (t3 || distT2 < distT3)){
                            if(doAgression && t2 == PA_TILE_BLOCK){
                                pa_enemyBreakBlock(self, PA_DIRECTION_NORTH, self->baseSpeed >> 1, tx, ty);
                            } else {
                                pa_enemyChangeDirection(self, PA_DIRECTION_NORTH, self->baseSpeed);
                            }
                            break;
                        }

                        if(!t3 || doAgression){
                            if(doAgression && t3 == PA_TILE_BLOCK){
                                pa_enemyBreakBlock(self, PA_DIRECTION_SOUTH, self->baseSpeed >> 1, tx, ty);
                            } else {
                                pa_enemyChangeDirection(self, PA_DIRECTION_SOUTH, self->baseSpeed);
                            }
                            break;
                        }

                        pa_enemyChangeDirection(self, PA_DIRECTION_EAST, self->baseSpeed);
                        break;
                    }
                    
                    break;
                case PA_DIRECTION_EAST:
                    if(hcof) {
                        break;
                    }
                    
                    t1 = pa_getTile(self->tilemap, tx+1, ty);
                    t2 = pa_getTile(self->tilemap, tx, ty-1);
                    t3 = pa_getTile(self->tilemap, tx, ty+1);

                    distT1 = PA_GET_TAXICAB_DISTANCE(tx+1, ty, self->targetTileX, self->targetTileY);
                    distT2 = PA_GET_TAXICAB_DISTANCE(tx, ty-1, self->targetTileX, self->targetTileY);
                    distT3 = PA_GET_TAXICAB_DISTANCE(tx, ty+1, self->targetTileX, self->targetTileY);

                    if((!t2 || doAgression) && distT2 < distT1 && (t3 || distT2 < distT3)){
                        
                        if(doAgression && t2 == PA_TILE_BLOCK){
                            pa_enemyBreakBlock(self, PA_DIRECTION_NORTH, self->baseSpeed >> 1, tx, ty);
                        } else {
                            pa_enemyChangeDirection(self, PA_DIRECTION_NORTH, self->baseSpeed);
                        }
                        break;
                    } 
                    if ((!t3 || doAgression) && distT3 < distT1){
                        
                        if(doAgression && t3 == PA_TILE_BLOCK){
                            pa_enemyBreakBlock(self, PA_DIRECTION_SOUTH, self->baseSpeed >> 1, tx, ty);
                        } else {
                            pa_enemyChangeDirection(self, PA_DIRECTION_SOUTH, self->baseSpeed);
                        }
                        break;
                    } 
                    if (t1){
                        if(doAgression && t1 == PA_TILE_BLOCK){
                            pa_enemyBreakBlock(self, PA_DIRECTION_EAST, self->baseSpeed >> 1, tx, ty);
                            break;
                        }

                        if((!t2 || doAgression) && (t3 || distT2 < distT3)) {
                            if(doAgression && t2 == PA_TILE_BLOCK){
                                pa_enemyBreakBlock(self, PA_DIRECTION_NORTH, self->baseSpeed >> 1, tx, ty);
                            } else {
                                pa_enemyChangeDirection(self, PA_DIRECTION_NORTH, self->baseSpeed);
                            }
                            break;
                        }

                        if(!t3 || doAgression){
                            if(doAgression && t2 == PA_TILE_BLOCK){
                                pa_enemyBreakBlock(self, PA_DIRECTION_SOUTH, self->baseSpeed >> 1, tx, ty);
                            } else {
                                pa_enemyChangeDirection(self, PA_DIRECTION_SOUTH, self->baseSpeed);
                            }
                            break;
                        }

                        pa_enemyChangeDirection(self, PA_DIRECTION_WEST, self->baseSpeed);
                        break;
                    }
                    
                    break;
                case PA_DIRECTION_NORTH:
                    if(vcof) {
                        break;
                    }
                    
                    t1 = pa_getTile(self->tilemap, tx, ty-1);
                    t2 = pa_getTile(self->tilemap, tx-1, ty);
                    t3 = pa_getTile(self->tilemap, tx+1, ty);

                    distT1 = PA_GET_TAXICAB_DISTANCE(tx, ty-1, self->targetTileX, self->targetTileY);
                    distT2 = PA_GET_TAXICAB_DISTANCE(tx-1, ty, self->targetTileX, self->targetTileY);
                    distT3 = PA_GET_TAXICAB_DISTANCE(tx+1, ty, self->targetTileX, self->targetTileY);

                    if((!t2 || doAgression) && distT2 < distT1 && (t3 || distT2 < distT3)){
                        if(doAgression && t2 == PA_TILE_BLOCK){
                            pa_enemyBreakBlock(self, PA_DIRECTION_WEST, self->baseSpeed >> 1, tx, ty);
                        } else {
                            pa_enemyChangeDirection(self, PA_DIRECTION_WEST, self->baseSpeed);
                        }
                        break;
                    } 

                    if((!t3 || doAgression) && distT3 < distT1){
                        if(doAgression && t3 == PA_TILE_BLOCK){
                            pa_enemyBreakBlock(self, PA_DIRECTION_EAST, self->baseSpeed >> 1, tx, ty);
                        } else {
                            pa_enemyChangeDirection(self, PA_DIRECTION_EAST, self->baseSpeed);
                        }
                        break;
                    } 

                    if (t1){
                        if(doAgression && t1 == PA_TILE_BLOCK){
                            pa_enemyBreakBlock(self, PA_DIRECTION_NORTH, self->baseSpeed >> 1, tx, ty);
                            break;
                        }

                        if((!t2 || doAgression) && (t3 || distT2 < distT3)){
                            if(doAgression && t2 == PA_TILE_BLOCK){
                                pa_enemyBreakBlock(self, PA_DIRECTION_WEST, self->baseSpeed >> 1, tx, ty);
                            } else {
                                pa_enemyChangeDirection(self, PA_DIRECTION_WEST, self->baseSpeed);
                            }
                            break;
                        }

                        if(!t3 || doAgression){
                            if(doAgression && t3 == PA_TILE_BLOCK){
                                pa_enemyBreakBlock(self, PA_DIRECTION_EAST, self->baseSpeed >> 1, tx, ty);
                            } else {
                                pa_enemyChangeDirection(self, PA_DIRECTION_EAST, self->baseSpeed);
                            }
                            
                            break;
                        }

                        pa_enemyChangeDirection(self, PA_DIRECTION_SOUTH, self->baseSpeed);
                        break;
                    }

                    break;
                case PA_DIRECTION_NONE:
                default:
                    pa_enemyChangeDirection(self, 1 >> (esp_random() % 3), self->baseSpeed);
                    break;
                case PA_DIRECTION_SOUTH:
                    if(vcof) {
                        break;
                    }
                    
                    t1 = pa_getTile(self->tilemap, tx, ty+1);
                    t2 = pa_getTile(self->tilemap, tx-1, ty);
                    t3 = pa_getTile(self->tilemap, tx+1, ty);
                    
                    distT1 = PA_GET_TAXICAB_DISTANCE(tx, ty+1, self->targetTileX, self->targetTileY);
                    distT2 = PA_GET_TAXICAB_DISTANCE(tx-1, ty, self->targetTileX, self->targetTileY);
                    distT3 = PA_GET_TAXICAB_DISTANCE(tx+1, ty, self->targetTileX, self->targetTileY);

                    if((!t2 || doAgression) && distT2 < distT1 && (t3 || distT2 < distT3)){ 
                        if(doAgression && t2 == PA_TILE_BLOCK){
                            pa_enemyBreakBlock(self, PA_DIRECTION_WEST, self->baseSpeed >> 1, tx, ty);
                        } else {
                            pa_enemyChangeDirection(self, PA_DIRECTION_WEST, self->baseSpeed);
                        }
                        break;
                    } 

                    if((!t3 || doAgression) && distT3 < distT1){
                        if(doAgression && t3 == PA_TILE_BLOCK){
                            pa_enemyBreakBlock(self, PA_DIRECTION_EAST, self->baseSpeed >> 1, tx, ty);
                        } else {
                            pa_enemyChangeDirection(self, PA_DIRECTION_EAST, self->baseSpeed);
                        }
                        break;
                    } 

                    if (t1){
                        if(doAgression && t1 == PA_TILE_BLOCK){
                            pa_enemyBreakBlock(self, PA_DIRECTION_SOUTH, self->baseSpeed >> 1, tx, ty);
                            break;
                        }

                        if((!t2 || doAgression) && (t3 || distT2 < distT3)){
                            if(doAgression && t2 == PA_TILE_BLOCK){
                                pa_enemyBreakBlock(self, PA_DIRECTION_WEST, self->baseSpeed >> 1, tx, ty);
                            } else {
                                pa_enemyChangeDirection(self, PA_DIRECTION_WEST, self->baseSpeed);
                            }
                            break;
                        }

                        if(!t3 || doAgression){
                            if(doAgression && t3 == PA_TILE_BLOCK){
                                pa_enemyBreakBlock(self, PA_DIRECTION_EAST, self->baseSpeed >> 1, tx, ty);
                            } else {
                                pa_enemyChangeDirection(self, PA_DIRECTION_EAST, self->baseSpeed);
                            }
                            break;
                        }

                        pa_enemyChangeDirection(self, PA_DIRECTION_NORTH, self->baseSpeed);
                        break;
                    }

                    break;
            }

            pa_animateEnemy(self);
            despawnWhenOffscreen(self);
            if(self->state != PA_EN_ST_BREAK_BLOCK){
                //Need to skip this if enemy has just changed to breaking block state
                //or else enemy will be stopped
                pa_moveEntityWithTileCollisions(self);
            }
            pa_detectEntityCollisions(self);
            
            break;
        }
        case PA_EN_ST_BREAK_BLOCK:{

            /*//Need to force a speed value because
            //tile collision will stop the enemy before we get here
            switch(self->facingDirection){
                case PA_DIRECTION_WEST:
                    self->xspeed = -8;
                    break;
                case PA_DIRECTION_EAST:
                    self->xspeed = 8;
                    break;
                case PA_DIRECTION_NORTH:
                    self->yspeed = -8;
                    break;
                case PA_DIRECTION_SOUTH:
                    self->yspeed = 8;
                    break;
                default:
                    break;
            }*/

            self->x += self->xspeed;
            self->y += self->yspeed;

            self->stateTimer--;
            if(self->stateTimer < 0){
                self->state = PA_EN_ST_AGGRESSIVE;
                self->xspeed *= 2;
                self->yspeed *= 2;
                self->stateTimer = self->tempStateTimer;
            }

            pa_animateEnemy(self);
            break;
        }
        default:{
            break;
        }
    }
}

void pa_enemyChangeDirection(paEntity_t* self, uint16_t newDirection, int16_t speed){
    switch(newDirection){
        case PA_DIRECTION_WEST:
            self->yspeed = 0;
            self->xspeed = -speed;
            break;
        case PA_DIRECTION_EAST:
            self->yspeed = 0;
            self->xspeed = speed;
            break;
        case PA_DIRECTION_NORTH:
            self->xspeed = 0;
            self->yspeed = -speed;
            break;
        case PA_DIRECTION_NONE:
        default:
            self->xspeed = 0;
            self->yspeed = 0;
            break;
        case PA_DIRECTION_SOUTH:
            self->xspeed = 0;
            self->yspeed = speed;
            break;
    }

    self->facingDirection = newDirection;
}

void pa_enemyBreakBlock(paEntity_t* self, uint16_t newDirection, int16_t speed, uint8_t tx, uint8_t ty){
    switch(newDirection){
        case PA_DIRECTION_WEST:
            pa_createBreakBlock(self->entityManager, ((tx-1) << SUBPIXEL_RESOLUTION) + PA_HALF_TILE_SIZE, (ty << SUBPIXEL_RESOLUTION) + PA_HALF_TILE_SIZE);
            break;
        case PA_DIRECTION_EAST:
            pa_createBreakBlock(self->entityManager, ((tx+1) << SUBPIXEL_RESOLUTION) + PA_HALF_TILE_SIZE, (ty << SUBPIXEL_RESOLUTION) + PA_HALF_TILE_SIZE);
            break;
        case PA_DIRECTION_NORTH:
            pa_createBreakBlock(self->entityManager, (tx << SUBPIXEL_RESOLUTION) + PA_HALF_TILE_SIZE, ((ty-1) << SUBPIXEL_RESOLUTION) + PA_HALF_TILE_SIZE);
            break;
        case PA_DIRECTION_NONE:
        default:
            break;
        case PA_DIRECTION_SOUTH:
            pa_createBreakBlock(self->entityManager, (tx << SUBPIXEL_RESOLUTION) + PA_HALF_TILE_SIZE, ((ty+1) << SUBPIXEL_RESOLUTION) + PA_HALF_TILE_SIZE);
            break;
    }

    self->state = PA_EN_ST_BREAK_BLOCK;
    self->tempStateTimer = self->stateTimer;
    self->stateTimer = 16;
    pa_enemyChangeDirection(self, newDirection, speed);
}

void pa_animateEnemy(paEntity_t* self){
    if (self->xspeed != 0)
    {
        if ((self->xspeed < 0)
            || (self->xspeed > 0))
        {
            // Running
            self->spriteFlipHorizontal = (self->xspeed > 0) ? 0 : 1;

            if (self->gameData->frameCount % 5 == 0)
            {
                self->spriteIndex = PA_SP_ENEMY_SIDE_1 + ((self->spriteIndex + 1) % 2) + ((self->state != PA_EN_ST_NORMAL) ? 4 : 0);
                self->facingDirection = self->spriteFlipHorizontal ? PA_DIRECTION_WEST : PA_DIRECTION_EAST;
            }
        }
        else
        {
            //self->spriteIndex = SP_PLAYER_SLIDE;
        }
    }
    else if (self->yspeed > 0){
        if (self->yspeed > 0){
            if (self->gameData->frameCount % 5 == 0)
            {
                self->spriteIndex = PA_SP_ENEMY_SOUTH + ((self->state != PA_EN_ST_NORMAL) ? 4 : 0);
                self->spriteFlipHorizontal = (self->gameData->frameCount >> 1) % 2;
                self->facingDirection = PA_DIRECTION_SOUTH;
            }
        }
    }
    else if (self->yspeed < 0){
        if (self->yspeed < 0){
            if (self->gameData->frameCount % 5 == 0)
            {
                self->spriteIndex = PA_SP_ENEMY_NORTH + ((self->state != PA_EN_ST_NORMAL) ? 4 : 0);
                self->spriteFlipHorizontal = (self->gameData->frameCount >> 1) % 2;
                self->facingDirection = PA_DIRECTION_NORTH;
            }
        }
    }
    else
    {
        self->facingDirection = PA_DIRECTION_NONE;
    }
}

void updateHitBlock(paEntity_t* self)
{
    self->animationTimer++;

    if (self->homeTileY > self->tilemap->mapHeight)
    {
        pa_destroyEntity(self, false);
        return;
    }

    pa_moveEntityWithTileCollisions(self);
}

void pa_moveEntityWithTileCollisions(paEntity_t* self)
{
    uint16_t newX = self->x;
    uint16_t newY = self->y;
    uint8_t tx    = PA_TO_TILECOORDS(self->x >> SUBPIXEL_RESOLUTION);
    uint8_t ty    = PA_TO_TILECOORDS(self->y >> SUBPIXEL_RESOLUTION);
    // bool collision = false;

    // Are we inside a block? Push self out of block
    uint8_t t = pa_getTile(self->tilemap, tx, ty);
    self->overlapTileHandler(self, t, tx, ty);

    if (pa_isSolid(t))
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
            int16_t hcof = (((self->x >> SUBPIXEL_RESOLUTION) % PA_TILE_SIZE) - PA_HALF_TILESIZE);

            // Handle halfway though tile
            uint8_t at = pa_getTile(self->tilemap, tx + SIGNOF(hcof), ty);

            if (pa_isSolid(at))
            {
                // collision = true;
                newX = ((tx + 1) * PA_TILE_SIZE - PA_HALF_TILESIZE) << SUBPIXEL_RESOLUTION;
            }

            uint8_t newTy = PA_TO_TILECOORDS(((self->y + self->yspeed) >> SUBPIXEL_RESOLUTION)
                                             + SIGNOF(self->yspeed) * PA_HALF_TILESIZE);

            if (newTy != ty)
            {
                uint8_t newVerticalTile = pa_getTile(self->tilemap, tx, newTy);

                //if (newVerticalTile > PA_TILE_UNUSED_29 && newVerticalTile < PA_TILE_BG_GOAL_ZONE)
                {
                    if (self->tileCollisionHandler(self, newVerticalTile, tx, newTy, 2 << (self->yspeed > 0)))
                    {
                        newY = ((newTy + ((ty < newTy) ? -1 : 1)) * PA_TILE_SIZE + PA_HALF_TILESIZE)
                               << SUBPIXEL_RESOLUTION;
                    }
                }
            }
        }

        if (self->xspeed != 0)
        {
            int16_t vcof = (((self->y >> SUBPIXEL_RESOLUTION) % PA_TILE_SIZE) - PA_HALF_TILESIZE);

            // Handle halfway though tile
            uint8_t att = pa_getTile(self->tilemap, tx, ty + SIGNOF(vcof));

            if (pa_isSolid(att))
            {
                // collision = true;
                newY = ((ty + 1) * PA_TILE_SIZE - PA_HALF_TILESIZE) << SUBPIXEL_RESOLUTION;
            }

            // Handle outside of tile
            uint8_t newTx = PA_TO_TILECOORDS(((self->x + self->xspeed) >> SUBPIXEL_RESOLUTION)
                                             + SIGNOF(self->xspeed) * PA_HALF_TILESIZE);

            if (newTx != tx)
            {
                uint8_t newHorizontalTile = pa_getTile(self->tilemap, newTx, ty);

                //if (newHorizontalTile > PA_TILE_UNUSED_29 && newHorizontalTile < PA_TILE_BG_GOAL_ZONE)
                {
                    if (self->tileCollisionHandler(self, newHorizontalTile, newTx, ty, (self->xspeed > 0)))
                    {
                        newX = ((newTx + ((tx < newTx) ? -1 : 1)) * PA_TILE_SIZE + PA_HALF_TILESIZE)
                               << SUBPIXEL_RESOLUTION;
                    }
                }

                if (!self->falling)
                {
                    uint8_t newBelowTile = pa_getTile(self->tilemap, tx, ty + 1);

                    if ((self->gravityEnabled
                         && !pa_isSolid(newBelowTile)) /*(|| (!self->gravityEnabled && newBelowTile != PA_TILE_LADDER)*/)
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

void defaultFallOffTileHandler(paEntity_t* self)
{
    self->falling = true;
}

void applyDamping(paEntity_t* self)
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

void applyGravity(paEntity_t* self)
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

void despawnWhenOffscreen(paEntity_t* self)
{
    if ((self->x >> SUBPIXEL_RESOLUTION) < (self->tilemap->mapOffsetX - DESPAWN_THRESHOLD)
        || (self->x >> SUBPIXEL_RESOLUTION)
               > (self->tilemap->mapOffsetX + PA_TILE_MAP_DISPLAY_WIDTH_PIXELS + DESPAWN_THRESHOLD))
    {
        pa_destroyEntity(self, true);
    }

    if (self->y > 63616)
    {
        return;
    }

    if ((self->y >> SUBPIXEL_RESOLUTION) < (self->tilemap->mapOffsetY - (DESPAWN_THRESHOLD << 2))
        || (self->y >> SUBPIXEL_RESOLUTION)
               > (self->tilemap->mapOffsetY + PA_TILE_MAP_DISPLAY_HEIGHT_PIXELS + DESPAWN_THRESHOLD))
    {
        pa_destroyEntity(self, true);
    }
}

void pa_destroyEntity(paEntity_t* self, bool respawn)
{
    /*if (respawn && !(self->homeTileX == 0 && self->homeTileY == 0))
    {
        self->tilemap->map[self->homeTileY * self->tilemap->mapWidth + self->homeTileX] = self->type + 128;
    }*/

    // self->entityManager->activeEntities--;
    self->active = false;
}

void animatePlayer(paEntity_t* self)
{
    if (abs(self->xspeed) > abs(self->yspeed))
    {
        if (((self->gameData->btnState & PB_LEFT) && self->xspeed < 0)
            || ((self->gameData->btnState & PB_RIGHT) && self->xspeed > 0))
        {
            // Running
            self->spriteFlipHorizontal = (self->xspeed > 0) ? 0 : 1;
            self->facingDirection = self->spriteFlipHorizontal ? PA_DIRECTION_WEST : PA_DIRECTION_EAST;

            if (self->gameData->frameCount % 7 == 0)
            {
                self->spriteIndex = PA_SP_PLAYER_SIDE + ((self->spriteIndex + 1) % 3);
            }
        }
        else
        {
            //self->spriteIndex = SP_PLAYER_SLIDE;
        }
    }
    else if (self->yspeed > 0){
        if ((self->gameData->btnState & PB_DOWN) && self->yspeed > 0){
            self->facingDirection = PA_DIRECTION_SOUTH;

            if (self->gameData->frameCount % 7 == 0)
            {
                self->spriteIndex = PA_SP_PLAYER_SOUTH + ((self->spriteIndex + 1) % 2);
                self->spriteFlipHorizontal = (self->gameData->frameCount >> 1) % 2;   
            }
        }
    }
     else if (self->yspeed < 0){
        if ((self->gameData->btnState & PB_UP) && self->yspeed < 0){
            self->facingDirection = PA_DIRECTION_NORTH;

            if (self->gameData->frameCount % 7 == 0)
            {
                self->spriteIndex = PA_SP_PLAYER_NORTH + ((self->spriteIndex + 1) % 2);
                self->spriteFlipHorizontal = (self->gameData->frameCount >> 1) % 2;
            }
        }
    }
    else
    {
        // Standing
        //self->spriteIndex = PA_SP_PLAYER_SOUTH;
    }
}

void pa_detectEntityCollisions(paEntity_t* self)
{
    for (uint8_t i = 0; i < MAX_ENTITIES; i++)
    {
        paEntity_t* checkEntity = &(self->entityManager->entities[i]);
        if (checkEntity->active && checkEntity != self)
        {
            uint32_t dist = abs(self->x - checkEntity->x) + abs(self->y - checkEntity->y);

            if (dist < 200)
            {
                self->collisionHandler(self, checkEntity);
            }
        }
    }
}

void pa_playerCollisionHandler(paEntity_t* self, paEntity_t* other)
{
    switch (other->type)
    {
        case PA_ENTITY_CRABDOZER:
        {
            other->xspeed = -other->xspeed;

            /*if (self->y < other->y || self->yspeed > 0)
            {
                pa_scorePoints(self->gameData, other->scoreValue);

                killEnemy(other);
                soundPlaySfx(&(self->soundManager->sndSquish), BZR_LEFT);

                self->yspeed    = -180;
                self->jumpPower = 64 + ((abs(self->xspeed) + 16) >> 3);
                self->falling   = true;
            }
            else*/ if (self->invincibilityFrames <= 0 && other->state != PA_EN_ST_STUN)
            {
                self->hp--;
                pa_updateLedsHpMeter(self->entityManager, self->gameData);
                self->gameData->comboTimer = 0;

                if (!self->gameData->debugMode && self->hp == 0)
                {
                    self->updateFunction        = &updateEntityDead;
                    self->type                  = ENTITY_DEAD;
                    self->xspeed                = 0;
                    self->yspeed                = -60;
                    self->spriteIndex           = PA_SP_PLAYER_HURT;
                    self->gameData->changeState = PA_ST_DEAD;
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
        case ENTITY_HIT_BLOCK:
        {
            if(self->x < other->x){
                self->x = other->x - (PA_TILE_SIZE << SUBPIXEL_RESOLUTION);
                self->xspeed = 0;
            } else if (self->x > other->x){
                self->x = other->x + (PA_TILE_SIZE << SUBPIXEL_RESOLUTION);
                self->xspeed = 0;
            } else if (self->y < other->y){
                self->y = other->y - (PA_TILE_SIZE << SUBPIXEL_RESOLUTION);
                self->yspeed = 0;
            } else if (self->y > other->y){
                self->y = other->y + (PA_TILE_SIZE << SUBPIXEL_RESOLUTION);
                self->yspeed = 0;
            }
            break;
        }
        default:
        {
            break;
        }
    }
}

void pa_enemyCollisionHandler(paEntity_t* self, paEntity_t* other)
{
    switch (other->type)
    {
        case PA_ENTITY_CRABDOZER:
            if ((self->xspeed > 0 && self->x < other->x) || (self->xspeed < 0 && self->x > other->x))
            {
                self->xspeed               = -self->xspeed;
                //self->spriteFlipHorizontal = -self->spriteFlipHorizontal;
            }

            if ((self->yspeed > 0 && self->y < other->y) || (self->yspeed < 0 && self->y > other->y))
            {
                self->yspeed               = -self->yspeed;
                //self->spriteFlipHorizontal = -self->spriteFlipHorizontal;
            }
            break;
        case ENTITY_HIT_BLOCK:
            self->xspeed = other->xspeed * 2;
            self->yspeed = other->yspeed * 2;
            pa_scorePoints(self->gameData, self->scoreValue);
            soundPlaySfx(&(self->soundManager->sndHurt), 2);
            killEnemy(self);
            break;
        default:
        {
            break;
        }
    }
}

void pa_dummyCollisionHandler(paEntity_t* self, paEntity_t* other)
{
    return;
}

bool pa_playerTileCollisionHandler(paEntity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction)
{
    /*switch (tileId)
    {
        case PA_TILE_COIN_1 ... PA_TILE_COIN_3:
        {
            pa_setTile(self->tilemap, tx, ty, PA_TILE_EMPTY);
            addCoins(self->gameData, 1);
            pa_scorePoints(self->gameData, 50);
            break;
        }
        case PA_TILE_LADDER:
        {
            self->gravityEnabled = false;
            self->falling = false;
            break;
        }
        default:
        {
            break;
        }
    }*/

    if (pa_isSolid(tileId))
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

bool pa_enemyTileCollisionHandler(paEntity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction)
{
    /*switch (tileId)
    {
        case PA_TILE_BOUNCE_BLOCK:
        {
            switch (direction)
            {
                case 0:
                    // hitBlock->xspeed = -64;
                    if (tileId == PA_TILE_BOUNCE_BLOCK)
                    {
                        self->xspeed = 48;
                    }
                    break;
                case 1:
                    // hitBlock->xspeed = 64;
                    if (tileId == PA_TILE_BOUNCE_BLOCK)
                    {
                        self->xspeed = -48;
                    }
                    break;
                case 2:
                    // hitBlock->yspeed = -128;
                    if (tileId == PA_TILE_BOUNCE_BLOCK)
                    {
                        self->yspeed = 48;
                    }
                    break;
                case 4:
                    // hitBlock->yspeed = (tileId == PA_TILE_BRICK_BLOCK) ? 32 : 64;
                    if (tileId == PA_TILE_BOUNCE_BLOCK)
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
    }*/

    if (pa_isSolid(tileId))
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

bool pa_dummyTileCollisionHandler(paEntity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction)
{
    return false;
}

bool pa_hitBlockTileCollisionHandler(paEntity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction)
{
    if (pa_isSolid(tileId))
    {
        soundPlaySfx(&(self->soundManager->sndHit), 1);
        pa_destroyEntity(self, false);
       
       if(PA_TO_TILECOORDS(self->x >> SUBPIXEL_RESOLUTION) == self->homeTileX && PA_TO_TILECOORDS(self->y >> SUBPIXEL_RESOLUTION) == self->homeTileY){
            pa_createBreakBlock(self->entityManager, self->x >> SUBPIXEL_RESOLUTION, self->y >> SUBPIXEL_RESOLUTION);
            if(self->jumpPower == PA_TILE_SPAWN_BLOCK_0){
                self->entityManager->gameData->remainingEnemies--;
            }
        } else {
            self->tilemap->map[PA_TO_TILECOORDS(self->y >> SUBPIXEL_RESOLUTION) * self->tilemap->mapWidth + PA_TO_TILECOORDS(self->x >> SUBPIXEL_RESOLUTION)] = self->jumpPower;
        }

        return true;
    }
    return false;
}

void dieWhenFallingOffScreen(paEntity_t* self)
{
    uint16_t deathBoundary = (self->tilemap->mapOffsetY + PA_TILE_MAP_DISPLAY_HEIGHT_PIXELS + DESPAWN_THRESHOLD);
    if (((self->y >> SUBPIXEL_RESOLUTION) > deathBoundary)
        && ((self->y >> SUBPIXEL_RESOLUTION) < deathBoundary + DESPAWN_THRESHOLD))
    {
        self->hp = 0;
        pa_updateLedsHpMeter(self->entityManager, self->gameData);
        self->gameData->changeState = PA_ST_DEAD;
        pa_destroyEntity(self, true);
    }
}

void pa_updateDummy(paEntity_t* self)
{
    // Do nothing, because that's what dummies do!
}

void pa_updateBreakBlock(paEntity_t* self)
{
    if (self->gameData->frameCount % 4 == 0)
    {
        self->spriteIndex++;

        if(self->spriteIndex > PA_SP_BREAK_BLOCK_3){
            pa_createBlockFragment(self->entityManager, self->x >> SUBPIXEL_RESOLUTION, self->y >> SUBPIXEL_RESOLUTION);
            pa_createBlockFragment(self->entityManager, self->x >> SUBPIXEL_RESOLUTION, self->y >> SUBPIXEL_RESOLUTION);
            pa_createBlockFragment(self->entityManager, self->x >> SUBPIXEL_RESOLUTION, self->y >> SUBPIXEL_RESOLUTION);
            pa_createBlockFragment(self->entityManager, self->x >> SUBPIXEL_RESOLUTION, self->y >> SUBPIXEL_RESOLUTION);
            pa_destroyEntity(self, false);
        }
    }
}

void updateEntityDead(paEntity_t* self)
{
    applyGravity(self);
    self->x += self->xspeed;
    self->y += self->yspeed;

    despawnWhenOffscreen(self);
}

void pa_updateBlockFragment(paEntity_t* self)
{
    self->animationTimer++;
    if(self->animationTimer > 8){
        pa_destroyEntity(self, false);
        return;
    }

    self->x += self->xspeed;
    self->y += self->yspeed;
   
    applyGravity(self);
    despawnWhenOffscreen(self);
}

void killEnemy(paEntity_t* target)
{
    target->homeTileX          = 0;
    target->homeTileY          = 0;
    target->gravityEnabled     = true;
    target->falling            = true;
    target->type               = ENTITY_DEAD;
    target->spriteFlipVertical = true;
    target->updateFunction     = &updateEntityDead;
    
    target->entityManager->activeEnemies--;
    if(target->state == PA_EN_ST_AGGRESSIVE){
        target->entityManager->aggroEnemies--;
    }

    if(target->entityManager->activeEnemies == 0 && target->entityManager->gameData->remainingEnemies == 0){
        target->gameData->changeState = PA_ST_LEVEL_CLEAR;
        target->entityManager->playerEntity->spriteIndex = PA_SP_PLAYER_WIN;
        target->entityManager->playerEntity->updateFunction = &pa_updateDummy;
    }
}

void pa_playerOverlapTileHandler(paEntity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty)
{
    /*switch (tileId)
    {
        case PA_TILE_COIN_1 ... PA_TILE_COIN_3:
        {
            pa_setTile(self->tilemap, tx, ty, PA_TILE_EMPTY);
            addCoins(self->gameData, 1);
            pa_scorePoints(self->gameData, 50);
            break;
        }
        case PA_TILE_LADDER:
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

    if (!self->gravityEnabled && tileId != PA_TILE_LADDER)
    {
        self->gravityEnabled = true;
        self->falling        = true;
        if (self->yspeed < 0)
        {
            self->yspeed = -32;
        }
    }*/
}

void pa_defaultOverlapTileHandler(paEntity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty)
{
    // Nothing to do.
}

void killPlayer(paEntity_t* self)
{
    self->hp = 0;
    pa_updateLedsHpMeter(self->entityManager, self->gameData);

    self->updateFunction        = &updateEntityDead;
    self->type                  = ENTITY_DEAD;
    self->xspeed                = 0;
    self->yspeed                = -60;
    self->spriteIndex           = PA_SP_PLAYER_HURT;
    self->gameData->changeState = PA_ST_DEAD;
    self->falling               = true;
}

void drawEntityTargetTile(paEntity_t* self){
    drawRect((self->targetTileX << PA_TILE_SIZE_IN_POWERS_OF_2) - self->tilemap->mapOffsetX, self->targetTileY << PA_TILE_SIZE_IN_POWERS_OF_2, (self->targetTileX << PA_TILE_SIZE_IN_POWERS_OF_2) + PA_TILE_SIZE - self->tilemap->mapOffsetX, (self->targetTileY << PA_TILE_SIZE_IN_POWERS_OF_2) + PA_TILE_SIZE, esp_random() % 216);
}
