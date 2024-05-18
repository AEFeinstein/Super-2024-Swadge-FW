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
    self->facingDirection = PA_DIRECTION_DOWN;

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
/*
    if (self->gameData->btnState & PB_A)
    {
        if (!self->falling && !(self->gameData->prevBtnState & PB_A))
        {
            // initiate jump
            self->jumpPower = 64 + ((abs(self->xspeed) + 16) >> 3);
            self->yspeed    = -self->jumpPower;
            self->falling   = true;
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

    if (self->invincibilityFrames > 0)
    {
        self->invincibilityFrames--;
        if (self->invincibilityFrames % 2)
        {
            self->visible = !self->visible;
        }

        if (self->invincibilityFrames <= 0)
        {
            self->visible = true;
        }
    }
*/
    if (self->animationTimer > 0)
    {
        self->animationTimer--;
    }

/*
    if (self->hp > 2 && self->gameData->btnState & PB_B && !(self->gameData->prevBtnState & PB_B)
        && self->animationTimer == 0)
    {
        paEntity_t* createdEntity = pa_createEntity(self->entityManager, ENTITY_WAVE_BALL,
                                                    self->x >> SUBPIXEL_RESOLUTION, self->y >> SUBPIXEL_RESOLUTION);
        if (createdEntity != NULL)
        {
            createdEntity->xspeed    = (self->spriteFlipHorizontal) ? -(128 + abs(self->xspeed) + abs(self->yspeed))
                                                                    : 128 + abs(self->xspeed) + abs(self->yspeed);
            createdEntity->homeTileX = 0;
            createdEntity->homeTileY = 0;
            soundPlaySfx(&(self->soundManager->sndWaveBall), BZR_LEFT);
        }
        self->animationTimer = 30;
    }
*/
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
        case PA_DIRECTION_LEFT:
            self->targetTileX = PA_TO_TILECOORDS(self->x >> SUBPIXEL_RESOLUTION) - 1;
            self->targetTileY = PA_TO_TILECOORDS(self->y >> SUBPIXEL_RESOLUTION);
            break;
        case PA_DIRECTION_RIGHT:
            self->targetTileX = PA_TO_TILECOORDS(self->x >> SUBPIXEL_RESOLUTION) + 1;
            self->targetTileY = PA_TO_TILECOORDS(self->y >> SUBPIXEL_RESOLUTION);
            break;
        case PA_DIRECTION_UP:
            self->targetTileX = PA_TO_TILECOORDS(self->x >> SUBPIXEL_RESOLUTION);
            self->targetTileY = PA_TO_TILECOORDS(self->y >> SUBPIXEL_RESOLUTION) - 1;
            break;
        case PA_DIRECTION_DOWN:
        default:
            self->targetTileX = PA_TO_TILECOORDS(self->x >> SUBPIXEL_RESOLUTION);
            self->targetTileY = PA_TO_TILECOORDS(self->y >> SUBPIXEL_RESOLUTION) + 1;
            break;
    }

    if (self->gameData->btnState & PB_A)
    {
        if(pa_getTile(self->tilemap, self->targetTileX, self->targetTileY) == PA_TILE_BLOCK){
            paEntity_t* newHitBlock = createHitBlock(self->entityManager, (self->targetTileX << SUBPIXEL_RESOLUTION) + PA_HALF_TILESIZE, (self->targetTileY << SUBPIXEL_RESOLUTION) + PA_HALF_TILESIZE);
            
            if(newHitBlock != NULL){
                pa_setTile(self->tilemap, self->targetTileX, self->targetTileY, PA_TILE_EMPTY);
                switch(self->facingDirection){
                    case PA_DIRECTION_LEFT:
                        newHitBlock->xspeed = -64;
                        break;
                    case PA_DIRECTION_RIGHT:
                        newHitBlock->xspeed = 64;
                        break;
                    case PA_DIRECTION_UP:
                        newHitBlock->yspeed = -64;
                        break;
                    case PA_DIRECTION_DOWN:
                    default:
                        newHitBlock->yspeed = 64;
                        break;
                }
                soundPlaySfx(&(self->soundManager->sndSquish), BZR_LEFT);
            }
        }
    }
    animatePlayer(self);
    pa_moveEntityWithTileCollisions(self);
    //dieWhenFallingOffScreen(self);
    //applyGravity(self);
    applyDamping(self);
    pa_detectEntityCollisions(self);
    
}

