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
#include "trigonometry.h"
#include <esp_log.h>
#include "soundFuncs.h"
#include "paTables.h"

//==============================================================================
// Constants
//==============================================================================
#define SUBPIXEL_RESOLUTION         4
#define PA_TILE_SIZE_IN_POWERS_OF_2 4
#define PA_TILE_SIZE                16
#define PA_HALF_TILESIZE            8
#define DESPAWN_THRESHOLD           64

#define SIGNOF(x)                               ((x > 0) - (x < 0))
#define PA_TO_TILECOORDS(x)                     ((x) >> PA_TILE_SIZE_IN_POWERS_OF_2)
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
    self->spriteFlipHorizontal = false;
    self->spriteFlipVertical   = false;
    self->facingDirection      = PA_DIRECTION_SOUTH;
    self->stateTimer           = -1;
    self->tempStateTimer       = -1;
    self->baseSpeed            = 0;
    self->stateFlag            = false;

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
    switch (self->state)
    {
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

                // Make the player face in the most recent direction pressed
                if (!(self->gameData->prevBtnState & PB_LEFT))
                {
                    self->facingDirection = PA_DIRECTION_WEST;
                }
            }

            if (self->gameData->btnState & PB_RIGHT)
            {
                self->xspeed += 4;

                if (self->xspeed > 16)
                {
                    self->xspeed = 16;
                }

                // Make the player face in the most recent direction pressed
                if (!(self->gameData->prevBtnState & PB_RIGHT))
                {
                    self->facingDirection = PA_DIRECTION_EAST;
                }
            }

            if (self->gameData->btnState & PB_UP)
            {
                self->yspeed -= 4;

                if (self->yspeed < -16)
                {
                    self->yspeed = -16;
                }

                // Make the player face in the most recent direction pressed
                if (!(self->gameData->prevBtnState & PB_UP))
                {
                    self->facingDirection = PA_DIRECTION_NORTH;
                }
            }

            if (self->gameData->btnState & PB_DOWN)
            {
                self->yspeed += 4;

                if (self->yspeed > 16)
                {
                    self->yspeed = 16;
                }

                // Make the player face in the most recent direction pressed
                if (!(self->gameData->prevBtnState & PB_DOWN))
                {
                    self->facingDirection = PA_DIRECTION_SOUTH;
                }
            }

            // Fix the player's facing direction if:
            //- the player has let go of the most recent direction held
            //- the player is pressing opposite directions
            self->facingDirection = pa_correctPlayerFacingDirection(self->gameData->btnState, self->facingDirection);

            if (self->animationTimer > 0)
            {
                self->animationTimer--;
            }

            switch (self->facingDirection)
            {
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
                if (t == PA_TILE_BLOCK || t == PA_TILE_SPAWN_BLOCK_0 || t == PA_TILE_BONUS_BLOCK_0)
                {
                    paEntity_t* newHitBlock = createHitBlock(
                        self->entityManager, (self->targetTileX << SUBPIXEL_RESOLUTION) + PA_HALF_TILESIZE,
                        (self->targetTileY << SUBPIXEL_RESOLUTION) + PA_HALF_TILESIZE);

                    if (newHitBlock != NULL)
                    {
                        pa_setTile(self->tilemap, self->targetTileX, self->targetTileY, PA_TILE_EMPTY);
                        newHitBlock->state = t;

                        if (t == PA_TILE_SPAWN_BLOCK_0)
                        {
                            newHitBlock->spriteIndex = PA_SP_BONUS_BLOCK;
                        }

                        switch (self->facingDirection)
                        {
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
                        soundPlaySfx(&(self->soundManager->sndSlide), BZR_LEFT);
                    }
                }

                self->state      = PA_PL_ST_PUSHING;
                self->stateTimer = 8;

                switch (self->facingDirection)
                {
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

            if (self->stateTimer < 0)
            {
                self->state = PA_PL_ST_NORMAL;
                break;
            }

            if (self->stateTimer == 2)
            {
                self->spriteIndex++;
            }

            break;
        }
    }

    pa_moveEntityWithTileCollisions(self);
    applyDamping(self);
    pa_detectEntityCollisions(self);
}

uint16_t pa_correctPlayerFacingDirection(int16_t btnState, uint16_t currentFacingDirection)
{
    // Mask out button bits that do not correspond to directional buttons
    int16_t directionBtnState = btnState & 0b1111;

    switch (directionBtnState)
    {
        case PA_DIRECTION_NORTH:
        case PA_DIRECTION_SOUTH:
        case PA_DIRECTION_WEST:
        case PA_DIRECTION_EAST:
            return directionBtnState;
            break;
        default:
            return currentFacingDirection;
            break;
    }
}

