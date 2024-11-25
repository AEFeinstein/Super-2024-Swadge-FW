//==============================================================================
// Includes
//==============================================================================
#include <stdlib.h>
#include <esp_log.h>
#include <math.h>
#include <limits.h>

#include "entity_bigbug.h"
#include "entityManager_bigbug.h"
#include "gameData_bigbug.h"
#include "lighting_bigbug.h"
#include "random_bigbug.h"

#include "soundFuncs.h"
#include "hdw-btn.h"
#include "esp_random.h"
#include "trigonometry.h"

//==============================================================================
// Constants
//==============================================================================
// #define SIGNOF(x) ((x > 0) - (x < 0))

//==============================================================================
// Functions
//==============================================================================
void bb_initializeEntity(bb_entity_t* self, bb_entityManager_t* entityManager, bb_gameData_t* gameData,
                         bb_soundManager_t* soundManager)
{
    self->active       = false;
    self->gameData     = gameData;
    self->soundManager = soundManager;
}

void bb_setData(bb_entity_t* self, void* data, bb_data_type_t dataType)
{
    if (self->data != NULL)
    {
        FREE_DBG(self->data);
        self->data = NULL;
    }
    self->data     = data;
    self->dataType = dataType;
}

void bb_clearCollisions(bb_entity_t* self, bool keepCollisionsCached)
{
    if (self->collisions != NULL && keepCollisionsCached == false)
    {
        // FREE WILLY FROM THE EVIL CLUTCHES OF SPIRAM!
        // remove & free all the nodes
        while (self->collisions->first != NULL)
        {
            // Remove from head
            bb_collision_t* shiftedCollision = shift(self->collisions);
            clear(shiftedCollision->checkOthers);
            FREE_DBG(shiftedCollision->checkOthers);
            FREE_DBG(shiftedCollision);
        }
        FREE_DBG(self->collisions);
    }
    self->collisions = NULL;
}

void bb_destroyEntity(bb_entity_t* self, bool caching)
{
    // Zero out most info (but not references to manager type things) for entity to be reused.
    self->active    = false;
    self->cacheable = false;

    if (self->data != NULL && caching == false)
    {
        if (self->spriteIndex == OVO_TALK)
        {
            bb_dialogueData_t* dData = (bb_dialogueData_t*)self->data;
            for (int i = 0; i < dData->numStrings; i++)
            {
                FREE_DBG(dData->strings[i]);
            }
            FREE_DBG(dData->strings);
            freeWsg(dData->sprite);
        }
        FREE_DBG(self->data);
    }
    self->data = NULL;

    bb_clearCollisions(self, caching);

    self->updateFunction              = NULL;
    self->updateFarFunction           = NULL;
    self->drawFunction                = NULL;
    self->pos                         = (vec_t){0, 0};
    self->type                        = 0;
    self->spriteIndex                 = 0;
    self->paused                      = false;
    self->hasLighting                 = false;
    self->animationTimer              = 0;
    self->gameFramesPerAnimationFrame = 1;
    self->currentAnimationFrame       = 0;
    self->halfWidth                   = 0;
    self->halfHeight                  = 0;
    self->cSquared                    = 0;
    self->tileCollisionHandler        = NULL;
    self->overlapTileHandler          = NULL;

    self->gameData->entityManager.activeEntities--;
    // printf("%d/%d entities v\n", self->gameData->entityManager.activeEntities, MAX_ENTITIES);
}

void bb_updateRocketLanding(bb_entity_t* self)
{
    bb_rocketData_t* rData = (bb_rocketData_t*)self->data;

    if (self->pos.y > -2600 && rData->flame == NULL)
    {
        rData->flame = bb_createEntity(&(self->gameData->entityManager), LOOPING_ANIMATION, false, FLAME_ANIM, 8,
                                       self->pos.x >> DECIMAL_BITS, self->pos.y >> DECIMAL_BITS, false, false);
    }

    else if (rData->flame != NULL)
    {
        if (rData->flame->currentAnimationFrame < 14)
        {
            rData->yVel -= 3;
        }
        // rData->flame->pos.y = self->pos.y + rData->yVel * self->gameData->elapsedUs / 100000;
        rData->flame->pos.y = self->pos.y + ((rData->yVel * self->gameData->elapsedUs) >> 17);
        if (rData->yVel <= 0)
        {
            self->gameData->entityManager.viewEntity = NULL;

            if (rData->flame->currentAnimationFrame == 0)
            {
                // animation has played through back to 0
                bb_heavyFallingData_t* hData
                    = HEAP_CAPS_CALLOC_DBG(1, sizeof(bb_heavyFallingData_t), MALLOC_CAP_SPIRAM);
                hData->yVel = rData->yVel >> 2;
                bb_destroyEntity(rData->flame, false);
                bb_setData(self, hData, HEAVY_FALLING_DATA);
                self->updateFunction = bb_updateHeavyFallingInit;
                return;
            }
        }
    }
    else if (rData->yVel < 300)
    {
        rData->yVel++;
    }
    self->pos.y += (rData->yVel * self->gameData->elapsedUs) >> 17;

    // music stuff
    midiPlayer_t* player = globalMidiPlayerGet(MIDI_BGM);
    if ((self->pos.y > -73360 && self->pos.y < -60000) || self->pos.y > -3000)
    {
        // 30 second fade out.
        // 9 = 16383 max volume / (60 fps * 30 seconds);
        int32_t volume = player->volume;
        volume -= 22;
        if (volume < 0)
        {
            volume = 0;
        }
        player->volume = volume;
    }

    for (int channel = 0; channel < MIDI_CHANNEL_COUNT; channel++)
    {
        // apply doppler effect to Garbotnik's home
        midiPitchWheel(player, channel, 0x2000 - rData->yVel * 16);
        // printf("pos.y %d\n",self->pos.y);
        if (self->pos.y > -63360)
        {
            // get the release
            int32_t release = (int32_t)midiGetControlValue(player, channel, MCC_SOUND_RELEASE_TIME);
            //-63360, 0
            //-43360, -128
            release -= -127 * self->pos.y / 20000 - 403;
            if (release < 0)
            {
                release = 0;
            }
            else if (release > 127)
            {
                release = 127;
            }
            midiControlChange(player, channel, MCC_SOUND_RELEASE_TIME, (uint8_t)release);
        }
    }
}

void bb_updateRocketLiftoff(bb_entity_t* self)
{
    bb_rocketData_t* rData = (bb_rocketData_t*)self->data;
    rData->yVel -= 3;
    if (rData->yVel < -300)
    {
        rData->yVel = -300;
    }
    if (self->pos.y < -77136)
    {
        self->pos.y = -77136;
        bb_destroyEntity(rData->flame, false);
        // Create a crumble animation
        bb_createEntity(&(self->gameData->entityManager), ONESHOT_ANIMATION, false, CRUMBLE_ANIM, 3,
                        self->pos.x >> DECIMAL_BITS, (self->pos.y >> DECIMAL_BITS) - 30, true, false);
        self->updateFunction = NULL;
        return;
    }
    self->pos.y += (rData->yVel * self->gameData->elapsedUs) >> 17;
    rData->flame->pos.y = self->pos.y;
}

void bb_updateHeavyFallingInit(bb_entity_t* self)
{
    bb_heavyFallingData_t* hfData = (bb_heavyFallingData_t*)self->data;
    hfData->yVel++;
    self->pos.y += (hfData->yVel * self->gameData->elapsedUs) >> 15;

    bb_hitInfo_t hitInfo = {0};
    bb_collisionCheck(&self->gameData->tilemap, self, NULL, &hitInfo);
    if (hitInfo.hit == false)
    {
        return;
    }

    self->pos.y = hitInfo.pos.y - self->halfHeight;
    if (hfData->yVel < 50)
    {
        midiPlayer_t* player = globalMidiPlayerGet(MIDI_BGM);
        midiPlayerReset(player);
        player->volume       = 16383;
        hfData->yVel         = 0;
        self->updateFunction = bb_updateGarbotnikDeploy;
        self->paused         = false;
    }
    else
    {
        hfData->yVel -= 50;
        // Update the dirt to air.
        self->gameData->tilemap.fgTiles[hitInfo.tile_i][hitInfo.tile_j].health = 0;
        // Create a crumble
        bb_crumbleDirt(self, 3, hitInfo.tile_i, hitInfo.tile_j, true);
    }
    return;
}

void bb_updateHeavyFalling(bb_entity_t* self)
{
    bb_heavyFallingData_t* hfData = (bb_heavyFallingData_t*)self->data;
    hfData->yVel++;
    self->pos.y += (hfData->yVel * self->gameData->elapsedUs) >> 15;

    // printf("tilemap addr: %p\n", &self->gameData->tilemap);
    // printf("self    addr: %p\n", self);
    bb_hitInfo_t hitInfo = {0};
    bb_collisionCheck(&self->gameData->tilemap, self, NULL, &hitInfo);
    if (hitInfo.hit == false)
    {
        return;
    }

    // self->pos.y -= hfData->yVel * self->gameData->elapsedUs / 100000;
    self->pos.y = hitInfo.pos.y - self->halfHeight;
    if (hfData->yVel < 50)
    {
        hfData->yVel = 0;
    }
    else
    {
        hfData->yVel -= 45;
        // Update the dirt to air.
        self->gameData->tilemap.fgTiles[hitInfo.tile_i][hitInfo.tile_j].health = 0;
        // Create a crumble
        bb_crumbleDirt(self, 3, hitInfo.tile_i, hitInfo.tile_j, true);
    }
    return;
}

