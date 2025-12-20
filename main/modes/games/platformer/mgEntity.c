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
#include "vector2d.h"
#include "cutscene.h"
#include "mgCutscenes.h"

//==============================================================================
// Constants
//==============================================================================

static const vec_t mg_sureYouCanVectors[] = {
    // These are in reverse order
    {.x = 8, .y = 4},     {.x = 8, .y = -2},   {.x = 8, .y = -4},   {.x = 8, .y = -8},
    {.x = 16, .y = -128}, {.x = 16, .y = -64}, {.x = 32, .y = -16}, {.x = 16, .y = -8}};

const mg_spriteDef_t playerDamageAnimFrames[]
    = {MG_SP_PLAYER_HURT, MG_SP_PLAYER_HURT_2, MG_SP_PLAYER_HURT, MG_SP_PLAYER_HURT_3};
const mg_spriteDef_t playerMicDropAnimFrames[]     = {MG_SP_PLAYER_MIC_DROP_1, MG_SP_PLAYER_MIC_DROP_2};
const mg_spriteDef_t playerSureYouCanAnimnFrames[] = {
    // These are in reverse order
    MG_SP_PLAYER_SUREYOUCAN_2, MG_SP_PLAYER_SUREYOUCAN_2, MG_SP_PLAYER_SUREYOUCAN_2, MG_SP_PLAYER_SUREYOUCAN_1,
    MG_SP_PLAYER_SUREYOUCAN_1, MG_SP_PLAYER_JUMP,         MG_WSG_PLAYER_WALK5,       MG_WSG_PLAYER_WALK6};

const mg_spriteDef_t playerDoubleJumpAnimFrames[]
    = {MG_SP_PLAYER_DOUBLE_JUMP_0, MG_SP_PLAYER_DOUBLE_JUMP_1, MG_SP_PLAYER_DOUBLE_JUMP_2,
       MG_SP_PLAYER_DOUBLE_JUMP_3, MG_SP_PLAYER_DOUBLE_JUMP_4, MG_SP_PLAYER_DOUBLE_JUMP_5};

const mg_spriteDef_t normalShotAnimFrames[] = {MG_SP_WAVEBALL_1, MG_SP_WAVEBALL_2, MG_SP_WAVEBALL_3};
const mg_spriteDef_t chargeShotAnimFrames[]
    = {MG_SP_CHARGE_SHOT_LVL1_1, MG_SP_CHARGE_SHOT_LVL1_2, MG_SP_CHARGE_SHOT_LVL1_3};
const mg_spriteDef_t maxChargeShotAnimFrames[]
    = {MG_SP_CHARGE_SHOT_MAX_1, MG_SP_CHARGE_SHOT_MAX_2, MG_SP_CHARGE_SHOT_MAX_3};

const mg_spriteDef_t grindPangolinRollingFrames[] = {MG_SP_BOSS_3, MG_SP_BOSS_4};

const mg_spriteDef_t kineticDonutIdleFrames[] = {MG_SP_BOSS_0, MG_SP_BOSS_1, MG_SP_BOSS_2};

const mg_spriteDef_t kineticDonutChargeFrames[] = {MG_SP_BOSS_3, MG_SP_BOSS_4};

const mg_spriteDef_t kineticDonutTeleportFrames[] = {MG_SP_BOSS_5, MG_SP_BOSS_6};

const mg_spriteDef_t severYatagaFlyingFrames[] = {MG_SP_BOSS_0, MG_SP_BOSS_1, MG_SP_BOSS_2, MG_SP_BOSS_1};

const mg_spriteDef_t drainBatAnimFrames[] = {MG_SP_BOSS_0, MG_SP_BOSS_1, MG_SP_BOSS_2, MG_SP_BOSS_3, MG_SP_BOSS_4};

//==============================================================================
// Functions Prototypes
//==============================================================================