void updateCrabdozer(paEntity_t* self)
{
    switch (self->state)
    {
        case PA_EN_ST_STUN:
            self->stateTimer--;
            if (self->stateTimer < 0)
            {
                self->facingDirection = PA_DIRECTION_NONE;
                self->state           = PA_EN_ST_NORMAL;
                self->stateTimer      = pa_enemySetAggroStateTimer(self);
            }
            else
            {
                if (self->gameData->frameCount % ((self->stateTimer >> 1) + 1) == 0)
                {
                    self->spriteIndex          = PA_SP_ENEMY_STUN;
                    self->spriteFlipHorizontal = !self->spriteFlipHorizontal;
                }
            }

            pa_detectEntityCollisions(self);
            break;
        case PA_EN_ST_NORMAL:
        case PA_EN_ST_AGGRESSIVE:
        case PA_EN_ST_RUNAWAY:
        {
            self->stateTimer--;

            if (self->state == PA_EN_ST_RUNAWAY && self->stateTimer < 0)
            {
                soundPlaySfx(&(self->soundManager->sndSquish), 2);
                self->spriteIndex = PA_SP_ENEMY_STUN;
                self->yspeed      = -32;
                self->gravity     = 4;
                killEnemy(self);
                break;
            }
            // The aggroEnemies < minAggroEnemies in the below condition is actually a bug.
            // It was originally intended to apply at any time when he enemy is in NORMAL state only.
            // This bug results in enemies "panicking" when there are fewer than minAggroEnemies active.
            // Similar to the RUNAWAY state, enemies cannot break blocks in this unintended state.
            // It gives the player a break in the later levels if they can maintain these conditions.
            // It looks cool and feels good to pull off, so I hereby decleare this bug a "feature".
            else if (self->stateTimer < 0 || self->entityManager->aggroEnemies < self->gameData->minAggroEnemies)
            {
                if (self->state == PA_EN_ST_NORMAL
                    && (self->entityManager->aggroEnemies < self->gameData->maxAggroEnemies))
                {
                    self->state = PA_EN_ST_AGGRESSIVE;
                    self->entityManager->aggroEnemies++;
                    self->baseSpeed += 2;
                    self->stateTimer = pa_enemySetAggroStateTimer(self);
                }
                else if (self->state == PA_EN_ST_AGGRESSIVE)
                {
                    self->state = PA_EN_ST_NORMAL;
                    self->entityManager->aggroEnemies--;
                    self->baseSpeed -= 2;
                    self->stateTimer                    = pa_enemySetAggroStateTimer(self);
                    self->gameData->leds[ledRemap[0]].r = 0xFF;
                }
            }

            if (self->state != PA_EN_ST_RUNAWAY)
            {
                self->targetTileX = PA_TO_TILECOORDS(self->entityManager->playerEntity->x >> SUBPIXEL_RESOLUTION);
                self->targetTileY = PA_TO_TILECOORDS(self->entityManager->playerEntity->y >> SUBPIXEL_RESOLUTION);

                if (self->entityManager->activeEnemies == 1 && self->gameData->remainingEnemies == 0)
                {
                    self->state                       = PA_EN_ST_RUNAWAY;
                    self->entityManager->aggroEnemies = 1;
                    self->baseSpeed += 4;
                    self->stateTimer = 480; // 8 seconds

                    self->targetTileX = (esp_random() % 2) ? 1 : 15;
                    self->targetTileY = (esp_random() % 2) ? 1 : 13;
                }
            }
            else if (!(self->gameData->frameCount % 30) && (self->baseSpeed < (self->gameData->enemyInitialSpeed << 1)))
            {
                self->baseSpeed++;
                pa_enemyChangeDirection(self, self->facingDirection, self->baseSpeed);
            }

            bool doAgression = (self->state == PA_EN_ST_AGGRESSIVE);

            pa_enemyDecideDirection(self, doAgression);

            pa_animateEnemy(self);
            despawnWhenOffscreen(self);
            if (self->state != PA_EN_ST_BREAK_BLOCK)
            {
                // Need to skip this if enemy has just changed to breaking block state
                // or else enemy will be stopped
                pa_moveEntityWithTileCollisions(self);
            }
            pa_detectEntityCollisions(self);

            break;
        }
        case PA_EN_ST_BREAK_BLOCK:
        {
            self->x += self->xspeed;
            self->y += self->yspeed;

            self->stateTimer--;
            if (self->stateTimer < 0)
            {
                self->state = PA_EN_ST_AGGRESSIVE;
                self->xspeed *= 2;
                self->yspeed *= 2;
                self->stateTimer = self->tempStateTimer;
            }

            pa_animateEnemy(self);
            pa_detectEntityCollisions(self);
            break;
        }
        default:
        {
            break;
        }
    }
}

int16_t pa_enemySetAggroStateTimer(paEntity_t* self)
{
    return (self->gameData->minAggroTime
            + esp_random() % (self->gameData->maxAggroTime - self->gameData->minAggroTime));
}