void bb_updatePhysicsObject(bb_entity_t* self)
{
    bb_physicsData_t* pData = (bb_physicsData_t*)self->data;
    pData->vel.y++;
    self->pos.x += (pData->vel.x * self->gameData->elapsedUs) >> 16;
    self->pos.y += (pData->vel.y * self->gameData->elapsedUs) >> 16;

    bb_hitInfo_t hitInfo = {0};
    bb_collisionCheck(&self->gameData->tilemap, self, NULL, &hitInfo);
    if (hitInfo.hit == false)
    {
        return;
    }
    pData->tileTime += 20;
    self->pos.x = hitInfo.pos.x + hitInfo.normal.x * self->halfWidth;
    self->pos.y = hitInfo.pos.y + hitInfo.normal.y * self->halfHeight;

    // Reflect the velocity vector along the normal
    // See http://www.sunshine2k.de/articles/coding/vectorreflection/vectorreflection.html
    pData->vel
        = divVec2d(mulVec2d(subVec2d(pData->vel, mulVec2d(hitInfo.normal, (2 * dotVec2d(pData->vel, hitInfo.normal)))),
                            pData->bounceNumerator),
                   pData->bounceDenominator);
}

void bb_updateGarbotnikDeploy(bb_entity_t* self)
{
    if (self->currentAnimationFrame == self->gameData->entityManager.sprites[self->spriteIndex].numFrames - 2)
    {
        bb_entity_t* arm
            = bb_createEntity(&self->gameData->entityManager, NO_ANIMATION, true, ATTACHMENT_ARM, 1,
                              self->pos.x >> DECIMAL_BITS, (self->pos.y >> DECIMAL_BITS) - 33, false, false);
        ((bb_attachmentArmData_t*)arm->data)->rocket = self;

        self->paused             = true;
        self->gameData->isPaused = true;
        // deploy garbotnik!!!
        bb_entity_t* garbotnik
            = bb_createEntity(&(self->gameData->entityManager), NO_ANIMATION, true, GARBOTNIK_FLYING, 1,
                              self->pos.x >> DECIMAL_BITS, (self->pos.y >> DECIMAL_BITS) - 50, true, false);
        self->gameData->entityManager.viewEntity = garbotnik;
        self->updateFunction                     = bb_updateHeavyFalling;
        bb_startGarbotnikLandingTalk(garbotnik);
    }
}

void bb_updateGarbotnikFlying(bb_entity_t* self)
{
    bb_garbotnikData_t* gData = (bb_garbotnikData_t*)self->data;

    // gData->fuel
    //     -= self->gameData->elapsedUs >> 10; // Fuel decrements with time. Right shifting by 10 is fairly close to
    // converting microseconds to milliseconds without requiring division.
    if (gData->fuel < 0)
    {
        bb_physicsData_t* physData  = HEAP_CAPS_CALLOC_DBG(1, sizeof(bb_physicsData_t), MALLOC_CAP_SPIRAM);
        physData->vel               = gData->vel;
        physData->bounceNumerator   = 1; // 25% bounce
        physData->bounceDenominator = 4;
        bb_setData(self, physData, PHYSICS_DATA);
        self->updateFunction = bb_updateGarbotnikDying;
        return;
    }

    // touchpad stuff
    gData->fire     = gData->touching;
    gData->touching = getTouchJoystick(&gData->phi, &gData->r, &gData->intensity);
    gData->fire     = gData->fire && !gData->touching; // is true for one frame upon touchpad release.
    if (gData->fire && gData->numHarpoons > 0)
    {
        // Create a harpoon
        bb_entity_t* harpoon = bb_createEntity(&(self->gameData->entityManager), LOOPING_ANIMATION, false, HARPOON, 1,
                                               self->pos.x >> DECIMAL_BITS, self->pos.y >> DECIMAL_BITS, false, false);
        if (harpoon != NULL)
        {
            midiPlayer_t* sfx = soundGetPlayerSfx();
            midiPlayerReset(sfx);
            midiPitchWheel(sfx, 14, bb_randomInt(14, 1683));

            midiPitchWheel(sfx, 15, bb_randomInt(15, 1683));
            soundPlaySfx(&self->gameData->sfxHarpoon, 1);
            gData->numHarpoons -= 1;
            bb_projectileData_t* pData = (bb_projectileData_t*)harpoon->data;
            int32_t x;
            int32_t y;
            getTouchCartesian(gData->phi, gData->r, &x, &y);
            // Set harpoon's velocity
            pData->vel.x = (x - 512) >> 4;
            pData->vel.y = (-y + 512) >> 4;
        }
    }

    // record the previous frame's position before any logic.
    gData->previousPos = self->pos;

    vec_t accel = {.x = 0, .y = 0};

    // Update garbotnik's velocity if a button is currently down
    switch (self->gameData->btnState)
    {
        // up
        case 0b0001:
        {
            accel.y = -25;
            break;
        }
        case 0b1101:
        {
            accel.y = -25;
            break;
        }
        // down
        case 0b0010:
        {
            accel.y = 25;
            break;
        }
        case 0b1110:
        {
            accel.y = 25;
            break;
        }
        // left
        case 0b0100:
        {
            accel.x = -25;
            break;
        }
        case 0b0111:
        {
            accel.x = -25;
            break;
        }
        // right
        case 0b1000:
        {
            accel.x = 25;
            break;
        }
        case 0b1011:
        {
            accel.x = 25;
            break;
        }
        // up,left
        case 0b0101:
        {
            accel.x = -18; // magnitude is sqrt(1/2) * 100000
            accel.y = -18;
            break;
        }
        // up,right
        case 0b1001:
        {
            accel.x = 18; // 35 707 7035
            accel.y = -18;
            break;
        }
        // down,right
        case 0b1010:
        {
            accel.x = 18;
            accel.y = 18;
            break;
        }
        // down,left
        case 0b0110:
        {
            accel.x = -18;
            accel.y = 18;
            break;
        }
        default:
        {
            break;
        }
    }

    // printf("accel x: %d\n", accel.x);
    // printf("elapsed: %d", (int32_t) elapsedUs);
    // printf("offender: %d\n", (int32_t) elapsedUs / 100000);
    // printf("now   x: %d\n", mulVec2d(accel, elapsedUs) / 100000).x);

    gData->accel.x = (accel.x * self->gameData->elapsedUs) >> 16;
    gData->accel.y = (accel.y * self->gameData->elapsedUs) >> 16;

    // physics
    gData->yaw.y += gData->accel.x;
    if (gData->yaw.x < 0)
    {
        gData->yaw.y -= (3 * self->gameData->elapsedUs) >> 14;
    }
    else
    {
        gData->yaw.y += (3 * self->gameData->elapsedUs) >> 14;
    }
    gData->yaw.x += gData->yaw.y;
    if (gData->yaw.x < -1440)
    {
        gData->yaw.x = -1440;
        gData->yaw.y = 0;
    }
    else if (gData->yaw.x > 1440)
    {
        gData->yaw.x = 1440;
        gData->yaw.y = 0;
    }

    // Apply garbotnik's drag
    int32_t sqMagX = gData->vel.x * gData->vel.x;
    if (sqMagX > 0)
    {
        int32_t drag = sqMagX >> 7;
        if (drag < 10)
        {
            drag = 10;
        }
        // Apply drag based on absolute value to smooth asymmetry
        int32_t dragEffect = (drag * self->gameData->elapsedUs) >> 17;

        // Adjust velocity symmetrically for both positive and negative values
        if (gData->vel.x > 0)
        {
            gData->vel.x -= dragEffect;
            if (gData->vel.x < 0)
                gData->vel.x = 0; // Prevent overshoot
        }
        else if (gData->vel.x < 0)
        {
            gData->vel.x += dragEffect;
            if (gData->vel.x > 0)
                gData->vel.x = 0;
        }
    }
    int32_t sqMagY = gData->vel.y * gData->vel.y;
    if (sqMagY > 0)
    {
        int32_t drag = sqMagY >> 7;
        if (drag < 10)
        {
            drag = 10;
        }
        // Apply drag based on absolute value to smooth asymmetry
        int32_t dragEffect = (drag * self->gameData->elapsedUs) >> 17;

        // Adjust velocity symmetrically for both positive and negative values
        if (gData->vel.y > 0)
        {
            gData->vel.y -= dragEffect;
            if (gData->vel.y < 0)
                gData->vel.y = 0; // Prevent overshoot
        }
        else if (gData->vel.y < 0)
        {
            gData->vel.y += dragEffect;
            if (gData->vel.y > 0)
                gData->vel.y = 0;
        }
    }

    // Update garbotnik's velocity
    gData->vel.x += gData->accel.x;
    gData->vel.y += gData->accel.y;

    // Update garbotnik's position
    self->pos.x += (gData->vel.x * self->gameData->elapsedUs) >> 16;
    self->pos.y += (gData->vel.y * self->gameData->elapsedUs) >> 16;

    // printf("Garbotnik X: %d\n", self->pos);
    // //keep the player in bounds
    // if(self->pos.x < 2304)
    // {
    //     self->pos.x == 2304;
    // }
    // else if(self->pos.x > )

    bb_hitInfo_t hitInfo = {0};
    bb_collisionCheck(&self->gameData->tilemap, self, &gData->previousPos, &hitInfo);

    if (hitInfo.hit == false)
    {
        gData->gettingCrushed = false;
        return;
    }

    if (gData->gettingCrushed)
    {
        gData->fuel -= 2000;
    }

    self->pos.x = hitInfo.pos.x + hitInfo.normal.x * self->halfWidth;
    self->pos.y = hitInfo.pos.y + hitInfo.normal.y * self->halfHeight;

    int32_t dot = dotVec2d(gData->vel, hitInfo.normal);
    if (dot < -40)
    { // velocity angle is opposing garbage normal vector. Tweak number for different threshold.
        ///////////////////////
        // digging detected! //
        ///////////////////////

        // crumble test
        //  uint32_t* val = HEAP_CAPS_CALLOC_DBG(2,sizeof(uint32_t), MALLOC_CAP_SPIRAM);
        //  val[0] = 5;
        //  val[1] = 3;
        //  push(self->gameData->unsupported, (void*)val);

        // Update the dirt by decrementing it.
        self->gameData->tilemap.fgTiles[hitInfo.tile_i][hitInfo.tile_j].health -= 1;

        bb_midgroundTileInfo_t* tile = &self->gameData->tilemap.fgTiles[hitInfo.tile_i][hitInfo.tile_j];
        if (tile->health == 0)
        {
            flagNeighbors(tile, self->gameData);
        }

        if (tile->health == 0 || tile->health == 1 || tile->health == 4)
        {
            // Create a crumble
            bb_crumbleDirt(self, 2, hitInfo.tile_i, hitInfo.tile_j, !tile->health);
        }
        else
        {
            // Create a bump animation
            bb_createEntity(&(self->gameData->entityManager), ONESHOT_ANIMATION, false, BUMP_ANIM, 2,
                            hitInfo.pos.x >> DECIMAL_BITS, hitInfo.pos.y >> DECIMAL_BITS, true, false);
        }

        ////////////////////////////////
        // Mirror garbotnik's velocity//
        ////////////////////////////////
        // Reflect the velocity vector along the normal
        // See http://www.sunshine2k.de/articles/coding/vectorreflection/vectorreflection.html
        // printf("hit squared speed: %" PRId32 "\n", sqMagVec2d(gData->vel));

        // range is roughly 2600 to 7100
        // printf("dot %d\n", dot);
        // printf("thing: %d\n",sqMagVec2d(gData->vel) * dot);
        int32_t bounceScalar = 3;
        if (sqMagVec2d(gData->vel) * dot < -360000)
        {
            bounceScalar = 2;
        }
        if (sqMagVec2d(gData->vel) * dot < -550000)
        {
            bounceScalar = 1;
        }
        printf("bounceScalar %" PRId32 "\n", bounceScalar);
        gData->vel = mulVec2d(
            subVec2d(gData->vel, mulVec2d(hitInfo.normal, (2 * dotVec2d(gData->vel, hitInfo.normal)))), bounceScalar);

        /////////////////////////////////
        // check neighbors for stability//
        /////////////////////////////////
        // for(uint8_t neighborIdx = 0; neighborIdx < 4; neighborIdx++)
        // {
        //     uint32_t check_x = hitInfo.tile_i + self->gameData->neighbors[neighborIdx][0];
        //     uint32_t check_y = hitInfo.tile_j + self->gameData->neighbors[neighborIdx][1];
        //     //Check if neighbor is in bounds of map (also not on left, right, or bottom, perimiter) and if it
        //     is dirt. if(check_x > 0 && check_x < TILE_FIELD_WIDTH - 1 && check_y > 0 && check_y <
        //     TILE_FIELD_HEIGHT - 1 && bigbug->tilemap.fgTiles[check_x][check_y] > 0)
        //     {
        //         uint32_t* val = HEAP_CAPS_CALLOC_DBG(4, sizeof(uint32_t), MALLOC_CAP_SPIRAM);
        //         val[0] = check_x;
        //         val[1] = check_y;
        //         val[2] = 1; //1 is for foreground. 0 is midground.
        //         val[3] = 0; //f value used in pathfinding.
        //         push(self->gameData->pleaseCheck, (void*)val);
        //     }
        // }
    }
}