void updateTestObject(paEntity_t* self)
{
    if (self->gameData->frameCount % 10 == 0)
    {
        self->spriteFlipHorizontal = !self->spriteFlipHorizontal;
    }

    despawnWhenOffscreen(self);
    pa_moveEntityWithTileCollisions(self);
    applyGravity(self);
    pa_detectEntityCollisions(self);
}

void updateHitBlock(paEntity_t* self)
{
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
    if (respawn && !(self->homeTileX == 0 && self->homeTileY == 0))
    {
        self->tilemap->map[self->homeTileY * self->tilemap->mapWidth + self->homeTileX] = self->type + 128;
    }

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

            if (self->gameData->frameCount % 7 == 0)
            {
                self->spriteIndex = PA_SP_PLAYER_SIDE + ((self->spriteIndex + 1) % 3);
                self->facingDirection = !self->spriteFlipHorizontal;
            }
        }
        else
        {
            //self->spriteIndex = SP_PLAYER_SLIDE;
        }
    }
    else if (self->yspeed > 0){
        if ((self->gameData->btnState & PB_DOWN) && self->yspeed > 0){
            if (self->gameData->frameCount % 7 == 0)
            {
                self->spriteIndex = PA_SP_PLAYER_SOUTH + ((self->spriteIndex + 1) % 2);
                self->spriteFlipHorizontal = (self->gameData->frameCount >> 1) % 2;
                self->facingDirection = PA_DIRECTION_DOWN;
            }
        }
    }
     else if (self->yspeed < 0){
        if ((self->gameData->btnState & PB_UP) && self->yspeed < 0){
            if (self->gameData->frameCount % 7 == 0)
            {
                self->spriteIndex = PA_SP_PLAYER_NORTH + ((self->spriteIndex + 1) % 2);
                self->spriteFlipHorizontal = (self->gameData->frameCount >> 1) % 2;
                self->facingDirection = PA_DIRECTION_UP;
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
        case paEntity_tEST:
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
                pa_scorePoints(self->gameData, other->scoreValue);

                killEnemy(other);
                soundPlaySfx(&(self->soundManager->sndSquish), BZR_LEFT);

                self->yspeed    = -180;
                self->jumpPower = 64 + ((abs(self->xspeed) + 16) >> 3);
                self->falling   = true;
            }
            else if (self->invincibilityFrames <= 0)
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
        case ENTITY_WARP:
        {
            // Execute warp
            self->x = (self->tilemap->warps[other->jumpPower].x * PA_TILE_SIZE + PA_HALF_TILESIZE)
                      << SUBPIXEL_RESOLUTION;
            self->y = (self->tilemap->warps[other->jumpPower].y * PA_TILE_SIZE + PA_HALF_TILESIZE)
                      << SUBPIXEL_RESOLUTION;
            self->falling = true;
            pa_viewFollowEntity(self->tilemap, self->entityManager->playerEntity);

            pa_unlockScrolling(self->tilemap);
            pa_deactivateAllEntities(self->entityManager, true);
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
            pa_scorePoints(self->gameData, 1000);
            soundPlaySfx(&(self->soundManager->sndPowerUp), BZR_LEFT);
            pa_updateLedsHpMeter(self->entityManager, self->gameData);
            pa_destroyEntity(other, false);
            break;
        }
        case ENTITY_1UP:
        {
            self->gameData->lives++;
            pa_scorePoints(self->gameData, 0);
            soundPlaySfx(&(self->soundManager->snd1up), BZR_LEFT);
            pa_destroyEntity(other, false);
            break;
        }
        case ENTITY_CHECKPOINT:
        {
            if (!other->xDamping)
            {
                // Get tile above checkpoint
                uint8_t aboveTile
                    = self->tilemap->map[(other->homeTileY - 1) * self->tilemap->mapWidth + other->homeTileX];

                /*if (aboveTile >= PA_TILE_WARP_0 && aboveTile <= PA_TILE_WARP_F)
                {
                    self->gameData->checkpoint = aboveTile - PA_TILE_WARP_0;
                    other->xDamping            = 1;
                    soundPlaySfx(&(self->soundManager->sndCheckpoint), BZR_LEFT);
                }*/
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
        case paEntity_tEST:
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
            pa_scorePoints(self->gameData, self->scoreValue);
            soundPlaySfx(&(self->soundManager->sndSquish), BZR_LEFT);
            killEnemy(self);
            break;
        case ENTITY_WAVE_BALL:
            self->xspeed = other->xspeed >> 1;
            self->yspeed = -abs(other->xspeed >> 1);
            pa_scorePoints(self->gameData, self->scoreValue);
            soundPlaySfx(&(self->soundManager->sndBreak), BZR_LEFT);
            killEnemy(self);
            pa_destroyEntity(other, false);
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
    switch (tileId)
    {
        case PA_TILE_CONTAINER_1:
        case PA_TILE_BRICK_BLOCK:
        case PA_TILE_INVISIBLE_CONTAINER:
        case PA_TILE_BOUNCE_BLOCK:
        {
            paEntity_t* hitBlock
                = pa_createEntity(self->entityManager, ENTITY_HIT_BLOCK, (tx * PA_TILE_SIZE) + PA_HALF_TILESIZE,
                                  (ty * PA_TILE_SIZE) + PA_HALF_TILESIZE);

            if (hitBlock != NULL)
            {
                pa_setTile(self->tilemap, tx, ty, PA_TILE_INVISIBLE_BLOCK);
                hitBlock->homeTileX = tx;
                hitBlock->homeTileY = ty;
                hitBlock->jumpPower = tileId;
                if (tileId == PA_TILE_BRICK_BLOCK)
                {
                    hitBlock->spriteIndex = SP_HITBLOCK_BRICKS;
                    if (abs(self->xspeed) > 51 && self->yspeed <= 0)
                    {
                        hitBlock->yDamping = 1;
                    }
                }

                if (tileId == PA_TILE_BOUNCE_BLOCK)
                {
                    hitBlock->spriteIndex = SP_BOUNCE_BLOCK;
                }

                switch (direction)
                {
                    case 0:
                        hitBlock->xspeed = -24;
                        if (tileId == PA_TILE_BOUNCE_BLOCK)
                        {
                            self->xspeed = 48;
                        }
                        break;
                    case 1:
                        hitBlock->xspeed = 24;
                        if (tileId == PA_TILE_BOUNCE_BLOCK)
                        {
                            self->xspeed = -48;
                        }
                        break;
                    case 2:
                        hitBlock->yspeed = -48;
                        if (tileId == PA_TILE_BOUNCE_BLOCK)
                        {
                            self->yspeed = 48;
                        }
                        break;
                    case 4:
                        hitBlock->yspeed = (tileId == PA_TILE_BRICK_BLOCK) ? 16 : 24;
                        if (tileId == PA_TILE_BOUNCE_BLOCK)
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
        case PA_TILE_GOAL_100PTS:
        {
            if (direction == 4)
            {
                pa_scorePoints(self->gameData, 100);
                soundStop(true);
                soundPlaySfx(&(self->soundManager->sndLevelClearD), BZR_LEFT);
                //self->spriteIndex           = SP_PLAYER_WIN;
                self->updateFunction        = &pa_updateDummy;
                self->gameData->changeState = PA_ST_LEVEL_CLEAR;
            }
            break;
        }
        case PA_TILE_GOAL_500PTS:
        {
            if (direction == 4)
            {
                pa_scorePoints(self->gameData, 500);
                soundStop(true);
                soundPlaySfx(&(self->soundManager->sndLevelClearC), BZR_LEFT);
                //self->spriteIndex           = SP_PLAYER_WIN;
                self->updateFunction        = &pa_updateDummy;
                self->gameData->changeState = PA_ST_LEVEL_CLEAR;
            }
            break;
        }
        case PA_TILE_GOAL_1000PTS:
        {
            if (direction == 4)
            {
                pa_scorePoints(self->gameData, 1000);
                soundStop(true);
                soundPlaySfx(&(self->soundManager->sndLevelClearB), BZR_LEFT);
                //self->spriteIndex           = SP_PLAYER_WIN;
                self->updateFunction        = &pa_updateDummy;
                self->gameData->changeState = PA_ST_LEVEL_CLEAR;
            }
            break;
        }
        case PA_TILE_GOAL_2000PTS:
        {
            if (direction == 4)
            {
                pa_scorePoints(self->gameData, 2000);
                soundStop(true);
                soundPlaySfx(&(self->soundManager->sndLevelClearA), BZR_LEFT);
                //self->spriteIndex           = SP_PLAYER_WIN;
                self->updateFunction        = &pa_updateDummy;
                self->gameData->changeState = PA_ST_LEVEL_CLEAR;
            }
            break;
        }
        case PA_TILE_GOAL_5000PTS:
        {
            if (direction == 4)
            {
                pa_scorePoints(self->gameData, 5000);
                soundStop(true);
                soundPlaySfx(&(self->soundManager->sndLevelClearS), BZR_LEFT);
                //self->spriteIndex           = SP_PLAYER_WIN;
                self->updateFunction        = &pa_updateDummy;
                self->gameData->changeState = PA_ST_LEVEL_CLEAR;
            }
            break;
        }
        /*case PA_TILE_COIN_1 ... PA_TILE_COIN_3:
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
        }*/
        default:
        {
            break;
        }
    }

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
    switch (tileId)
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
    }

    if (pa_isSolid(tileId))
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

bool pa_dummyTileCollisionHandler(paEntity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction)
{
    return false;
}

bool pa_hitBlockTileCollisionHandler(paEntity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction)
{
    if (pa_isSolid(tileId))
    {
        self->tilemap->map[PA_TO_TILECOORDS(self->y >> SUBPIXEL_RESOLUTION) * self->tilemap->mapWidth + PA_TO_TILECOORDS(self->x >> SUBPIXEL_RESOLUTION)] = PA_TILE_BLOCK;
        soundPlaySfx(&(self->soundManager->sndHit), BZR_LEFT);
        pa_destroyEntity(self, false);
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

void updateScrollLockLeft(paEntity_t* self)
{
    self->tilemap->minMapOffsetX = (self->x >> SUBPIXEL_RESOLUTION) - 8;
    pa_viewFollowEntity(self->entityManager->tilemap, self->entityManager->viewEntity);
    pa_destroyEntity(self, true);
}

void updateScrollLockRight(paEntity_t* self)
{
    self->tilemap->maxMapOffsetX = (self->x >> SUBPIXEL_RESOLUTION) + 8 - PA_TILE_MAP_DISPLAY_WIDTH_PIXELS;
    pa_viewFollowEntity(self->entityManager->tilemap, self->entityManager->viewEntity);
    pa_destroyEntity(self, true);
}

void updateScrollLockUp(paEntity_t* self)
{
    self->tilemap->minMapOffsetY = (self->y >> SUBPIXEL_RESOLUTION) - 8;
    pa_viewFollowEntity(self->entityManager->tilemap, self->entityManager->viewEntity);
    pa_destroyEntity(self, true);
}

void updateScrollLockDown(paEntity_t* self)
{
    self->tilemap->maxMapOffsetY = (self->y >> SUBPIXEL_RESOLUTION) + 8 - PA_TILE_MAP_DISPLAY_HEIGHT_PIXELS;
    pa_viewFollowEntity(self->entityManager->tilemap, self->entityManager->viewEntity);
    pa_destroyEntity(self, true);
}

void updateScrollUnlock(paEntity_t* self)
{
    pa_unlockScrolling(self->tilemap);
    pa_viewFollowEntity(self->entityManager->tilemap, self->entityManager->viewEntity);
    pa_destroyEntity(self, true);
}

void updateEntityDead(paEntity_t* self)
{
    applyGravity(self);
    self->x += self->xspeed;
    self->y += self->yspeed;

    despawnWhenOffscreen(self);
}

void updatePowerUp(paEntity_t* self)
{
    if (self->gameData->frameCount % 10 == 0)
    {
        self->spriteIndex
            = ((self->entityManager->playerEntity->hp < 2) ? SP_GAMING_1 : SP_MUSIC_1) + ((self->spriteIndex + 1) % 3);
    }

    pa_moveEntityWithTileCollisions(self);
    applyGravity(self);
    despawnWhenOffscreen(self);
}

void update1up(paEntity_t* self)
{
    if (self->gameData->frameCount % 10 == 0)
    {
        self->spriteIndex = SP_1UP_1 + ((self->spriteIndex + 1) % 3);
    }

    pa_moveEntityWithTileCollisions(self);
    applyGravity(self);
    despawnWhenOffscreen(self);
}

void updateWarp(paEntity_t* self)
{
    if (self->gameData->frameCount % 10 == 0)
    {
        self->spriteIndex = SP_WARP_1 + ((self->spriteIndex + 1) % 3);
    }

    // Destroy self and respawn warp container block when offscreen
    if ((self->x >> SUBPIXEL_RESOLUTION) < (self->tilemap->mapOffsetX - DESPAWN_THRESHOLD)
        || (self->x >> SUBPIXEL_RESOLUTION)
               > (self->tilemap->mapOffsetX + PA_TILE_MAP_DISPLAY_WIDTH_PIXELS + DESPAWN_THRESHOLD)
        || (self->y >> SUBPIXEL_RESOLUTION) < (self->tilemap->mapOffsetY - DESPAWN_THRESHOLD)
        || (self->y >> SUBPIXEL_RESOLUTION)
               > (self->tilemap->mapOffsetY + PA_TILE_MAP_DISPLAY_HEIGHT_PIXELS + DESPAWN_THRESHOLD))
    {
        // In pa_destroyEntity, this will overflow to the correct value.
        self->type = 128 + PA_TILE_CONTAINER_1;

        pa_destroyEntity(self, true);
    }
}

void updateDustBunny(paEntity_t* self)
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
                    self->spriteIndex          = SP_DUSTBUNNY_JUMP;
                    self->spriteFlipHorizontal = directionToPlayer;
                    break;
                }
                case 1:
                {
                    self->xDamping             = 0;
                    self->yDamping             = 30;
                    self->spriteIndex          = SP_DUSTBUNNY_CHARGE;
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
    pa_moveEntityWithTileCollisions(self);
    applyGravity(self);
    pa_detectEntityCollisions(self);
}

void updateDustBunnyL2(paEntity_t* self)
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
                    self->spriteIndex = SP_DUSTBUNNY_L2_JUMP;
                    break;
                }
                case 1:
                {
                    self->xDamping    = 0;
                    self->yDamping    = 15;
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
    pa_moveEntityWithTileCollisions(self);
    applyGravity(self);
    pa_detectEntityCollisions(self);
}

void updateDustBunnyL3(paEntity_t* self)
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
                    self->spriteIndex          = SP_DUSTBUNNY_L3_JUMP;
                    self->spriteFlipHorizontal = directionToPlayer;
                    break;
                }
                case 1:
                {
                    self->xDamping             = 0;
                    self->yDamping             = 30;
                    self->spriteIndex          = SP_DUSTBUNNY_L3_CHARGE;
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
    pa_moveEntityWithTileCollisions(self);
    applyGravity(self);
    pa_detectEntityCollisions(self);
}

bool dustBunnyTileCollisionHandler(paEntity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction)
{
    switch (tileId)
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

    if (pa_isSolid(tileId))
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
                self->spriteIndex = SP_DUSTBUNNY_IDLE;
                break;
            default: // Should never hit
                return false;
        }
        // trigger tile collision resolution
        return true;
    }

    return false;
}

bool dustBunnyL2TileCollisionHandler(paEntity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction)
{
    switch (tileId)
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

    if (pa_isSolid(tileId))
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
                self->spriteIndex = SP_DUSTBUNNY_L2_IDLE;
                break;
            default: // Should never hit
                return false;
        }
        // trigger tile collision resolution
        return true;
    }

    return false;
}

bool dustBunnyL3TileCollisionHandler(paEntity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction)
{
    switch (tileId)
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

    if (pa_isSolid(tileId))
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
                self->spriteIndex = SP_DUSTBUNNY_L3_IDLE;
                break;
            default: // Should never hit
                return false;
        }
        // trigger tile collision resolution
        return true;
    }

    return false;
}