void pa_enemyDecideDirection(paEntity_t* self, bool doAgression)
{
    uint8_t tx = PA_TO_TILECOORDS(self->x >> SUBPIXEL_RESOLUTION);
    uint8_t ty = PA_TO_TILECOORDS(self->y >> SUBPIXEL_RESOLUTION);

    uint8_t t1, t2, t3 = 0;
    uint8_t distT1, distT2, distT3;

    int16_t hcof = (((self->x >> SUBPIXEL_RESOLUTION) % PA_TILE_SIZE) - PA_HALF_TILESIZE);
    int16_t vcof = (((self->y >> SUBPIXEL_RESOLUTION) % PA_TILE_SIZE) - PA_HALF_TILESIZE);

    switch (self->facingDirection)
    {
        case PA_DIRECTION_WEST:
            if (hcof)
            {
                break;
            }

            t1 = pa_getTile(self->tilemap, tx - 1, ty);
            t2 = pa_getTile(self->tilemap, tx, ty - 1);
            t3 = pa_getTile(self->tilemap, tx, ty + 1);

            distT1 = PA_GET_TAXICAB_DISTANCE(tx - 1, ty, self->targetTileX, self->targetTileY);
            distT2 = PA_GET_TAXICAB_DISTANCE(tx, ty - 1, self->targetTileX, self->targetTileY);
            distT3 = PA_GET_TAXICAB_DISTANCE(tx, ty + 1, self->targetTileX, self->targetTileY);

            if ((!t2 || doAgression) && distT2 < distT1 && (t3 || distT2 < distT3))
            {
                if (doAgression && t2 == PA_TILE_BLOCK)
                {
                    pa_enemyBreakBlock(self, PA_DIRECTION_NORTH, self->baseSpeed >> 1, tx, ty);
                }
                else
                {
                    pa_enemyChangeDirection(self, PA_DIRECTION_NORTH, self->baseSpeed);
                }
                break;
            }

            if ((!t3 || doAgression) && distT3 < distT1)
            {
                if (doAgression && t3 == PA_TILE_BLOCK)
                {
                    pa_enemyBreakBlock(self, PA_DIRECTION_SOUTH, self->baseSpeed >> 1, tx, ty);
                }
                else
                {
                    pa_enemyChangeDirection(self, PA_DIRECTION_SOUTH, self->baseSpeed);
                }
                break;
            }

            if (t1)
            {
                if (doAgression && t1 == PA_TILE_BLOCK)
                {
                    pa_enemyBreakBlock(self, PA_DIRECTION_WEST, self->baseSpeed >> 1, tx, ty);
                    break;
                }

                if ((!t2 || doAgression) && (t3 || distT2 < distT3))
                {
                    if (doAgression && t2 == PA_TILE_BLOCK)
                    {
                        pa_enemyBreakBlock(self, PA_DIRECTION_NORTH, self->baseSpeed >> 1, tx, ty);
                    }
                    else
                    {
                        pa_enemyChangeDirection(self, PA_DIRECTION_NORTH, self->baseSpeed);
                    }
                    break;
                }

                if (!t3 || doAgression)
                {
                    if (doAgression && t3 == PA_TILE_BLOCK)
                    {
                        pa_enemyBreakBlock(self, PA_DIRECTION_SOUTH, self->baseSpeed >> 1, tx, ty);
                    }
                    else
                    {
                        pa_enemyChangeDirection(self, PA_DIRECTION_SOUTH, self->baseSpeed);
                    }
                    break;
                }

                pa_enemyChangeDirection(self, PA_DIRECTION_EAST, self->baseSpeed);
                break;
            }

            break;
        case PA_DIRECTION_EAST:
            if (hcof)
            {
                break;
            }

            t1 = pa_getTile(self->tilemap, tx + 1, ty);
            t2 = pa_getTile(self->tilemap, tx, ty - 1);
            t3 = pa_getTile(self->tilemap, tx, ty + 1);

            distT1 = PA_GET_TAXICAB_DISTANCE(tx + 1, ty, self->targetTileX, self->targetTileY);
            distT2 = PA_GET_TAXICAB_DISTANCE(tx, ty - 1, self->targetTileX, self->targetTileY);
            distT3 = PA_GET_TAXICAB_DISTANCE(tx, ty + 1, self->targetTileX, self->targetTileY);

            if ((!t2 || doAgression) && distT2 < distT1 && (t3 || distT2 < distT3))
            {
                if (doAgression && t2 == PA_TILE_BLOCK)
                {
                    pa_enemyBreakBlock(self, PA_DIRECTION_NORTH, self->baseSpeed >> 1, tx, ty);
                }
                else
                {
                    pa_enemyChangeDirection(self, PA_DIRECTION_NORTH, self->baseSpeed);
                }
                break;
            }
            if ((!t3 || doAgression) && distT3 < distT1)
            {
                if (doAgression && t3 == PA_TILE_BLOCK)
                {
                    pa_enemyBreakBlock(self, PA_DIRECTION_SOUTH, self->baseSpeed >> 1, tx, ty);
                }
                else
                {
                    pa_enemyChangeDirection(self, PA_DIRECTION_SOUTH, self->baseSpeed);
                }
                break;
            }
            if (t1)
            {
                if (doAgression && t1 == PA_TILE_BLOCK)
                {
                    pa_enemyBreakBlock(self, PA_DIRECTION_EAST, self->baseSpeed >> 1, tx, ty);
                    break;
                }

                if ((!t2 || doAgression) && (t3 || distT2 < distT3))
                {
                    if (doAgression && t2 == PA_TILE_BLOCK)
                    {
                        pa_enemyBreakBlock(self, PA_DIRECTION_NORTH, self->baseSpeed >> 1, tx, ty);
                    }
                    else
                    {
                        pa_enemyChangeDirection(self, PA_DIRECTION_NORTH, self->baseSpeed);
                    }
                    break;
                }

                if (!t3 || doAgression)
                {
                    if (doAgression && t2 == PA_TILE_BLOCK)
                    {
                        pa_enemyBreakBlock(self, PA_DIRECTION_SOUTH, self->baseSpeed >> 1, tx, ty);
                    }
                    else
                    {
                        pa_enemyChangeDirection(self, PA_DIRECTION_SOUTH, self->baseSpeed);
                    }
                    break;
                }

                pa_enemyChangeDirection(self, PA_DIRECTION_WEST, self->baseSpeed);
                break;
            }

            break;
        case PA_DIRECTION_NORTH:
            if (vcof)
            {
                break;
            }

            t1 = pa_getTile(self->tilemap, tx, ty - 1);
            t2 = pa_getTile(self->tilemap, tx - 1, ty);
            t3 = pa_getTile(self->tilemap, tx + 1, ty);

            distT1 = PA_GET_TAXICAB_DISTANCE(tx, ty - 1, self->targetTileX, self->targetTileY);
            distT2 = PA_GET_TAXICAB_DISTANCE(tx - 1, ty, self->targetTileX, self->targetTileY);
            distT3 = PA_GET_TAXICAB_DISTANCE(tx + 1, ty, self->targetTileX, self->targetTileY);

            if ((!t2 || doAgression) && distT2 < distT1 && (t3 || distT2 < distT3))
            {
                if (doAgression && t2 == PA_TILE_BLOCK)
                {
                    pa_enemyBreakBlock(self, PA_DIRECTION_WEST, self->baseSpeed >> 1, tx, ty);
                }
                else
                {
                    pa_enemyChangeDirection(self, PA_DIRECTION_WEST, self->baseSpeed);
                }
                break;
            }

            if ((!t3 || doAgression) && distT3 < distT1)
            {
                if (doAgression && t3 == PA_TILE_BLOCK)
                {
                    pa_enemyBreakBlock(self, PA_DIRECTION_EAST, self->baseSpeed >> 1, tx, ty);
                }
                else
                {
                    pa_enemyChangeDirection(self, PA_DIRECTION_EAST, self->baseSpeed);
                }
                break;
            }

            if (t1)
            {
                if (doAgression && t1 == PA_TILE_BLOCK)
                {
                    pa_enemyBreakBlock(self, PA_DIRECTION_NORTH, self->baseSpeed >> 1, tx, ty);
                    break;
                }

                if ((!t2 || doAgression) && (t3 || distT2 < distT3))
                {
                    if (doAgression && t2 == PA_TILE_BLOCK)
                    {
                        pa_enemyBreakBlock(self, PA_DIRECTION_WEST, self->baseSpeed >> 1, tx, ty);
                    }
                    else
                    {
                        pa_enemyChangeDirection(self, PA_DIRECTION_WEST, self->baseSpeed);
                    }
                    break;
                }

                if (!t3 || doAgression)
                {
                    if (doAgression && t3 == PA_TILE_BLOCK)
                    {
                        pa_enemyBreakBlock(self, PA_DIRECTION_EAST, self->baseSpeed >> 1, tx, ty);
                    }
                    else
                    {
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
            if (vcof)
            {
                break;
            }

            t1 = pa_getTile(self->tilemap, tx, ty + 1);
            t2 = pa_getTile(self->tilemap, tx - 1, ty);
            t3 = pa_getTile(self->tilemap, tx + 1, ty);

            distT1 = PA_GET_TAXICAB_DISTANCE(tx, ty + 1, self->targetTileX, self->targetTileY);
            distT2 = PA_GET_TAXICAB_DISTANCE(tx - 1, ty, self->targetTileX, self->targetTileY);
            distT3 = PA_GET_TAXICAB_DISTANCE(tx + 1, ty, self->targetTileX, self->targetTileY);

            if ((!t2 || doAgression) && distT2 < distT1 && (t3 || distT2 < distT3))
            {
                if (doAgression && t2 == PA_TILE_BLOCK)
                {
                    pa_enemyBreakBlock(self, PA_DIRECTION_WEST, self->baseSpeed >> 1, tx, ty);
                }
                else
                {
                    pa_enemyChangeDirection(self, PA_DIRECTION_WEST, self->baseSpeed);
                }
                break;
            }

            if ((!t3 || doAgression) && distT3 < distT1)
            {
                if (doAgression && t3 == PA_TILE_BLOCK)
                {
                    pa_enemyBreakBlock(self, PA_DIRECTION_EAST, self->baseSpeed >> 1, tx, ty);
                }
                else
                {
                    pa_enemyChangeDirection(self, PA_DIRECTION_EAST, self->baseSpeed);
                }
                break;
            }

            if (t1)
            {
                if (doAgression && t1 == PA_TILE_BLOCK)
                {
                    pa_enemyBreakBlock(self, PA_DIRECTION_SOUTH, self->baseSpeed >> 1, tx, ty);
                    break;
                }

                if ((!t2 || doAgression) && (t3 || distT2 < distT3))
                {
                    if (doAgression && t2 == PA_TILE_BLOCK)
                    {
                        pa_enemyBreakBlock(self, PA_DIRECTION_WEST, self->baseSpeed >> 1, tx, ty);
                    }
                    else
                    {
                        pa_enemyChangeDirection(self, PA_DIRECTION_WEST, self->baseSpeed);
                    }
                    break;
                }

                if (!t3 || doAgression)
                {
                    if (doAgression && t3 == PA_TILE_BLOCK)
                    {
                        pa_enemyBreakBlock(self, PA_DIRECTION_EAST, self->baseSpeed >> 1, tx, ty);
                    }
                    else
                    {
                        pa_enemyChangeDirection(self, PA_DIRECTION_EAST, self->baseSpeed);
                    }
                    break;
                }

                pa_enemyChangeDirection(self, PA_DIRECTION_NORTH, self->baseSpeed);
                break;
            }

            break;
    }
}

void pa_enemyChangeDirection(paEntity_t* self, uint16_t newDirection, int16_t speed)
{
    switch (newDirection)
    {
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

void pa_enemyBreakBlock(paEntity_t* self, uint16_t newDirection, int16_t speed, uint8_t tx, uint8_t ty)
{
    switch (newDirection)
    {
        case PA_DIRECTION_WEST:
            pa_createBreakBlock(self->entityManager, ((tx - 1) << SUBPIXEL_RESOLUTION) + PA_HALF_TILE_SIZE,
                                (ty << SUBPIXEL_RESOLUTION) + PA_HALF_TILE_SIZE);
            break;
        case PA_DIRECTION_EAST:
            pa_createBreakBlock(self->entityManager, ((tx + 1) << SUBPIXEL_RESOLUTION) + PA_HALF_TILE_SIZE,
                                (ty << SUBPIXEL_RESOLUTION) + PA_HALF_TILE_SIZE);
            break;
        case PA_DIRECTION_NORTH:
            pa_createBreakBlock(self->entityManager, (tx << SUBPIXEL_RESOLUTION) + PA_HALF_TILE_SIZE,
                                ((ty - 1) << SUBPIXEL_RESOLUTION) + PA_HALF_TILE_SIZE);
            break;
        case PA_DIRECTION_NONE:
        default:
            break;
        case PA_DIRECTION_SOUTH:
            pa_createBreakBlock(self->entityManager, (tx << SUBPIXEL_RESOLUTION) + PA_HALF_TILE_SIZE,
                                ((ty + 1) << SUBPIXEL_RESOLUTION) + PA_HALF_TILE_SIZE);
            break;
    }

    self->state          = PA_EN_ST_BREAK_BLOCK;
    self->tempStateTimer = self->stateTimer;
    self->stateTimer     = 16;
    pa_enemyChangeDirection(self, newDirection, speed);
}

void pa_animateEnemy(paEntity_t* self)
{
    bool useAggressiveSprites = false;
    switch (self->state)
    {
        case PA_EN_ST_NORMAL:
        default:
            useAggressiveSprites = false;
            break;
        case PA_EN_ST_AGGRESSIVE:
        case PA_EN_ST_BREAK_BLOCK:
            useAggressiveSprites = true;
            break;
        case PA_EN_ST_RUNAWAY:
            useAggressiveSprites = (self->gameData->frameCount % 2);
            break;
    }

    if (self->xspeed != 0)
    {
        if ((self->xspeed < 0) || (self->xspeed > 0))
        {
            // Running
            self->spriteFlipHorizontal = (self->xspeed > 0) ? 0 : 1;

            if (self->gameData->frameCount % 5 == 0)
            {
                self->spriteIndex = PA_SP_ENEMY_SIDE_1 + ((self->spriteIndex + 1) % 2) + (useAggressiveSprites ? 4 : 0);
                self->facingDirection = self->spriteFlipHorizontal ? PA_DIRECTION_WEST : PA_DIRECTION_EAST;
            }
        }
        else
        {
            // self->spriteIndex = SP_PLAYER_SLIDE;
        }
    }
    else if (self->yspeed > 0)
    {
        if (self->gameData->frameCount % 5 == 0)
        {
            self->spriteIndex          = PA_SP_ENEMY_SOUTH + (useAggressiveSprites ? 4 : 0);
            self->spriteFlipHorizontal = (self->gameData->frameCount >> 1) % 2;
            self->facingDirection      = PA_DIRECTION_SOUTH;
        }
    }
    else if (self->yspeed < 0)
    {
        if (self->gameData->frameCount % 5 == 0)
        {
            self->spriteIndex          = PA_SP_ENEMY_NORTH + (useAggressiveSprites ? 4 : 0);
            self->spriteFlipHorizontal = (self->gameData->frameCount >> 1) % 2;
            self->facingDirection      = PA_DIRECTION_NORTH;
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

                // if (newVerticalTile > PA_TILE_UNUSED_29 && newVerticalTile < PA_TILE_BG_GOAL_ZONE)
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

                // if (newHorizontalTile > PA_TILE_UNUSED_29 && newHorizontalTile < PA_TILE_BG_GOAL_ZONE)
                {
                    if (self->tileCollisionHandler(self, newHorizontalTile, newTx, ty, (self->xspeed > 0)))
                    {
                        newX = ((newTx + ((tx < newTx) ? -1 : 1)) * PA_TILE_SIZE + PA_HALF_TILESIZE)
                               << SUBPIXEL_RESOLUTION;
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
    self->active = false;
}

void animatePlayer(paEntity_t* self)
{
    switch (self->facingDirection)
    {
        case PA_DIRECTION_NORTH:
            if ((self->gameData->btnState & PB_UP) && self->yspeed < 0)
            {
                if (self->gameData->frameCount % 7 == 0)
                {
                    self->spriteIndex          = PA_SP_PLAYER_NORTH + ((self->spriteIndex + 1) % 2);
                    self->spriteFlipHorizontal = (self->gameData->frameCount >> 1) % 2;
                }
            }
            break;
        case PA_DIRECTION_SOUTH:
            if ((self->gameData->btnState & PB_DOWN) && self->yspeed > 0)
            {
                if (self->gameData->frameCount % 7 == 0)
                {
                    self->spriteIndex          = PA_SP_PLAYER_SOUTH + ((self->spriteIndex + 1) % 2);
                    self->spriteFlipHorizontal = (self->gameData->frameCount >> 1) % 2;
                }
            }
            break;
        case PA_DIRECTION_WEST:
            if ((self->gameData->btnState & PB_LEFT) && self->xspeed < 0)
            {
                self->spriteFlipHorizontal = 1;
                if (self->gameData->frameCount % 7 == 0)
                {
                    self->spriteIndex = PA_SP_PLAYER_SIDE + ((self->spriteIndex + 1) % 3);
                }
            }
            break;
        case PA_DIRECTION_EAST:
            if ((self->gameData->btnState & PB_RIGHT) && self->xspeed > 0)
            {
                self->spriteFlipHorizontal = 0;
                if (self->gameData->frameCount % 7 == 0)
                {
                    self->spriteIndex = PA_SP_PLAYER_SIDE + ((self->spriteIndex + 1) % 3);
                }
            }
            break;
        default:
            break;
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

            if (other->state != PA_EN_ST_STUN)
            {
                if (!self->gameData->debugMode)
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
                    self->xspeed = 0;
                    self->yspeed = 0;
                    soundPlaySfx(&(self->soundManager->sndSquish), BZR_LEFT);
                }
            }

            break;
        }
        case ENTITY_HIT_BLOCK:
        {
            if (self->x < other->x)
            {
                self->x      = other->x - (PA_TILE_SIZE << SUBPIXEL_RESOLUTION);
                self->xspeed = 0;
            }
            else if (self->x > other->x)
            {
                self->x      = other->x + (PA_TILE_SIZE << SUBPIXEL_RESOLUTION);
                self->xspeed = 0;
            }
            else if (self->y < other->y)
            {
                self->y      = other->y - (PA_TILE_SIZE << SUBPIXEL_RESOLUTION);
                self->yspeed = 0;
            }
            else if (self->y > other->y)
            {
                self->y      = other->y + (PA_TILE_SIZE << SUBPIXEL_RESOLUTION);
                self->yspeed = 0;
            }
            break;
        }
        case PA_ENTITY_BONUS_ITEM:
        {
            pa_scorePoints(self->gameData, other->scoreValue);
            self->gameData->leds[ledRemap[0]].g = 0xFF;

            soundPlaySfx(&self->soundManager->sndBonus, MIDI_SFX);

            paEntity_t* createdEntity = pa_createScoreDisplay(self->entityManager, (self->x >> SUBPIXEL_RESOLUTION) - 4,
                                                              (self->y >> SUBPIXEL_RESOLUTION) - 4);
            if (createdEntity != NULL)
            {
                createdEntity->scoreValue = other->scoreValue;
            }

            pa_destroyEntity(other, false);
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
        {
            if ((self->xspeed > 0 && self->x < other->x) || (self->xspeed < 0 && self->x > other->x))
            {
                self->xspeed          = -self->xspeed;
                self->facingDirection = (self->xspeed > 0) ? PA_DIRECTION_EAST : PA_DIRECTION_WEST;
            }

            if ((self->yspeed > 0 && self->y < other->y) || (self->yspeed < 0 && self->y > other->y))
            {
                self->yspeed          = -self->yspeed;
                self->facingDirection = (self->yspeed > 0) ? PA_DIRECTION_SOUTH : PA_DIRECTION_NORTH;
            }
            break;
        }
        case ENTITY_HIT_BLOCK:
        {
            self->xspeed = other->xspeed * 2;
            self->yspeed = other->yspeed * 2;

            uint16_t pointsScored = hitBlockComboScores[(other->scoreValue < SPAWN_BLOCK_COMBO_SCORE_TABLE_LENGTH - 1)
                                                            ? other->scoreValue
                                                            : SPAWN_BLOCK_COMBO_SCORE_TABLE_LENGTH - 1];
            pa_scorePoints(self->gameData, pointsScored);
            other->scoreValue++;

            paEntity_t* createdEntity = pa_createScoreDisplay(self->entityManager, (self->x >> SUBPIXEL_RESOLUTION) - 4,
                                                              (self->y >> SUBPIXEL_RESOLUTION) - 4);
            if (createdEntity != NULL)
            {
                createdEntity->scoreValue = pointsScored;
            }

            soundPlaySfx(&(self->soundManager->sndSquish), 2);
            killEnemy(self);
            break;
        }
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

bool pa_enemyTileCollisionHandler(paEntity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction)
{
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

bool pa_dummyTileCollisionHandler(paEntity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction)
{
    return false;
}

bool pa_hitBlockTileCollisionHandler(paEntity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction)
{
    if (pa_isSolid(tileId))
    {
        soundPlaySfx(&(self->soundManager->sndBlockStop), 1);

        self->tilemap->map[PA_TO_TILECOORDS(self->y >> SUBPIXEL_RESOLUTION) * self->tilemap->mapWidth
                           + PA_TO_TILECOORDS(self->x >> SUBPIXEL_RESOLUTION)]
            = self->state;

        if (PA_TO_TILECOORDS(self->x >> SUBPIXEL_RESOLUTION) == self->homeTileX
            && PA_TO_TILECOORDS(self->y >> SUBPIXEL_RESOLUTION) == self->homeTileY)
        {
            if (self->state == PA_TILE_SPAWN_BLOCK_0)
            {
                pa_executeSpawnBlockCombo(self, self->homeTileX, self->homeTileY, 0);
            }
            else
            {
                pa_createBreakBlock(self->entityManager, self->x >> SUBPIXEL_RESOLUTION,
                                    self->y >> SUBPIXEL_RESOLUTION);
                pa_scorePoints(self->gameData, 10);
            }
        }

        pa_destroyEntity(self, false);

        return true;
    }
    return false;
}

void pa_executeSpawnBlockCombo(paEntity_t* self, uint8_t tx, uint8_t ty, uint16_t scoreIndex)
{
    uint8_t t = pa_getTile(self->tilemap, tx, ty);
    paEntity_t* newEntity;

    switch (t)
    {
        case PA_TILE_SPAWN_BLOCK_0:
            newEntity
                = pa_createBreakBlock(self->entityManager, (tx << PA_TILE_SIZE_IN_POWERS_OF_2) + PA_HALF_TILE_SIZE,
                                      (ty << PA_TILE_SIZE_IN_POWERS_OF_2) + PA_HALF_TILE_SIZE);
            if (newEntity == NULL)
            {
                pa_setTile(self->tilemap, tx, ty, PA_TILE_EMPTY);
                self->gameData->remainingBlocks--;
                pa_scorePoints(self->gameData,
                               spawnBlockComboScores[(self->scoreValue < SPAWN_BLOCK_COMBO_SCORE_TABLE_LENGTH - 1)
                                                         ? self->scoreValue
                                                         : SPAWN_BLOCK_COMBO_SCORE_TABLE_LENGTH - 1]);
                self->entityManager->gameData->remainingEnemies--;
            }
            else
            {
                newEntity->state      = PA_TILE_SPAWN_BLOCK_0;
                newEntity->scoreValue = scoreIndex;
            }
            break;
        default:
            break;
    }
}

void dieWhenFallingOffScreen(paEntity_t* self)
{
    uint16_t deathBoundary = (self->tilemap->mapOffsetY + PA_TILE_MAP_DISPLAY_HEIGHT_PIXELS + DESPAWN_THRESHOLD);
    if (((self->y >> SUBPIXEL_RESOLUTION) > deathBoundary)
        && ((self->y >> SUBPIXEL_RESOLUTION) < deathBoundary + DESPAWN_THRESHOLD))
    {
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

        if (self->spriteIndex > PA_SP_BREAK_BLOCK_3)
        {
            uint8_t tx = (self->x >> SUBPIXEL_RESOLUTION >> PA_TILE_SIZE_IN_POWERS_OF_2);
            uint8_t ty = (self->y >> SUBPIXEL_RESOLUTION >> PA_TILE_SIZE_IN_POWERS_OF_2);
            uint16_t pointsScored;

            switch (self->state)
            {
                case PA_TILE_SPAWN_BLOCK_0:
                {
                    pointsScored = spawnBlockComboScores[self->scoreValue];

                    pa_scorePoints(self->gameData, pointsScored);
                    soundPlaySfx(&(self->soundManager->sndBonus), BZR_STEREO);
                    self->gameData->leds[4].r = 0xFF;
                    self->gameData->leds[4].g = 0xFF;

                    self->entityManager->gameData->remainingEnemies--;

                    pa_executeSpawnBlockCombo(self, tx + 1, ty, self->scoreValue + 1);
                    pa_executeSpawnBlockCombo(self, tx - 1, ty, self->scoreValue + 1);
                    pa_executeSpawnBlockCombo(self, tx, ty + 1, self->scoreValue + 1);
                    pa_executeSpawnBlockCombo(self, tx, ty - 1, self->scoreValue + 1);

                    paEntity_t* createdEntity
                        = pa_createScoreDisplay(self->entityManager, (self->x >> SUBPIXEL_RESOLUTION) - 4,
                                                (self->y >> SUBPIXEL_RESOLUTION) - 4);
                    if (createdEntity != NULL)
                    {
                        createdEntity->scoreValue = pointsScored;
                    }

                    createdEntity = pa_createHotDog(self->entityManager, (self->x >> SUBPIXEL_RESOLUTION) - 4,
                                                    (self->y >> SUBPIXEL_RESOLUTION) - 4);
                    break;
                }
                default:
                {
                    pa_createBlockFragment(self->entityManager, self->x >> SUBPIXEL_RESOLUTION,
                                           self->y >> SUBPIXEL_RESOLUTION);
                    pa_createBlockFragment(self->entityManager, self->x >> SUBPIXEL_RESOLUTION,
                                           self->y >> SUBPIXEL_RESOLUTION);
                    pa_createBlockFragment(self->entityManager, self->x >> SUBPIXEL_RESOLUTION,
                                           self->y >> SUBPIXEL_RESOLUTION);
                    pa_createBlockFragment(self->entityManager, self->x >> SUBPIXEL_RESOLUTION,
                                           self->y >> SUBPIXEL_RESOLUTION);
                    break;
                }
            }

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

void pa_updateScoreDisplay(paEntity_t* self)
{
    self->stateTimer++;

    if (self->stateTimer > 120)
    {
        pa_destroyEntity(self, false);
    }
}

void pa_updateBlockFragment(paEntity_t* self)
{
    self->stateTimer--;
    if (self->stateTimer < 0)
    {
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
    if (target->state == PA_EN_ST_AGGRESSIVE)
    {
        target->entityManager->aggroEnemies--;
    }

    if (target->entityManager->activeEnemies == 0 && target->entityManager->gameData->remainingEnemies == 0)
    {
        target->gameData->changeState                       = PA_ST_LEVEL_CLEAR;
        target->entityManager->playerEntity->spriteIndex    = PA_SP_PLAYER_WIN;
        target->entityManager->playerEntity->updateFunction = &pa_updateDummy;
    }
}

void pa_playerOverlapTileHandler(paEntity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty)
{
    // Nothing to do.
}

void pa_defaultOverlapTileHandler(paEntity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty)
{
    // Nothing to do.
}

void killPlayer(paEntity_t* self)
{
    self->updateFunction        = &updateEntityDead;
    self->type                  = ENTITY_DEAD;
    self->xspeed                = 0;
    self->yspeed                = -60;
    self->spriteIndex           = PA_SP_PLAYER_HURT;
    self->gameData->changeState = PA_ST_DEAD;
    self->falling               = true;
}

void drawEntityTargetTile(paEntity_t* self)
{
    drawRect((self->targetTileX << PA_TILE_SIZE_IN_POWERS_OF_2) - self->tilemap->mapOffsetX,
             self->targetTileY << PA_TILE_SIZE_IN_POWERS_OF_2,
             (self->targetTileX << PA_TILE_SIZE_IN_POWERS_OF_2) + PA_TILE_SIZE - self->tilemap->mapOffsetX,
             (self->targetTileY << PA_TILE_SIZE_IN_POWERS_OF_2) + PA_TILE_SIZE, esp_random() % 216);
}

void pa_defaultEntityDrawHandler(paEntity_t* self)
{
    drawWsg(self->entityManager->wsgManager->sprites[self->spriteIndex].wsg,
            (self->x >> SUBPIXEL_RESOLUTION) - self->entityManager->wsgManager->sprites[self->spriteIndex].originX
                - self->entityManager->tilemap->mapOffsetX,
            (self->y >> SUBPIXEL_RESOLUTION) - self->entityManager->tilemap->mapOffsetY
                - self->entityManager->wsgManager->sprites[self->spriteIndex].originY,
            self->spriteFlipHorizontal, self->spriteFlipVertical, 0);
}

void pa_scoreDisplayDrawHandler(paEntity_t* self)
{
    char scoreStr[32];
    snprintf(scoreStr, sizeof(scoreStr) - 1, "+%" PRIu16, self->scoreValue);
    drawText(&(self->gameData->scoreFont), greenColors[(self->stateTimer >> 3) % 4], scoreStr,
             self->x >> SUBPIXEL_RESOLUTION, self->y >> SUBPIXEL_RESOLUTION);
}

void pa_updateBonusItem(paEntity_t* self)
{
    pa_enemyDecideDirection(self, false);
    despawnWhenOffscreen(self);

    switch (self->state)
    {
        case 0:
            // Ignore solid tiles if the bonus item spawned inside one
            if (!pa_isSolid(pa_getTile(self->tilemap, PA_TO_TILECOORDS(self->x >> SUBPIXEL_RESOLUTION),
                                       PA_TO_TILECOORDS(self->y >> SUBPIXEL_RESOLUTION))))
            {
                self->state = 1;
            }
            self->x += self->xspeed;
            self->y += self->yspeed;
            break;
        case 1:
        default:
            pa_moveEntityWithTileCollisions(self);
            break;
    }

    pa_detectEntityCollisions(self);
}

uint16_t pa_getBonusItemValue(int16_t elapsedTime)
{
    switch (elapsedTime)
    {
        case 0 ... 19:
            return 5000;
        case 20 ... 29:
            return 2000;
        case 30 ... 39:
            return 1000;
        case 40 ... 49:
            return 500;
        default:
            return 100;
    }
}