void bb_updateGarbotnikDying(bb_entity_t* self)
{
    bb_physicsData_t* physData = (bb_physicsData_t*)self->data;
    bb_updatePhysicsObject(self);

    if (physData->tileTime > 0)
    {
        physData->tileTime--;
    }
    printf("tileTime: %d\n", physData->tileTime);
    if (physData->tileTime > 101)
    {
        bb_createEntity(&self->gameData->entityManager, NO_ANIMATION, true, BB_GAME_OVER, 1,
                        self->gameData->camera.camera.pos.x, self->gameData->camera.camera.pos.y, true, true);
        self->updateFunction = NULL;

        self->gameData->entityManager.viewEntity
            = bb_createEntity(&(self->gameData->entityManager), NO_ANIMATION, true, NO_SPRITE_POI, 1,
                              (self->gameData->entityManager.playerEntity->pos.x >> DECIMAL_BITS),
                              (self->gameData->entityManager.playerEntity->pos.y >> DECIMAL_BITS), true, false);
        bb_goToData* tData = (bb_goToData*)self->gameData->entityManager.viewEntity->data;
        tData->tracking    = self->gameData->entityManager.deathDumpster;
        tData->midPointSqDist
            = sqMagVec2d(divVec2d((vec_t){(tData->tracking->pos.x >> DECIMAL_BITS)
                                              - (self->gameData->entityManager.viewEntity->pos.x >> DECIMAL_BITS),
                                          (tData->tracking->pos.y >> DECIMAL_BITS)
                                              - (self->gameData->entityManager.viewEntity->pos.y >> DECIMAL_BITS)},
                                  2));
        tData->executeOnArrival                                  = &bb_startGarbotnikCloningTalk;
        self->gameData->entityManager.viewEntity->updateFunction = &bb_updatePOI;
    }
}

void bb_updateHarpoon(bb_entity_t* self)
{
    bb_projectileData_t* pData = (bb_projectileData_t*)self->data;

    // Update harpoon's lifetime. I think not using elapsed time is good enough.
    pData->lifetime++;
    if (pData->lifetime > 140)
    {
        bb_destroyEntity(self, false);
        return;
    }

    // Update harpoon's velocity
    pData->vel.y++;
    // Update harpoon's position
    self->pos = addVec2d(self->pos, mulVec2d(pData->vel, (self->gameData->elapsedUs >> 12)));

    bb_hitInfo_t hitInfo = {0};
    bb_collisionCheck(&self->gameData->tilemap, self, NULL, &hitInfo);
    if (hitInfo.hit && pData->prevFrameInAir && pData->vel.y > 0)
    {
        vecFl_t floatVel              = {(float)pData->vel.x, (float)pData->vel.y};
        bb_stuckHarpoonData_t* shData = HEAP_CAPS_CALLOC_DBG(1, sizeof(bb_stuckHarpoonData_t), MALLOC_CAP_SPIRAM);
        shData->floatVel              = normVecFl2d(floatVel);
        bb_setData(self, shData, STUCK_HARPOON_DATA);

        bb_clearCollisions(self, false);
        self->updateFunction = bb_updateStuckHarpoon;
        self->drawFunction   = bb_drawStuckHarpoon;
    }
    else
    {
        pData->prevFrameInAir = !hitInfo.hit;
    }
}

void bb_updateStuckHarpoon(bb_entity_t* self)
{
    bb_stuckHarpoonData_t* shData = (bb_stuckHarpoonData_t*)self->data;
    shData->lifetime++;
    if (shData->lifetime > 140)
    {
        bb_destroyEntity(self, false);
        return;
    }

    if (shData->parent != NULL)
    {
        self->pos = addVec2d(shData->parent->pos, shData->offset);
    }
}

void bb_updateEggLeaves(bb_entity_t* self)
{
    if (self->gameData->entityManager.playerEntity != NULL)
    {
        bb_eggLeavesData_t* elData = (bb_eggLeavesData_t*)self->data;
        vec_t lookup
            = {.x = (self->pos.x >> DECIMAL_BITS) - (self->gameData->entityManager.playerEntity->pos.x >> DECIMAL_BITS)
                    + self->gameData->tilemap.headlampWsg.w,
               .y = (self->pos.y >> DECIMAL_BITS) - (self->gameData->entityManager.playerEntity->pos.y >> DECIMAL_BITS)
                    + self->gameData->tilemap.headlampWsg.h};

        lookup = divVec2d(lookup, 2);
        if (self->gameData->entityManager.playerEntity == NULL)
        {
            elData->brightness = 0;
        }
        else if (GARBOTNIK_DATA == self->gameData->entityManager.playerEntity->dataType)
        {
            elData->brightness = bb_foregroundLighting(
                &(self->gameData->tilemap.headlampWsg), &lookup,
                &(((bb_garbotnikData_t*)self->gameData->entityManager.playerEntity->data)->yaw.x));
        }

        bb_eggData_t* eData = (bb_eggData_t*)elData->egg->data;
        eData->stimulation += elData->brightness;
        if (eData->stimulation > 0)
        {
            eData->stimulation -= 1;
        }
        if (eData->stimulation > 439)
        {
            ((bb_entity_t*)elData->egg)->pos.x += bb_randomInt(-1, 1);
            ((bb_entity_t*)elData->egg)->pos.y += bb_randomInt(-1, 1);
        }
        if (eData->stimulation > 719)
        {
            eData->stimulation = 719;
            // transform "hatch" into a bug...
            self->pos = elData->egg->pos; // sets what will be the bug to the egg position, because eggs tend to wiggle
                                          // about before hatching.

            // create a bug
            bb_entity_t* bug
                = bb_createEntity(&self->gameData->entityManager, LOOPING_ANIMATION, false, bb_randomInt(8, 13), 1,
                                  elData->egg->pos.x >> DECIMAL_BITS, elData->egg->pos.y >> DECIMAL_BITS, false, false);

            if (bug != NULL)
            {
                // destroy the egg
                bb_destroyEntity(elData->egg, false);
                bb_hitInfo_t hitInfo = {0};
                bb_collisionCheck(&self->gameData->tilemap, bug, NULL, &hitInfo);
                if (hitInfo.hit == true)
                {
                    // Update the dirt to air.
                    self->gameData->tilemap.fgTiles[hitInfo.tile_i][hitInfo.tile_j].health = 0;
                    self->gameData->tilemap.fgTiles[hitInfo.tile_i][hitInfo.tile_j].embed  = NOTHING_EMBED;
                    self->gameData->tilemap.fgTiles[hitInfo.tile_i][hitInfo.tile_j].entity = NULL;
                    // Create a crumble
                    bb_crumbleDirt(self, 2, hitInfo.tile_i, hitInfo.tile_j, false);
                }
                // destroy this
                bb_destroyEntity(self, false);
            }
        }
    }
}