void startOutroCutscene(mgEntity_t* self);

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
    self->doubleJumpAnimTimer = 0;
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
    if (self->gameData->level == 1 && !self->gameData->kineticSkipped && self->x > 59770 && self->y < 15400)
    {
        self->gameData->kineticSkipped = true;
        bossIntroCutscene(self->gameData);
    }
    switch (self->state)
    {
        case MG_PL_ST_NORMAL:
        default:
            if (self->gameData->doubleTapBtnTimer > 0)
            {
                self->gameData->doubleTapBtnTimer--;

                if (self->gameData->doubleTapBtnTimer <= 0)
                {
                    self->gameData->doubleTapBtnState = 0;
                }
            }

            if (self->gameData->btnState & PB_LEFT)
            {
                self->xspeed -= (self->falling && self->xspeed < 0) ? (self->xspeed < -24) ? 0 : 8 : 8;

                if (!self->falling && self->xspeed < -self->xMaxSpeed)
                {
                    self->xspeed += 8;
                }
            }
            else if (self->gameData->btnState & PB_RIGHT)
            {
                self->xspeed += (self->falling && self->xspeed > 0) ? (self->xspeed > 24) ? 0 : 8 : 8;

                if (!self->falling && self->xspeed > self->xMaxSpeed)
                {
                    self->xspeed -= 8;
                }
            }

            if ((self->gameData->btnState & PB_DOWN) && (self->gameData->btnState & PB_B)
                && !(self->gameData->prevBtnState & PB_B)
                && (self->gameData->abilities & (1U << MG_REFLECTOR_SHIELD_ABILITY)))
            {
                self->state = MG_PL_ST_SHIELD;
                if (self->yspeed > 0)
                {
                    self->yspeed = -16;
                }
                self->jumpPower  = -1;
                self->stateTimer = 60;
            }

            break;
        case MG_PL_ST_DASHING:

            if (self->spriteFlipHorizontal)
            {
                self->xspeed = -64;

                if ((self->gameData->btnState & PB_RIGHT) && !(self->gameData->prevBtnState & PB_RIGHT))
                {
                    self->spriteFlipHorizontal = false;
                    self->xspeed               = 64;
                    soundPlaySfx(&(self->soundManager->sndJump3), BZR_LEFT);
                }
            }
            else
            {
                self->xspeed = 64;

                if ((self->gameData->btnState & PB_LEFT) && !(self->gameData->prevBtnState & PB_LEFT))
                {
                    self->spriteFlipHorizontal = true;
                    self->xspeed               = -64;
                    soundPlaySfx(&(self->soundManager->sndJump3), BZR_LEFT);
                }
            }

            // Ugh... this is repeated here unfortunately
            if ((self->gameData->btnState & PB_DOWN) && (self->gameData->btnState & PB_B)
                && !(self->gameData->prevBtnState & PB_B)
                && (self->gameData->abilities & (1U << MG_REFLECTOR_SHIELD_ABILITY)))
            {
                self->state = MG_PL_ST_SHIELD;
                if (self->yspeed > 0)
                {
                    self->yspeed = -16;
                }
                self->jumpPower  = -1;
                self->stateTimer = 60;
            }

            self->stateTimer--;
            if (self->stateTimer <= 0)
            {
                if (mg_canExitDashSlide(self))
                {
                    self->state        = MG_PL_ST_NORMAL;
                    self->tileCollider = &entityTileCollider_1x2;
                    self->gravity      = 4;
                }
                else
                {
                    self->stateTimer = 1;
                }
            }
            break;
        case MG_PL_ST_SHIELD:
            if (((self->gameData->btnState & PB_LEFT) && !(self->gameData->prevBtnState & PB_LEFT))
                || ((self->gameData->btnState & PB_RIGHT) && !(self->gameData->prevBtnState & PB_RIGHT))
                || ((self->gameData->btnState & PB_A) && !(self->gameData->prevBtnState & PB_A)))
            {
                self->state = MG_PL_ST_NORMAL;
                break;
            }

            self->stateTimer--;
            if (self->stateTimer <= 0)
            {
                self->state = MG_PL_ST_NORMAL;
            }
            break;
        case MG_PL_ST_MIC_DROP:
            if (self->yspeed > 0)
            {
                self->yspeed += 8;
            }
            // fallthrough
        case MG_PL_ST_HURT:
            self->stateTimer--;
            if (self->stateTimer <= 0)
            {
                self->state             = MG_PL_ST_NORMAL;
                self->spriteRotateAngle = 0;
                self->yMaxSpeed         = 72;
                // self->spriteFlipVertical = false;
            }
            break;
        case MG_PL_ST_UPPERCUT:
            int16_t stupidVectorThing = CLAMP((self->stateTimer >> 2), 0, 7);
            self->xspeed = mg_sureYouCanVectors[stupidVectorThing].x * (self->spriteFlipHorizontal ? -1 : 1);
            self->yspeed = mg_sureYouCanVectors[stupidVectorThing].y;
            self->stateTimer--;
            if (self->stateTimer <= 0)
            {
                self->state = MG_PL_ST_NORMAL;
            }
            break;
    }

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
        if (!(self->gameData->prevBtnState & PB_A))
        {
            if (!self->falling)
            {
                if ((self->gameData->btnState & PB_DOWN)
                    && (self->gameData->abilities & (1U << MG_TROMBONE_SLIDE_ABILITY)))
                {
                    // initiate dash slide
                    self->state        = MG_PL_ST_DASHING;
                    self->tileCollider = &entityTileCollider_1x2_dash_slide;

                    if (self->falling)
                    {
                        self->stateTimer = 20;
                        self->jumpPower  = 0;
                        self->yspeed     = 0;
                        self->canDash    = false;
                    }
                    else
                    {
                        self->stateTimer = 32;
                    }
                }
                else
                {
                    // initiate jump
                    self->jumpPower = 60; //+ ((abs(self->xspeed) + 16) >> 3);
                    self->yspeed    = -self->jumpPower;
                    self->falling   = true;
                    self->canDash   = true;

                    // if (self->state == MG_PL_ST_DASHING)
                    //{
                    //     self->canDash = false;
                    // }

                    soundPlaySfx(&(self->soundManager->sndJump1), BZR_LEFT);
                }
            }
            else if (self->state != MG_PL_ST_MIC_DROP && (self->gameData->btnState & PB_DOWN)
                     && (self->gameData->abilities & (1U << MG_DROP_THE_MIC_ABILITY)))
            {
                self->xspeed       = 0;
                self->yspeed       = -32;
                self->state        = MG_PL_ST_MIC_DROP;
                self->tileCollider = &entityTileCollider_1x2;
                self->yMaxSpeed    = 120;
                self->stateTimer   = 180;
            }
            else if (mg_canWallJump(self))
            {
                // initiate wall jump
                self->jumpPower = 60; //+ ((abs(self->xspeed) + 16) >> 3);
                self->xspeed    = (self->spriteFlipHorizontal) ? 32 : -32;
                self->yspeed    = -self->jumpPower;
                self->falling   = true;

                if (self->state == MG_PL_ST_DASHING)
                {
                    self->state        = MG_PL_ST_NORMAL;
                    self->tileCollider = &entityTileCollider_1x2;
                    self->stateTimer   = -1;
                    self->gravity      = 4;
                }

                self->spriteFlipHorizontal = (self->xspeed > 0) ? 0 : 1;
                // soundPlaySfx(&(self->soundManager->sndJump1), BZR_LEFT);
            }
            else if (self->canDash && (self->gameData->abilities & (1U << MG_OBNOXIOUS_NOODLING_ABILITY)))
            {
                // initiate double jump
                self->jumpPower = 60;
                self->xspeed    = (self->spriteFlipHorizontal) ? -32 : 32;
                self->yspeed    = -self->jumpPower;
                self->falling   = true;
                self->canDash   = false;
                /* play the six-frame double-jump animation */
                self->doubleJumpAnimTimer = 6 * 4; /* 6 frames, 4 ticks each */
                self->spriteIndex         = MG_SP_PLAYER_DOUBLE_JUMP_0;
            }
        }
        else if (self->jumpPower > 0 && self->yspeed < 0)
        {
            // dampen jump
            self->jumpPower -= 2; // 32
            self->yspeed = -self->jumpPower;

            /*if (self->jumpPower > 35 && self->jumpPower < 37)
            {
                soundPlaySfx(&(self->soundManager->sndJump2), BZR_LEFT);
            }

            if (self->yspeed > -6 && self->yspeed < -2)
            {
                soundPlaySfx(&(self->soundManager->sndJump3), BZR_LEFT);
            }*/

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

        if (self->animationTimer == 0)
        {
            mg_remapPlayerNotShootWsg(self->tilemap->wsgManager);
        }
    }

    if (self->doubleJumpAnimTimer > 0)
    {
        self->doubleJumpAnimTimer--;
    }

    if (self->gameData->btnState & PB_B && !(self->gameData->prevBtnState & PB_B) && self->shotsFired < self->shotLimit)
    {
        switch (self->state)
        {
            case MG_PL_ST_NORMAL:
            case MG_PL_ST_DASHING:
                if ((self->gameData->btnState & PB_UP) && ((self->jumpPower >= 0) || (!self->falling))
                    && (self->gameData->abilities & (1U << MG_SURE_YOU_CAN_ABILITY)))
                {
                    self->state      = MG_PL_ST_UPPERCUT;
                    self->yspeed     = 0;
                    self->falling    = true;
                    self->jumpPower  = -1;
                    self->stateTimer = 32;
                }
                else if (self->shotsFired < self->shotLimit)
                {
                    mgEntity_t* createdEntity = mg_createEntity(self->entityManager, ENTITY_WAVE_BALL,
                                                                TO_PIXEL_COORDS(self->x), TO_PIXEL_COORDS(self->y) - 2);
                    if (createdEntity != NULL)
                    {
                        createdEntity->xspeed       = (self->spriteFlipHorizontal)
                                                          ? -(96 + abs(self->xspeed) /*+ abs(self->yspeed)*/)
                                                          : 96 + abs(self->xspeed) /*+ abs(self->yspeed)*/;
                        createdEntity->homeTileX    = 0;
                        createdEntity->homeTileY    = 0;
                        createdEntity->linkedEntity = self;

                        if (self->shotsFired <= -63 && (self->gameData->abilities & (1U << MG_SHOOP_DA_WOOP_ABILITY)))
                        {
                            createdEntity->state       = 2;
                            createdEntity->spriteIndex = MG_SP_CHARGE_SHOT_MAX_1;
                        }
                        else if (self->shotsFired <= -31
                                 && (self->gameData->abilities & (1U << MG_SHOOP_DA_WOOP_ABILITY)))
                        {
                            createdEntity->state       = 1;
                            createdEntity->spriteIndex = MG_SP_CHARGE_SHOT_LVL1_1;
                        }
                        else
                        {
                            createdEntity->state = 0;
                        }

                        if (self->shotsFired < 0)
                        {
                            self->shotsFired = 0;
                        }

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

    if ((self->gameData->frameCount & 0b1) && self->shotsFired <= 0 && self->shotsFired > -63)
    {
        self->shotsFired--;
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

void mg_updateInvincibilityFrames(mgEntity_t* self)
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
        if (self->yDamping != 1)
        {
            self->xspeed = -self->xspeed;
            self->yspeed = -self->yspeed;
        }
        else
        {
            self->spriteFlipHorizontal = !self->spriteFlipHorizontal;
        }
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

        if (self->jumpPower == MG_TILE_BRICK_BLOCK && (self->yspeed > 0 || self->yDamping == 1)
            && createdEntity == NULL)
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

            uint8_t newTy
                = MG_TO_TILECOORDS(TO_PIXEL_COORDS((self->y + self->yspeed)) + SIGNOF(self->yspeed) * MG_HALF_TILESIZE);

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
            uint8_t newTx
                = MG_TO_TILECOORDS(TO_PIXEL_COORDS((self->x + self->xspeed)) + SIGNOF(self->xspeed) * MG_HALF_TILESIZE);

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
    uint16_t x                                  = TO_PIXEL_COORDS(self->x);
    uint16_t y                                  = TO_PIXEL_COORDS(self->y);
    int16_t xspeed                              = TO_PIXEL_COORDS(self->xspeed);
    int16_t yspeed                              = TO_PIXEL_COORDS(self->yspeed);
    const mg_EntityTileCollider_t* tileCollider = self->tileCollider;

    int16_t offX, offY, tempX, tempY, tempTx, tempTy, tempT, newX, newY, onGround;

    newX = 0;
    newY = 0;

    if (self->xspeed > 0)
    {
        const mg_EntityTileCollisionPointList_t* rightEdge = tileCollider->rightEdge;
        for (int i = 0; i < rightEdge->size; i++)
        {
            offX  = rightEdge->collisionPoints[i].x;
            offY  = rightEdge->collisionPoints[i].y;
            tempX = x + offX + xspeed;
            tempY = y + offY;

            tempTx = MG_TO_TILECOORDS(tempX);
            tempTy = MG_TO_TILECOORDS(tempY);

            // drawLine(tempX - self->tilemap->mapOffsetX, tempY  - self->tilemap->mapOffsetY, tempX + xspeed +
            // (SIGNOF(xspeed) * MG_HALF_TILESIZE)  - self->tilemap->mapOffsetX, tempY + yspeed + (SIGNOF(yspeed) *
            // MG_HALF_TILESIZE)  - self->tilemap->mapOffsetY, c500, 0); drawRect((tempTx << MG_TILESIZE_IN_POWERS_OF_2)
            // - self->tilemap->mapOffsetX, (tempTy << MG_TILESIZE_IN_POWERS_OF_2) - self->tilemap->mapOffsetY, (tempTx
            // << MG_TILESIZE_IN_POWERS_OF_2) + MG_TILESIZE - self->tilemap->mapOffsetX, (tempTy <<
            // MG_TILESIZE_IN_POWERS_OF_2) + MG_TILESIZE - self->tilemap->mapOffsetY, c500);

            tempT = mg_getTile(self->tilemap, tempTx, tempTy);

            if (self->tileCollisionHandler(self, tempT, tempTx, tempTy, 1))
            {
                newX = ((tempTx) << MG_TILESIZE_IN_POWERS_OF_2) - offX;
                break;
            }
        }
    }
    else if (self->xspeed < 0)
    {
        const mg_EntityTileCollisionPointList_t* leftEdge = tileCollider->leftEdge;
        for (int i = 0; i < leftEdge->size; i++)
        {
            offX  = leftEdge->collisionPoints[i].x;
            offY  = leftEdge->collisionPoints[i].y;
            tempX = x + offX + xspeed;
            tempY = y + offY;

            tempTx = MG_TO_TILECOORDS(tempX);
            tempTy = MG_TO_TILECOORDS(tempY);

            // drawLine(tempX - self->tilemap->mapOffsetX, tempY  - self->tilemap->mapOffsetY, tempX + xspeed +
            // (SIGNOF(xspeed) * MG_HALF_TILESIZE)  - self->tilemap->mapOffsetX, tempY + yspeed + (SIGNOF(yspeed) *
            // MG_HALF_TILESIZE)  - self->tilemap->mapOffsetY, c500, 0); drawRect((tempTx << MG_TILESIZE_IN_POWERS_OF_2)
            // - self->tilemap->mapOffsetX, (tempTy << MG_TILESIZE_IN_POWERS_OF_2) - self->tilemap->mapOffsetY, (tempTx
            // << MG_TILESIZE_IN_POWERS_OF_2) + MG_TILESIZE - self->tilemap->mapOffsetX, (tempTy <<
            // MG_TILESIZE_IN_POWERS_OF_2) + MG_TILESIZE - self->tilemap->mapOffsetY, c500);

            tempT = mg_getTile(self->tilemap, tempTx, tempTy);

            if (self->tileCollisionHandler(self, tempT, tempTx, tempTy, 0))
            {
                newX = ((tempTx + 1) << MG_TILESIZE_IN_POWERS_OF_2) - offX;
                break;
            }
        }
    }

    if (self->yspeed > 0)
    {
        const mg_EntityTileCollisionPointList_t* bottomEdge = tileCollider->bottomEdge;
        for (int i = 0; i < bottomEdge->size; i++)
        {
            offX  = bottomEdge->collisionPoints[i].x;
            offY  = bottomEdge->collisionPoints[i].y;
            tempX = x + offX;
            tempY = y + offY + yspeed;

            tempTx = MG_TO_TILECOORDS(tempX);
            tempTy = MG_TO_TILECOORDS(tempY);

            // drawLine(tempX - self->tilemap->mapOffsetX, tempY  - self->tilemap->mapOffsetY, tempX + xspeed +
            // (SIGNOF(xspeed) * MG_HALF_TILESIZE)  - self->tilemap->mapOffsetX, tempY + yspeed + (SIGNOF(yspeed) *
            // MG_HALF_TILESIZE)  - self->tilemap->mapOffsetY, c500, 0); drawRect((tempTx << MG_TILESIZE_IN_POWERS_OF_2)
            // - self->tilemap->mapOffsetX, (tempTy << MG_TILESIZE_IN_POWERS_OF_2) - self->tilemap->mapOffsetY, (tempTx
            // << MG_TILESIZE_IN_POWERS_OF_2) + MG_TILESIZE - self->tilemap->mapOffsetX, (tempTy <<
            // MG_TILESIZE_IN_POWERS_OF_2) + MG_TILESIZE - self->tilemap->mapOffsetY, c500);

            tempT = mg_getTile(self->tilemap, tempTx, tempTy);

            if (self->tileCollisionHandler(self, tempT, tempTx, tempTy, 4))
            {
                newY          = ((tempTy) << MG_TILESIZE_IN_POWERS_OF_2) - offY;
                self->falling = false;
                break;
            }
        }
    }
    else if (self->yspeed < 0)
    {
        const mg_EntityTileCollisionPointList_t* topEdge = tileCollider->topEdge;
        for (int i = 0; i < topEdge->size; i++)
        {
            offX  = topEdge->collisionPoints[i].x;
            offY  = topEdge->collisionPoints[i].y;
            tempX = x + offX;
            tempY = y + offY + yspeed;

            tempTx = MG_TO_TILECOORDS(tempX);
            tempTy = MG_TO_TILECOORDS(tempY);

            // drawLine(tempX - self->tilemap->mapOffsetX, tempY  - self->tilemap->mapOffsetY, tempX + xspeed +
            // (SIGNOF(xspeed) * MG_HALF_TILESIZE)  - self->tilemap->mapOffsetX, tempY + yspeed + (SIGNOF(yspeed) *
            // MG_HALF_TILESIZE)  - self->tilemap->mapOffsetY, c500, 0); drawRect((tempTx << MG_TILESIZE_IN_POWERS_OF_2)
            // - self->tilemap->mapOffsetX, (tempTy << MG_TILESIZE_IN_POWERS_OF_2) - self->tilemap->mapOffsetY, (tempTx
            // << MG_TILESIZE_IN_POWERS_OF_2) + MG_TILESIZE - self->tilemap->mapOffsetX, (tempTy <<
            // MG_TILESIZE_IN_POWERS_OF_2) + MG_TILESIZE - self->tilemap->mapOffsetY, c500);

            tempT = mg_getTile(self->tilemap, tempTx, tempTy);

            if (self->tileCollisionHandler(self, tempT, tempTx, tempTy, 2))
            {
                newY = ((tempTy + 1) << MG_TILESIZE_IN_POWERS_OF_2) - offY;
                break;
            }
        }
    }
    else if (!self->falling)
    {
        onGround                                            = false;
        const mg_EntityTileCollisionPointList_t* bottomEdge = tileCollider->bottomEdge;

        for (int i = 0; i < bottomEdge->size; i++)
        {
            offX  = bottomEdge->collisionPoints[i].x;
            offY  = bottomEdge->collisionPoints[i].y;
            tempX = x + offX;
            tempY = y + offY + 1;

            tempTx = MG_TO_TILECOORDS(tempX);
            tempTy = MG_TO_TILECOORDS(tempY);

            tempT = mg_getTile(self->tilemap, tempTx, tempTy);

            if (self->tileCollisionHandler(self, tempT, tempTx, tempTy, 4))
            {
                onGround = true;
            }
        }

        self->falling = !onGround;
        if (self->falling)
        {
            self->fallOffTileHandler(self);
        }
    }

    self->x = newX ? TO_SUBPIXEL_COORDS(newX) : self->x + self->xspeed;
    self->y = newY ? TO_SUBPIXEL_COORDS(newY) : self->y + self->yspeed;
}

bool mg_canWallJump(mgEntity_t* self)
{
    if (!self->falling)
    {
        return false;
    }

    uint16_t x                                  = TO_PIXEL_COORDS(self->x);
    uint16_t y                                  = TO_PIXEL_COORDS(self->y);
    const mg_EntityTileCollider_t* tileCollider = self->tileCollider;

    int16_t offX, offY, tempX, tempY, tempTx, tempTy, tempT;

    if (self->xspeed > 0 && !self->spriteFlipHorizontal)
    {
        const mg_EntityTileCollisionPointList_t* rightEdge = tileCollider->rightEdge;
        for (int i = 0; i < rightEdge->size; i++)
        {
            offX  = rightEdge->collisionPoints[i].x;
            offY  = rightEdge->collisionPoints[i].y;
            tempX = x + offX + 1;
            tempY = y + offY;

            tempTx = MG_TO_TILECOORDS(tempX);
            tempTy = MG_TO_TILECOORDS(tempY);

            // drawLine(tempX - self->tilemap->mapOffsetX, tempY  - self->tilemap->mapOffsetY, tempX + xspeed +
            // (SIGNOF(xspeed) * MG_HALF_TILESIZE)  - self->tilemap->mapOffsetX, tempY + yspeed + (SIGNOF(yspeed) *
            // MG_HALF_TILESIZE)  - self->tilemap->mapOffsetY, c500, 0); drawRect((tempTx << MG_TILESIZE_IN_POWERS_OF_2)
            // - self->tilemap->mapOffsetX, (tempTy << MG_TILESIZE_IN_POWERS_OF_2) - self->tilemap->mapOffsetY, (tempTx
            // << MG_TILESIZE_IN_POWERS_OF_2) + MG_TILESIZE - self->tilemap->mapOffsetX, (tempTy <<
            // MG_TILESIZE_IN_POWERS_OF_2) + MG_TILESIZE - self->tilemap->mapOffsetY, c500);

            tempT = mg_getTile(self->tilemap, tempTx, tempTy);

            if (self->tileCollisionHandler(self, tempT, tempTx, tempTy, 1))
            {
                return true;
            }
        }
    }
    else if (self->xspeed < 0 && self->spriteFlipHorizontal)
    {
        const mg_EntityTileCollisionPointList_t* leftEdge = tileCollider->leftEdge;
        for (int i = 0; i < leftEdge->size; i++)
        {
            offX  = leftEdge->collisionPoints[i].x;
            offY  = leftEdge->collisionPoints[i].y;
            tempX = x + offX - 1;
            tempY = y + offY;

            tempTx = MG_TO_TILECOORDS(tempX);
            tempTy = MG_TO_TILECOORDS(tempY);

            // drawLine(tempX - self->tilemap->mapOffsetX, tempY  - self->tilemap->mapOffsetY, tempX + xspeed +
            // (SIGNOF(xspeed) * MG_HALF_TILESIZE)  - self->tilemap->mapOffsetX, tempY + yspeed + (SIGNOF(yspeed) *
            // MG_HALF_TILESIZE)  - self->tilemap->mapOffsetY, c500, 0); drawRect((tempTx << MG_TILESIZE_IN_POWERS_OF_2)
            // - self->tilemap->mapOffsetX, (tempTy << MG_TILESIZE_IN_POWERS_OF_2) - self->tilemap->mapOffsetY, (tempTx
            // << MG_TILESIZE_IN_POWERS_OF_2) + MG_TILESIZE - self->tilemap->mapOffsetX, (tempTy <<
            // MG_TILESIZE_IN_POWERS_OF_2) + MG_TILESIZE - self->tilemap->mapOffsetY, c500);

            tempT = mg_getTile(self->tilemap, tempTx, tempTy);

            if (self->tileCollisionHandler(self, tempT, tempTx, tempTy, 0))
            {
                return true;
            }
        }
    }

    return false;
}

bool mg_canExitDashSlide(mgEntity_t* self)
{
    uint16_t x                                  = TO_PIXEL_COORDS(self->x);
    uint16_t y                                  = TO_PIXEL_COORDS(self->y);
    const mg_EntityTileCollider_t* tileCollider = self->tileCollider;

    int16_t offX, offY, tempX, tempY, tempTx, tempTy, tempT;

    const mg_EntityTileCollisionPointList_t* topEdge = tileCollider->topEdge;
    for (int i = 0; i < topEdge->size; i++)
    {
        offX  = topEdge->collisionPoints[i].x;
        offY  = topEdge->collisionPoints[i].y;
        tempX = x + offX;
        tempY = y + offY - 2;

        tempTx = MG_TO_TILECOORDS(tempX);
        tempTy = MG_TO_TILECOORDS(tempY);

        //  drawLine(tempX - self->tilemap->mapOffsetX, tempY  - self->tilemap->mapOffsetY, tempX + self->xspeed +
        //  (SIGNOF(self->xspeed) * MG_HALF_TILESIZE)  - self->tilemap->mapOffsetX, tempY + self->yspeed +
        //  (SIGNOF(self->yspeed) * MG_HALF_TILESIZE)  - self->tilemap->mapOffsetY, c500, 0); drawRect((tempTx <<
        //  MG_TILESIZE_IN_POWERS_OF_2)
        //  - self->tilemap->mapOffsetX, (tempTy << MG_TILESIZE_IN_POWERS_OF_2) - self->tilemap->mapOffsetY, (tempTx
        //  << MG_TILESIZE_IN_POWERS_OF_2) + MG_TILESIZE - self->tilemap->mapOffsetX, (tempTy <<
        //  MG_TILESIZE_IN_POWERS_OF_2) + MG_TILESIZE - self->tilemap->mapOffsetY, c500);

        tempT = mg_getTile(self->tilemap, tempTx, tempTy);

        // if (self->tileCollisionHandler(self, tempT, tempTx, tempTy, 1))
        // {
        //     return false;
        // }

        if (mg_isSolid(tempT))
        {
            return false;
        }
    }
    // soundPlaySfx(&(self->soundManager->sndBreak), BZR_LEFT);
    return true;
}

void defaultFallOffTileHandler(mgEntity_t* self)
{
    self->falling = true;
}

void mg_playerFallOffTileHandler(mgEntity_t* self)
{
    self->falling = true;
    if (self->state == MG_PL_ST_DASHING)
    {
        self->stateTimer = 1;
    }
}

void applyDamping(mgEntity_t* self)
{
    // if (!self->gravityEnabled)
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
        || TO_PIXEL_COORDS(self->x) > (self->tilemap->mapOffsetX + MG_TILEMAP_DISPLAY_WIDTH_PIXELS + DESPAWN_THRESHOLD))
    {
        mg_bossRushLogic(self);
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
        mg_bossRushLogic(self);
        mg_destroyEntity(self, true);
    }
}

void mg_bossRushLogic(mgEntity_t* self)
{
    // boss rush logic
    if (self->gameData->level != 11)
    {
        return;
    }
    bool isABoss      = false;
    uint8_t nextBoss  = 0;
    uint8_t nextLevel = 0;

    isABoss = self->spriteFlipVertical && self->entityManager->wsgManager->sprites[self->spriteIndex].wsg->w > 30
              && self->entityManager->wsgManager->sprites[self->spriteIndex].wsg->h > 30;

    if (self->entityManager->wsgManager->wsgSetIndex == MG_WSGSET_KINETIC_DONUT)
    {
        nextBoss  = ENTITY_BOSS_GRIND_PANGOLIN;
        nextLevel = 2;
    }
    else if (self->entityManager->wsgManager->wsgSetIndex == MG_WSGSET_GRIND_PANGOLIN)
    {
        nextBoss  = ENTITY_BOSS_SEVER_YATAGA;
        nextLevel = 3;
    }
    else if (self->entityManager->wsgManager->wsgSetIndex == MG_WSGSET_SEVER_YATAGA)
    {
        nextBoss  = ENTITY_BOSS_TRASH_MAN;
        nextLevel = 4;
    }
    else if (self->entityManager->wsgManager->wsgSetIndex == MG_WSGSET_TRASH_MAN)
    {
        nextBoss  = ENTITY_BOSS_SMASH_GORILLA;
        nextLevel = 6;
    }
    else if (self->entityManager->wsgManager->wsgSetIndex == MG_WSGSET_SMASH_GORILLA)
    {
        nextBoss  = ENTITY_BOSS_DEADEYE_CHIRPZI;
        nextLevel = 7;
    }
    else if (self->entityManager->wsgManager->wsgSetIndex == MG_WSGSET_DEADEYE_CHIRPZI)
    {
        nextBoss  = ENTITY_BOSS_DRAIN_BAT;
        nextLevel = 8;
    }
    else if (self->entityManager->wsgManager->wsgSetIndex == MG_WSGSET_DRAIN_BAT)
    {
        nextBoss  = ENTITY_BOSS_FLARE_GRYFFYN;
        nextLevel = 9;
    }
    else if (self->entityManager->wsgManager->wsgSetIndex == MG_WSGSET_FLARE_GRYFFYN)
    {
        nextBoss  = 0;
        nextLevel = 12;
    }

    if (isABoss)
    {
        // mg_loadMapFromFile((self->entityManager->tilemap), leveldef[11].filename,
        //                 self->entityManager);
        self->gameData->bgColors = leveldef[nextLevel].bgColors;
        if (nextBoss > 0)
        {
            mg_loadWsgSet(self->entityManager->wsgManager, leveldef[nextLevel].defaultWsgSetIndex);
            mg_setBgm(self->soundManager, leveldef[nextLevel].bossBgmIndex);
            soundPlayBgm(&self->soundManager->currentBgm, BZR_STEREO);
            mgEntity_t* boss = mg_createEntity(self->entityManager, nextBoss, self->entityManager->bossSpawnX,
                                               self->entityManager->bossSpawnY);
            boss->state      = 0;
            if (nextBoss == ENTITY_BOSS_TRASH_MAN)
            {
                boss->y -= 100 << SUBPIXEL_RESOLUTION;
            }
            else if (nextBoss == ENTITY_BOSS_SMASH_GORILLA || nextBoss == ENTITY_BOSS_SEVER_YATAGA)
            {
                boss->y -= 10 << SUBPIXEL_RESOLUTION;
            }
            else if (nextBoss == ENTITY_BOSS_GRIND_PANGOLIN)
            {
                boss->y -= 4 << SUBPIXEL_RESOLUTION;
            }
        }
        else
        {
            mg_setBgm(self->soundManager, MG_BGM_POST_FIGHT);
            soundPlayBgm(&self->soundManager->currentBgm, BZR_STEREO);
        }
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
        self->spawnData->spawnable     = true;
    }

    // self->entityManager->activeEntities--;
    self->active = false;
}

void animatePlayer(mgEntity_t* self)
{
    if (self->spriteIndex == MG_SP_PLAYER_WIN)
    {
        // Win pose has been set; don't change it!
        return;
    }

    if (self->state == MG_PL_ST_HURT)
    {
        self->spriteIndex
            = playerDamageAnimFrames[((self->stateTimer + self->visible) >> 2) % ARRAY_SIZE(playerDamageAnimFrames)];
        return;
    }
    else if (self->state == MG_PL_ST_MIC_DROP)
    {
        self->spriteIndex = playerMicDropAnimFrames[(self->stateTimer >> 2) % ARRAY_SIZE(playerMicDropAnimFrames)];
        self->spriteRotateAngle = getAtan2(self->yspeed, self->xspeed) - 90;
        return;
    }
    else if (self->state == MG_PL_ST_UPPERCUT)
    {
        self->spriteIndex
            = playerSureYouCanAnimnFrames[(self->stateTimer >> 2) % ARRAY_SIZE(playerSureYouCanAnimnFrames)];
        if (self->stateTimer == 4)
        {
            self->spriteFlipHorizontal = !self->spriteFlipHorizontal;
            self->falling              = true;
        }
        return;
    }

    /* Double-jump animation has priority over normal jump/fall sprites */
    if (self->doubleJumpAnimTimer > 0)
    {
        const uint8_t total = ARRAY_SIZE(playerDoubleJumpAnimFrames);
        uint8_t idx         = (total - 1) - ((self->doubleJumpAnimTimer - 1) / 4);
        if (idx >= total)
        {
            idx = 0;
        }
        self->spriteIndex = playerDoubleJumpAnimFrames[idx];
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
            if (self->jumpPower > 32)
            {
                self->spriteIndex = MG_SP_PLAYER_JUMP;
            }
            else
            {
                self->spriteIndex = MG_SP_PLAYER_JUMP1;
            }
        }
        else
        {
            // Falling
            if (self->yspeed != self->yMaxSpeed)
            {
                self->spriteIndex = MG_SP_PLAYER_JUMP2;
            }
            else
            {
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
        switch (self->state)
        {
            default:
                if (((self->gameData->btnState & PB_LEFT) && self->xspeed < 0)
                    || ((self->gameData->btnState & PB_RIGHT) && self->xspeed > 0))
                {
                    // Running
                    self->spriteFlipHorizontal = (self->xspeed > 0) ? 0 : 1;

                    if (self->spriteIndex < MG_SP_PLAYER_WALK1 || self->spriteIndex > MG_SP_PLAYER_WALK10)
                    {
                        self->spriteIndex = MG_SP_PLAYER_WALK1;
                    }
                    else if (self->gameData->frameCount % (5 /*- (abs(self->xspeed) >> 1)*/) == 0)
                    {
                        self->spriteIndex++;
                        if (self->spriteIndex > MG_SP_PLAYER_WALK10)
                        {
                            self->spriteIndex = MG_SP_PLAYER_WALK1;
                        }
                    }
                }
                else
                {
                    self->spriteIndex = MG_SP_PLAYER_SLIDE;
                }
                break;
            case MG_PL_ST_DASHING:
                self->spriteFlipHorizontal = (self->xspeed > 0) ? 0 : 1;
                self->spriteIndex          = MG_SP_PLAYER_DASH_SLIDE;
                break;
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

    mgSprite_t* selfSprite     = &(self->tilemap->wsgManager->sprites[self->spriteIndex]);
    const box_t* selfSpriteBox = selfSprite->hitBox;
    if (selfSpriteBox == NULL)
    {
        return;
    }

    box_t selfBox;
    selfBox.x0 = (self->x >> SUBPIXEL_RESOLUTION) - selfSprite->origin->x + selfSpriteBox->x0;
    selfBox.y0 = (self->y >> SUBPIXEL_RESOLUTION) - selfSprite->origin->y + selfSpriteBox->y0;
    selfBox.x1 = (self->x >> SUBPIXEL_RESOLUTION) - selfSprite->origin->x + selfSpriteBox->x1;
    selfBox.y1 = (self->y >> SUBPIXEL_RESOLUTION) - selfSprite->origin->y + selfSpriteBox->y1;

    //  drawRect(selfBox.x0 - self->tilemap->mapOffsetX, selfBox.y0 - self->tilemap->mapOffsetY, selfBox.x1 -
    //      self->tilemap->mapOffsetX, selfBox.y1 - self->tilemap->mapOffsetY, c500);

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

            if (checkEntitySpriteBox == NULL)
            {
                continue;
            }

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

            //  drawRect(checkEntityBox.x0 - self->tilemap->mapOffsetX, checkEntityBox.y0 - self->tilemap->mapOffsetY,
            //  checkEntityBox.x1 - self->tilemap->mapOffsetX, checkEntityBox.y1 - self->tilemap->mapOffsetY, c005);
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
        case ENTITY_SHRUBBLE_LV4:
        case ENTITY_DUST_BUNNY_2:
        case ENTITY_DUST_BUNNY_3:
        case ENTITY_WASP_2:
        case ENTITY_WASP_3:
        case ENTITY_CHARGIN_SCHMUCK:
        case ENTITY_BOUNCIN_SCHMUCK:
        case ENTITY_SPIKY_MCGEE:
        case ENTITY_TURRET:
        case ENTITY_BOSS_SEVER_YATAGA:
        case ENTITY_BOSS_TRASH_MAN:
        case ENTITY_BOSS_GRIND_PANGOLIN:
        {
            if (self->state == MG_PL_ST_MIC_DROP)
            {
                self->state             = MG_PL_ST_NORMAL;
                self->yspeed            = -self->yspeed;
                self->spriteRotateAngle = 0;
                // self->spriteFlipVertical  = false;
                self->invincibilityFrames = 5;

                if (other->invincibilityFrames)
                {
                    break;
                }

                other->hp -= 8;
                other->invincibilityFrames = 4;

                if (other->hp <= 0)
                {
                    other->xspeed = self->xspeed >> 1;
                    other->yspeed = -abs(self->xspeed >> 1);
                    mg_scorePoints(self->gameData, other->scoreValue);
                    soundPlaySfx(&(self->soundManager->sndBreak), BZR_LEFT);
                    killEnemy(other);

                    break;
                }
            }
            else if (self->state == MG_PL_ST_UPPERCUT)
            {
                self->invincibilityFrames = 5;

                if (other->invincibilityFrames)
                {
                    break;
                }

                other->hp -= 2;
                other->invincibilityFrames = 3;
                other->xspeed += (self->xspeed + (SIGNOF(self->xspeed) * (self->xspeed >> 1)));
                other->yspeed += (self->yspeed + (SIGNOF(self->yspeed) * (self->yspeed >> 1)));
                other->falling = true;

                if (other->hp <= 0)
                {
                    other->xspeed = self->xspeed >> 1;
                    other->yspeed = -abs(self->yspeed);
                    mg_scorePoints(self->gameData, other->scoreValue);
                    soundPlaySfx(&(self->soundManager->sndBreak), BZR_LEFT);
                    killEnemy(other);

                    break;
                }
            }

            if (other->type == ENTITY_SHRUBBLE_LV4)
            {
                crawlerSetMoveState(other, ((other->animationTimer + 4) % 8) + 1);
            }
            else
            {
                other->xspeed = -other->xspeed;
            }

            /*if (self->y < other->y || self->yspeed > 0)
            {
                mg_scorePoints(self->gameData, other->scoreValue);

                killEnemy(other);
                soundPlaySfx(&(self->soundManager->sndSquish), BZR_LEFT);

                self->yspeed    = -180;
                self->jumpPower = 64 + ((abs(self->xspeed) + 16) >> 3);
                self->falling   = true;
            }
            else */
            if (self->invincibilityFrames <= 0)
            {
                if (!self->gameData->cheatMode)
                {
                    // pulse takes damage (doubled if no plot armor)
                    self->hp -= 5 + (5 * !(self->gameData->abilities & (1U << MG_PLOT_ARMOR_ABILITY)));
                }

                self->gameData->comboTimer = 0;

                if (self->shotsFired < 0)
                {
                    self->shotsFired = 0;
                }

                if (!self->gameData->debugMode && self->hp <= 0)
                {
                    self->updateFunction        = &updateEntityDead;
                    self->type                  = ENTITY_DEAD;
                    self->xspeed                = 0;
                    self->yspeed                = -60;
                    self->state                 = MG_PL_ST_HURT;
                    self->stateTimer            = 20;
                    self->gameData->changeState = MG_ST_DEAD;
                    self->gravityEnabled        = true;
                    self->falling               = true;
                }
                else
                {
                    self->xspeed              = (self->x > other->x) ? 32 : -32;
                    self->yspeed              = 0;
                    self->jumpPower           = 0;
                    self->invincibilityFrames = 120;
                    self->state               = MG_PL_ST_HURT;
                    self->stateTimer          = 20;
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
            self->hp += 12; // Previously 6, doubled to keep the same amount since PULSE went from 30 to 60 health.
            if ((self->gameData->abilities & (1U << MG_CAN_OF_SALSA_ABILITY)))
            {
                if (self->hp > 72)
                {
                    self->hp = 72;
                }
            }
            else if (self->hp > 60)
            {
                self->hp = 60;
            }
            mg_scorePoints(self->gameData, 1000);
            soundPlaySfx(&(self->soundManager->sndPowerUp), BZR_LEFT);
            // mg_updateLedsHpMeter(self->entityManager, self->gameData);
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
                // uint8_t aboveTile
                //    = self->tilemap->map[(other->homeTileY - 1) * self->tilemap->mapWidth + other->homeTileX];

                // if (aboveTile >= MG_TILE_WARP_0 && aboveTile <= MG_TILE_WARP_F)
                //{
                self->gameData->checkpointLevel      = self->gameData->level;
                self->gameData->checkpointSpawnIndex = other->spawnData->id;
                other->xDamping                      = 1;
                soundPlaySfx(&(self->soundManager->sndCheckpoint), BZR_LEFT);
                //}
            }
            break;
        }
        case ENTITY_WARP_ENTRANCE_WALL:
        case ENTITY_WARP_ENTRANCE_FLOOR:
        {
            if (other->spawnData->linkedEntitySpawn != NULL)
            {
                // Execute warp within the current map

                int32_t warpXoffset = other->x - self->x;
                int32_t warpYoffset = other->y - self->y;

                self->x = TO_SUBPIXEL_COORDS((other->spawnData->linkedEntitySpawn->tx << SUBPIXEL_RESOLUTION)
                                             + other->spawnData->linkedEntitySpawn->xOffsetInPixels)
                          - warpXoffset;
                self->y = TO_SUBPIXEL_COORDS((other->spawnData->linkedEntitySpawn->ty << SUBPIXEL_RESOLUTION)
                                             + other->spawnData->linkedEntitySpawn->yOffsetInPixels)
                          - warpYoffset;
                self->falling = true;
                mg_viewFollowEntity(self->tilemap, self->entityManager->playerEntity);

                mg_unlockScrolling(self->tilemap);
                mg_deactivateAllEntities(self->entityManager, true);
                self->tilemap->executeTileSpawnAll = true;
                soundPlaySfx(&(self->soundManager->sndWarp), BZR_LEFT);
            }
            else
            {
                // Execute warp to a different map
                uint8_t newLevelIndex               = other->spawnData->special2;
                uint8_t modifiedPlayerSpawn_tx      = other->spawnData->special3;
                uint8_t modifiedPlayerSpawn_ty      = other->spawnData->special4;
                uint8_t modifiedPlayerSpawn_xOffset = other->spawnData->special5;
                uint8_t modifiedPlayerSpawn_yOffset = other->spawnData->special6;

                self->gameData->level = newLevelIndex;

                mg_deactivateAllEntities(self->entityManager, true);
                mg_loadMapFromFile(self->tilemap, leveldef[newLevelIndex].filename, self->entityManager);
                mg_loadWsgSet(self->tilemap->wsgManager, leveldef[newLevelIndex].defaultWsgSetIndex);

                if (self->tilemap->defaultPlayerSpawn != NULL)
                {
                    self->tilemap->defaultPlayerSpawn->tx              = modifiedPlayerSpawn_tx;
                    self->tilemap->defaultPlayerSpawn->ty              = modifiedPlayerSpawn_ty;
                    self->tilemap->defaultPlayerSpawn->xOffsetInPixels = modifiedPlayerSpawn_xOffset;
                    self->tilemap->defaultPlayerSpawn->yOffsetInPixels = modifiedPlayerSpawn_yOffset;
                }

                self->x = TO_SUBPIXEL_COORDS((modifiedPlayerSpawn_tx * 16) + modifiedPlayerSpawn_xOffset);
                self->y = TO_SUBPIXEL_COORDS((modifiedPlayerSpawn_ty * 16) + modifiedPlayerSpawn_yOffset);

                self->tilemap->executeTileSpawnAll = true;

                self->gameData->countdown = leveldef[newLevelIndex].timeLimit;

                stageStartCutscene(self->gameData);
                self->gameData->changeState = MG_ST_CUTSCENE;
            }

            break;
        }
        case ENTITY_BOUNCE_PAD:
        {
            self->yspeed  = (other->spriteFlipVertical) ? 112 : -112;
            self->falling = true;
            self->canDash = true;
            soundPlaySfx(&(self->soundManager->sndHit), BZR_LEFT);
            if (self->state == MG_PL_ST_MIC_DROP)
            {
                self->stateTimer = -1;
            }

            break;
        }
        case ENTITY_BOUNCE_PAD_DIAGONAL:
        {
            self->xspeed  = (other->spriteFlipHorizontal) ? -112 : 112;
            self->yspeed  = (other->spriteFlipVertical) ? 112 : -112;
            self->falling = true;
            self->canDash = true;
            soundPlaySfx(&(self->soundManager->sndHit), BZR_LEFT);
            break;
        }
        case ENTITY_WAVE_BALL:
        {
            if (other->linkedEntity == self)
            {
                break;
            }

            if (self->state == MG_PL_ST_SHIELD)
            {
                other->xspeed       = -other->xspeed;
                other->yspeed       = -other->yspeed;
                other->linkedEntity = self;
                break;
            }

            // TODO: This is a repeat of above code; move to its own function
            if (self->invincibilityFrames <= 0 && other->scoreValue)
            {
                if (!self->gameData->cheatMode)
                {
                    // pulse takes damage (doubled if no plot armor)
                    self->hp -= other->scoreValue
                                + (other->scoreValue * !(self->gameData->abilities & (1U << MG_PLOT_ARMOR_ABILITY)));
                }
                mg_updateLedsHpMeter(self->entityManager, self->gameData);
                self->gameData->comboTimer = 0;

                if (!self->gameData->debugMode && self->hp <= 0)
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
                    self->xspeed              = (self->x > other->x) ? 16 : -16;
                    self->yspeed              = 0;
                    self->jumpPower           = 0;
                    self->invincibilityFrames = 120;
                    soundPlaySfx(&(self->soundManager->sndHurt), BZR_LEFT);
                }
            }
            break;
        }
        case ENTITY_MIXTAPE:
        {
            soundStop(true);
            mg_setBgm(self->soundManager, MG_BGM_LEVEL_CLEAR_JINGLE);
            soundPlayBgm(&self->soundManager->currentBgm, BZR_STEREO);
            globalMidiPlayerGet(MIDI_BGM)->loop = false;
            self->visible                       = true;
            self->spriteIndex                   = MG_SP_PLAYER_WIN;
            self->updateFunction                = &mg_updateDummy;
            self->gameData->changeState         = MG_ST_LEVEL_CLEAR;
            other->x                            = (self->spriteFlipHorizontal) ? (self->x - (9 << SUBPIXEL_RESOLUTION))
                                                                               : (self->x + (9 << SUBPIXEL_RESOLUTION));
            other->y                            = (self->y - (12 << SUBPIXEL_RESOLUTION));
            break;
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
            if (self->type == ENTITY_BOSS_TRASH_MAN)
            {
                break;
            }

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
            if (other->linkedEntity == self || !other->visible)
            {
                break;
            }

            if (self->invincibilityFrames)
            {
                break;
            }

            /* Hank is invincible while his visor is closed (state 0). Use state check instead
             * of invincibilityFrames to avoid blinking visibility toggles.
             */
            if (self->type == ENTITY_BOSS_HANK_WADDLE && (self->state == 0 || self->state == 7))
            {
                mg_destroyShot(other);
                break;
            }

            if (self->type == ENTITY_BOSS_GRIND_PANGOLIN && (self->state == 1 || self->state == 2 || self->state == 4))
            {
                other->xspeed = -other->xspeed;
                other->yspeed = -64;
                other->linkedEntity->shotsFired--;
                other->linkedEntity = self;
                soundPlaySfx(&self->soundManager->sndTally, MIDI_SFX);
                break;
            }

            switch (other->state)
            {
                case 0:
                default:
                    self->hp--;
                    break;
                case 1:
                    self->hp -= 4;
                    break;
                case 2:
                    self->hp -= 8;
                    break;
            }

            /* If Hank Waddle was hit, flash damage and enter damage state */
            if (self->type == ENTITY_BOSS_HANK_WADDLE && self->state == 3)
            {
                self->state       = 4; /* damage flash */
                self->stateTimer  = 0;
                self->spriteIndex = MG_SP_BOSS_4;
                soundPlaySfx(&(self->soundManager->sndHurt), BZR_LEFT);
            }

            if (self->hp <= 0)
            {
                self->xspeed = other->xspeed >> 1;
                self->yspeed = -abs(other->xspeed >> 1);
                mg_scorePoints(self->gameData, self->scoreValue);
                soundPlaySfx(&(self->soundManager->sndBreak), BZR_LEFT);
                killEnemy(self);

                if (other->state != 2)
                {
                    mg_destroyShot(other);
                }

                break;
            }
            else
            {
                mg_destroyShot(other);
            }

            self->invincibilityFrames = 4;

            break;
        case ENTITY_PLAYER:
            if (other->state == MG_PL_ST_MIC_DROP)
            {
                if (self->invincibilityFrames)
                {
                    break;
                }

                self->hp -= 8;
                self->invincibilityFrames = 4;

                if (self->hp <= 0)
                {
                    self->xspeed = other->xspeed >> 1;
                    self->yspeed = -abs(other->xspeed >> 1);
                    mg_scorePoints(self->gameData, self->scoreValue);
                    soundPlaySfx(&(self->soundManager->sndBreak), BZR_LEFT);
                    killEnemy(self);

                    break;
                }
            }
            break;
        case ENTITY_SHRUBBLE_LV4:
            if (self->type == ENTITY_SHRUBBLE_LV4)
            {
                crawlerSetMoveState(self, ((self->animationTimer + 4) % 8) + 1);
                crawlerSetMoveState(other, ((other->animationTimer + 4) % 8) + 1);
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
        case MG_TILE_NONSOLID_VISIBLE_INTERACTIVE_A1:
        {
            mgEntity_t* hitBlock
                = mg_createEntity(self->entityManager, ENTITY_HIT_BLOCK, (tx * MG_TILESIZE) + MG_HALF_TILESIZE,
                                  (ty * MG_TILESIZE) + MG_HALF_TILESIZE);

            if (hitBlock != NULL)
            {
                mg_setTile(self->tilemap, tx, ty,
                           (tileId == MG_TILE_NONSOLID_VISIBLE_INTERACTIVE_A1) ? MG_TILE_EMPTY
                                                                               : MG_TILE_INVISIBLE_BLOCK);
                hitBlock->homeTileX = tx;
                hitBlock->homeTileY = ty;
                hitBlock->jumpPower = tileId;
                if (tileId == MG_TILE_BRICK_BLOCK)
                {
                    if (/*abs(self->xspeed) > 51 && */ self->yspeed >= 0)
                    {
                        hitBlock->yDamping             = 1;
                        hitBlock->spriteIndex          = MG_SP_CRUMBLED_BLOCK;
                        hitBlock->spriteFlipHorizontal = (esp_random() % 2);
                    }
                    else
                    {
                        hitBlock->spriteIndex = MG_SP_HITBLOCK_BRICKS;
                    }
                }

                if (tileId == MG_TILE_NONSOLID_VISIBLE_INTERACTIVE_A1)
                {
                    hitBlock->spriteIndex = MG_SP_BOUNCE_BLOCK;
                }

                switch (direction)
                {
                    case 0:
                        hitBlock->xspeed = -24;
                        if (tileId == MG_TILE_NONSOLID_VISIBLE_INTERACTIVE_A1)
                        {
                            self->xspeed = 48;
                        }
                        break;
                    case 1:
                        hitBlock->xspeed = 24;
                        if (tileId == MG_TILE_NONSOLID_VISIBLE_INTERACTIVE_A1)
                        {
                            self->xspeed = -48;
                        }
                        break;
                    case 2:
                        hitBlock->yspeed = -48;
                        if (tileId == MG_TILE_NONSOLID_VISIBLE_INTERACTIVE_A1)
                        {
                            self->yspeed = 48;
                        }
                        break;
                    case 4:
                        hitBlock->yspeed = (tileId == MG_TILE_BRICK_BLOCK) ? 16 : 24;
                        if (tileId == MG_TILE_NONSOLID_VISIBLE_INTERACTIVE_A1)
                        {
                            self->yspeed  = -64;
                            self->falling = true;
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
        // Spike or Lava Tiles
        case MG_TILE_NONSOLID_VISIBLE_INTERACTIVE_A2 ... MG_TILE_NONSOLID_VISIBLE_INTERACTIVE_A5:
        {
            if (!self->invincibilityFrames && !self->gameData->debugMode)
            {
                killPlayer(self);
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

                if (self->state == MG_PL_ST_MIC_DROP)
                {
                    self->state             = MG_PL_ST_NORMAL;
                    self->spriteRotateAngle = 0;
                    self->yMaxSpeed         = 72;
                    // self->spriteFlipVertical = false;
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

bool mg_trashManTileCollisionHandler(mgEntity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction)
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
        uint8_t createShrubbleTxOffset = 0;
        uint8_t createShrubbleTyOffset = 0;

        switch (direction)
        {
            case 0: // LEFT
                self->xspeed           = -self->xspeed;
                createShrubbleTxOffset = 1;
                break;
            case 1: // RIGHT
                self->xspeed           = -self->xspeed;
                createShrubbleTxOffset = -1;
                break;
            case 2: // UP
                self->yspeed           = -self->yspeed;
                createShrubbleTyOffset = 1;
                break;
            case 4: // DOWN
                self->yspeed           = -self->yspeed;
                createShrubbleTyOffset = -1;
                break;
            default: // Should never hit
                return false;
        }

        if (!(self->shotsFired % 3))
        {
            mgEntity_t* createdEntity = createDustBunnyL2(self->entityManager, ((tx + createShrubbleTxOffset) << 4) + 7,
                                                          (ty + createShrubbleTyOffset) << 4);
            if (createdEntity != NULL)
            {
                self->linkedEntity        = createdEntity;
                createdEntity->scoreValue = 0;
                createdEntity->hp         = 2;
            }
            else if (self->linkedEntity != NULL)
            {
                killEnemy(self->linkedEntity);
            }
        }

        self->shotsFired++;
        // Trigger tile collision resolution
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
    if ((TO_PIXEL_COORDS(self->y) > deathBoundary) && (TO_PIXEL_COORDS(self->y) < deathBoundary + DESPAWN_THRESHOLD))
    {
        self->hp = 0;
        mg_updateLedsHpMeter(self->entityManager, self->gameData);
        self->gameData->changeState = MG_ST_DEAD;
        mg_destroyEntity(self, true);
    }
}

void mg_updateDummy(mgEntity_t* self)
{
    despawnWhenOffscreen(self);
}

void updateScrollLockLeft(mgEntity_t* self)
{
    self->tilemap->minMapOffsetX = TO_PIXEL_COORDS(self->x) - 8;
    mg_viewFollowEntity(self->entityManager->tilemap, self->entityManager->viewEntity);
    mg_destroyEntity(self, true);
}

void updateScrollLockRight(mgEntity_t* self)
{
    uint8_t tx, ty;

    self->tilemap->maxMapOffsetX = TO_PIXEL_COORDS(self->x) + 8 - MG_TILEMAP_DISPLAY_WIDTH_PIXELS;
    self->tilemap->minMapOffsetX = self->tilemap->maxMapOffsetX;
    self->tilemap->mapOffsetX    = self->tilemap->minMapOffsetX;

    // Close off left wall of boss room
    for (uint8_t i = 0; i < MG_TILEMAP_DISPLAY_HEIGHT_TILES; i++)
    {
        tx = (self->tilemap->mapOffsetX) >> MG_TILESIZE_IN_POWERS_OF_2;
        ty = ((self->tilemap->mapOffsetY) >> MG_TILESIZE_IN_POWERS_OF_2) + i;

        if (/*tx < 0 ||*/ tx > self->tilemap->mapWidth || /* ty < 0 || */ ty > self->tilemap->mapHeight)
        {
            break;
        }

        uint8_t checkTile = mg_getTile(self->tilemap, tx, ty);

        if (!mg_isSolid(checkTile))
        {
            mg_setTile(self->tilemap, tx, ty, MG_TILE_SOLID_VISIBLE_NONINTERACTIVE_20);
        }
    }

    // Initiate boss battle.
    // For this to work, the boss must be placed to the left of the scroll lock.
    if (self->entityManager->bossEntity != NULL)
    {
        // Cutscene before the boss fight
        mg_setBgm(self->soundManager, MG_BGM_PRE_FIGHT);
        soundPlayBgm(&self->soundManager->currentBgm, BZR_STEREO);
        bossIntroCutscene(self->gameData);
    }
    else if (self->gameData->level == 5)
    {
        bossIntroCutscene(self->gameData);
    }

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
    /*if (self->gameData->frameCount % 10 == 0)
    {
        self->spriteIndex
            = ((self->entityManager->playerEntity->hp < 2) ? MG_SP_GAMING_1 : MG_SP_MUSIC_1) + ((self->spriteIndex + 1)
    % 3);
    }*/

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
        || TO_PIXEL_COORDS(self->x) > (self->tilemap->mapOffsetX + MG_TILEMAP_DISPLAY_WIDTH_PIXELS + DESPAWN_THRESHOLD)
        || TO_PIXEL_COORDS(self->y) < (self->tilemap->mapOffsetY - DESPAWN_THRESHOLD)
        || TO_PIXEL_COORDS(self->y)
               > (self->tilemap->mapOffsetY + MG_TILEMAP_DISPLAY_HEIGHT_PIXELS + DESPAWN_THRESHOLD))
    {
        // In mg_destroyEntity, this will overflow to the correct value.
        // Mod 256 is used to suppress a -Woverflow warning, since type is 8 bits
        self->type = (128 + MG_TILE_CONTAINER_1) % 256;

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
    if (target->spawnData != NULL)
    {
        target->spawnData->spawnedEntity = NULL;
        target->spawnData                = NULL;
    }

    // eh... that pretty much means it's a boss
    if (target->entityManager->wsgManager->sprites[target->spriteIndex].wsg->w > 30
        && target->entityManager->wsgManager->sprites[target->spriteIndex].wsg->h > 30)
    {
        if (target->gameData->level == 11)
        {
            // give some freaking help on boss rush, geeze
            createPowerUp(target->entityManager, TO_PIXEL_COORDS(target->x), TO_PIXEL_COORDS(target->y));
        }
    }
    else if ((esp_random() % 100) > 90)
    {
        createPowerUp(target->entityManager, TO_PIXEL_COORDS(target->x), TO_PIXEL_COORDS(target->y));
    }

    target->updateFunction = &updateEntityDead;
}

void updateBgCol(mgEntity_t* self)
{
    self->gameData->bgColors = bgGradientGray;
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
    self->spriteRotateAngle = getAtan2(self->yspeed, self->xspeed);

    self->animationTimer++;
    if (self->gameData->frameCount % 2 == 0)
    {
        switch (self->state)
        {
            case 0:
            default:
                self->spriteIndex = normalShotAnimFrames[(self->animationTimer % ARRAY_SIZE(normalShotAnimFrames))];
                break;
            case 1:
                self->spriteIndex = chargeShotAnimFrames[(self->animationTimer % ARRAY_SIZE(chargeShotAnimFrames))];
                break;
            case 2:
                self->spriteIndex
                    = maxChargeShotAnimFrames[(self->animationTimer % ARRAY_SIZE(maxChargeShotAnimFrames))];
                break;
        }
    }

    applyGravity(self);

    // mg_moveEntityWithTileCollisions(self);
    // despawnWhenOffscreen(self);

    uint8_t tx = MG_TO_TILECOORDS(TO_PIXEL_COORDS(self->x));
    uint8_t ty = MG_TO_TILECOORDS(TO_PIXEL_COORDS(self->y));
    self->overlapTileHandler(self, mg_getTile(self->tilemap, tx, ty), tx, ty);

    self->x += self->xspeed;
    self->y += self->yspeed;

    if (TO_PIXEL_COORDS(self->x) < (self->tilemap->mapOffsetX - DESPAWN_THRESHOLD)
        || TO_PIXEL_COORDS(self->x) > (self->tilemap->mapOffsetX + MG_TILEMAP_DISPLAY_WIDTH_PIXELS + DESPAWN_THRESHOLD))
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

    if (self->collisionHandler != &mg_dummyCollisionHandler)
    {
        mg_detectEntityCollisions(self);
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

        if (self->visible)
        {
            soundPlaySfx(&(self->soundManager->sndHit), BZR_LEFT);
        }
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
            self->spriteFlipHorizontal, self->spriteFlipVertical, self->spriteRotateAngle);
}

void mg_hankDrawHandler(mgEntity_t* self)
{
    /* Force Hank to draw using the fixed Hank origin so frame changes don't shift position */
    drawWsg(self->entityManager->wsgManager->sprites[self->spriteIndex].wsg,
            (self->x >> SUBPIXEL_RESOLUTION) - origin_hank.x - self->entityManager->tilemap->mapOffsetX,
            (self->y >> SUBPIXEL_RESOLUTION) - self->entityManager->tilemap->mapOffsetY - origin_hank.y,
            self->spriteFlipHorizontal, self->spriteFlipVertical, self->spriteRotateAngle);
}

void mg_playerDrawHandler(mgEntity_t* self)
{
    drawWsg(self->entityManager->wsgManager->sprites[self->spriteIndex].wsg,
            (self->x >> SUBPIXEL_RESOLUTION) - self->entityManager->wsgManager->sprites[self->spriteIndex].origin->x
                - self->entityManager->tilemap->mapOffsetX,
            (self->y >> SUBPIXEL_RESOLUTION) - self->entityManager->tilemap->mapOffsetY
                - self->entityManager->wsgManager->sprites[self->spriteIndex].origin->y,
            self->spriteFlipHorizontal, self->spriteFlipVertical, self->spriteRotateAngle);

    if (self->state == MG_PL_ST_SHIELD)
    {
        drawWsg(&(self->entityManager->wsgManager->wsgs[MG_WSG_PLAYER_SHIELD_1 + ((self->stateTimer >> 1) & 0b11)]),
                (self->x >> SUBPIXEL_RESOLUTION) - 15 - self->entityManager->tilemap->mapOffsetX,
                (self->y >> SUBPIXEL_RESOLUTION) - self->entityManager->tilemap->mapOffsetY - 15,
                self->spriteFlipHorizontal, self->spriteFlipVertical, 0);
    }
}

void mg_destroyShot(mgEntity_t* self)
{
    if (self->linkedEntity != NULL && self->linkedEntity->active)
    {
        self->linkedEntity->shotsFired--;
    }

    mg_destroyEntity(self, false);
}

void mg_updateTurret(mgEntity_t* self)
{
    switch (self->state)
    {
        case 0:
        default:
            self->stateTimer++;

            if (self->stateTimer > 60)
            {
                self->shotsFired = 0;
                self->stateTimer = 0;

                if (self->entityManager->playerEntity != NULL)
                {
                    self->jumpPower = getAtan2(self->entityManager->playerEntity->y - self->y,
                                               self->entityManager->playerEntity->x - self->x);
                }

                self->jumpPower = clampAngleTo8way(self->jumpPower);

                self->state = 1;
            }
            break;
        case 1 ... 3:
            self->stateTimer++;

            if (self->stateTimer > 5)
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

                if (self->state >= 4)
                {
                    self->state = 0;
                }

                self->stateTimer = 0;
            }
            break;
    }

    despawnWhenOffscreen(self);
    mg_updateInvincibilityFrames(self);
    mg_detectEntityCollisions(self);
}

int16_t clampAngleTo8way(int16_t angle)
{
    switch (angle)
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

void mg_updateCharginSchmuck(mgEntity_t* self)
{
    switch (self->state)
    {
        case 0:
        default:
            if (self->stateTimer == -69)
            {
                self->state      = 1;
                self->stateTimer = 0;

                break;
            }

            self->stateTimer++;
            if ((self->stateTimer >> 2) & 0b1)
            {
                mgEntity_t* createdEntity = mg_createEntity(self->entityManager, ENTITY_WAVE_BALL,
                                                            TO_PIXEL_COORDS(self->x), TO_PIXEL_COORDS(self->y));
                if (createdEntity != NULL)
                {
                    createdEntity->xspeed             = self->spriteFlipHorizontal ? -256 : 256;
                    createdEntity->linkedEntity       = self;
                    createdEntity->overlapTileHandler = &waveBallOverlapTileHandler;
                    createdEntity->collisionHandler   = &mg_enemySightBulletCollisionHandler;
                    createdEntity->scoreValue         = 0;
                    createdEntity->visible            = 0;
                }
            }
            break;
        case 1:

            self->stateTimer++;

            if (!(self->stateTimer % 10))
            {
                self->xspeed = (self->spriteFlipHorizontal) ? -64 : 64;
            }

            if (self->stateTimer > 180)
            {
                self->state      = 0;
                self->stateTimer = 0;
            }
            break;
    }

    mg_updateInvincibilityFrames(self);

    despawnWhenOffscreen(self);
    mg_moveEntityWithTileCollisions3(self);

    if (self->xspeed)
    {
        self->spriteFlipHorizontal = (self->xspeed > 0) ? false : true;
    }

    applyDamping(self);
    applyGravity(self);
    mg_detectEntityCollisions(self);
}

void mg_enemySightBulletCollisionHandler(mgEntity_t* self, mgEntity_t* other)
{
    if (self->linkedEntity == other)
    {
        return;
    }

    switch (other->type)
    {
        case ENTITY_PLAYER:
            if (self->linkedEntity != NULL && self->linkedEntity->active)
            {
                self->linkedEntity->stateTimer = -69;
            }
        default:
            break;
    }

    mg_destroyEntity(self, false);
}

void mg_updateBossDoor(mgEntity_t* self)
{
    switch (self->state)
    {
        case 0:
        {
            mg_detectEntityCollisions(self);
            break;
        }
        case 1:
        {
            self->stateTimer++;
            if (self->stateTimer > 16)
            {
                self->state      = 2;
                self->stateTimer = 0;
                // self->tilemap->minMapOffsetX=TO_PIXEL_COORDS(self->x) - 64;
                break;
            }

            self->y -= 64;
            break;
        }
        case 2:
        {
            self->stateTimer++;
            if (self->stateTimer > 32)
            {
                self->state                  = 3;
                self->stateTimer             = 0;
                self->tilemap->maxMapOffsetX = TO_PIXEL_COORDS(self->x) + 280;
                self->tilemap->minMapOffsetX = TO_PIXEL_COORDS(self->x) - 280;
                mg_viewFollowEntity(self->entityManager->tilemap, self->entityManager->viewEntity);
                break;
            }

            if (self->entityManager->playerEntity != NULL)
            {
                self->entityManager->playerEntity->xspeed = 32;
                self->entityManager->playerEntity->state  = MG_PL_ST_NORMAL;
            }
            break;
        }
        case 3:
        {
            self->stateTimer++;
            if (self->stateTimer > 24)
            {
                self->state      = 4;
                self->stateTimer = 0;
                break;
            }

            // self->tilemap->minMapOffsetX = TO_PIXEL_COORDS(self->x);
            // mg_viewFollowEntity(self->entityManager->tilemap, self->entityManager->viewEntity);

            self->y += 32;
            break;
        }
        default:
        {
            break;
        }
    }
}

void mg_bossDoorCollisionHandler(mgEntity_t* self, mgEntity_t* other)
{
    switch (other->type)
    {
        case ENTITY_PLAYER:
        {
            self->state = 1;
            break;
        }
        default:
        {
            break;
        }
    }
}

void mg_updateShrubbleLv4(mgEntity_t* self)
{
    if (self->gameData->frameCount % 10 == 0)
    {
        self->spriteFlipHorizontal = !self->spriteFlipHorizontal;
    }

    /*if (self->breakInfiniteLoopBounceThreshold > 0)
    {
        self->breakInfiniteLoopBounceThreshold--;
    }*/

    mg_detectEntityCollisions(self);

    uint8_t tx = MG_TO_TILECOORDS(self->x >> SUBPIXEL_RESOLUTION);
    uint8_t ty = MG_TO_TILECOORDS(self->y >> SUBPIXEL_RESOLUTION);
    uint8_t t;

    switch (self->animationTimer)
    {
        case CRAWLER_NONE:
        {
            if (self->spawnData != NULL)
            {
                self->animationTimer
                    = mg_crawlerGettInitialMoveState(self->spriteRotateAngle, (bool)self->spawnData->special2);
                self->xspeed = (self->spawnData->special3);

                crawlerSetMoveState(self, self->animationTimer);
            }
            break;
        }
        // CLOCKWISE
        case CRAWLER_TOP_TO_RIGHT: // On top of a block, going right
            if (((self->x % (MG_TILESIZE << SUBPIXEL_RESOLUTION)) - (MG_HALF_TILESIZE << SUBPIXEL_RESOLUTION)))
            {
                break;
            }

            t = mg_getTile(self->tilemap, tx, ty + 1);
            if (!mg_isSolid(t))
            {
                crawlerSetMoveState(self, CRAWLER_RIGHT_TO_BOTTOM);
                // self->bouncesOffUnbreakableBlocks++;
            }
            else
            {
                // self->bouncesOffUnbreakableBlocks = 0;
            }

            t = mg_getTile(self->tilemap, tx + 1, ty);
            if (mg_isSolid(t))
            {
                crawlerSetMoveState(self, CRAWLER_LEFT_TO_TOP);
            }

            break;

        case CRAWLER_RIGHT_TO_BOTTOM: // On the right side of a block, going down
            if (((self->y % (MG_TILESIZE << SUBPIXEL_RESOLUTION)) - (MG_HALF_TILESIZE << SUBPIXEL_RESOLUTION)))
            {
                break;
            }

            t = mg_getTile(self->tilemap, tx - 1, ty);
            if (!mg_isSolid(t))
            {
                crawlerSetMoveState(self, CRAWLER_BOTTOM_TO_LEFT);
                // self->bouncesOffUnbreakableBlocks++;
            }
            else
            {
                // self->bouncesOffUnbreakableBlocks = 0;
            }
            t = mg_getTile(self->tilemap, tx, ty + 1);
            if (mg_isSolid(t))
            {
                crawlerSetMoveState(self, CRAWLER_TOP_TO_RIGHT);
            }

            break;

        case CRAWLER_BOTTOM_TO_LEFT: // On the bottom of a block, going left
            if (((self->x % (MG_TILESIZE << SUBPIXEL_RESOLUTION)) - (MG_HALF_TILESIZE << SUBPIXEL_RESOLUTION)))
            {
                break;
            }

            t = mg_getTile(self->tilemap, tx, ty - 1);
            if (!mg_isSolid(t))
            {
                crawlerSetMoveState(self, CRAWLER_LEFT_TO_TOP);
                // self->bouncesOffUnbreakableBlocks++;
            }
            else
            {
                // self->bouncesOffUnbreakableBlocks = 0;
            }
            t = mg_getTile(self->tilemap, tx - 1, ty);
            if (mg_isSolid(t))
            {
                crawlerSetMoveState(self, CRAWLER_RIGHT_TO_BOTTOM);
            }

            break;

        case CRAWLER_LEFT_TO_TOP: // On the left side of a block, going up
            if (((self->y % (MG_TILESIZE << SUBPIXEL_RESOLUTION)) - (MG_HALF_TILESIZE << SUBPIXEL_RESOLUTION)))
            {
                break;
            }

            t = mg_getTile(self->tilemap, tx + 1, ty);
            if (!mg_isSolid(t))
            {
                crawlerSetMoveState(self, CRAWLER_TOP_TO_RIGHT);
                // self->bouncesOffUnbreakableBlocks++;
            }
            else
            {
                // self->bouncesOffUnbreakableBlocks = 0;
            }
            t = mg_getTile(self->tilemap, tx, ty - 1);
            if (mg_isSolid(t))
            {
                crawlerSetMoveState(self, CRAWLER_BOTTOM_TO_LEFT);
            }

            break;

        // COUNTER-CLOCKWISE
        case CRAWLER_TOP_TO_LEFT: // On top of a block, going left
            if (((self->x % (MG_TILESIZE << SUBPIXEL_RESOLUTION)) - (MG_HALF_TILESIZE << SUBPIXEL_RESOLUTION)))
            {
                break;
            }

            t = mg_getTile(self->tilemap, tx, ty + 1);
            if (!mg_isSolid(t))
            {
                crawlerSetMoveState(self, CRAWLER_LEFT_TO_BOTTOM);
                // self->bouncesOffUnbreakableBlocks++;
            }
            else
            {
                // self->bouncesOffUnbreakableBlocks = 0;
            }
            t = mg_getTile(self->tilemap, tx - 1, ty);
            if (mg_isSolid(t))
            {
                crawlerSetMoveState(self, CRAWLER_RIGHT_TO_TOP);
            }

            break;

        case CRAWLER_LEFT_TO_BOTTOM: // On the left side of a block, going down
            if (((self->y % (MG_TILESIZE << SUBPIXEL_RESOLUTION)) - (MG_HALF_TILESIZE << SUBPIXEL_RESOLUTION)))
            {
                break;
            }

            t = mg_getTile(self->tilemap, tx + 1, ty);
            if (!mg_isSolid(t))
            {
                crawlerSetMoveState(self, CRAWLER_BOTTOM_TO_RIGHT);
                // self->bouncesOffUnbreakableBlocks++;
            }
            else
            {
                // self->bouncesOffUnbreakableBlocks = 0;
            }
            t = mg_getTile(self->tilemap, tx, ty + 1);
            if (mg_isSolid(t))
            {
                crawlerSetMoveState(self, CRAWLER_TOP_TO_LEFT);
            }

            break;

        case CRAWLER_BOTTOM_TO_RIGHT: // On the bottom of a block, going right
            if (((self->x % (MG_TILESIZE << SUBPIXEL_RESOLUTION)) - (MG_HALF_TILESIZE << SUBPIXEL_RESOLUTION)))
            {
                break;
            }

            t = mg_getTile(self->tilemap, tx, ty - 1);
            if (!mg_isSolid(t))
            {
                crawlerSetMoveState(self, CRAWLER_RIGHT_TO_TOP);
                // self->bouncesOffUnbreakableBlocks++;
            }
            else
            {
                // self->bouncesOffUnbreakableBlocks = 0;
            }

            t = mg_getTile(self->tilemap, tx + 1, ty);
            if (mg_isSolid(t))
            {
                crawlerSetMoveState(self, CRAWLER_LEFT_TO_BOTTOM);
            }

            break;

        case CRAWLER_RIGHT_TO_TOP: // On the right side of a block, going up
            if (((self->y % (MG_TILESIZE << SUBPIXEL_RESOLUTION)) - (MG_HALF_TILESIZE << SUBPIXEL_RESOLUTION)))
            {
                break;
            }

            t = mg_getTile(self->tilemap, tx - 1, ty);
            if (!mg_isSolid(t))
            {
                crawlerSetMoveState(self, CRAWLER_TOP_TO_LEFT);
                // self->bouncesOffUnbreakableBlocks++;
            }
            else
            {
                // self->bouncesOffUnbreakableBlocks = 0;
            }

            t = mg_getTile(self->tilemap, tx, ty - 1);
            if (mg_isSolid(t))
            {
                crawlerSetMoveState(self, CRAWLER_BOTTOM_TO_RIGHT);
            }
            break;

        default:
            break;
    }

    /*if (self->bouncesOffUnbreakableBlocks > 4)
    {
        scorePoints(self->gameData, 100, 1);
        explodeBomb(self);
    }*/

    /*if (isOutsidePlayfield(self))
    {
        destroyEntity(self, false);
    }*/

    mg_updateInvincibilityFrames(self);
    despawnWhenOffscreen(self);

    self->x += self->xspeed;
    self->y += self->yspeed;
}

void crawlerSetMoveState(mgEntity_t* self, uint8_t state)
{
    int16_t baseSpeed = abs((self->xspeed != 0) ? self->xspeed : self->yspeed);

    switch (state)
    {
        // CLOCKWISE
        case CRAWLER_TOP_TO_RIGHT: // On top of a block, going right
            self->xspeed            = baseSpeed;
            self->yspeed            = 0;
            self->spriteRotateAngle = 0;
            self->spriteIndex       = MG_SP_CRAWLER_TOP;
            break;
        case CRAWLER_RIGHT_TO_BOTTOM: // On the right side of a block, going down
            self->yspeed            = baseSpeed;
            self->xspeed            = 0;
            self->spriteRotateAngle = 90;
            self->spriteIndex       = MG_SP_CRAWLER_RIGHT;
            break;
        case CRAWLER_BOTTOM_TO_LEFT: // On the bottom of a block, going left
            self->xspeed            = -baseSpeed;
            self->yspeed            = 0;
            self->spriteRotateAngle = 180;
            self->spriteIndex       = MG_SP_CRAWLER_BOTTOM;
            break;
        case CRAWLER_LEFT_TO_TOP: // On the left side of a block, going up
            self->yspeed            = -baseSpeed;
            self->xspeed            = 0;
            self->spriteRotateAngle = 270;
            self->spriteIndex       = MG_SP_CRAWLER_LEFT;
            break;
        // COUNTER-CLOCKWISE
        case CRAWLER_TOP_TO_LEFT: // On top of a block, going left
            self->xspeed            = -baseSpeed;
            self->yspeed            = 0;
            self->spriteRotateAngle = 0;
            self->spriteIndex       = MG_SP_CRAWLER_TOP;
            break;
        case CRAWLER_LEFT_TO_BOTTOM: // On the left side of a block, going down
            self->yspeed            = baseSpeed;
            self->xspeed            = 0;
            self->spriteRotateAngle = 270;
            self->spriteIndex       = MG_SP_CRAWLER_LEFT;
            break;
        case CRAWLER_BOTTOM_TO_RIGHT: // On the bottom of a block, going right
            self->xspeed            = baseSpeed;
            self->yspeed            = 0;
            self->spriteRotateAngle = 180;
            self->spriteIndex       = MG_SP_CRAWLER_BOTTOM;
            break;
        case CRAWLER_RIGHT_TO_TOP: // On the right side of a block, going up
            self->yspeed            = -baseSpeed;
            self->xspeed            = 0;
            self->spriteRotateAngle = 90;
            self->spriteIndex       = MG_SP_CRAWLER_RIGHT;
            break;
        default:
            break;
    }

    self->animationTimer = state;
}

uint8_t mg_crawlerGettInitialMoveState(int16_t angle, bool clockwise)
{
    switch (angle)
    {
        case 0 ... 22:
        default:
            return clockwise ? CRAWLER_TOP_TO_RIGHT : CRAWLER_TOP_TO_LEFT;
        case 23 ... 67:
            return clockwise ? CRAWLER_TOP_TO_RIGHT : CRAWLER_TOP_TO_LEFT;
        case 68 ... 112:
            return clockwise ? CRAWLER_RIGHT_TO_BOTTOM : CRAWLER_LEFT_TO_BOTTOM;
        case 113 ... 157:
            return clockwise ? CRAWLER_RIGHT_TO_BOTTOM : CRAWLER_LEFT_TO_BOTTOM;
        case 158 ... 202:
            return clockwise ? CRAWLER_BOTTOM_TO_LEFT : CRAWLER_BOTTOM_TO_RIGHT;
        case 203 ... 247:
            return clockwise ? CRAWLER_BOTTOM_TO_LEFT : CRAWLER_BOTTOM_TO_RIGHT;
        case 248 ... 292:
            return clockwise ? CRAWLER_LEFT_TO_TOP : CRAWLER_RIGHT_TO_TOP;
        case 293 ... 337:
            return clockwise ? CRAWLER_LEFT_TO_TOP : CRAWLER_RIGHT_TO_TOP;
        case 338 ... 359:
            return clockwise ? CRAWLER_TOP_TO_RIGHT : CRAWLER_TOP_TO_LEFT;
    }
}

void mg_updateBossSeverYataga(mgEntity_t* self)
{
    switch (self->state)
    {
        case 65535:
            return;
        case 0:
        default:
            self->spriteIndex          = severYatagaFlyingFrames[(self->stateTimer >> 2) % 4];
            self->spriteFlipHorizontal = (self->entityManager->playerEntity->x > self->x) ? false : true;

            if (TO_PIXEL_COORDS(self->y) > self->tilemap->mapOffsetY + 64)
            {
                self->yspeed -= 4;
            }

            if (TO_PIXEL_COORDS(self->x) < self->tilemap->mapOffsetX + 160)
            {
                self->xspeed = 64;
            }

            if (self->stateTimer == 120)
            {
                self->jumpPower = 1;
            }

            self->stateTimer++;
            if (self->stateTimer > 180)
            {
                self->stateTimer = 0;
                switch (esp_random() % 3)
                {
                    case 0:
                        self->state = 1;
                        break;
                    case 1:
                        self->state = 2;
                        break;
                    case 2:
                        self->state = 3;
                        break;
                }
            }
            break;
        case 1:
            self->spriteIndex          = severYatagaFlyingFrames[(self->stateTimer >> 2) % 4];
            self->spriteFlipHorizontal = (self->entityManager->playerEntity->x > self->x) ? false : true;

            if (TO_PIXEL_COORDS(self->y) > self->tilemap->mapOffsetY + 64)
            {
                self->yspeed -= 4;
            }

            if (TO_PIXEL_COORDS(self->x) > self->tilemap->mapOffsetX + 120)
            {
                self->xspeed = -64;
            }

            if (self->stateTimer == 120)
            {
                self->jumpPower = 1;
            }

            self->stateTimer++;
            if (self->stateTimer > 180)
            {
                self->stateTimer = 0;
                switch (esp_random() % 3)
                {
                    case 0:
                        self->state = 0;
                        break;
                    case 1:
                        self->state = 2;
                        break;
                    case 2:
                        self->state = 3;
                        break;
                }
            }

            break;
        case 2:
            self->spriteIndex          = severYatagaFlyingFrames[(self->stateTimer >> 1) % 4];
            self->spriteFlipHorizontal = (self->entityManager->playerEntity->x > self->x) ? false : true;

            if (self->stateTimer < 60)
            {
                if (self->x < self->entityManager->playerEntity->x)
                {
                    self->xspeed = 32;
                }
                else
                {
                    self->xspeed = -32;
                }
            }

            if (!self->falling || self->y > self->entityManager->playerEntity->y)
            {
                self->state      = ((esp_random() % 100) > 50) ? 0 : 1;
                self->stateTimer = 0;
            }

            self->stateTimer++;

            break;
        case 3:
            self->stateTimer++;

            if (!((self->stateTimer + 8) % 60))
            {
                self->spriteIndex = MG_SP_BOSS_2;
            }
            else if (!((self->stateTimer + 4) % 60))
            {
                self->spriteIndex = MG_SP_BOSS_3;
            }
            else if (!(self->stateTimer % 60))
            {
                self->spriteIndex = MG_SP_BOSS_4;
                self->jumpPower   = 1;
            }
            else if (!((self->stateTimer - 8) % 60))
            {
                self->spriteIndex = MG_SP_BOSS_5;
            }
            else if (!((self->stateTimer - 16) % 60))
            {
                self->spriteIndex = MG_SP_BOSS_3;
            }

            if (self->stateTimer > 239)
            {
                self->state      = ((esp_random() % 100) > 50) ? 0 : 1;
                self->stateTimer = 0;
            }
            break;
    }

    if (self->jumpPower > 0)
    {
        mgEntity_t* createdEntity = mg_createEntity(self->entityManager, ENTITY_WAVE_BALL, TO_PIXEL_COORDS(self->x),
                                                    TO_PIXEL_COORDS(self->y));
        if (createdEntity != NULL)
        {
            int16_t angle = getAtan2(self->entityManager->playerEntity->y - self->y,
                                     self->entityManager->playerEntity->x - self->x);
            int16_t sin   = getSin1024(angle);
            int16_t cos   = getCos1024(angle);

            createdEntity->xspeed = (80 * cos) / 1024;
            createdEntity->yspeed = (80 * sin) / 1024;

            createdEntity->linkedEntity = self;
            soundPlaySfx(&(self->soundManager->sndWaveBall), BZR_LEFT);
            self->jumpPower = 0;
        }
    }

    mg_updateInvincibilityFrames(self);
    mg_moveEntityWithTileCollisions3(self);
    applyDamping(self);
    applyGravity(self);
    mg_detectEntityCollisions(self);

    if (self->type == ENTITY_DEAD && self->linkedEntity == NULL && self->gameData->level != 11)
    {
        if (self->gameData->level != 11)
        {
            self->spriteIndex  = MG_SP_BOSS_6;
            self->linkedEntity = createMixtape(self->entityManager, TO_PIXEL_COORDS(self->x), TO_PIXEL_COORDS(self->y));
            startOutroCutscene(self);
        }
    }
    despawnWhenOffscreen(self);
}

void mg_updateBossSmashGorilla(mgEntity_t* self)
{
    switch (self->state)
    {
        case 65535:
            // Prefight
            return;
        case 0:
        default:
            // Idle
            self->stateTimer++;
            if (self->stateTimer > 60)
            {
                self->stateTimer = 0;
                switch (esp_random() % 2)
                {
                    case 0:
                    default:
                        // To "Traverse - charge across"
                        self->state = 1;

                        if (TO_PIXEL_COORDS(self->x) > self->tilemap->mapOffsetX + 120)
                        {
                            self->spriteFlipHorizontal = true;
                        }
                        else
                        {
                            self->spriteFlipHorizontal = false;
                        }

                        break;
                        // case 1:
                        // To "Attack - Large ground projectiles"
                        //    self->state = 1;
                        //    break;
                }
            }
            break;
        case 1:
            //"Traverse - charge across"

            // Move to other side of screen
            if (self->spriteFlipHorizontal)
            {
                self->xspeed = -64;
            }
            else
            {
                self->xspeed = 64;
            }

            // Reached other side of screen...
            if ((self->xspeed < 0
                 && (TO_PIXEL_COORDS(self->x)
                     < self->tilemap->mapOffsetX + 64 /* adjust this number based on collision box size */))
                || (self->xspeed > 0
                    && (TO_PIXEL_COORDS(self->x)
                        > self->tilemap->mapOffsetX
                              + 176 /*240 - 64*/ /* adjust this number based on collision box size */)))
            {
                self->stateTimer = 0;
                // To "Attack - Large ground projectiles"
                self->state = 2;
            }

            // failsafe
            self->stateTimer++;
            if (self->stateTimer > 180)
            {
                self->stateTimer = 0;
                // To "Attack - Large ground projectiles"
                self->state = 2;
            }

            break;
        case 2:
            //"Attack - Large ground projectiles"
            self->stateTimer++;

            if (!(self->stateTimer % 20))
            {
                // Launch projectiles toward player
                mgEntity_t* createdEntity = mg_createEntity(self->entityManager, ENTITY_WAVE_BALL,
                                                            TO_PIXEL_COORDS(self->x), TO_PIXEL_COORDS(self->y));
                if (createdEntity != NULL && self->entityManager->playerEntity != NULL)
                {
                    createdEntity->xspeed = (self->entityManager->playerEntity->x > self->x) ? 64 : -64;
                    createdEntity->yspeed = 0;

                    // TODO: make sprite larger, change into rocks?

                    createdEntity->linkedEntity = self;
                    soundPlaySfx(&(self->soundManager->sndWaveBall), BZR_LEFT);
                }
            }

            if (self->stateTimer > 180)
            {
                self->stateTimer = 0;
                switch (esp_random() % 2)
                {
                    case 0:
                    default:
                        // To Idle
                        self->state = 0;
                        break;
                    case 1:
                        // To "Traverse - Dig and disappear"
                        self->state = 3;
                        break;
                }
            }
            break;
        case 3:
            //"Traverse - Dig and disappear"
            self->stateTimer++;

            if (self->stateTimer > 60)
            {
                self->visible    = false;
                self->stateTimer = 0;
                // To "Traverse - Dig in from random location, track player"
                self->state = 4;
                break;
            }
            break;
        case 4:
            //"Traverse - Dig in from random location, track player"
            self->stateTimer++;

            if (self->entityManager->playerEntity == NULL || self->stateTimer > 600)
            {
                self->visible    = true;
                self->stateTimer = 0;

                // To "Attack - Emerge, Launch rocks"
                self->state = 5;
                break;
            }

            if (!(self->stateTimer % 20))
            {
                if (self->x < self->entityManager->playerEntity->x)
                {
                    self->xspeed = 64;
                }
                else
                {
                    self->xspeed = -64;
                }
            }

            break;
        case 5:
            //"Attack - Emerge, launch rocks"
            self->stateTimer++;
            self->yspeed = -64;

            if (!(self->stateTimer % 8))
            {
                mgEntity_t* createdEntity = mg_createEntity(self->entityManager, ENTITY_WAVE_BALL,
                                                            TO_PIXEL_COORDS(self->x), TO_PIXEL_COORDS(self->y));
                if (createdEntity != NULL && self->entityManager->playerEntity != NULL)
                {
                    int16_t angle = -60 + (esp_random() % 120);
                    int16_t sin   = getSin1024(angle);
                    int16_t cos   = getCos1024(angle);

                    createdEntity->xspeed         = (80 * cos) / 1024;
                    createdEntity->yspeed         = (80 * sin) / 1024;
                    createdEntity->gravityEnabled = true;

                    // TODO: make sprite larger, change into rocks?

                    createdEntity->linkedEntity = self;
                    soundPlaySfx(&(self->soundManager->sndWaveBall), BZR_LEFT);
                }
            }

            if (self->stateTimer > 30 && !self->falling)
            {
                self->stateTimer = 0;
                // To Idle
                self->state = 0;
                break;
            }
            break;
    }

    if (self->jumpPower > 0)
    {
        mgEntity_t* createdEntity = mg_createEntity(self->entityManager, ENTITY_WAVE_BALL, TO_PIXEL_COORDS(self->x),
                                                    TO_PIXEL_COORDS(self->y));
        if (createdEntity != NULL)
        {
            int16_t angle = getAtan2(self->entityManager->playerEntity->y - self->y,
                                     self->entityManager->playerEntity->x - self->x);
            int16_t sin   = getSin1024(angle);
            int16_t cos   = getCos1024(angle);

            createdEntity->xspeed = (80 * cos) / 1024;
            createdEntity->yspeed = (80 * sin) / 1024;

            createdEntity->linkedEntity = self;
            soundPlaySfx(&(self->soundManager->sndWaveBall), BZR_LEFT);
            self->jumpPower = 0;
        }
    }

    mg_updateInvincibilityFrames(self);
    mg_moveEntityWithTileCollisions3(self);
    applyDamping(self);
    applyGravity(self);
    mg_detectEntityCollisions(self);

    if (self->type == ENTITY_DEAD && self->linkedEntity == NULL && self->gameData->level != 11)
    {
        self->linkedEntity = createMixtape(self->entityManager, TO_PIXEL_COORDS(self->x), TO_PIXEL_COORDS(self->y));
        // after defeating smash gorilla (level 6) create a bunch of power ups to try out can of salsa.
        for (int i = 1; i < 5; i++)
        {
            createPowerUp(self->entityManager, TO_PIXEL_COORDS(self->x) - 25 * i, TO_PIXEL_COORDS(self->y));
            createPowerUp(self->entityManager, TO_PIXEL_COORDS(self->x) + 25 * i, TO_PIXEL_COORDS(self->y));
        }
        startOutroCutscene(self);
    }
    despawnWhenOffscreen(self);
}

void mg_updateBossGrindPangolin(mgEntity_t* self)
{
    switch (self->state)
    {
        case 65535:
            // Prefight
            return;
        case 0:
        default:
            // Idle
            applyDamping(self);
            self->spriteFlipHorizontal = (self->entityManager->playerEntity->x > self->x) ? false : true;

            self->stateTimer++;
            if (self->stateTimer > 60)
            {
                self->stateTimer = 0;
                switch (esp_random() % 2)
                {
                    case 0:
                    default:
                        self->state = 1; // To "Charge"
                        break;
                    case 1:
                        self->xspeed = SIGNOF(self->entityManager->playerEntity->x - self->x)
                                       * (abs(self->x - self->entityManager->playerEntity->x) >> 6);
                        self->yspeed      = -96;
                        self->falling     = true;
                        self->spriteIndex = MG_SP_BOSS_1;
                        self->state       = 3; // To "Traverse - Jump towards player"
                        break;
                }
            }
            break;
        case 1:
            // Charge

            if (self->spriteIndex < MG_SP_BOSS_3)
            {
                if (!(self->stateTimer % 7))
                {
                    self->spriteIndex++;
                }
            }
            else
            {
                self->spriteIndex
                    = grindPangolinRollingFrames[(self->stateTimer >> ((self->stateTimer > 120) ? 1 : 2)) % 2];
                self->tileCollider = &entityTileCollider_grind_pangolin_rolling;
            }

            self->stateTimer++;
            if (self->stateTimer > 180)
            {
                self->stateTimer = 0;
                self->xspeed     = ((self->spriteFlipHorizontal) ? -96 : 96);
                self->state      = 2; // To "Roll"
            }

            break;
        case 2:
            // Roll

            self->spriteIndex          = grindPangolinRollingFrames[(self->stateTimer) % 2];
            self->spriteFlipHorizontal = (self->xspeed > 0) ? false : true;

            if (self->stateTimer > 120)
            {
                applyDamping(self);
            }

            if (self->stateTimer > 180)
            {
                self->tileCollider = &entityTileCollider_grind_pangolin;
                self->y -= 384;
                self->xspeed = SIGNOF(self->entityManager->playerEntity->x - self->x)
                               * (abs(self->x - self->entityManager->playerEntity->x) >> 6);
                self->yspeed      = -96;
                self->falling     = true;
                self->spriteIndex = MG_SP_BOSS_1;
                self->state       = 3; // To "Traverse - Jump towards player"
            }

            self->stateTimer++;

            break;
        case 3:
            // Jump towards player
            if (!self->falling)
            {
                self->spriteIndex          = MG_SP_BOSS_5;
                self->state                = 4; // TO "Attack - Tailwhip"
                self->spriteFlipHorizontal = (self->entityManager->playerEntity->x > self->x) ? false : true;
                self->stateTimer           = 0;
            }

            self->stateTimer++;

            // failsafe
            if (self->stateTimer > 239)
            {
                self->state       = 0;
                self->spriteIndex = MG_SP_BOSS_0;
                self->stateTimer  = 0;
            }
            break;
        case 4:
            applyDamping(self);
            self->stateTimer++;

            if (self->stateTimer > 60)
            {
                self->state = 0;
                self->x -= (self->spriteFlipHorizontal) ? -(32 << SUBPIXEL_RESOLUTION) : (32 << SUBPIXEL_RESOLUTION);
                self->spriteIndex = MG_SP_BOSS_0;
                self->stateTimer  = 0;
            }

            if (self->stateTimer == 30 && self->spriteIndex != MG_SP_BOSS_6)
            {
                self->spriteIndex = MG_SP_BOSS_6;
                self->x += (self->spriteFlipHorizontal) ? -(32 << SUBPIXEL_RESOLUTION) : (32 << SUBPIXEL_RESOLUTION);
                self->xspeed = 0;
            }

            break;
    }

    mg_updateInvincibilityFrames(self);
    mg_moveEntityWithTileCollisions3(self);
    // applyDamping(self);
    applyGravity(self);
    mg_detectEntityCollisions(self);

    if (self->type == ENTITY_DEAD && self->linkedEntity == NULL && self->gameData->level != 11)
    {
        self->spriteIndex  = MG_SP_BOSS_7;
        self->linkedEntity = createMixtape(self->entityManager, TO_PIXEL_COORDS(self->x), TO_PIXEL_COORDS(self->y));
        startOutroCutscene(self);
    }
    despawnWhenOffscreen(self);
}

void mg_updateBossDrainBat(mgEntity_t* self)
{
    switch (self->state)
    {
        case 65535:
            return;
        case 0:
        default:
            self->stateTimer++;

            // if(!self->invincibilityFrames)
            {
                self->spriteIndex = drainBatAnimFrames[(self->stateTimer >> 3) % 5];
            }

            if (self->stateTimer == 30)
            {
                self->invincibilityFrames = 60;
            }

            if (self->stateTimer > 60)
            {
                self->stateTimer = 0;

                switch (esp_random() % 2)
                {
                    case 0:
                    default:
                        self->x     = TO_SUBPIXEL_COORDS(self->tilemap->mapOffsetX + 48 + (esp_random() % 184));
                        self->y     = self->entityManager->playerEntity->y - 128;
                        self->state = 1;
                        break;
                    case 1:
                        self->x     = (TO_PIXEL_COORDS(self->x) > self->tilemap->mapOffsetX + 120)
                                          ? TO_SUBPIXEL_COORDS(self->tilemap->mapOffsetX + 48)
                                          : TO_SUBPIXEL_COORDS(self->tilemap->mapOffsetX + 232);
                        self->y     = (TO_SUBPIXEL_COORDS(self->tilemap->mapOffsetY + 48));
                        self->state = 2;
                        break;
                }
            }
            break;
        case 1:
            self->stateTimer++;

            if (self->stateTimer > 15 && self->stateTimer < 30)
            {
                self->spriteFlipHorizontal = (self->entityManager->playerEntity->x > self->x) ? false : true;
            }

            if (self->stateTimer == 30)
            {
                mgEntity_t* createdEntity;
                int16_t angle;
                int16_t sin;
                int16_t cos;

                createdEntity = mg_createEntity(self->entityManager, ENTITY_WAVE_BALL, TO_PIXEL_COORDS(self->x),
                                                TO_PIXEL_COORDS(self->y));
                if (createdEntity != NULL)
                {
                    angle = (self->entityManager->playerEntity->x > self->x) ? 350 : 190;
                    sin   = getSin1024(angle);
                    cos   = getCos1024(angle);

                    createdEntity->xspeed       = (80 * cos) / 1024;
                    createdEntity->yspeed       = (80 * -sin) / 1024;
                    createdEntity->linkedEntity = self;
                }

                createdEntity = mg_createEntity(self->entityManager, ENTITY_WAVE_BALL, TO_PIXEL_COORDS(self->x),
                                                TO_PIXEL_COORDS(self->y));
                if (createdEntity != NULL)
                {
                    angle = (self->entityManager->playerEntity->x > self->x) ? 30 : 150;
                    sin   = getSin1024(angle);
                    cos   = getCos1024(angle);

                    createdEntity->xspeed       = (80 * cos) / 1024;
                    createdEntity->yspeed       = (80 * -sin) / 1024;
                    createdEntity->linkedEntity = self;
                }
            }

            if (self->stateTimer > 60)
            {
                self->stateTimer = 0;
                self->state      = 0;
            }
            break;

        case 2:

            self->stateTimer++;

            if (self->stateTimer > 15 && self->stateTimer < 30)
            {
                self->spriteFlipHorizontal = (self->entityManager->playerEntity->x > self->x) ? false : true;
            }

            if (self->stateTimer == 30)
            {
                mgEntity_t* createdEntity;
                createdEntity = mg_createEntity(self->entityManager, ENTITY_WAVE_BALL, TO_PIXEL_COORDS(self->x),
                                                TO_PIXEL_COORDS(self->y));
                if (createdEntity != NULL)
                {
                    createdEntity->xspeed = (TO_PIXEL_COORDS(self->x) > self->tilemap->mapOffsetX + 120) ? -128 : 128;
                    createdEntity->gravityEnabled = true;
                    createdEntity->falling        = true;
                    createdEntity->linkedEntity   = self;
                }
                createdEntity = mg_createEntity(self->entityManager, ENTITY_WAVE_BALL, TO_PIXEL_COORDS(self->x),
                                                TO_PIXEL_COORDS(self->y));
                if (createdEntity != NULL)
                {
                    createdEntity->xspeed = (TO_PIXEL_COORDS(self->x) > self->tilemap->mapOffsetX + 120) ? -96 : 96;
                    createdEntity->gravityEnabled = true;
                    createdEntity->falling        = true;
                    createdEntity->linkedEntity   = self;
                }
                createdEntity = mg_createEntity(self->entityManager, ENTITY_WAVE_BALL, TO_PIXEL_COORDS(self->x),
                                                TO_PIXEL_COORDS(self->y));
                if (createdEntity != NULL)
                {
                    createdEntity->xspeed = (TO_PIXEL_COORDS(self->x) > self->tilemap->mapOffsetX + 120) ? -64 : 64;
                    createdEntity->gravityEnabled = true;
                    createdEntity->falling        = true;
                    createdEntity->linkedEntity   = self;
                }
                createdEntity = mg_createEntity(self->entityManager, ENTITY_WAVE_BALL, TO_PIXEL_COORDS(self->x),
                                                TO_PIXEL_COORDS(self->y));
                if (createdEntity != NULL)
                {
                    createdEntity->xspeed = (TO_PIXEL_COORDS(self->x) > self->tilemap->mapOffsetX + 120) ? -32 : 32;
                    createdEntity->gravityEnabled = true;
                    createdEntity->falling        = true;
                    createdEntity->linkedEntity   = self;
                }
                createdEntity = mg_createEntity(self->entityManager, ENTITY_WAVE_BALL, TO_PIXEL_COORDS(self->x),
                                                TO_PIXEL_COORDS(self->y));
                if (createdEntity != NULL)
                {
                    createdEntity->gravityEnabled = true;
                    createdEntity->falling        = true;
                    createdEntity->linkedEntity   = self;
                }
            }

            if (self->stateTimer > 60)
            {
                self->stateTimer = 0;
                self->state      = 0;
            }
            break;
    }

    mg_updateInvincibilityFrames(self);
    mg_moveEntityWithTileCollisions3(self);
    applyDamping(self);
    applyGravity(self);
    mg_detectEntityCollisions(self);

    if (self->type == ENTITY_DEAD && self->linkedEntity == NULL && self->gameData->level != 11)
    {
        self->spriteIndex = MG_SP_BOSS_5;
        mg_deactivateAllEntities(self->entityManager, true);
        self->active       = true;
        self->linkedEntity = createMixtape(self->entityManager, TO_PIXEL_COORDS(self->x), TO_PIXEL_COORDS(self->y));
        startOutroCutscene(self);
    }
    despawnWhenOffscreen(self);
}

void mg_updateBossKineticDonut(mgEntity_t* self)
{
    switch (self->state)
    {
        case 65535:
            // Prefight
            return;
        case 0:
        default:
            // Idle
            applyDamping(self);

            self->spriteIndex          = kineticDonutIdleFrames[(self->stateTimer >> 2) % 3];
            self->spriteFlipHorizontal = (self->entityManager->playerEntity->x > self->x) ? false : true;

            self->stateTimer++;
            if (self->stateTimer > 30)
            {
                self->stateTimer = 0;
                switch (esp_random() % 2)
                {
                    case 0:
                    default:
                        self->state      = 1; // To "Charge"
                        self->stateTimer = 32;
                        break;
                    case 1:
                        self->xspeed = SIGNOF(self->entityManager->playerEntity->x - self->x)
                                       * (abs(self->entityManager->playerEntity->x - self->x) >> 5);
                        self->yspeed      = -128;
                        self->falling     = true;
                        self->spriteIndex = MG_SP_BOSS_3;
                        self->state       = 3; // To "Traverse - Jump towards player"
                        self->stateTimer  = -500;
                        break;
                }
            }
            break;
        case 1:
            // Charge
            if (self->stateTimer < 96)
            {
                self->spriteIndex
                    = kineticDonutChargeFrames[(self->stateTimer >> ((self->stateTimer > 60) ? 1 : 2)) % 2];
            }
            else
            {
                self->spriteIndex = kineticDonutTeleportFrames[(self->stateTimer) % 2];
                self->spriteFlipHorizontal
                    = (TO_PIXEL_COORDS(self->x) > self->tilemap->mapOffsetX + 120) ? true : false;
            }

            self->stateTimer++;
            if (self->stateTimer > 112)
            {
                self->stateTimer = 0;
                self->visible    = false;

                mgEntity_t* createdEntity;
                createdEntity = mg_createEntity(self->entityManager, ENTITY_WAVE_BALL, TO_PIXEL_COORDS(self->x),
                                                TO_PIXEL_COORDS(self->y));
                if (createdEntity != NULL)
                {
                    createdEntity->xspeed       = (self->spriteFlipHorizontal) ? -96 : 96;
                    createdEntity->linkedEntity = self;
                }

                createdEntity = mg_createEntity(self->entityManager, ENTITY_WAVE_BALL, TO_PIXEL_COORDS(self->x),
                                                TO_PIXEL_COORDS(self->y));
                if (createdEntity != NULL)
                {
                    createdEntity->xspeed       = (self->spriteFlipHorizontal) ? -24 : 24;
                    createdEntity->linkedEntity = self;
                }

                if (self->hp > 12)
                {
                    self->x = (TO_PIXEL_COORDS(self->x) > self->tilemap->mapOffsetX + 120)
                                  ? TO_SUBPIXEL_COORDS(self->tilemap->mapOffsetX + 48)
                                  : TO_SUBPIXEL_COORDS(self->tilemap->mapOffsetX + 232);
                }
                else
                {
                    self->x = TO_SUBPIXEL_COORDS(self->tilemap->mapOffsetX + 48 + (esp_random() % 184));
                }

                self->state = 2; // To "Teleport"
            }

            break;
        case 2:
            // Teleport
            if (self->stateTimer > 120)
            {
                self->state      = 0;
                self->stateTimer = 0;

                // self->xspeed = SIGNOF(self->entityManager->playerEntity->x - self->x) *
                // (abs(self->entityManager->playerEntity->x - self->x) >> 5); self->yspeed = -128; self->falling =
                // true; self->spriteIndex = MG_SP_BOSS_3; self->state = 3; //To "Traverse - Jump towards player"
                // self->stateTimer = -500;
            }
            else if (self->stateTimer > 60)
            {
                self->visible              = true;
                self->spriteIndex          = kineticDonutTeleportFrames[(self->stateTimer) % 2];
                self->spriteFlipHorizontal = (self->entityManager->playerEntity->x > self->x) ? false : true;
            }

            self->stateTimer++;

            break;
        case 3:
            // Jump towards player
            if (self->yspeed > 0)
            {
                // self->spriteIndex = MG_SP_BOSS_5;
                self->xspeed               = 0;
                self->yspeed               = 0;
                self->gravityEnabled       = false;
                self->state                = 4; // TO "Attack - Tailwhip"
                self->spriteFlipHorizontal = (self->entityManager->playerEntity->x > self->x) ? false : true;
                self->stateTimer           = 0;
            }

            self->stateTimer++;

            // failsafe
            if (self->stateTimer > 239)
            {
                self->state       = 0;
                self->spriteIndex = MG_SP_BOSS_0;
                self->stateTimer  = 0;
            }
            break;
        case 4:
            applyDamping(self);
            self->stateTimer++;
            self->spriteIndex = kineticDonutChargeFrames[(self->stateTimer >> ((self->stateTimer > 30) ? 1 : 2)) % 2];

            if (self->stateTimer > 60)
            {
                self->state          = 5;
                self->spriteIndex    = MG_SP_BOSS_0;
                self->gravityEnabled = true;
                self->falling        = true;
                self->yspeed         = 96;
                self->stateTimer     = 0;
            }
            else if (self->stateTimer > 30)
            {
                self->spriteFlipVertical = true;
            }

            break;
        case 5:
            if (!self->falling)
            {
                // self->y += 64;
                self->spriteIndex          = MG_SP_BOSS_0;
                self->state                = 0; // TO "Attack - Tailwhip"
                self->spriteFlipHorizontal = (self->entityManager->playerEntity->x > self->x) ? false : true;
                self->stateTimer           = 0;
                self->spriteFlipVertical   = false;

                mgEntity_t* createdEntity;
                createdEntity = mg_createEntity(self->entityManager, ENTITY_WAVE_BALL, TO_PIXEL_COORDS(self->x),
                                                TO_PIXEL_COORDS(self->y) + 20);
                if (createdEntity != NULL)
                {
                    createdEntity->xspeed       = 48;
                    createdEntity->linkedEntity = self;
                }

                createdEntity = mg_createEntity(self->entityManager, ENTITY_WAVE_BALL, TO_PIXEL_COORDS(self->x),
                                                TO_PIXEL_COORDS(self->y) + 20);
                if (createdEntity != NULL)
                {
                    createdEntity->xspeed       = -48;
                    createdEntity->linkedEntity = self;
                }

                createdEntity = mg_createEntity(self->entityManager, ENTITY_WAVE_BALL, TO_PIXEL_COORDS(self->x),
                                                TO_PIXEL_COORDS(self->y) + 20);
                if (createdEntity != NULL)
                {
                    createdEntity->xspeed       = 96;
                    createdEntity->linkedEntity = self;
                }

                createdEntity = mg_createEntity(self->entityManager, ENTITY_WAVE_BALL, TO_PIXEL_COORDS(self->x),
                                                TO_PIXEL_COORDS(self->y) + 20);
                if (createdEntity != NULL)
                {
                    createdEntity->xspeed       = -96;
                    createdEntity->linkedEntity = self;
                }
            }

            // failsafe
            if (self->stateTimer > 239)
            {
                self->state              = 0;
                self->spriteIndex        = MG_SP_BOSS_0;
                self->stateTimer         = 0;
                self->spriteFlipVertical = false;
            }
            break;
    }

    mg_updateInvincibilityFrames(self);
    mg_moveEntityWithTileCollisions3(self);
    // applyDamping(self);
    applyGravity(self);
    mg_detectEntityCollisions(self);

    if (self->type == ENTITY_DEAD && self->linkedEntity == NULL && self->gameData->level != 11)
    {
        self->spriteIndex = MG_SP_BOSS_7;
        mg_deactivateAllEntities(self->entityManager, true);
        self->active       = true;
        self->linkedEntity = createMixtape(self->entityManager, TO_PIXEL_COORDS(self->x), TO_PIXEL_COORDS(self->y));
        startOutroCutscene(self);
    }
    despawnWhenOffscreen(self);
}

void mg_updateBossTrashMan(mgEntity_t* self)
{
    switch (self->state)
    {
        case 65535:
            return;
        case 0:
        default:
            self->stateTimer--;
            if (self->stateTimer < 0)
            {
                self->state      = 1;
                self->stateTimer = 59;
                self->xspeed     = 0;
                self->yspeed     = 0;
                break;
            }

            self->spriteIndex          = MG_SP_BOSS_0 + ((self->stateTimer >> 2) % 2);
            self->spriteFlipHorizontal = (self->xspeed > 0) ? false : true;

            break;
        case 1:
            self->stateTimer--;
            if (self->stateTimer < 0)
            {
                int16_t angle = getAtan2(self->entityManager->playerEntity->y - self->y,
                                         self->entityManager->playerEntity->x - self->x);
                int16_t sin   = getSin1024(angle);
                int16_t cos   = getCos1024(angle);

                self->xspeed = ((60 - self->hp) * cos) / 1024;
                self->yspeed = ((60 - self->hp) * sin) / 1024;

                self->stateTimer = 299;
                self->state      = 0;
            }

            if (self->stateTimer < 20)
            {
                self->spriteFlipHorizontal = (self->entityManager->playerEntity->x > self->x) ? false : true;
            }

            self->spriteIndex = MG_SP_BOSS_0 + ((self->stateTimer >> 3) % 2);
            break;
    }

    mg_updateInvincibilityFrames(self);
    mg_moveEntityWithTileCollisions3(self);
    // applyDamping(self);
    // applyGravity(self);
    mg_detectEntityCollisions(self);

    if (self->type == ENTITY_DEAD)
    {
        if ((self->linkedEntity != NULL) && (self->linkedEntity->type != ENTITY_MIXTAPE))
        {
            mg_deactivateAllEntities(self->entityManager, true);
            self->active       = true;
            self->linkedEntity = NULL;
        }

        if (self->linkedEntity == NULL && self->gameData->level != 11)
        {
            self->linkedEntity = createMixtape(self->entityManager, TO_PIXEL_COORDS(self->x), TO_PIXEL_COORDS(self->y));
            startOutroCutscene(self);
        }
    }
    despawnWhenOffscreen(self);
}

void mg_updateBossFlareGryffyn(mgEntity_t* self)
{
    switch (self->state)
    {
        case 65535:
            return;
        case 0:
        default:
            if (TO_PIXEL_COORDS(self->y) > self->tilemap->mapOffsetY + 64)
            {
                self->yspeed -= 4;
            }

            if (TO_PIXEL_COORDS(self->x) < self->tilemap->mapOffsetX + 160)
            {
                self->xspeed = 64;
            }

            if (self->stateTimer == 120)
            {
                self->jumpPower = 1;
            }

            self->stateTimer++;
            if (self->stateTimer > 180)
            {
                self->stateTimer = 0;
                switch (esp_random() % 3)
                {
                    case 0:
                        self->state = 1;
                        break;
                    case 1:
                        self->state = 2;
                        break;
                    case 2:
                        self->state = 3;
                        break;
                }
            }
            break;
        case 1:
            if (TO_PIXEL_COORDS(self->y) > self->tilemap->mapOffsetY + 64)
            {
                self->yspeed -= 4;
            }

            if (TO_PIXEL_COORDS(self->x) > self->tilemap->mapOffsetX + 120)
            {
                self->xspeed = -64;
            }

            if (self->stateTimer == 120)
            {
                self->jumpPower = 1;
            }

            self->stateTimer++;
            if (self->stateTimer > 180)
            {
                self->stateTimer = 0;
                switch (esp_random() % 3)
                {
                    case 0:
                        self->state = 0;
                        break;
                    case 1:
                        self->state = 2;
                        break;
                    case 2:
                        self->state = 3;
                        break;
                }
            }

            break;
        case 2:

            if (self->stateTimer < 60)
            {
                if (self->x < self->entityManager->playerEntity->x)
                {
                    self->xspeed = 32;
                }
                else
                {
                    self->xspeed = -32;
                }
            }

            if (!self->falling || self->y > self->entityManager->playerEntity->y)
            {
                self->state      = ((esp_random() % 100) > 50) ? 0 : 1;
                self->stateTimer = 0;
            }

            self->stateTimer++;

            break;
        case 3:
            self->stateTimer++;

            if (!(self->stateTimer % 60))
            {
                self->jumpPower = 1;
            }

            if (self->stateTimer > 239)
            {
                self->state      = ((esp_random() % 100) > 50) ? 0 : 1;
                self->stateTimer = 0;
            }
            break;
    }

    if (self->jumpPower > 0)
    {
        mgEntity_t* createdEntity = mg_createEntity(self->entityManager, ENTITY_WAVE_BALL, TO_PIXEL_COORDS(self->x),
                                                    TO_PIXEL_COORDS(self->y));
        if (createdEntity != NULL)
        {
            int16_t angle = getAtan2(self->entityManager->playerEntity->y - self->y,
                                     self->entityManager->playerEntity->x - self->x);
            int16_t sin   = getSin1024(angle);
            int16_t cos   = getCos1024(angle);

            createdEntity->xspeed = (80 * cos) / 1024;
            createdEntity->yspeed = (80 * sin) / 1024;

            createdEntity->linkedEntity = self;
            soundPlaySfx(&(self->soundManager->sndWaveBall), BZR_LEFT);
            self->jumpPower = 0;
        }
    }

    mg_updateInvincibilityFrames(self);
    mg_moveEntityWithTileCollisions3(self);
    applyDamping(self);
    applyGravity(self);
    mg_detectEntityCollisions(self);

    if (self->type == ENTITY_DEAD
        && self->linkedEntity == NULL) // Flare gryffyn ALWAYS spawns mixtape because he's the end of the boss rush.
    {
        self->linkedEntity = createMixtape(self->entityManager, TO_PIXEL_COORDS(self->x), TO_PIXEL_COORDS(self->y));
        startOutroCutscene(self);
    }
    despawnWhenOffscreen(self);
}

void mg_updateBossDeadeyeChirpzi(mgEntity_t* self)
{
    switch (self->state)
    {
        case 65535:
            return;
        case 0:
        default:
            if (TO_PIXEL_COORDS(self->y) > self->tilemap->mapOffsetY + 64)
            {
                self->yspeed -= 4;
            }

            if (TO_PIXEL_COORDS(self->x) < self->tilemap->mapOffsetX + 160)
            {
                self->xspeed = 64;
            }

            if (self->stateTimer == 120)
            {
                self->jumpPower = 1;
            }

            self->stateTimer++;
            if (self->stateTimer > 180)
            {
                self->stateTimer = 0;
                switch (esp_random() % 3)
                {
                    case 0:
                        self->state = 1;
                        break;
                    case 1:
                        self->state = 2;
                        break;
                    case 2:
                        self->state = 3;
                        break;
                }
            }
            break;
        case 1:
            if (TO_PIXEL_COORDS(self->y) > self->tilemap->mapOffsetY + 64)
            {
                self->yspeed -= 4;
            }

            if (TO_PIXEL_COORDS(self->x) > self->tilemap->mapOffsetX + 120)
            {
                self->xspeed = -64;
            }

            if (self->stateTimer == 120)
            {
                self->jumpPower = 1;
            }

            self->stateTimer++;
            if (self->stateTimer > 180)
            {
                self->stateTimer = 0;
                switch (esp_random() % 3)
                {
                    case 0:
                        self->state = 0;
                        break;
                    case 1:
                        self->state = 2;
                        break;
                    case 2:
                        self->state = 3;
                        break;
                }
            }

            break;
        case 2:

            if (self->stateTimer < 60)
            {
                if (self->x < self->entityManager->playerEntity->x)
                {
                    self->xspeed = 32;
                }
                else
                {
                    self->xspeed = -32;
                }
            }

            if (!self->falling || self->y > self->entityManager->playerEntity->y)
            {
                self->state      = ((esp_random() % 100) > 50) ? 0 : 1;
                self->stateTimer = 0;
            }

            self->stateTimer++;

            break;
        case 3:
            self->stateTimer++;

            if (!(self->stateTimer % 60))
            {
                self->jumpPower = 1;
            }

            if (self->stateTimer > 239)
            {
                self->state      = ((esp_random() % 100) > 50) ? 0 : 1;
                self->stateTimer = 0;
            }
            break;
    }

    if (self->jumpPower > 0)
    {
        mgEntity_t* createdEntity = mg_createEntity(self->entityManager, ENTITY_WAVE_BALL, TO_PIXEL_COORDS(self->x),
                                                    TO_PIXEL_COORDS(self->y));
        if (createdEntity != NULL)
        {
            int16_t angle = getAtan2(self->entityManager->playerEntity->y - self->y,
                                     self->entityManager->playerEntity->x - self->x);
            int16_t sin   = getSin1024(angle);
            int16_t cos   = getCos1024(angle);

            createdEntity->xspeed = (80 * cos) / 1024;
            createdEntity->yspeed = (80 * sin) / 1024;

            createdEntity->linkedEntity = self;
            soundPlaySfx(&(self->soundManager->sndWaveBall), BZR_LEFT);
            self->jumpPower = 0;
        }
    }

    mg_updateInvincibilityFrames(self);
    mg_moveEntityWithTileCollisions3(self);
    applyDamping(self);
    applyGravity(self);
    mg_detectEntityCollisions(self);

    if (self->type == ENTITY_DEAD && self->linkedEntity == NULL && self->gameData->level != 11)
    {
        self->linkedEntity = createMixtape(self->entityManager, TO_PIXEL_COORDS(self->x), TO_PIXEL_COORDS(self->y));
        startOutroCutscene(self);
    }
    despawnWhenOffscreen(self);
}

void mg_updateBossBigma(mgEntity_t* self)
{
    switch (self->state)
    {
        case 65535:
            return;
        case 0:
        default:
            if (TO_PIXEL_COORDS(self->y) > self->tilemap->mapOffsetY + 64)
            {
                self->yspeed -= 4;
            }

            if (TO_PIXEL_COORDS(self->x) < self->tilemap->mapOffsetX + 160)
            {
                self->xspeed = 64;
            }

            if (self->stateTimer == 120)
            {
                self->jumpPower = 1;
            }

            self->stateTimer++;
            if (self->stateTimer > 180)
            {
                self->stateTimer = 0;
                switch (esp_random() % 3)
                {
                    case 0:
                        self->state = 1;
                        break;
                    case 1:
                        self->state = 2;
                        break;
                    case 2:
                        self->state = 3;
                        break;
                }
            }
            break;
        case 1:
            if (TO_PIXEL_COORDS(self->y) > self->tilemap->mapOffsetY + 64)
            {
                self->yspeed -= 4;
            }

            if (TO_PIXEL_COORDS(self->x) > self->tilemap->mapOffsetX + 120)
            {
                self->xspeed = -64;
            }

            if (self->stateTimer == 120)
            {
                self->jumpPower = 1;
            }

            self->stateTimer++;
            if (self->stateTimer > 180)
            {
                self->stateTimer = 0;
                switch (esp_random() % 3)
                {
                    case 0:
                        self->state = 0;
                        break;
                    case 1:
                        self->state = 2;
                        break;
                    case 2:
                        self->state = 3;
                        break;
                }
            }

            break;
        case 2:

            if (self->stateTimer < 60)
            {
                if (self->x < self->entityManager->playerEntity->x)
                {
                    self->xspeed = 32;
                }
                else
                {
                    self->xspeed = -32;
                }
            }

            if (!self->falling || self->y > self->entityManager->playerEntity->y)
            {
                self->state      = ((esp_random() % 100) > 50) ? 0 : 1;
                self->stateTimer = 0;
            }

            self->stateTimer++;

            break;
        case 3:
            self->stateTimer++;

            if (!(self->stateTimer % 60))
            {
                self->jumpPower = 1;
            }

            if (self->stateTimer > 239)
            {
                self->state      = ((esp_random() % 100) > 50) ? 0 : 1;
                self->stateTimer = 0;
            }
            break;
    }

    if (self->jumpPower > 0)
    {
        mgEntity_t* createdEntity = mg_createEntity(self->entityManager, ENTITY_WAVE_BALL, TO_PIXEL_COORDS(self->x),
                                                    TO_PIXEL_COORDS(self->y));
        if (createdEntity != NULL)
        {
            int16_t angle = getAtan2(self->entityManager->playerEntity->y - self->y,
                                     self->entityManager->playerEntity->x - self->x);
            int16_t sin   = getSin1024(angle);
            int16_t cos   = getCos1024(angle);

            createdEntity->xspeed = (80 * cos) / 1024;
            createdEntity->yspeed = (80 * sin) / 1024;

            createdEntity->linkedEntity = self;
            soundPlaySfx(&(self->soundManager->sndWaveBall), BZR_LEFT);
            self->jumpPower = 0;
        }
    }

    mg_updateInvincibilityFrames(self);
    mg_moveEntityWithTileCollisions3(self);
    applyDamping(self);
    applyGravity(self);
    mg_detectEntityCollisions(self);

    if (self->type == ENTITY_DEAD && self->linkedEntity == NULL && self->gameData->level != 11)
    {
        self->linkedEntity = createMixtape(self->entityManager, TO_PIXEL_COORDS(self->x), TO_PIXEL_COORDS(self->y));
        startOutroCutscene(self);
    }
    despawnWhenOffscreen(self);
}

void mg_updateBossHankWaddle(mgEntity_t* self)
{
    /*
     * Hank Waddle behavior:
     * - Stationary (no gravity/movement)
     * - State machine for visor / vulnerability:
     *   state -1 : init -> go to 0
     *   state 0    : visor down (invincible)
     *   state 1-2  : opening / closing frames
     *   state 3    : vulnerable (can be damaged)
     *   state 4    : damage flash -> returns to 3
     * - Fires random downward projectiles periodically
     */

    mg_updateInvincibilityFrames(self);
    /* Keep Hank from being affected by gravity so he stays stationary */
    self->gravityEnabled = false;
    self->xspeed         = 0;
    self->yspeed         = 0;

    self->gameData->bgColors = leveldef[self->gameData->level].bgColors;

    /* Shooting: either periodic random downward shots, or a dense bottom-to-top wave when special1 is set */
    if (self->special1)
    {
        /* Bottom wave in progress */
        self->animationTimer++;
        if (self->animationTimer >= 3 && self->specialN > 0)
        {
            self->animationTimer      = 0;
            int16_t spawnXpix         = self->specialX;
            int16_t spawnYpix         = self->tilemap->mapOffsetY + MG_TILEMAP_DISPLAY_HEIGHT_PIXELS - 8;
            mgEntity_t* createdEntity = mg_createEntity(self->entityManager, ENTITY_WAVE_BALL, spawnXpix, spawnYpix);
            if (createdEntity != NULL)
            {
                createdEntity->xspeed       = 0;
                createdEntity->yspeed       = -160; /* fire straight up */
                createdEntity->linkedEntity = self;
                soundPlaySfx(&(self->soundManager->sndWaveBall), BZR_LEFT);
            }
            self->specialX -= 8; /* slide left */
            self->specialN--;
        }

        if (self->specialN <= 0)
        {
            self->special1       = 0; /* wave finished */
            self->animationTimer = esp_random() % 10;
        }
    }
    else if (self->state != 7)
    {
        /* Normal conical downward fire */
        self->animationTimer++;
        if (self->animationTimer >= 30)
        {
            self->animationTimer      = esp_random() % 10; /* small jitter */
            mgEntity_t* createdEntity = mg_createEntity(self->entityManager, ENTITY_WAVE_BALL, TO_PIXEL_COORDS(self->x),
                                                        TO_PIXEL_COORDS(self->y));
            if (createdEntity != NULL)
            {
                int16_t angle = getAtan2(esp_random() % 101 - 5, esp_random() % 101 - 50);
                int16_t sin   = getSin1024(angle);
                int16_t cos   = getCos1024(angle);

                createdEntity->xspeed       = (72 * cos) / 1024;
                createdEntity->yspeed       = (72 * sin) / 1024;
                createdEntity->linkedEntity = self;
                soundPlaySfx(&(self->soundManager->sndWaveBall), BZR_LEFT);
            }
        }
    }
    //.1% chance to start a bullet wave
    if (self->state != 7 && (esp_random() % 1000) < 2)
    {
        self->special1       = 1;
        self->animationTimer = 0;
        self->specialX       = self->tilemap->mapOffsetX + MG_TILEMAP_DISPLAY_WIDTH_PIXELS - 8;
        self->specialN       = (MG_TILEMAP_DISPLAY_WIDTH_PIXELS / 8) + 4; /* cover the room bottom */
    }

    /* State machine */
    switch (self->state)
    {
        case 0: /* visor down - invincible */
            self->spriteIndex = MG_SP_BOSS_0;
            /* keep visible and invincible via state check (avoid invincibilityFrames blinking)
             * visible must be true so Hank draws consistently while visor is closed
             */
            self->visible = true;
            self->stateTimer++;
            if (self->stateTimer > 600) /* time before opening */
            {
                self->state      = 1;
                self->stateTimer = 0;
            }
            break;
        case 1: /* opening */
            self->spriteIndex = MG_SP_BOSS_1;
            self->stateTimer++;
            if (self->stateTimer > 20)
            {
                self->state      = 2;
                self->stateTimer = 0;
            }
            break;
        case 2: /* open further */
            self->spriteIndex = MG_SP_BOSS_2;
            self->stateTimer++;
            if (self->stateTimer > 20)
            {
                self->state      = 3;
                self->stateTimer = 0;

                /* When the visor opens (entering vulnerable state), start a dense bottom-to-top
                 * bullet wave that spawns from the bottom-right and slides to the bottom-left.
                 */
                self->special1       = 1;
                self->animationTimer = 0;
                self->specialX       = self->tilemap->mapOffsetX + MG_TILEMAP_DISPLAY_WIDTH_PIXELS - 8;
                self->specialN       = (MG_TILEMAP_DISPLAY_WIDTH_PIXELS / 8) + 4; /* cover the room bottom */
            }
            break;
        case 3: /* vulnerable */
            self->spriteIndex = MG_SP_BOSS_3;
            /* normal behavior while vulnerable */
            self->stateTimer++;
            /* After a shorter vulnerable window, occasionally close again; start a closing animation
             * that goes 2 -> 1 -> 0 so the visor animates backward when closing.
             */
            if (self->stateTimer > 120 && (esp_random() % 100) > 95)
            {
                self->state      = 5; /* begin closing at frame 2 */
                self->stateTimer = 0;
            }
            break;
        case 5: /* closing: frame 2 -> 1 */
            self->spriteIndex = MG_SP_BOSS_2;
            self->stateTimer++;
            if (self->stateTimer > 8)
            {
                self->state      = 6; /* next closing frame */
                self->stateTimer = 0;
            }
            break;
        case 6: /* closing: frame 1 -> 0 */
            self->spriteIndex = MG_SP_BOSS_1;
            self->stateTimer++;
            if (self->stateTimer > 8)
            {
                self->state      = 0; /* visor closed */
                self->stateTimer = 0;
            }
            break;
        case 4: /* damage flash */
            self->spriteIndex        = MG_SP_BOSS_4;
            self->gameData->bgColors = leveldef[esp_random() % 10].bgColors;
            self->stateTimer++;
            if (self->stateTimer > 120 && (esp_random() % 100) > 95)
            {
                self->state      = 5; /* begin closing at frame 2 */
                self->stateTimer = 0;
            }
            else if (self->stateTimer > 12)
            {
                self->state = 3;
            }
            break;
        case 7: /*pre fight, idle*/
            self->visible = true;
            if (self->entityManager->playerEntity->x > self->x)
            {
                self->state = 0;
            }
            break;
        default:
            break;
    }

    mg_moveEntityWithTileCollisions3(self);
    applyDamping(self);
    mg_detectEntityCollisions(self);

    if (self->type == ENTITY_DEAD && self->linkedEntity == NULL && self->gameData->level != 11)
    {
        self->linkedEntity = createMixtape(self->entityManager, TO_PIXEL_COORDS(self->x), TO_PIXEL_COORDS(self->y));
        startOutroCutscene(self);
    }
    despawnWhenOffscreen(self);
}

void startOutroCutscene(mgEntity_t* self)
{
    mg_deactivateAllEntitiesOfType(self->entityManager, ENTITY_WAVE_BALL); // so the player doesn't get hurt right after
                                                                           // the winning cutscene.
    if (self->gameData->level != 7)
    {
        // just don't pause the countdown on level 7 because then you can't try out the newly acquired Shoop Da Woop.
        self->gameData->pauseCountdown = true;
    }
    // Cutscene after the boss fight
    if (self->gameData->level == 9) // I really liked this song earlier in development for sunny's reveal.
    {
        mg_setBgm(self->soundManager, MG_BGM_BOSS_DRAIN_BAT);
        soundPlayBgm(&self->soundManager->currentBgm, BZR_STEREO);
    }
    bossOutroCutscene(self->gameData);
}