void updateWasp(paEntity_t* self)
{
    switch (self->xDamping)
    {
        case 0:
            if (self->gameData->frameCount % 5 == 0)
            {
                self->spriteIndex = SP_WASP_1 + ((self->spriteIndex + 1) % 2);
            }
            self->yDamping--;

            if (self->entityManager->playerEntity->y > self->y && self->yDamping < 0
                && abs(self->x - self->entityManager->playerEntity->x) < 512)
            {
                self->xDamping       = 1;
                self->gravityEnabled = true;
                self->falling        = true;
                self->spriteIndex    = SP_WASP_DIVE;
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
                self->spriteIndex = SP_WASP_1 + ((self->spriteIndex + 1) % 2);
            }

            self->yDamping--;
            if (self->yDamping < 0 || self->y <= ((self->homeTileY * PA_TILE_SIZE + 8) << SUBPIXEL_RESOLUTION))
            {
                self->xDamping = 0;
                self->xspeed   = (self->spriteFlipHorizontal) ? -16 : 16;
                self->yspeed   = 0;
                self->yDamping = (1 + esp_random() % 2) * 20;
            }
        default:
            break;
    }

    despawnWhenOffscreen(self);
    pa_moveEntityWithTileCollisions(self);
    applyGravity(self);
    pa_detectEntityCollisions(self);
}