void bb_updateFarEggleaves(bb_entity_t* self)
{
    bb_hitInfo_t hitInfo = {0};
    bb_collisionCheck(&self->gameData->tilemap, self, NULL, &hitInfo);
    if (hitInfo.hit == true)
    {
        self->gameData->tilemap.fgTiles[hitInfo.tile_i][hitInfo.tile_j].entity = NULL;
    }
    else
    {
        printf("entity_bigbug.c this should not happen\n");
    }

    // destroy the egg
    bb_destroyEntity(((bb_eggLeavesData_t*)(self->data))->egg, false);

    // destroy this
    bb_destroyEntity(self, false);
}

void bb_updateFarDestroy(bb_entity_t* self)
{
    bb_destroyEntity(self, false);
}

void bb_updateFarMenu(bb_entity_t* self)
{
    if ((self->pos.y >> DECIMAL_BITS) < self->gameData->camera.camera.pos.y)
    {
        bb_destroyEntity(self, false);
    }
}

void bb_updateFarMenuAndUnload(bb_entity_t* self)
{
    if ((self->pos.y >> DECIMAL_BITS) < self->gameData->camera.camera.pos.y)
    {
        // unload the menu sprites, because I don't foresee ever coming back to the main menu from gameplay.
        for (int frameIdx = 0; frameIdx < self->gameData->entityManager.sprites[self->spriteIndex].numFrames;
             frameIdx++)
        {
            freeWsg(&self->gameData->entityManager.sprites[self->spriteIndex].frames[frameIdx]);
        }

        bb_destroyEntity(self, false);
    }
}

void bb_updateMenuBug(bb_entity_t* self)
{
    bb_menuBugData_t* mbData = (bb_menuBugData_t*)self->data;
    self->pos.x += (mbData->xVel - 1) << 3;
    if (mbData->firstTrip && self->pos.x < self->gameData->entityManager.viewEntity->pos.x - (130 << DECIMAL_BITS))
    {
        mbData->firstTrip = false;
        mbData->xVel      = bb_randomInt(-2, 2);
        mbData->xVel
            = mbData->xVel == 1 ? mbData->xVel - 1 : mbData->xVel; // So as not to match the treadmill speed exactly.
        self->gameFramesPerAnimationFrame = (3 - abs(mbData->xVel)) * 4;
        if (mbData->xVel == 0)
        {
            self->gameFramesPerAnimationFrame = 255;
        }
    }
}

void bb_updateMoveLeft(bb_entity_t* self)
{
    self->pos.x -= 1 << 3;
}

void bb_updateBug(bb_entity_t* self)
{
}

void bb_updateMenu(bb_entity_t* self)
{
    bb_menuData_t* mData = (bb_menuData_t*)self->data;
    if (self->gameData->btnDownState & PB_UP)
    {
        mData->selectionIdx--;
        mData->selectionIdx = mData->selectionIdx < 0 ? 1 : mData->selectionIdx;
    }
    if (self->gameData->btnDownState & PB_DOWN)
    {
        mData->selectionIdx++;
        mData->selectionIdx = mData->selectionIdx > 1 ? 0 : mData->selectionIdx;
    }
    if (self->gameData->btnDownState & PB_A)
    {
        if (mData->selectionIdx == 0)
        {
            // start game

            // destroy the cursor
            bb_destroyEntity(mData->cursor, false);
            mData->cursor = NULL;

            // create the death dumpster
            bb_goToData* tData = (bb_goToData*)self->gameData->entityManager.viewEntity->data;
            self->gameData->entityManager.deathDumpster
                = bb_createEntity(&self->gameData->entityManager, NO_ANIMATION, true, BB_DEATH_DUMPSTER, 1,
                                  self->pos.x >> DECIMAL_BITS, (self->pos.y >> DECIMAL_BITS) + 400, true, false);
            tData->tracking = self->gameData->entityManager.deathDumpster;
            tData->midPointSqDist
                = sqMagVec2d(divVec2d((vec_t){(tData->tracking->pos.x >> DECIMAL_BITS)
                                                  - (self->gameData->entityManager.viewEntity->pos.x >> DECIMAL_BITS),
                                              (tData->tracking->pos.y >> DECIMAL_BITS)
                                                  - (self->gameData->entityManager.viewEntity->pos.y >> DECIMAL_BITS)},
                                      2));

            self->gameData->entityManager.viewEntity->updateFunction = &bb_updatePOI;

            // create 3 rockets
            for (int rocketIdx = 0; rocketIdx < 3; rocketIdx++)
            {
                self->gameData->entityManager.boosterEntities[rocketIdx]
                    = bb_createEntity(&self->gameData->entityManager, NO_ANIMATION, true, ROCKET_ANIM, 8,
                                      (self->pos.x >> DECIMAL_BITS) - 96 + 96 * rocketIdx,
                                      (self->pos.y >> DECIMAL_BITS) + 375, true, false);

                self->gameData->entityManager.boosterEntities[rocketIdx]->updateFunction = NULL;

                bb_rocketData_t* rData
                    = (bb_rocketData_t*)self->gameData->entityManager.boosterEntities[rocketIdx]->data;

                rData->flame = bb_createEntity(
                    &(self->gameData->entityManager), LOOPING_ANIMATION, false, FLAME_ANIM, 6,
                    self->gameData->entityManager.boosterEntities[rocketIdx]->pos.x >> DECIMAL_BITS,
                    self->gameData->entityManager.boosterEntities[rocketIdx]->pos.y >> DECIMAL_BITS, true, false);

                rData->flame->updateFunction = &bb_updateFlame;
            }

            self->updateFunction = NULL;
            return;
        }
        else if (mData->selectionIdx == 1)
        {
            // exit the game
            self->gameData->exit = true;
        }
    }

    mData->cursor->pos.y = self->pos.y - (209 << DECIMAL_BITS) + mData->selectionIdx * (22 << DECIMAL_BITS);

    if (self->gameData->menuBug == NULL || self->gameData->menuBug->active == false)
    {
        self->gameData->menuBug
            = bb_createEntity(&self->gameData->entityManager, LOOPING_ANIMATION, false, bb_randomInt(8, 13), 1,
                              (self->pos.x >> DECIMAL_BITS) + 135, (self->pos.y >> DECIMAL_BITS) - 172, true, false);
        self->gameData->menuBug->cacheable         = false;
        self->gameData->menuBug->drawFunction      = &bb_drawMenuBug;
        self->gameData->menuBug->updateFunction    = &bb_updateMenuBug;
        self->gameData->menuBug->updateFarFunction = &bb_updateFarDestroy;
        bb_menuBugData_t* mbData = HEAP_CAPS_CALLOC_DBG(1, sizeof(bb_menuBugData_t), MALLOC_CAP_SPIRAM);
        mbData->xVel             = bb_randomInt(-2, 2);
        mbData->xVel
            = mbData->xVel == 1 ? mbData->xVel - 1 : mbData->xVel; // So as not to match the treadmill speed exactly.
        mbData->firstTrip = true;
        FREE_DBG(self->gameData->menuBug->data);
        self->gameData->menuBug->data                        = mbData;
        self->gameData->menuBug->gameFramesPerAnimationFrame = abs(6 - mbData->xVel);
        if (mbData->xVel == 0)
        {
            self->gameData->menuBug->gameFramesPerAnimationFrame = 255;
        }
    }

    if (bb_randomInt(0, 8) < 1) //%50
    {
        bb_entity_t* treadmillDust = bb_createEntity(
            &self->gameData->entityManager, NO_ANIMATION, true, NO_SPRITE_STAR, 1, (self->pos.x >> DECIMAL_BITS) + 140,
            (self->pos.y >> DECIMAL_BITS) - 165 + bb_randomInt(-10, 10), false, false);
        treadmillDust->updateFunction = &bb_updateMoveLeft;
    }
}

void bb_updatePOI(bb_entity_t* self)
{
    bb_goToData* tData = (bb_goToData*)self->data;
    if (tData != NULL)
    {
        vec_t ToFrom = (vec_t){(tData->tracking->pos.x >> DECIMAL_BITS) - (self->pos.x >> DECIMAL_BITS),
                               (tData->tracking->pos.y >> DECIMAL_BITS) - (self->pos.y >> DECIMAL_BITS)};
        if (sqMagVec2d(ToFrom) > tData->midPointSqDist)
        {
            tData->speed++;
        }
        else if (tData->speed > 0)
        {
            tData->speed--;
        }
        else if (tData->speed == 0)
        {
            if (ToFrom.x > 0 || ToFrom.x <= -16 || ToFrom.y > 0 || ToFrom.y <= -16)
            {
                if (ToFrom.x > 0)
                {
                    self->pos.x += 16;
                    self->gameData->camera.camera.pos.x++;
                    self->gameData->camera.velocity.x = 1;
                }
                else if (ToFrom.x <= -16)
                {
                    self->pos.x -= 16;
                    self->gameData->camera.camera.pos.x--;
                    self->gameData->camera.velocity.x--;
                }
                if (ToFrom.y > 0)
                {
                    self->pos.y += 16;
                    self->gameData->camera.camera.pos.y++;
                    self->gameData->camera.velocity.y = 1;
                }
                else if (ToFrom.y <= -16)
                {
                    self->pos.y -= 16;
                    self->gameData->camera.camera.pos.y--;
                    self->gameData->camera.velocity.y--;
                }
            }
            else
            {
                self->updateFunction = NULL;
                tData->executeOnArrival(self);
                return;
            }
        }
        if (sqMagVec2d(ToFrom) > (tData->speed >> DECIMAL_BITS) * (tData->speed >> DECIMAL_BITS))
        {
            fastNormVec(&ToFrom.x, &ToFrom.y);
            ToFrom   = mulVec2d(ToFrom, tData->speed >> DECIMAL_BITS);
            ToFrom.x = ToFrom.x >> 7;
            ToFrom.y = ToFrom.y >> 7;
        }
        self->pos         = addVec2d(self->pos, ToFrom);
        vec_t previousPos = self->gameData->camera.camera.pos;
        self->gameData->camera.camera.pos
            = (vec_t){(self->pos.x >> DECIMAL_BITS) - 140, (self->pos.y >> DECIMAL_BITS) - 120};
        self->gameData->camera.velocity
            = addVec2d(self->gameData->camera.velocity, subVec2d(self->gameData->camera.camera.pos, previousPos));
    }
}

void bb_updateFlame(bb_entity_t* self)
{
    if (self->currentAnimationFrame > 2)
    {
        if (bb_randomInt(0, 1))
        {
            int newFrame = 0;
            while (bb_randomInt(0, 1) && newFrame < self->currentAnimationFrame - 1)
            {
                newFrame++;
            }
            self->animationTimer        = newFrame;
            self->currentAnimationFrame = newFrame;
        }
    }
}

void bb_updateCharacterTalk(bb_entity_t* self)
{
    bb_dialogueData_t* dData = (bb_dialogueData_t*)self->data;

    if (dData->offsetY < 0 && dData->curString < dData->numStrings)
    {
        dData->offsetY += 3;
    }
    else if (dData->curString >= dData->numStrings)
    {
        dData->offsetY -= 3;
        if (dData->offsetY <= -240)
        {
            dData->endDialogueCB(self);
            bb_destroyEntity(self, false);
            return;
        }
    }
    else
    {
        if (dData->curString < 0)
        {
            dData->curString = 0;
        }

        if (self->gameData->btnDownState & PB_A)
        {
            dData->curString++;
            if (dData->curString < dData->numStrings)
            {
                dData->loadedIdx = bb_randomInt(0, 7);
                char wsg_name[strlen("ovo_talk") + 9]; // 6 extra characters makes room for up to a 2 digit number +
                                                       // ".wsg" + null terminator ('\0')
                snprintf(wsg_name, sizeof(wsg_name), "%s%d.wsg", "ovo_talk", dData->loadedIdx);
                loadWsg(wsg_name, dData->sprite, true);

                midiPlayer_t* bgm = globalMidiPlayerGet(MIDI_BGM);
                // Play a random note within an octave at half velocity on channel 1
                int deepBlueseyPitches[] = {31, 34, 36, 37, 38, 41, 43, 54, 55, 50};
                uint8_t pitch            = bb_randomInt(0, 9);
                midiNoteOn(bgm, 12, deepBlueseyPitches[pitch], 0x40);
                midiNoteOff(bgm, 12, deepBlueseyPitches[pitch], 0x7F);
            }
        }
    }
}

void bb_updateAttachmentArm(bb_entity_t* self)
{
    if (self->gameData->entityManager.playerEntity == NULL)
    {
        // this is for when garbotnik dies.
        bb_destroyEntity(self, false);
        return;
    }

    bb_attachmentArmData_t* aData = (bb_attachmentArmData_t*)self->data;
    self->pos                     = aData->rocket->pos;
    self->pos.y -= 29 << DECIMAL_BITS; // 67 is ok
    if (aData->angle > 180 << DECIMAL_BITS)
    {
        aData->angle -= 1 << DECIMAL_BITS;
    }
    if (aData->angle >= 359 << DECIMAL_BITS)
    {
        bb_destroyEntity(self->gameData->entityManager.playerEntity, false);
        self->gameData->entityManager.playerEntity = NULL;
        self->gameData->entityManager.viewEntity   = aData->rocket;
        aData->rocket->currentAnimationFrame       = 0;

        bb_rocketData_t* rData = HEAP_CAPS_CALLOC_DBG(1, sizeof(bb_rocketData_t), MALLOC_CAP_SPIRAM);

        rData->flame
            = bb_createEntity(&(self->gameData->entityManager), LOOPING_ANIMATION, false, FLAME_ANIM, 6,
                              aData->rocket->pos.x >> DECIMAL_BITS, aData->rocket->pos.y >> DECIMAL_BITS, true, false);

        rData->flame->updateFunction = &bb_updateFlame;
        bb_setData(aData->rocket, rData, ROCKET_DATA);
        aData->rocket->updateFunction = &bb_updateRocketLiftoff;

        bb_destroyEntity(self, false);
    }
}

void bb_updateGameOver(bb_entity_t* self)
{
    if (self->gameData->btnDownState & PB_A)
    {
        if (self->currentAnimationFrame == 0)
        {
            self->currentAnimationFrame++;
        }
        else
        {
            bb_destroyEntity(self->gameData->entityManager.playerEntity, false);
            self->gameData->entityManager.playerEntity = NULL;

            self->gameData->entityManager.activeBooster->currentAnimationFrame++;

            // get the current booster idx
            int i = 0;
            while (i < 3)
            {
                if (self->gameData->entityManager.boosterEntities[i] != NULL)
                {
                    self->gameData->entityManager.boosterEntities[i] = NULL;
                    break;
                }
                i++;
            }

            self->gameData->entityManager.activeBooster = NULL;
            i++;
            if (i == 3)
            {
                // IDK it is really really game over here.
                printf("finish me\n");
            }
            else
            {
                self->gameData->entityManager.activeBooster = self->gameData->entityManager.boosterEntities[i];

                bb_destroyEntity(self, false);
            }
        }
    }
}

void bb_drawGarbotnikFlying(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self)
{
    if (GARBOTNIK_DATA != self->dataType)
    {
        return;
    }

    bb_garbotnikData_t* gData = (bb_garbotnikData_t*)self->data;

    int16_t xOff = (self->pos.x >> DECIMAL_BITS) - entityManager->sprites[self->spriteIndex].originX - camera->pos.x;
    int16_t yOff = (self->pos.y >> DECIMAL_BITS) - entityManager->sprites[self->spriteIndex].originY - camera->pos.y;

    // Draw garbotnik
    if (gData->yaw.x < -1400)
    {
        drawWsgSimple(&entityManager->sprites[self->spriteIndex].frames[0], xOff, yOff);
    }
    else if (gData->yaw.x < -400)
    {
        drawWsgSimple(&entityManager->sprites[self->spriteIndex].frames[1], xOff, yOff);
    }
    else if (gData->yaw.x < 400)
    {
        drawWsgSimple(&entityManager->sprites[self->spriteIndex].frames[2], xOff, yOff);
    }
    else if (gData->yaw.x < 1400)
    {
        drawWsg(&entityManager->sprites[self->spriteIndex].frames[1], xOff, yOff, true, false, 0);
    }
    else
    {
        drawWsg(&entityManager->sprites[self->spriteIndex].frames[0], xOff, yOff, true, false, 0);
    }

    if (gData->touching)
    {
        xOff += entityManager->sprites[self->spriteIndex].originX;
        yOff += entityManager->sprites[self->spriteIndex].originY;
        int32_t x;
        int32_t y;
        getTouchCartesian(gData->phi, gData->r, &x, &y);
        drawLineFast(xOff, yOff, xOff + (x - 511) / 5, yOff - (y - 511) / 5, c305);
    }

    char harpoonText[13]; // 13 characters makes room for up to a 3 digit number + " harpooons" + null
                          // terminator ('\0')
    snprintf(harpoonText, sizeof(harpoonText), "%d harpoons", gData->numHarpoons);

    int32_t tWidth = textWidth(&self->gameData->font, harpoonText);
    drawText(&self->gameData->font, c344, harpoonText, TFT_WIDTH - tWidth - 30,
             TFT_HEIGHT - self->gameData->font.height - 3);
    drawText(&self->gameData->font, c223, harpoonText, TFT_WIDTH - tWidth - 30,
             TFT_HEIGHT - self->gameData->font.height - 2);
}