void updateWaspL2(paEntity_t* self)
{
    switch (self->xDamping)
    {
        case 0:
            if (self->gameData->frameCount % 5 == 0)
            {
                self->spriteIndex = SP_WASP_L2_1 + ((self->spriteIndex) % 2);
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
                self->spriteIndex    = SP_WASP_L2_DIVE;
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
                self->spriteIndex = SP_WASP_L2_1 + ((self->spriteIndex) % 2);
            }

            self->yDamping--;
            if (self->yDamping < 0 || self->y <= ((self->homeTileY * PA_TILE_SIZE + 8) << SUBPIXEL_RESOLUTION))
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
    pa_moveEntityWithTileCollisions(self);
    applyGravity(self);
    pa_detectEntityCollisions(self);
}

void updateWaspL3(paEntity_t* self)
{
    switch (self->xDamping)
    {
        case 0:
            if (self->gameData->frameCount % 5 == 0)
            {
                self->spriteIndex = SP_WASP_L3_1 + ((self->spriteIndex + 1) % 2);
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
                self->spriteIndex    = SP_WASP_L3_DIVE;
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
                self->spriteIndex = SP_WASP_L3_1 + ((self->spriteIndex + 1) % 2);
            }

            self->yDamping--;
            if (self->yDamping < 0 || self->y <= ((self->homeTileY * PA_TILE_SIZE + 8) << SUBPIXEL_RESOLUTION))
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

    despawnWhenOffscreen(self);
    pa_moveEntityWithTileCollisions(self);
    applyGravity(self);
    pa_detectEntityCollisions(self);
}

bool waspTileCollisionHandler(paEntity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction)
{
    switch (tileId)
    {
        case PA_TILE_BOUNCE_BLOCK:
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

    if (pa_isSolid(tileId))
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

void killEnemy(paEntity_t* target)
{
    target->homeTileX          = 0;
    target->homeTileY          = 0;
    target->gravityEnabled     = true;
    target->falling            = true;
    target->type               = ENTITY_DEAD;
    target->spriteFlipVertical = true;
    target->updateFunction     = &updateEntityDead;
}

void updateBgCol(paEntity_t* self)
{
    self->gameData->bgColor = self->xDamping;
    pa_destroyEntity(self, true);
}

void turnAroundAtEdgeOfTileHandler(paEntity_t* self)
{
    self->falling = true;
    self->xspeed  = -self->xspeed;
    self->yspeed  = -self->gravity * 4;
}

void updateEnemyBushL3(paEntity_t* self)
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

    despawnWhenOffscreen(self);
    pa_moveEntityWithTileCollisions(self);
    applyGravity(self);
    pa_detectEntityCollisions(self);
}

void updateCheckpoint(paEntity_t* self)
{
    if (self->xDamping)
    {
        if (self->gameData->frameCount % 15 == 0)
        {
            self->spriteIndex = SP_CHECKPOINT_ACTIVE_1 + ((self->spriteIndex + 1) % 2);
        }
    }

    despawnWhenOffscreen(self);
}

void pa_playerOverlapTileHandler(paEntity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty)
{
    switch (tileId)
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
    }
}

void pa_defaultOverlapTileHandler(paEntity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty)
{
    // Nothing to do.
}

void updateBgmChange(paEntity_t* self)
{
    self->gameData->changeBgm = self->xDamping;
    pa_destroyEntity(self, true);
}

void updateWaveBall(paEntity_t* self)
{
    if (self->gameData->frameCount % 4 == 0)
    {
        self->spriteIndex = (SP_WAVEBALL_1 + ((self->spriteIndex + 1) % 3));
    }

    if (self->gameData->frameCount % 4 == 0)
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
    }

    // self->yDamping++;

    pa_moveEntityWithTileCollisions(self);
    despawnWhenOffscreen(self);
}

// bool waveBallTileCollisionHandler(paEntity_t *self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction){
//     if(self->yspeed == 0){
//         pa_destroyEntity(self, false);
//     }
//     return false;
// }

void waveBallOverlapTileHandler(paEntity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty)
{
    if (pa_isSolid(tileId) || tileId == PA_TILE_BOUNCE_BLOCK)
    {
        pa_destroyEntity(self, false);
        soundPlaySfx(&(self->soundManager->sndHit), BZR_LEFT);
    }
}

void powerUpCollisionHandler(paEntity_t* self, paEntity_t* other)
{
    switch (other->type)
    {
        case paEntity_tEST:
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