void bb_drawHarpoon(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self)
{
    int16_t xOff = (self->pos.x >> DECIMAL_BITS) - camera->pos.x;
    int16_t yOff = (self->pos.y >> DECIMAL_BITS) - camera->pos.y;

    bb_projectileData_t* pData = (bb_projectileData_t*)self->data;

    vecFl_t floatVel = {(float)pData->vel.x, (float)pData->vel.y};

    int16_t angle = (int16_t)(atan2f(floatVel.y, floatVel.x) * (180.0 / M_PI));
    angle += 180;
    while (angle < 0)
    {
        angle += 360;
    }
    while (angle > 359)
    {
        angle -= 360;
    }

    floatVel = normVecFl2d(floatVel);

    int16_t xOff2 = xOff - floatVel.x * 25;
    int16_t yOff2 = yOff - floatVel.y * 25;

    drawLineFast(xOff, yOff - 1, xOff2, yOff2 - 1, c455);
    drawLineFast(xOff, yOff, xOff2, yOff2, c123);
    drawLineFast(xOff, yOff + 1, xOff2, yOff2 + 1, c011);

    drawWsg(&entityManager->sprites[self->spriteIndex].frames[self->currentAnimationFrame],
            xOff - entityManager->sprites[self->spriteIndex].originX,
            yOff - entityManager->sprites[self->spriteIndex].originY, false, false, angle);
}

void bb_drawStuckHarpoon(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self)
{
    int16_t xOff = (self->pos.x >> DECIMAL_BITS) - camera->pos.x;
    int16_t yOff = (self->pos.y >> DECIMAL_BITS) - camera->pos.y;

    vecFl_t floatVel = ((bb_stuckHarpoonData_t*)self->data)->floatVel;

    drawLineFast(xOff, yOff, xOff - floatVel.x * 20, yOff - floatVel.y * 20, c112);
    drawLineFast(xOff, yOff - 1, xOff - floatVel.x * 20, yOff - floatVel.y * 20 - 1, c344);
}

void bb_drawEggLeaves(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self)
{
    int16_t xOff = (self->pos.x >> DECIMAL_BITS) - entityManager->sprites[self->spriteIndex].originX - camera->pos.x;
    int16_t yOff = (self->pos.y >> DECIMAL_BITS) - entityManager->sprites[self->spriteIndex].originY - camera->pos.y;

    drawWsgSimple(&entityManager->sprites[self->spriteIndex].frames[((bb_eggLeavesData_t*)self->data)->brightness],
                  xOff, yOff);
}

void bb_drawEgg(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self)
{
    int16_t xOff = (self->pos.x >> DECIMAL_BITS) - entityManager->sprites[self->spriteIndex].originX - camera->pos.x;
    int16_t yOff = (self->pos.y >> DECIMAL_BITS) - entityManager->sprites[self->spriteIndex].originY - camera->pos.y;

    drawWsgSimple(&entityManager->sprites[self->spriteIndex].frames[((bb_eggData_t*)self->data)->stimulation / 120],
                  xOff, yOff);
}

void bb_drawMenu(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self)
{
    int16_t xDrawPos
        = (self->pos.x >> DECIMAL_BITS) - entityManager->sprites[self->spriteIndex].originX - camera->pos.x;

    int32_t yDrawPosFront
        = (self->pos.y >> DECIMAL_BITS) - entityManager->sprites[self->spriteIndex].originY - camera->pos.y;
    int32_t YDrawPosBack = yDrawPosFront - yDrawPosFront / 3;
    int32_t YDrawPosMid  = yDrawPosFront - yDrawPosFront / 2;

    // Background
    drawWsgSimple(&entityManager->sprites[BB_MENU].frames[0], xDrawPos, YDrawPosBack);
    // Text
    drawWsgSimple(&entityManager->sprites[BB_MENU].frames[1], xDrawPos + 14, YDrawPosMid + 28);
    // Midground
    drawWsgSimple(&entityManager->sprites[BB_MENU].frames[2], xDrawPos, yDrawPosFront);
}

void bb_drawMenuForeground(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self)
{
    int16_t xDrawPos
        = (self->pos.x >> DECIMAL_BITS) - entityManager->sprites[self->spriteIndex].originX - camera->pos.x;

    int32_t yDrawPosFront
        = (self->pos.y >> DECIMAL_BITS) - entityManager->sprites[self->spriteIndex].originY - camera->pos.y;
    // Foreground
    drawWsgSimple(&entityManager->sprites[BB_MENU].frames[3], xDrawPos, yDrawPosFront + 97);
}

void bb_drawStar(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self)
{
    int32_t xDrawPos = (self->pos.x >> DECIMAL_BITS) - camera->pos.x;
    // xDrawPos =  xDrawPos - xDrawPos/sData->parallaxDenominator;
    int32_t yDrawPos = (self->pos.y >> DECIMAL_BITS) - camera->pos.y;
    // yDrawPos =  yDrawPos - yDrawPos/sData->parallaxDenominator;
    drawLineFast(xDrawPos, yDrawPos, xDrawPos, yDrawPos, c555);
}

void bb_drawNothing(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self)
{
    // Do nothing lol
}

void bb_drawMenuBug(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self)
{
    bb_menuBugData_t* mbData = (bb_menuBugData_t*)self->data;
    int16_t xOff = (self->pos.x >> DECIMAL_BITS) - entityManager->sprites[self->spriteIndex].originX - camera->pos.x;
    int16_t yOff = (self->pos.y >> DECIMAL_BITS) - entityManager->sprites[self->spriteIndex].originY - camera->pos.y;

    uint8_t brightness = abs((self->pos.x >> DECIMAL_BITS) - (entityManager->viewEntity->pos.x >> DECIMAL_BITS));
    brightness         = (140 - brightness) / 23;
    brightness         = brightness > 5 ? 5 : brightness;

    drawWsg(&entityManager->sprites[self->spriteIndex].frames[brightness + self->currentAnimationFrame * 6], xOff, yOff,
            mbData->xVel < 0, false, 0);
}

void bb_drawCharacterTalk(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self)
{
    bb_dialogueData_t* dData = (bb_dialogueData_t*)self->data;

    drawWsgSimple(dData->sprite, 0, -dData->offsetY);

    if (dData->curString >= 0 && dData->curString < dData->numStrings)
    {
        drawText(&self->gameData->font, c344, dData->character, 13, 152);

        int16_t xOff = 13;
        int16_t yOff = 177;
        drawTextWordWrap(&self->gameData->font, c344, dData->strings[dData->curString], &xOff, &yOff, 253, 238);
    }
}

void bb_drawSimple(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self)
{
    drawWsgSimple(&entityManager->sprites[self->spriteIndex].frames[self->currentAnimationFrame],
                  -entityManager->sprites[self->spriteIndex].originX,
                  0 - entityManager->sprites[self->spriteIndex].originY);
}

void bb_drawAttachmentArm(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self)
{
    bb_attachmentArmData_t* aData = (bb_attachmentArmData_t*)self->data;
    drawWsg(&entityManager->sprites[self->spriteIndex].frames[self->currentAnimationFrame],
            (self->pos.x >> DECIMAL_BITS) - entityManager->sprites[self->spriteIndex].originX - camera->pos.x - 17,
            (self->pos.y >> DECIMAL_BITS) - entityManager->sprites[self->spriteIndex].originY - camera->pos.y + 14,
            false, false, (int16_t)aData->angle >> DECIMAL_BITS);
}

void bb_onCollisionHarpoon(bb_entity_t* self, bb_entity_t* other, bb_hitInfo_t* hitInfo)
{
    bb_projectileData_t* pData = (bb_projectileData_t*)self->data;
    bb_bugData_t* bData        = (bb_bugData_t*)other->data;
    // pause the harpoon animation as the tip will no longer even be rendered.
    self->paused = true;

    // Bug got stabbed
    bData->health -= 20;
    if (bData->health < 0)
    {
        midiPlayer_t* sfx = soundGetPlayerSfx();
        midiPlayerReset(sfx);
        soundPlaySfx(&self->gameData->sfxDirt, 0);

        bData->health               = 0;
        other->paused               = true;
        bb_physicsData_t* physData  = HEAP_CAPS_CALLOC_DBG(1, sizeof(bb_physicsData_t), MALLOC_CAP_SPIRAM);
        physData->vel               = divVec2d(pData->vel, 2);
        physData->bounceNumerator   = 2; // 66% bounce
        physData->bounceDenominator = 3;
        bb_setData(other, physData, PHYSICS_DATA);
        other->updateFunction = bb_updatePhysicsObject;
    }
    vecFl_t floatVel              = {(float)pData->vel.x, (float)pData->vel.y};
    bb_stuckHarpoonData_t* shData = HEAP_CAPS_CALLOC_DBG(1, sizeof(bb_stuckHarpoonData_t), MALLOC_CAP_SPIRAM);
    shData->parent                = other;
    shData->offset                = subVec2d(self->pos, other->pos);
    shData->floatVel              = normVecFl2d(floatVel);
    bb_setData(self, shData, STUCK_HARPOON_DATA);

    bb_clearCollisions(self, false);

    self->updateFunction = bb_updateStuckHarpoon;
    self->drawFunction   = bb_drawStuckHarpoon;
}

void bb_onCollisionSimple(bb_entity_t* self, bb_entity_t* other, bb_hitInfo_t* hitInfo)
{
    bb_garbotnikData_t* gData = (bb_garbotnikData_t*)other->data;
    other->pos.x              = hitInfo->pos.x + hitInfo->normal.x * other->halfWidth;
    other->pos.y              = hitInfo->pos.y + hitInfo->normal.y * other->halfHeight;
    if (hitInfo->normal.x == 0)
    {
        gData->vel.y = 0;
    }
    else
    {
        gData->vel.x = 0;
    }
}

void bb_onCollisionHeavyFalling(bb_entity_t* self, bb_entity_t* other, bb_hitInfo_t* hitInfo)
{
    bb_onCollisionSimple(self, other, hitInfo);
    bb_garbotnikData_t* gData = (bb_garbotnikData_t*)other->data;
    if (hitInfo->normal.y == 1)
    {
        gData->gettingCrushed = true;
    }
}

void bb_onCollisionCarIdle(bb_entity_t* self, bb_entity_t* other, bb_hitInfo_t* hitInfo)
{
    bb_onCollisionSimple(self, other, hitInfo);
    if (bb_createEntity(&self->gameData->entityManager, LOOPING_ANIMATION, false, BB_CAR_ACTIVE, 6,
                        self->pos.x >> DECIMAL_BITS, self->pos.y >> DECIMAL_BITS, false, false)
        != NULL)
    {
        bb_destroyEntity(self, false);
    }
}

void bb_onCollisionAttachmentArm(bb_entity_t* self, bb_entity_t* other, bb_hitInfo_t* hitInfo)
{
    bb_attachmentArmData_t* aData = (bb_attachmentArmData_t*)self->data;
    aData->angle += 18;
}

void bb_startGarbotnikIntro(bb_entity_t* self)
{
    // Force draw a loading screen
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c123);

    const char loadingStr[] = "Loading...";
    int32_t tWidth          = textWidth(&self->gameData->font, loadingStr);
    drawText(&self->gameData->font, c542, loadingStr, (TFT_WIDTH - tWidth) / 2,
             (TFT_HEIGHT - self->gameData->font.height) / 2);
    drawDisplayTft(NULL);

    // load all the tile sprites now that menu sprites where unloaded and camera motion has stopped.
    bb_loadWsgs(&self->gameData->tilemap);

    bb_entity_t* ovo
        = bb_createEntity(&self->gameData->entityManager, NO_ANIMATION, true, OVO_TALK, 1,
                          self->gameData->camera.camera.pos.x, self->gameData->camera.camera.pos.y, true, true);

    bb_dialogueData_t* dData = bb_createDialogueData(29); // 29

    strncpy(dData->character, "Dr. Ovo", sizeof(dData->character) - 1);
    dData->character[sizeof(dData->character) - 1] = '\0';

    bb_setCharacterLine(dData, 0, "Holy bug farts!\0");
    bb_setCharacterLine(dData, 1, "After I marketed the chilidog car freshener at MAGFest,\0");
    bb_setCharacterLine(dData, 2, "Garbotnik Industries' stock went up by 6,969%!\0"); // V longest possible string here
    bb_setCharacterLine(dData, 3, "I'm going to use my time machine\0");
    bb_setCharacterLine(dData, 4, "to steal the next big-selling trinket from the future now.\0");
    bb_setCharacterLine(dData, 5, "That will floor all my stakeholders and make me UNDEFINED money!\0");
    bb_setCharacterLine(dData, 6, "With that kind of cash, I can recruit 200 professional bassoon players\0");
    bb_setCharacterLine(dData, 7, "to the MAGFest Community Orchestra.\0");
    bb_setCharacterLine(dData, 8, "I'm so hyped to turn on my time machine for the first time!\0");
    bb_setCharacterLine(dData, 9, "Everything's in order.\0");
    bb_setCharacterLine(dData, 10, "Even Pango can't stop me!\0");
    bb_setCharacterLine(dData, 11, "I just have to attach the chaos core right here.\0");
    bb_setCharacterLine(dData, 12, "Where did I put that core?\0");
    bb_setCharacterLine(dData, 13, "hmmm...\0");
    bb_setCharacterLine(dData, 14, "What about in the freezer?\0");
    bb_setCharacterLine(dData, 15, "I've checked every inch of the death dumpster.\0");
    bb_setCharacterLine(dData, 16, "Glitch my circuits!\0");
    bb_setCharacterLine(dData, 17, "It must have gone out with the trash last Wednesday.\0");
    bb_setCharacterLine(dData, 18, "Can I get an F in the chat?\0");
    bb_setCharacterLine(dData, 19, "...\0");
    bb_setCharacterLine(dData, 20, "The chaos core is three times denser than a black hole.\0");
    bb_setCharacterLine(dData, 21, "Well if Garbotnik Sanitation Industries took it to the landfill,\0");
    bb_setCharacterLine(dData, 22, "then it is definitely at the VERY BOTTOM of the dump.\0");
    bb_setCharacterLine(dData, 23, "Not a problem.\0");
    bb_setCharacterLine(dData, 24, "We have the technology to retrieve it.\0");
    bb_setCharacterLine(dData, 25, "Safety first.\0");
    bb_setCharacterLine(dData, 26, "I've activated my cloning machine up here\0");
    bb_setCharacterLine(dData, 27, "in case I should perish on that nuclear wasteland.\0");
    bb_setCharacterLine(dData, 28, "YOLO!\0");

    dData->curString = -1;

    dData->endDialogueCB = &bb_afterGarbotnikIntro;

    bb_setData(ovo, dData, DIALOGUE_DATA);
}

void bb_startGarbotnikLandingTalk(bb_entity_t* self)
{
    bb_garbotnikData_t* gData = (bb_garbotnikData_t*)self->data;

    bb_entity_t* ovo
        = bb_createEntity(&self->gameData->entityManager, NO_ANIMATION, true, OVO_TALK, 1,
                          self->gameData->camera.camera.pos.x, self->gameData->camera.camera.pos.y, true, true);

    bb_dialogueData_t* dData = bb_createDialogueData(1);

    strncpy(dData->character, "Dr. Ovo", sizeof(dData->character) - 1);
    dData->character[sizeof(dData->character) - 1] = '\0';

    int16_t phraseIdx = 0;
    int16_t arraySize = sizeof(gData->landingPhrases) / sizeof(gData->landingPhrases[0]);
    while (gData->landingPhrases[phraseIdx] == -1)
    {
        if (phraseIdx == arraySize - 1)
        {
            // We've reached the end of saying everything.
            // create sequential numbers of all phrase indices
            for (int16_t i = 0; i < arraySize; i++)
                gData->landingPhrases[i] = i;
            // shuffle the array
            for (int16_t i = arraySize - 1; i > 0; i--)
            {
                int16_t j                = (int16_t)(bb_randomInt(0, INT_MAX) % (i + 1));
                int16_t temp             = gData->landingPhrases[i];
                gData->landingPhrases[i] = gData->landingPhrases[j];
                gData->landingPhrases[j] = temp;
            }
            phraseIdx = 0;
        }
        else
        {
            phraseIdx++;
        }
    }
    switch (gData->landingPhrases[phraseIdx])
    {
        case 0:
        {
            // Max dialogue string roughly: here----V
            bb_setCharacterLine(dData, 0, "Ah, sweet stench! How I've longed for your orlfactory embrace.\0");
            break;
        }
        case 1:
        {
            bb_setCharacterLine(dData, 0, "Tonight's special: Beetle Bruschetta with a side of centipede salad!\0");
            break;
        }
        case 2:
        {
            bb_setCharacterLine(dData, 0, "Another day, another dump full of delectable delights!\0");
            break;
        }
        case 3:
        {
            bb_setCharacterLine(dData, 0, "Step aside, garbage! The doctor is in!\0");
            break;
        }
        case 4:
        {
            bb_setCharacterLine(dData, 0, "I must find that chaos core at all costs!\0");
            break;
        }
        default:
        {
            break;
        }
    }

    dData->curString     = -1;
    dData->endDialogueCB = &bb_afterGarbotnikLandingTalk;

    bb_setData(ovo, dData, DIALOGUE_DATA);
}

void bb_startGarbotnikCloningTalk(bb_entity_t* self)
{
    bb_entity_t* ovo
        = bb_createEntity(&self->gameData->entityManager, NO_ANIMATION, true, OVO_TALK, 1, 0, 0, true, true);

    bb_dialogueData_t* dData = bb_createDialogueData(2); // 29

    strncpy(dData->character, "Dr. Ovo", sizeof(dData->character) - 1);
    dData->character[sizeof(dData->character) - 1] = '\0';

    bb_setCharacterLine(dData, 0, "I'm feeling fresh, baby!"); // V longest possible string here
    bb_setCharacterLine(dData, 1, "It was a good move taking omega3 fish oils before backing up my brain.\0");

    dData->curString = -1;

    dData->endDialogueCB = &bb_afterGarbotnikIntro;

    bb_setData(ovo, dData, DIALOGUE_DATA);
}

void bb_startGarbotnikEggTutorialTalk(bb_entity_t* self)
{
    bb_entity_t* ovo
        = bb_createEntity(&self->gameData->entityManager, NO_ANIMATION, true, OVO_TALK, 1,
                          self->gameData->camera.camera.pos.x, self->gameData->camera.camera.pos.y, true, true);

    bb_dialogueData_t* dData = bb_createDialogueData(2);

    strncpy(dData->character, "Dr. Ovo", sizeof(dData->character) - 1);
    dData->character[sizeof(dData->character) - 1] = '\0';

    // Max dialogue string roughly:                                                                         here----V
    bb_setCharacterLine(dData, 0, "Oooey Gooey! Look at that dark gelatinous mass!\0");
    bb_setCharacterLine(dData, 1, "I can use the directional buttons on my swadge to fly over there.\0");

    dData->curString     = -1;
    dData->endDialogueCB = &bb_afterGarbotnikEggTutorialTalk;

    bb_setData(ovo, dData, DIALOGUE_DATA);
}

void bb_startGarbotnikFuelTutorialTalk(bb_entity_t* self)
{
    bb_destroyEntity(self->gameData->entityManager.viewEntity, false);
    self->gameData->entityManager.viewEntity = self->gameData->entityManager.playerEntity;

    bb_entity_t* ovo
        = bb_createEntity(&self->gameData->entityManager, NO_ANIMATION, true, OVO_TALK, 1,
                          self->gameData->camera.camera.pos.x, self->gameData->camera.camera.pos.y, true, true);

    bb_dialogueData_t* dData = bb_createDialogueData(4);

    strncpy(dData->character, "Dr. Ovo", sizeof(dData->character) - 1);
    dData->character[sizeof(dData->character) - 1] = '\0';

    // Max dialogue string roughly:                                                                         here----V
    bb_setCharacterLine(dData, 0, "When I travel away from the booster,\0");
    bb_setCharacterLine(dData, 1, "I've got to keep an eye on my fuel level at all times.\0");
    bb_setCharacterLine(dData, 2, "Sit back atop the booster before all the lights\0");
    bb_setCharacterLine(dData, 3, "around the outside of the swadge turn off.\0");

    dData->curString     = -1;
    dData->endDialogueCB = &bb_afterGarbotnikFuelTutorialTalk;

    bb_setData(ovo, dData, DIALOGUE_DATA);
}

void bb_afterGarbotnikFuelTutorialTalk(bb_entity_t* self)
{
    self->gameData->isPaused = false;
}

void bb_afterGarbotnikEggTutorialTalk(bb_entity_t* self)
{
    bb_goToData* tData = (bb_goToData*)self->gameData->entityManager.viewEntity->data;
    tData->tracking    = self->gameData->entityManager.playerEntity;
    tData->midPointSqDist
        = sqMagVec2d(divVec2d((vec_t){(tData->tracking->pos.x >> DECIMAL_BITS)
                                          - (self->gameData->entityManager.viewEntity->pos.x >> DECIMAL_BITS),
                                      (tData->tracking->pos.y >> DECIMAL_BITS)
                                          - (self->gameData->entityManager.viewEntity->pos.y >> DECIMAL_BITS)},
                              2));

    tData->executeOnArrival = &bb_startGarbotnikFuelTutorialTalk;

    self->gameData->entityManager.viewEntity->updateFunction = &bb_updatePOI;
}

void bb_afterGarbotnikIntro(bb_entity_t* self)
{
    for (int i = 0; i < 3; i++)
    {
        if (self->gameData->entityManager.boosterEntities[i] != NULL)
        {
            self->gameData->entityManager.activeBooster = self->gameData->entityManager.boosterEntities[i];

            bb_goToData* tData = (bb_goToData*)self->gameData->entityManager.viewEntity->data;
            tData->tracking    = self->gameData->entityManager.activeBooster;
            tData->midPointSqDist
                = sqMagVec2d(divVec2d((vec_t){(tData->tracking->pos.x >> DECIMAL_BITS)
                                                  - (self->gameData->entityManager.viewEntity->pos.x >> DECIMAL_BITS),
                                              (tData->tracking->pos.y >> DECIMAL_BITS)
                                                  - (self->gameData->entityManager.viewEntity->pos.y >> DECIMAL_BITS)},
                                      2));

            tData->executeOnArrival = &bb_deployBooster;

            self->gameData->entityManager.viewEntity->updateFunction = &bb_updatePOI;

            return;
        }
    }
}

void bb_afterGarbotnikLandingTalk(bb_entity_t* self)
{
    globalMidiPlayerSetVolume(MIDI_BGM, 13);
    midiPlayer_t* player = globalMidiPlayerGet(MIDI_BGM);
    midiPlayerReset(player);
    player->songFinishedCallback = NULL;
    midiGmOn(player);
    player->loop = true;
    globalMidiPlayerPlaySong(&self->gameData->bgm, MIDI_BGM);

    for (uint8_t i = 0; i < MAX_ENTITIES; i++)
    {
        bb_entity_t* curEntity = &self->gameData->entityManager.entities[i];
        if (curEntity->spriteIndex == EGG_LEAVES)
        {
            self->gameData->entityManager.viewEntity
                = bb_createEntity(&(self->gameData->entityManager), NO_ANIMATION, true, NO_SPRITE_POI, 1,
                                  (self->gameData->entityManager.playerEntity->pos.x >> DECIMAL_BITS),
                                  (self->gameData->entityManager.playerEntity->pos.y >> DECIMAL_BITS), true, false);
            bb_goToData* tData = (bb_goToData*)self->gameData->entityManager.viewEntity->data;
            tData->tracking    = curEntity;

            tData->midPointSqDist
                = sqMagVec2d(divVec2d((vec_t){(tData->tracking->pos.x >> DECIMAL_BITS)
                                                  - (self->gameData->entityManager.viewEntity->pos.x >> DECIMAL_BITS),
                                              (tData->tracking->pos.y >> DECIMAL_BITS)
                                                  - (self->gameData->entityManager.viewEntity->pos.y >> DECIMAL_BITS)},
                                      2));
            tData->executeOnArrival                                  = &bb_startGarbotnikEggTutorialTalk;
            self->gameData->entityManager.viewEntity->updateFunction = &bb_updatePOI;
        }
    }
}

void bb_deployBooster(bb_entity_t* self) // separates from the death dumpster in orbit.
{
    bb_destroyEntity(self->gameData->entityManager.viewEntity, false);
    self->gameData->entityManager.viewEntity = self->gameData->entityManager.activeBooster;

    bb_rocketData_t* rData = (bb_rocketData_t*)self->gameData->entityManager.activeBooster->data;
    bb_destroyEntity(rData->flame, false);
    rData->flame = NULL;

    self->gameData->entityManager.activeBooster->updateFunction = &bb_updateRocketLanding;
}

void bb_crumbleDirt(bb_entity_t* self, uint8_t gameFramesPerAnimationFrame, uint8_t tile_i, uint8_t tile_j,
                    bool zeroHealth)
{
    // Create a crumble animation
    bb_createEntity(&(self->gameData->entityManager), ONESHOT_ANIMATION, false, CRUMBLE_ANIM,
                    gameFramesPerAnimationFrame, tile_i * TILE_SIZE + HALF_TILE, tile_j * TILE_SIZE + HALF_TILE, false,
                    false);

    // Play sfx
    midiPlayer_t* sfx = soundGetPlayerSfx();
    midiPlayerReset(sfx);
    soundPlaySfx(&self->gameData->sfxBump, 0);

    if (zeroHealth)
    {
        if (self->gameData->tilemap.fgTiles[tile_i][tile_j].embed == EGG_EMBED)
        {
            vec_t tilePos = {.x = tile_i * TILE_SIZE + HALF_TILE, .y = tile_j * TILE_SIZE + HALF_TILE};
            // create a bug
            bb_entity_t* bug = bb_createEntity(&self->gameData->entityManager, LOOPING_ANIMATION, false,
                                               bb_randomInt(8, 13), 1, tilePos.x, tilePos.y, false, false);
            if (bug != NULL)
            {
                if (self->gameData->tilemap.fgTiles[tile_i][tile_j].entity != NULL)
                {
                    bb_entity_t* egg
                        = ((bb_eggLeavesData_t*)(self->gameData->tilemap.fgTiles[tile_i][tile_j].entity->data))->egg;
                    if (egg != NULL)
                    {
                        // destroy the egg
                        bb_destroyEntity(egg, false);
                    }
                    // destroy this (eggLeaves)
                    bb_destroyEntity(self->gameData->tilemap.fgTiles[tile_i][tile_j].entity, false);
                }
                self->gameData->tilemap.fgTiles[tile_i][tile_j].embed = NOTHING_EMBED;
            }
        }
    }
}

bb_dialogueData_t* bb_createDialogueData(uint8_t numStrings)
{
    bb_dialogueData_t* dData = HEAP_CAPS_CALLOC_DBG(1, sizeof(bb_dialogueData_t), MALLOC_CAP_SPIRAM);
    dData->numStrings        = numStrings;
    dData->offsetY           = -240;
    dData->loadedIdx         = bb_randomInt(0, 7);
    dData->sprite            = HEAP_CAPS_CALLOC_DBG(1, sizeof(wsg_t), MALLOC_CAP_SPIRAM);
    char wsg_name[strlen("ovo_talk") + 9]; // 6 extra characters makes room for up to a 2 digit number + ".wsg" + null
                                           // terminator ('\0')
    snprintf(wsg_name, sizeof(wsg_name), "%s%d.wsg", "ovo_talk", dData->loadedIdx);
    loadWsg(wsg_name, dData->sprite, true);

    dData->strings = HEAP_CAPS_CALLOC_DBG(numStrings, sizeof(char*), MALLOC_CAP_SPIRAM);
    return dData;
}

void bb_setCharacterLine(bb_dialogueData_t* dData, uint8_t index, const char* str)
{
    dData->strings[index] = HEAP_CAPS_CALLOC_DBG(strlen(str) + 1, sizeof(char), MALLOC_CAP_SPIRAM);
    strcpy(dData->strings[index], str);
}

void bb_freeDialogueData(bb_dialogueData_t* dData)
{
    for (int i = 0; i < dData->numStrings; i++)
    {
        FREE_DBG(dData->strings[i]); // Free each string
    }
    FREE_DBG(dData->strings); // Free the array of string pointers
    FREE_DBG(dData);          // Free the struct itself
}