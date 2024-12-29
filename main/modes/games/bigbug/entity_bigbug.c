//==============================================================================
// Includes
//==============================================================================
#include <stdlib.h>
#include <esp_log.h>
#include <math.h>
#include <limits.h>

#include "mode_bigbug.h"
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
void bb_initializeEntity(bb_entity_t* self, bb_entityManager_t* entityManager, bb_gameData_t* gameData)
{
    self->active   = false;
    self->gameData = gameData;
}

void bb_setData(bb_entity_t* self, void* data, bb_data_type_t dataType)
{
    if (self->data != NULL)
    {
        heap_caps_free(self->data);
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
            heap_caps_free(shiftedCollision->checkOthers);
            heap_caps_free(shiftedCollision);
        }
        heap_caps_free(self->collisions);
    }
    self->collisions = NULL;
}

void bb_destroyEntity(bb_entity_t* self, bool caching)
{
    if (NULL == self)
    {
        return;
    }

    // some particular entities get sprites freed.
    switch (self->spriteIndex)
    {
        case BB_CAR:
        {
            if(self->currentAnimationFrame == 59)
            {
                freeWsg(&self->gameData->entityManager.sprites[BB_CAR].frames[59]);
            }
            else
            {
                for (int frame = 0; frame < 60; frame++)
                {
                    freeWsg(&self->gameData->entityManager.sprites[BB_CAR].frames[frame]);
                }
            }
            break;
        }
        case BB_GRABBY_HAND:
        {
            for (int frame = 0; frame < 3; frame++)
            {
                freeWsg(&self->gameData->entityManager.sprites[BB_GRABBY_HAND].frames[frame]);
            }
            break;
        }
        case BB_DOOR:
        {
            for (int frame = 0; frame < 2; frame++)
            {
                freeWsg(&self->gameData->entityManager.sprites[BB_DOOR].frames[frame]);
            }
            break;
        }
        case BB_SWADGE:
        {
            for (int frame = 0; frame < 12; frame++)
            {
                freeWsg(&self->gameData->entityManager.sprites[BB_SWADGE].frames[frame]);
            }
            break;
        }
        case BB_FOOD_CART:
        {
            bb_foodCartData_t* fcData = (bb_foodCartData_t*)self->data;
            //The food cart needs to track its own caching status to communicate just-in-time loading between both pieces.
            fcData->isCached          = caching;
            if (((bb_foodCartData_t*)fcData->partner)->isCached)
            {
                for (int frame = 0; frame < 2; frame++)
                {
                    freeWsg(&self->gameData->entityManager.sprites[BB_FOOD_CART].frames[frame]);
                }
            }
            break;
        }
        default:
        {
            break;
        }
    }

    // Zero out most info (but not references to manager type things) for entity to be reused.
    self->active    = false;
    self->cacheable = false;

    if (self->data != NULL && caching == false)
    {
        switch (self->dataType)
        {
            case DIALOGUE_DATA:
            {
                bb_dialogueData_t* dData = (bb_dialogueData_t*)self->data;
                for (int i = 0; i < dData->numStrings; i++)
                {
                    heap_caps_free(dData->strings[i]);
                }
                heap_caps_free(dData->strings);
                freeWsg(&dData->sprite);
                freeWsg(&dData->spriteNext);
                break;
            }
            case GAME_OVER_DATA:
            {
                bb_gameOverData_t* goData = (bb_gameOverData_t*)self->data;
                if (goData->wsgLoaded)
                {
                    freeWsg(&goData->fullscreenGraphic);
                    goData->wsgLoaded = false;
                }
                break;
            }
            case CAR_DATA:
            {
                bb_carData_t* cData = (bb_carData_t*)self->data;
                if (cData->midiLoaded)
                {
                    unloadMidiFile(&cData->alarm);
                    cData->midiLoaded = false;
                }
                break;
            }
            default:
            {
                break;
            }
        }
        heap_caps_free(self->data);
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
    self->animationTimer              = 0;
    self->gameFramesPerAnimationFrame = 1;
    self->currentAnimationFrame       = 0;
    self->halfWidth                   = 0;
    self->halfHeight                  = 0;
    self->cSquared                    = 0;

    self->gameData->entityManager.activeEntities--;
    // ESP_LOGD(BB_TAG,"%d/%d entities v\n", self->gameData->entityManager.activeEntities, MAX_ENTITIES);
}

void bb_updateRocketLanding(bb_entity_t* self)
{
    bb_rocketData_t* rData = (bb_rocketData_t*)self->data;

    if (self->pos.y > -2600 && rData->flame == NULL)
    {
        rData->flame = bb_createEntity(&(self->gameData->entityManager), LOOPING_ANIMATION, false, FLAME_ANIM, 16,
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
                rData->yVel = rData->yVel >> 4;
                bb_destroyEntity(rData->flame, false);
                rData->flame         = NULL;
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

    // load gameplay sprites
    if (self->pos.y > -50000 && !self->gameData->tilemap.wsgsLoaded)
    {
        // load all the tile sprites now that menu sprites where unloaded.
        bb_loadWsgs(&self->gameData->tilemap);
    }

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
        // ESP_LOGD(BB_TAG,"pos.y %d\n",self->pos.y);
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
    if (!(self->gameData->endDayChecks & (1 << 0))) // if the pause illusion bit is not set
    {
        self->pos.y += (rData->yVel * self->gameData->elapsedUs) >> 17;
    }
    else //this will fake the starfield scrolling during character talk.
    {
        self->gameData->camera.velocity.y = (rData->yVel * self->gameData->elapsedUs) >> 17;
        // iterate all entities
        for (int i = 0; i < MAX_ENTITIES; i++)
        {
            bb_entity_t* curEntity = &(self->gameData->entityManager.entities[i]);
            if (curEntity->active && curEntity->spriteIndex == NO_SPRITE_STAR)
            {
                curEntity->pos.y -= self->gameData->camera.velocity.y;
            }
        }
    }
    rData->flame->pos.y = self->pos.y;

    if (self->pos.y < -77136)//reached the death dumpster
    {
        self->pos.y = -77136;
        rData->yVel = 0;

        freeFont(&self->gameData->cgFont);
        freeFont(&self->gameData->cgThinFont);

        bb_setupMidi();
        unloadMidiFile(&self->gameData->bgm);
        loadMidiFile("BigBug_Dr.Garbotniks Home.mid", &self->gameData->bgm, true);
        globalMidiPlayerPlaySong(&self->gameData->bgm, MIDI_BGM);

        self->gameData->day++;

        bb_entity_t* ovo
            = bb_createEntity(&self->gameData->entityManager, NO_ANIMATION, true, OVO_TALK, 1,
                              self->gameData->camera.camera.pos.x, self->gameData->camera.camera.pos.y, true, true);
        bb_dialogueData_t* dData = bb_createDialogueData(5, "Ovo");

        bb_setCharacterLine(dData, 0, "Ovo", "Gaaaash dangit.");
        bb_setCharacterLine(dData, 1, "Ovo", "DARN IT! DARN IT! DARN IT! DARN IT!");
        bb_setCharacterLine(dData, 2, "Ovo", "GLITCH MY CIRCUITS!");
        bb_setCharacterLine(dData, 3, "Ovo", "I'll have to patch this up before all the air gets out.");
        bb_setCharacterLine(dData, 4, "Ovo", "And the hardware store closes so early.");
        dData->curString     = -1;
        dData->endDialogueCB = &bb_afterGarbotnikIntro;
        bb_setData(ovo, dData, DIALOGUE_DATA);

        self->gameData->entityManager.viewEntity = bb_createEntity(
            &(self->gameData->entityManager), NO_ANIMATION, true, NO_SPRITE_POI, 1,
            self->gameData->camera.camera.pos.x + 140, self->gameData->camera.camera.pos.y + 120, true, false);

        // Create a crumble animation
        bb_createEntity(&(self->gameData->entityManager), ONESHOT_ANIMATION, false, CRUMBLE_ANIM, 3,
                        self->pos.x >> DECIMAL_BITS, (self->pos.y >> DECIMAL_BITS) - 30, true, false);

        self->updateFunction = NULL;
        return;
    }
    else if (self->pos.y < -68000 && !(self->gameData->endDayChecks & (1 << 0))
             && !(self->gameData->endDayChecks & (1 << 2))) // if not pause illusion and dive summary hasn't shown yet.
    {
        self->gameData->endDayChecks = self->gameData->endDayChecks | (1 << 0); // set the pause illusion bit.
        self->gameData->endDayChecks = self->gameData->endDayChecks | (1 << 2); // set the dive summary bit.
        bb_createEntity(&(self->gameData->entityManager), NO_ANIMATION, true, BB_DIVE_SUMMARY, 1,
                        (self->pos.x >> DECIMAL_BITS) - 105, (self->pos.y >> DECIMAL_BITS) + 150, true, true);
        loadFont("cg_font_body.font", &self->gameData->cgFont, false);
        loadFont("cg_font_body_thin.font", &self->gameData->cgThinFont, false);
    }
    else if (self->pos.y < -52000 && !(self->gameData->endDayChecks & (1 << 0))
             && !(self->gameData->endDayChecks & (1 << 1))) // if not pause illusion and pangos have not spoken
    {
        self->gameData->endDayChecks = self->gameData->endDayChecks | (1 << 0); // set the pause illusion bit.
        self->gameData->endDayChecks = self->gameData->endDayChecks | (1 << 1); // set the pango has spoken bit.
        bb_createEntity(&(self->gameData->entityManager), LOOPING_ANIMATION, false, BB_PANGO_AND_FRIENDS, 3,
                        (self->pos.x >> DECIMAL_BITS) - 77, (self->pos.y >> DECIMAL_BITS) - 100, true, false);
    }
    else if (self->pos.y < -40000 && self->gameData->tilemap.wsgsLoaded)
    {
        bb_freeWsgs(&self->gameData->tilemap);
    }
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
        bb_setupMidi(); // stops the music

        bb_loadSprite("rocket", 42, 1, &self->gameData->entityManager.sprites[ROCKET_ANIM]);
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
        bb_crumbleDirt(self->gameData, 3, hitInfo.tile_i, hitInfo.tile_j, true);
    }
    return;
}

void bb_updateHeavyFalling(bb_entity_t* self)
{
    bb_heavyFallingData_t* hfData = (bb_heavyFallingData_t*)self->data;
    hfData->yVel++;
    self->pos.y += (hfData->yVel * self->gameData->elapsedUs) >> 15;

    // ESP_LOGD(BB_TAG,"tilemap addr: %p\n", &self->gameData->tilemap);
    // ESP_LOGD(BB_TAG,"self    addr: %p\n", self);
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
        bb_crumbleDirt(self->gameData, 3, hitInfo.tile_i, hitInfo.tile_j, true);
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
        ((bb_rocketData_t*)self->data)->armAngle     = 2880; // That is 180 down position.
        ((bb_attachmentArmData_t*)arm->data)->rocket = self;

        bb_entity_t* grabbyHand
            = bb_createEntity(&self->gameData->entityManager, LOOPING_ANIMATION, true, BB_GRABBY_HAND, 5,
                              self->pos.x >> DECIMAL_BITS, (self->pos.y >> DECIMAL_BITS) - 53, false, false);
        ((bb_grabbyHandData_t*)grabbyHand->data)->rocket = self;

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

    if (gData->damageEffect > 0)
    {
        gData->damageEffect -= self->gameData->elapsedUs >> 11;
    }

    // Fuel decrements with time. Right shifting by 10 is fairly close to
    // converting microseconds to milliseconds without requiring division.
    gData->fuel -= (((self->gameData->elapsedUs >> 10) * self->gameData->GarbotnikStat_fuelConsumptionRate) >> 2);
    if (gData->fuel < 0)
    {
        bb_physicsData_t* physData  = heap_caps_calloc(1, sizeof(bb_physicsData_t), MALLOC_CAP_SPIRAM);
        physData->vel               = gData->vel;
        physData->bounceNumerator   = 1; // 25% bounce
        physData->bounceDenominator = 4;
        bb_setData(self, physData, PHYSICS_DATA);
        self->updateFunction = bb_updateGarbotnikDying;
        self->drawFunction   = NULL;
        return;
    }
    else if (gData->fuel < 38000 && self->gameData->bgm.length == 7217)
    {
        // exploration song length 7217
        // hurry up song length 6480
        bb_setupMidi();
        unloadMidiFile(&self->gameData->bgm);
        loadMidiFile("Big Bug Hurry up.mid", &self->gameData->bgm, true);
        globalMidiPlayerPlaySong(&self->gameData->bgm, MIDI_BGM);
    }
    else if (gData->fuel >= 38000 && self->gameData->bgm.length == 6480)
    {
        bb_setupMidi();
        unloadMidiFile(&self->gameData->bgm);
        loadMidiFile("BigBugExploration.mid", &self->gameData->bgm, true);
        globalMidiPlayerPlaySong(&self->gameData->bgm, MIDI_BGM);
    }

    // touchpad stuff
    gData->fire     = gData->touching;
    gData->touching = getTouchJoystick(&gData->phi, &gData->r, &gData->intensity);
    // The outer half of the circle launches the same velocity so you don't have to touch at the very edge.
    if (gData->r > 561)
    {
        gData->r = 561;
    }

    gData->fire = gData->fire && !gData->touching; // is true for one frame upon touchpad release.

    gData->harpoonCooldown -= self->gameData->elapsedUs >> 11;
    if (gData->harpoonCooldown < -250)
    {
        gData->harpoonCooldown = -250;
    }

    if (gData->touching && gData->harpoonCooldown < 0 && gData->numHarpoons > 0)
    {
        gData->harpoonCooldown = self->gameData->GarbotnikStat_fireTime;
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
            pData->vel.x = (x - 512) >> 3;
            pData->vel.y = (-y + 512) >> 3;
        }
    }

    // record the previous frame's position before any logic.
    gData->previousPos = self->pos;

    vec_t accel = {.x = 0, .y = 0};

    // Update garbotnik's velocity if a button is currently down
    switch (self->gameData->btnState & 0b1111)
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

    // ESP_LOGD(BB_TAG,"accel x: %d\n", accel.x);
    // ESP_LOGD(BB_TAG,"elapsed: %d", (int32_t) elapsedUs);
    // ESP_LOGD(BB_TAG,"offender: %d\n", (int32_t) elapsedUs / 100000);
    // ESP_LOGD(BB_TAG,"now   x: %d\n", mulVec2d(accel, elapsedUs) / 100000).x);

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

    // ESP_LOGD(BB_TAG,"Garbotnik X: %d\n", self->pos);
    // //keep the player in bounds
    if (self->pos.x < 2560)
    {
        self->pos.x = 2560;
    }
    else if (self->pos.x > 35216)
    {
        self->pos.x = 35216;
    }

    // tow cable stuff
    // apply spring force and unhook far towed entities
    node_t* current = gData->towedEntities.first;
    while (current != NULL)
    {
        bb_entity_t* curEntity = (bb_entity_t*)current->val;
        vec_t toFrom           = subVec2d(curEntity->pos, self->pos);
        int32_t dist           = sqMagVec2d(toFrom);
        // if more than 100px away
        if (dist > 2560000)
        {
            // detach
            node_t* next = current->next;

            // if(self->spriteIndex == BB_DONUT)
            // {
            //     //turn off physics for the donut when not towed.
            //     heap_caps_free(self->data);
            //     self->data = NULL;
            //     ((bb_entity_t*)current->val)->updateFunction = NULL;
            // }

            removeEntry(&gData->towedEntities, current);
            current = next;
        }
        else
        {
            // apply the spring force
            // Spring and damping coefficients
            const int32_t SPRING_CONSTANT  = 40000; // Adjust for desired springiness
            const int32_t DAMPING_CONSTANT = 30;    // Adjust for desired damping

            bb_physicsData_t* pData = (bb_physicsData_t*)curEntity->data;

            // Distance squared
            int64_t distSquared = (int64_t)toFrom.x * toFrom.x + (int64_t)toFrom.y * toFrom.y;
            // Compute distance and normalize displacement vector
            dist = sqrt(distSquared);
            fastNormVec(&toFrom.x, &toFrom.y);

            // Spring force: F_spring = -k * displacement
            int32_t springForceX = -(toFrom.x * dist) / SPRING_CONSTANT;
            int32_t springForceY = -(toFrom.y * dist) / SPRING_CONSTANT;

            // Damping force: F_damping = -b * relative_velocity
            int32_t dampingForceX = (pData->vel.x - gData->vel.x) / DAMPING_CONSTANT;
            int32_t dampingForceY = (pData->vel.y - gData->vel.y) / DAMPING_CONSTANT;

            // Apply the force
            if (curEntity->updateFunction == &bb_updatePhysicsObject) // dead bug or donut
            {
                pData->vel.x += springForceX - dampingForceX;
                pData->vel.y += springForceY - dampingForceY + 4;
            }
            else // live bug applies force to garbotnik
            {
                gData->vel.x -= springForceX;
                gData->vel.y -= springForceY;
            }

            current = current->next;
        }
    }

    // if 'a' button down, tether another entity if it's close enough
    if ((self->gameData->btnState & 0b10000) >> 4)
    {
        int16_t best_i     = -1;     // negative 1 means no valid candidates found
        uint16_t best_dist = 0xFFFF; // the distance of the best_i
        for (uint8_t i = 0; i < MAX_ENTITIES; i++)
        {
            bb_entity_t* curEntity = &self->gameData->entityManager.entities[i];
            // if it is a bug or a donut
            if ((curEntity->spriteIndex >= 8 && curEntity->spriteIndex <= 13) || curEntity->spriteIndex == BB_DONUT)
            {
                // if it is not already towed
                bool isTowed = false;
                node_t* cur  = gData->towedEntities.first;
                while (cur != NULL)
                {
                    void* curNode = (bb_midgroundTileInfo_t*)cur->val;
                    if (curNode == curEntity)
                    {
                        isTowed = true;
                        break;
                    }
                    cur = cur->next;
                }
                if (!isTowed)
                {
                    uint16_t dist = (uint16_t)sqMagVec2d(
                        (vec_t){(curEntity->pos.x - self->pos.x) >> 5, (curEntity->pos.y - self->pos.y) >> 5});
                    // if the bug is within 70px of garbotnik
                    if (dist < 1225 && dist < best_dist)
                    {
                        // new best candidate found!
                        best_i    = i;
                        best_dist = dist;
                    }
                }
            }
        }
        if (best_i != -1)
        {
            // attach a tow cable
            midiPlayer_t* sfx = soundGetPlayerSfx();
            midiPlayerReset(sfx);
            soundPlaySfx(&self->gameData->sfxTether, 0);
            push(&gData->towedEntities, (void*)&self->gameData->entityManager.entities[best_i]);
        }
    }

    bb_hitInfo_t hitInfo = {0};
    bb_collisionCheck(&self->gameData->tilemap, self, &gData->previousPos, &hitInfo);
    if (hitInfo.hit == false)
    {
        return;
    }
    self->pos.x = hitInfo.pos.x + hitInfo.normal.x * self->halfWidth;
    self->pos.y = hitInfo.pos.y + hitInfo.normal.y * self->halfHeight;

    // Check for digging
    int32_t dot = dotVec2d(gData->vel, hitInfo.normal);
    if (dot < -40)
    { // velocity angle is opposing garbage normal vector. Tweak number for different threshold.
        ///////////////////////
        // digging detected! //
        ///////////////////////

        // Update the dirt by decrementing it.
        self->gameData->tilemap.fgTiles[hitInfo.tile_i][hitInfo.tile_j].health
            -= self->gameData->GarbotnikStat_diggingStrength;
        if (self->gameData->tilemap.fgTiles[hitInfo.tile_i][hitInfo.tile_j].health < 0)
        {
            self->gameData->tilemap.fgTiles[hitInfo.tile_i][hitInfo.tile_j].health = 0;
        }

        bb_midgroundTileInfo_t* tile
            = (bb_midgroundTileInfo_t*)&self->gameData->tilemap.fgTiles[hitInfo.tile_i][hitInfo.tile_j];

        if (tile->health == 0 || tile->health == 1
            || (tile->health < 5 && tile->health + self->gameData->GarbotnikStat_diggingStrength >= 5))
        {
            // Create a crumble
            bb_crumbleDirt(self->gameData, 2, hitInfo.tile_i, hitInfo.tile_j, !tile->health);
        }
        else
        {
            // Create a bump animation
            bb_createEntity(&(self->gameData->entityManager), ONESHOT_ANIMATION, false, BUMP_ANIM, 4,
                            hitInfo.pos.x >> DECIMAL_BITS, hitInfo.pos.y >> DECIMAL_BITS, true, false);
        }

        ////////////////////////////////
        // Mirror garbotnik's velocity//
        ////////////////////////////////
        // Reflect the velocity vector along the normal
        // See http://www.sunshine2k.de/articles/coding/vectorreflection/vectorreflection.html
        // ESP_LOGD(BB_TAG,"hit squared speed: %" PRId32 "\n", sqMagVec2d(gData->vel));

        // range is roughly 2600 to 7100
        // ESP_LOGD(BB_TAG,"dot %d\n", dot);
        // ESP_LOGD(BB_TAG,"thing: %d\n",sqMagVec2d(gData->vel) * dot);
        int32_t bounceScalar = 3;
        if (sqMagVec2d(gData->vel) * dot < -360000)
        {
            bounceScalar = 2;
        }
        if (sqMagVec2d(gData->vel) * dot < -550000)
        {
            bounceScalar = 1;
        }
        ESP_LOGD(BB_TAG, "bounceScalar %" PRId32 "\n", bounceScalar);
        gData->vel = mulVec2d(
            subVec2d(gData->vel, mulVec2d(hitInfo.normal, (2 * dotVec2d(gData->vel, hitInfo.normal)))), bounceScalar);
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
    if (physData->tileTime > 101)
    {
        bb_triggerGameOver(self);
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
        bb_stuckHarpoonData_t* shData = heap_caps_calloc(1, sizeof(bb_stuckHarpoonData_t), MALLOC_CAP_SPIRAM);
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

        if (GARBOTNIK_DATA == self->gameData->entityManager.playerEntity->dataType)
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
                    bb_crumbleDirt(self->gameData, 2, hitInfo.tile_i, hitInfo.tile_j, true);
                    midiPlayer_t* sfx = soundGetPlayerSfx();
                    midiPlayerReset(sfx);
                    soundPlaySfx(&self->gameData->sfxEgg, 0);
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
        ESP_LOGD(BB_TAG, "entity_bigbug.c this should not happen\n");
    }

    // destroy the egg
    bb_destroyEntity(((bb_eggLeavesData_t*)(self->data))->egg, false);

    // destroy this
    bb_destroyEntity(self, false);
}

void bb_updateFarDestroy(bb_entity_t* self)
{
    if (self->spriteIndex == BB_PANGO_AND_FRIENDS)
    {
        freeWsg(&self->gameData->entityManager.sprites[BB_PANGO_AND_FRIENDS].frames[0]);
        freeWsg(&self->gameData->entityManager.sprites[BB_PANGO_AND_FRIENDS].frames[1]);
    }
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
        bb_freeSprite(&self->gameData->entityManager.sprites[BB_MENU]);

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

// 1 for 90 degrees clockwise. -2 for 180 degrees counterclockwise, etc...
void bb_rotateBug(bb_entity_t* self, int8_t orthogonalRotations)
{
    if (orthogonalRotations & 1) // if odd
    {
        // rotate hitbox
        int16_t temp     = self->halfHeight;
        self->halfHeight = self->halfWidth;
        self->halfWidth  = temp;
    }
    bb_buData_t* bData = (bb_buData_t*)self->data;
    // keep gravity in range 0 through 3 for down, left, up, right
    bData->gravity = ((bData->gravity + orthogonalRotations) % 4 + 4) % 4;
}

void bb_updateBugShooting(bb_entity_t* self)
{
    if (bb_randomInt(0, 100) < 1)
    {
        // call it paused and update frames in it's own update function because this one uses another spriteIdx.
        bb_entity_t* spit = bb_createEntity(&self->gameData->entityManager, LOOPING_ANIMATION, true, BB_SPIT, 10,
                                            self->pos.x >> DECIMAL_BITS, self->pos.y >> DECIMAL_BITS, true, false);
        if (self->gameData->entityManager.playerEntity != NULL)
        {
            bb_spitData_t* sData = (bb_spitData_t*)spit->data;
            sData->vel           = subVec2d(self->gameData->entityManager.playerEntity->pos, spit->pos);
            fastNormVec(&sData->vel.x, &sData->vel.y);
            sData->vel = (vec_t){sData->vel.x >> 5, sData->vel.y >> 5};
        }
    }
}

void bb_updateWalkingBug(bb_entity_t* self)
{
    bb_buData_t* bData = (bb_buData_t*)self->data;

    if (bData->damageEffect > 0)
    {
        bData->damageEffect -= self->gameData->elapsedUs >> 11;
    }

    if (!self->cacheable && self->gameData->carFightState == 0)
    {
        self->cacheable = true;
    }
    if (bData->fallSpeed > 19)
    {
        switch (bData->gravity)
        {
            case BB_LEFT:
                bb_rotateBug(self, -1);
                break;
            case BB_UP:
                bb_rotateBug(self, 2);
                break;
            case BB_RIGHT:
                bb_rotateBug(self, 1);
                break;
            default:
                break;
        }
        bData->gravity = BB_DOWN;
    }
    if (bData->fallSpeed < 30)
    {
        bData->fallSpeed++;
    }

    switch (bData->gravity)
    {
        case BB_DOWN:
        {
            self->pos.y += bData->fallSpeed;
            break;
        }
        case BB_LEFT:
        {
            self->pos.x -= bData->fallSpeed;
            break;
        }
        case BB_UP:
        {
            self->pos.y -= bData->fallSpeed;
            break;
        }
        default: // right:
        {
            self->pos.x += bData->fallSpeed;
            break;
        }
    }
    bb_hitInfo_t hitInfo = {0};
    bb_collisionCheck(&self->gameData->tilemap, self, NULL, &hitInfo);
    if (hitInfo.hit == true)
    {
        bData->fallSpeed = 0;
        switch (bData->gravity)
        {
            case BB_DOWN:
            {
                if (self->pos.y > ((hitInfo.tile_j * TILE_SIZE + 16) << DECIMAL_BITS))
                {
                    if (hitInfo.normal.x == 1)
                    {
                        if (bData->faceLeft)
                        {
                            bb_rotateBug(self, 1);
                        }
                        self->pos.x = ((hitInfo.tile_i * TILE_SIZE + TILE_SIZE) << DECIMAL_BITS)
                                      + hitInfo.normal.x * self->halfWidth;
                    }
                    else if (hitInfo.normal.x == -1)
                    {
                        if (!bData->faceLeft)
                        {
                            bb_rotateBug(self, -1);
                        }
                        self->pos.x
                            = ((hitInfo.tile_i * TILE_SIZE) << DECIMAL_BITS) + hitInfo.normal.x * self->halfWidth;
                    }
                }
                else
                {
                    self->pos.x = hitInfo.pos.x + hitInfo.normal.x * self->halfWidth;
                    self->pos.x += (bData->faceLeft * -2 + 1) * bData->speed;
                }
                self->pos.y = hitInfo.pos.y + hitInfo.normal.y * self->halfHeight;
                break;
            }
            case BB_LEFT:
            {
                if (self->pos.x < ((hitInfo.tile_i * TILE_SIZE + 16) << DECIMAL_BITS))
                {
                    if (hitInfo.normal.y == 1)
                    {
                        if (bData->faceLeft)
                        {
                            bb_rotateBug(self, 1);
                        }
                        self->pos.y = ((hitInfo.tile_j * TILE_SIZE + TILE_SIZE) << DECIMAL_BITS)
                                      + hitInfo.normal.y * self->halfHeight;
                    }
                    else if (hitInfo.normal.y == -1)
                    {
                        if (!bData->faceLeft)
                        {
                            bb_rotateBug(self, -1);
                        }
                        self->pos.y
                            = ((hitInfo.tile_j * TILE_SIZE) << DECIMAL_BITS) + hitInfo.normal.y * self->halfHeight;
                    }
                }
                else
                {
                    self->pos.y = hitInfo.pos.y + hitInfo.normal.y * self->halfHeight;
                    self->pos.y += (bData->faceLeft * -2 + 1) * bData->speed;
                }
                self->pos.x = hitInfo.pos.x + hitInfo.normal.x * self->halfWidth;
                break;
            }
            case BB_UP:
            {
                if (self->pos.y < ((hitInfo.tile_j * TILE_SIZE + 16) << DECIMAL_BITS))
                {
                    if (hitInfo.normal.x == -1)
                    {
                        if (bData->faceLeft)
                        {
                            bb_rotateBug(self, 1);
                        }
                        self->pos.x
                            = ((hitInfo.tile_i * TILE_SIZE) << DECIMAL_BITS) + hitInfo.normal.x * self->halfWidth;
                    }
                    else if (hitInfo.normal.x == 1)
                    {
                        if (!bData->faceLeft)
                        {
                            bb_rotateBug(self, -1);
                        }
                        self->pos.x = ((hitInfo.tile_i * TILE_SIZE + TILE_SIZE) << DECIMAL_BITS)
                                      + hitInfo.normal.x * self->halfWidth;
                    }
                }
                else
                {
                    self->pos.x = hitInfo.pos.x + hitInfo.normal.x * self->halfWidth;
                    self->pos.x -= (bData->faceLeft * -2 + 1) * bData->speed;
                }
                self->pos.y = hitInfo.pos.y + hitInfo.normal.y * self->halfHeight;
                break;
            }
            default: // right
            {
                if (self->pos.x > ((hitInfo.tile_i * TILE_SIZE + 16) << DECIMAL_BITS))
                {
                    if (hitInfo.normal.y == -1)
                    {
                        if (bData->faceLeft)
                        {
                            bb_rotateBug(self, 1);
                        }
                        self->pos.y
                            = ((hitInfo.tile_j * TILE_SIZE) << DECIMAL_BITS) + hitInfo.normal.y * self->halfHeight;
                    }
                    else if (hitInfo.normal.y == 1)
                    {
                        if (!bData->faceLeft)
                        {
                            bb_rotateBug(self, -1);
                        }
                        self->pos.y = ((hitInfo.tile_j * TILE_SIZE + TILE_SIZE) << DECIMAL_BITS)
                                      + hitInfo.normal.y * self->halfHeight;
                    }
                }
                else
                {
                    self->pos.y = hitInfo.pos.y + hitInfo.normal.y * self->halfHeight;
                    self->pos.y -= (bData->faceLeft * -2 + 1) * bData->speed;
                }
                self->pos.x = hitInfo.pos.x + hitInfo.normal.x * self->halfWidth;
                break;
            }
        }
    }
    else if (bData->fallSpeed == 18)
    {
        bb_rotateBug(self, bData->faceLeft * -2 + 1);
        switch (bData->gravity)
        {
            case BB_DOWN:
                self->pos.y += abs(self->halfWidth - self->halfHeight) + bData->fallSpeed;
                break;
            case BB_LEFT:
                self->pos.x -= abs(self->halfWidth - self->halfHeight) + bData->fallSpeed;
                break;
            case BB_UP:
                self->pos.y -= abs(self->halfWidth - self->halfHeight) + bData->fallSpeed;
                break;
            default: // BB_RIGHT
                self->pos.x += abs(self->halfWidth - self->halfHeight) + bData->fallSpeed;
                break;
        }
    }
    bb_updateBugShooting(self);
}

void bb_updateFlyingBug(bb_entity_t* self)
{
    bb_buggoData_t* bData = (bb_buggoData_t*)self->data;
    if (bData->damageEffect > 0)
    {
        bData->damageEffect -= self->gameData->elapsedUs >> 11;
    }

    vec_t previousPos    = self->pos;
    self->pos            = addVec2d(self->pos, mulVec2d(bData->direction, self->gameData->elapsedUs >> 11));
    bb_hitInfo_t hitInfo = {0};
    bb_collisionCheck(&self->gameData->tilemap, self, NULL, &hitInfo);
    if (hitInfo.hit == true)
    {
        self->pos        = previousPos;
        bData->direction = rotateVec2d(divVec2d((vec_t){0, bData->speed * 200}, 800), bb_randomInt(0, 359));
        bData->faceLeft  = bData->direction.x < 0;
    }

    bb_updateBugShooting(self);
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
                    = bb_createEntity(&self->gameData->entityManager, NO_ANIMATION, true, ROCKET_ANIM, 16,
                                      (self->pos.x >> DECIMAL_BITS) - 96 + 96 * rocketIdx,
                                      (self->pos.y >> DECIMAL_BITS) + 375, true, false);

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
        bb_menuBugData_t* mbData                   = heap_caps_calloc(1, sizeof(bb_menuBugData_t), MALLOC_CAP_SPIRAM);
        mbData->xVel                               = bb_randomInt(-2, 2);
        mbData->xVel
            = mbData->xVel == 1 ? mbData->xVel - 1 : mbData->xVel; // So as not to match the treadmill speed exactly.
        mbData->firstTrip = true;
        heap_caps_free(self->gameData->menuBug->data);
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
            tData->speed+=4;
        }
        else if (tData->midPointSqDist == 0)
        {
            self->updateFunction = NULL;
            tData->executeOnArrival(self);
            return;
        }
        else if (tData->speed > 0)
        {
            tData->speed-=4;
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

    dData->blinkTimer += 3;

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
                freeWsg(&dData->sprite);
                freeWsg(&dData->spriteNext);
                int8_t characterSprite = 0;
                if (strcmp(dData->characters[dData->curString], "Ovo") == 0)
                {
                    characterSprite = bb_randomInt(0, 6);
                    loadWsgInplace("dialogue_next.wsg", &dData->spriteNext, true, bb_decodeSpace, bb_hsd); // TODO free
                }
                else if (strcmp(dData->characters[dData->curString], "Pixel") == 0)
                {
                    characterSprite = bb_randomInt(7, 8);
                    //borrow sprite from UTT
                    loadWsgInplace("pixil_rs.wsg", &dData->spriteNext, true, bb_decodeSpace, bb_hsd);
                }
                else if (strcmp(dData->characters[dData->curString], "Pango") == 0)
                {
                    characterSprite = bb_randomInt(9, 10);
                    //borrow sprite from UTT
                    loadWsgInplace("hotdog_rs.wsg", &dData->spriteNext, true, bb_decodeSpace, bb_hsd);
                }
                else if (strcmp(dData->characters[dData->curString], "Po") == 0)
                {
                    characterSprite = bb_randomInt(11, 13);
                    //borrow sprite from UTT
                    loadWsgInplace("hand_rs.wsg", &dData->spriteNext, true, bb_decodeSpace, bb_hsd);
                }

                char wsg_name[strlen("ovo-talk-") + 9]; // 6 extra characters makes room for up to a 2 digit number +
                                                        // ".wsg" + null terminator ('\0')
                snprintf(wsg_name, sizeof(wsg_name), "%s%d.wsg", "ovo_talk", characterSprite);
                loadWsgInplace(wsg_name, &dData->sprite, true, bb_decodeSpace, bb_hsd);

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
    bb_rocketData_t* rData        = (bb_rocketData_t*)aData->rocket->data;
    self->pos                     = aData->rocket->pos;
    self->pos.y -= 464;         // that is 29 << DECIMAL_BITS
    if (rData->armAngle > 2880) // that is 180 << DECIMAL_BITS
    {
        rData->armAngle -= 16; // that is 1 << DECIMAL_BITS
    }
    if (rData->armAngle >= 5744) // that is 359 << DECIMAL_BITS
    {
        bb_setupMidi();
        unloadMidiFile(&self->gameData->bgm);
        loadMidiFile("BigBug_Space Travel.mid", &self->gameData->bgm, true);
        globalMidiPlayerPlaySong(&self->gameData->bgm, MIDI_BGM);

        bb_destroyEntity(self->gameData->entityManager.playerEntity, false);
        self->gameData->entityManager.playerEntity = NULL;
        self->gameData->entityManager.viewEntity   = aData->rocket;
        aData->rocket->currentAnimationFrame       = 0;

        rData->flame
            = bb_createEntity(&(self->gameData->entityManager), LOOPING_ANIMATION, false, FLAME_ANIM, 6,
                              aData->rocket->pos.x >> DECIMAL_BITS, aData->rocket->pos.y >> DECIMAL_BITS, true, false);

        rData->flame->updateFunction  = &bb_updateFlame;
        aData->rocket->updateFunction = &bb_updateRocketLiftoff;

        bb_destroyEntity(self, false);
    }
}

void bb_updateGameOver(bb_entity_t* self)
{
    bb_gameOverData_t* goData = (bb_gameOverData_t*)self->data;

    if (self->gameData->btnDownState & PB_A)
    {
        if (self->currentAnimationFrame == 0)
        {
            self->currentAnimationFrame = 1;
            if (goData->wsgLoaded)
            {
                freeWsg(&goData->fullscreenGraphic);
                goData->wsgLoaded = false;
            }
            loadWsgInplace("GameOver1.wsg", &goData->fullscreenGraphic, true, bb_decodeSpace, bb_hsd);
            goData->wsgLoaded = true;
        }
        else
        {
            // increment booster animation frame to look destroyed
            self->gameData->entityManager.activeBooster->currentAnimationFrame++;
            bb_heavyFallingData_t* hfData = heap_caps_calloc(1, sizeof(bb_heavyFallingData_t), MALLOC_CAP_SPIRAM);
            hfData->yVel                  = ((bb_rocketData_t*)self->gameData->entityManager.activeBooster->data)->yVel;
            bb_setData(self->gameData->entityManager.activeBooster, hfData, HEAVY_FALLING_DATA);
            self->gameData->entityManager.activeBooster->drawFunction = NULL;
            // this booster's grabby hand will destroy itself next time in it's own update loop.

            uint8_t boosterIdx = 0;
            while (boosterIdx < 3)
            {
                if (self->gameData->entityManager.boosterEntities[boosterIdx]->currentAnimationFrame != 41)
                {
                    break;
                }
                boosterIdx++;
            }

            self->gameData->entityManager.activeBooster = NULL;
            if (boosterIdx > 2)
            {
                // IDK it is really really game over here.
                ESP_LOGD(BB_TAG, "finish me\n");
            }
            else
            {
                self->gameData->entityManager.activeBooster = self->gameData->entityManager.boosterEntities[boosterIdx];

                bb_destroyEntity(self, false);
                bb_startGarbotnikCloningTalk(self->gameData->entityManager.deathDumpster);
            }
        }
    }
}

void bb_updateRadarPing(bb_entity_t* self)
{
    bb_radarPingData_t* rpData = (bb_radarPingData_t*)self->data;
    rpData->timer -= self->gameData->elapsedUs >> 10;
    // printf("timer: %d\n", rpData->timer);
    if ((rpData->timer < 100)
        && (rpData->reflectionIdx < (sizeof(rpData->reflections) / sizeof(rpData->reflections[0]))))
    {
        rpData->timer = bb_randomInt(150, 250);
        rpData->reflections[rpData->reflectionIdx].pos
            = addVec2d(self->pos, rotateVec2d((vec_t){rpData->radius << DECIMAL_BITS, 0}, bb_randomInt(0, 359)));
        rpData->reflectionIdx++;
        // printf("reflectionIdx: %d\n", rpData->reflectionIdx);
    }

    for (int reflectionIdx = 0; reflectionIdx < rpData->reflectionIdx; reflectionIdx++)
    {
        rpData->reflections[reflectionIdx].radius += 5;
    }

    rpData->radius += 5;
    if (rpData->radius > 1300)
    {
        rpData->executeAfterPing(self);
        bb_destroyEntity(self, false);
    }
}

void bb_updateGrabbyHand(bb_entity_t* self)
{
    bb_grabbyHandData_t* ghData = (bb_grabbyHandData_t*)self->data;
    // destroy grabby hand if the booster is broken.
    if (ghData->rocket->currentAnimationFrame == 41)
    {
        bb_destroyEntity(self, false);
        return;
    }

    self->pos.y = ghData->rocket->pos.y - 848; // that is 53 << 4

    // retreat into the booster
    if (self->gameData->entityManager.sprites[BB_GRABBY_HAND].originY > -26)
    {
        self->gameData->entityManager.sprites[BB_GRABBY_HAND].originY -= 2;
        if (ghData->grabbed != NULL)
        {
            ghData->grabbed->pos.x = self->pos.x;
            ghData->grabbed->pos.y
                = self->pos.y - (self->gameData->entityManager.sprites[BB_GRABBY_HAND].originY << DECIMAL_BITS);
        }
    }
    else if (ghData->grabbed != NULL)
    {
        if (self->gameData->entityManager.playerEntity != NULL && self->gameData->entityManager.playerEntity->dataType == GARBOTNIK_DATA)
        {
            // iterate towed entities
            bb_garbotnikData_t* gData = (bb_garbotnikData_t*)self->gameData->entityManager.playerEntity->data;
            node_t* current           = gData->towedEntities.first;
            while (current != NULL)
            {
                if (current->val == ghData->grabbed)
                {
                    // detach tow cable
                    node_t* next = current->next;
                    removeEntry(&gData->towedEntities, current);
                    current = next;
                }
                else
                {
                    current = current->next;
                }
            }
        }
        bb_destroyEntity(ghData->grabbed, false);
        ghData->grabbed = NULL;

        bb_rocketData_t* rData = (bb_rocketData_t*)ghData->rocket->data;
        rData->numBugs++;
        midiPlayer_t* sfx = soundGetPlayerSfx();
        midiPlayerReset(sfx);
        soundPlaySfx(&self->gameData->sfxCollection, 0);
        if (rData->numBugs % 10 == 0) // set to % 1 for quick testing the entire radar tech tree
        {
            bb_entity_t* radarPing
                = bb_createEntity(&self->gameData->entityManager, NO_ANIMATION, true, BB_RADAR_PING, 1,
                                  self->pos.x >> DECIMAL_BITS, self->pos.y >> DECIMAL_BITS, true, false);
            bb_radarPingData_t* rpData = (bb_radarPingData_t*)radarPing->data;
            rpData->color              = c541;
            rpData->executeAfterPing   = &bb_upgradeRadar;
        }

        self->currentAnimationFrame = 0;
        self->paused                = true;
    }
    if (self->currentAnimationFrame == 2)
    {
        self->paused = true;
    }
}

void bb_updateDoor(bb_entity_t* self)
{
    if (self->gameData->carFightState == 0) // no fight
    {
        self->cacheable             = true;
        self->currentAnimationFrame = 0;
        if (self->collisions != NULL)
        {
            bb_clearCollisions(self, false);
        }
    }
}

void bb_updateCarActive(bb_entity_t* self)
{
    if (bb_randomInt(0, 60) == 0)
    {
        bb_playCarAlarm(self);
    }

    if (self->currentAnimationFrame == 22)
    {
        self->currentAnimationFrame = 2;
        self->animationTimer        = self->currentAnimationFrame * self->gameFramesPerAnimationFrame;
    }
    if (self->gameData->carFightState == 0) // all the arena bugs are dead
    {
        // make it cacheable again.
        self->cacheable = true;
        // transition to car opening
        self->currentAnimationFrame = 22;
        self->animationTimer        = self->currentAnimationFrame * self->gameFramesPerAnimationFrame;
        self->updateFunction        = &bb_updateCarOpen;
    }
}

void bb_updateCarOpen(bb_entity_t* self)
{
    if (self->currentAnimationFrame == 59 && !self->paused)
    {
        self->cacheable = true;

        bb_setupMidi();
        unloadMidiFile(&self->gameData->bgm);
        loadMidiFile("BigBugExploration.mid", &self->gameData->bgm, true);
        globalMidiPlayerPlaySong(&self->gameData->bgm, MIDI_BGM);

        // free most previous car frames.
        for (int i = 1; i < 59; i++)
        {
            freeWsg(&self->gameData->entityManager.sprites[BB_CAR].frames[i]);
        }
        switch (((bb_carData_t*)self->data)->reward)
        {
            case BB_DONUT:
            {
                // spawn a donut as a reward for completing the fight
                bb_createEntity(&self->gameData->entityManager, NO_ANIMATION, true, BB_DONUT, 1,
                                (self->pos.x >> DECIMAL_BITS) + 5, (self->pos.y >> DECIMAL_BITS), true, false);
                break;
            }
            default: // BB_SWADGE
            {
                // spawn a swadge as a reward for completing the fight
                bb_createEntity(&self->gameData->entityManager, LOOPING_ANIMATION, false, BB_SWADGE, 9,
                                (self->pos.x >> DECIMAL_BITS) + 40, (self->pos.y >> DECIMAL_BITS) - 32, true, false);
                break;
            }
        }
        self->paused = true;
    }
    else
    {
        self->cacheable = false;
    }
}

void bb_updateSpit(bb_entity_t* self)
{
    // increment the frame counter
    self->animationTimer++;
    self->currentAnimationFrame = self->animationTimer / self->gameFramesPerAnimationFrame;
    // if frame reached the end of the animation
    if (self->currentAnimationFrame >= self->gameData->entityManager.sprites[BB_FUEL].numFrames)
    {
        // reset the animation
        self->animationTimer        = 0;
        self->currentAnimationFrame = 0;
    }

    bb_spitData_t* sData = (bb_spitData_t*)self->data;

    // Update spits's lifetime. I think not using elapsed time is good enough.
    sData->lifetime++;
    if (sData->lifetime > 1000)
    {
        bb_destroyEntity(self, false);
        return;
    }

    // Update spits's position
    self->pos = addVec2d(self->pos, mulVec2d(sData->vel, (self->gameData->elapsedUs >> 13)));

    bb_hitInfo_t hitInfo = {0};
    bb_collisionCheck(&self->gameData->tilemap, self, NULL, &hitInfo);
    if (hitInfo.hit)
    {
        bb_destroyEntity(self, false);
    }
}

void bb_updatePangoAndFriends(bb_entity_t* self)
{
    if (self->gameData->entityManager.sprites[BB_PANGO_AND_FRIENDS].originY < 0)
    {
        self->gameData->entityManager.sprites[BB_PANGO_AND_FRIENDS].originY++;
    }
    else
    {
        bb_entity_t* ovo
            = bb_createEntity(&self->gameData->entityManager, NO_ANIMATION, true, OVO_TALK, 1,
                              self->gameData->camera.camera.pos.x, self->gameData->camera.camera.pos.y, true, true);

        if (self->gameData->day == 0)
        {
            bb_dialogueData_t* dData = bb_createDialogueData(18, "Pixel");

            // longest possible string     " "
            //  bb_setCharacterLine(dData, 0, "A A A A A A A A A A A A A A A A A A A A A A A A A A A A A A A A A A A A A
            //  A A A A A A A A A A A A A A A A A A A A A A A");
            bb_setCharacterLine(dData, 0, "Pixel", "huff... huff... we actually caught up to it!");
            bb_setCharacterLine(dData, 1, "Pixel",
                                "ugfhh, my hair is starting to hurt. Can you make this quick, Pango?");
            bb_setCharacterLine(dData, 2, "Pixel", "Hey, it looks like it's Garbotnik after all!");
            bb_setCharacterLine(dData, 3, "Po", "That's a cool ride, bro.");
            bb_setCharacterLine(dData, 4, "Ovo", "What could you idiots possibly want?");
            bb_setCharacterLine(dData, 5, "Pango",
                                "We were fascinated with your shiny metal tube falling from space this morning.");
            bb_setCharacterLine(dData, 6, "Po", "Yeah, we were wondering if you could give us a ride.");
            bb_setCharacterLine(dData, 7, "Ovo", "I'm not a taxi service.");
            bb_setCharacterLine(dData, 8, "Pixel", "We'll pay you in swadges.");
            bb_setCharacterLine(dData, 9, "Po", "What are you doing at the landfill, anyway?");
            bb_setCharacterLine(dData, 10, "Ovo", "None of your business!");
            bb_setCharacterLine(dData, 11, "Ovo", "Get off my freaking back!");
            bb_setCharacterLine(dData, 12, "Ovo", "I have a dive summary to review.");
            bb_setCharacterLine(dData, 13, "Pixel", "What's a dive summary?");
            bb_setCharacterLine(dData, 14, "Ovo", "It tells me how many bugs I annihilated.");
            bb_setCharacterLine(dData, 15, "Pixel", "Hmm... That sounds like he's up to no good.");
            bb_setCharacterLine(dData, 16, "Ovo", "Bug off!");
            bb_setCharacterLine(dData, 17, "Ovo", "Filthy peasants.");

            dData->curString     = -1;
            dData->endDialogueCB = &bb_afterLiftoffInteraction;
            bb_setData(ovo, dData, DIALOGUE_DATA);
        }
        else if (self->gameData->day == 1)
        {
            bb_dialogueData_t* dData = bb_createDialogueData(10, "Pixel");

            bb_setCharacterLine(dData, 0, "Pixel",
                                "Hey, Mr. Garbotnik! I really love the color of your umbrella so much!");
            bb_setCharacterLine(dData, 1, "Ovo", "Glitch my circuits!");
            bb_setCharacterLine(dData, 2, "Ovo", "Now I have to find a new one!");
            bb_setCharacterLine(dData, 3, "Po", "What's your problem, dude? Pink is a great color!");
            bb_setCharacterLine(dData, 4, "Pango", "Nobody spends their Thursdays at the dump for leisure.");
            bb_setCharacterLine(dData, 5, "Pango", "We're going to find out what you're up to here.");
            bb_setCharacterLine(dData, 6, "Ovo", "Oh, bug off!");
            bb_setCharacterLine(dData, 7, "Ovo", "Don't you have anything better to do?");
            bb_setCharacterLine(dData, 8, "Ovo", "Lousy ingrates.");
            bb_setCharacterLine(dData, 9, "Pixel", "That man doesn't know the meaning of friendship.");

            dData->curString     = -1;
            dData->endDialogueCB = &bb_afterLiftoffInteraction;
            bb_setData(ovo, dData, DIALOGUE_DATA);
        }
        else if (self->gameData->day == 99)
        {
            bb_dialogueData_t* dData = bb_createDialogueData(3, "Pango");

            bb_setCharacterLine(dData, 0, "Pango", "You're completely unhinged.");
            bb_setCharacterLine(dData, 1, "Ovo", "Aaaaaaaand...");
            bb_setCharacterLine(dData, 2, "Ovo", "You're not?");

            dData->curString     = -1;
            dData->endDialogueCB = &bb_afterLiftoffInteraction;
            bb_setData(ovo, dData, DIALOGUE_DATA);
        }
        else
        {
            bb_dialogueData_t* dData = bb_createDialogueData(3, "Po");

            bb_setCharacterLine(dData, 0, "Po", "Still looking?");
            char pangosLine[34];
            snprintf(pangosLine, sizeof(pangosLine), "It's been %d days at the dump.", self->gameData->day + 1);
            bb_setCharacterLine(dData, 1, "Pango", pangosLine);
            bb_setCharacterLine(dData, 2, "Pixel", "I guess that means you're having no luck.");
            bb_setCharacterLine(dData, 2, "Ovo", "Glitch my circuits!");
            bb_setCharacterLine(dData, 2, "Ovo", "I. know.");
            bb_setCharacterLine(dData, 2, "Ovo", "Bug off!");

            dData->curString     = -1;
            dData->endDialogueCB = &bb_afterLiftoffInteraction;
            bb_setData(ovo, dData, DIALOGUE_DATA);
        }
        self->updateFunction = NULL;
    }
}

void bb_updateDiveSummary(bb_entity_t* self)
{
    // decrement y pos until it is 1500 units lower than active booster y.
    if (self->pos.y > self->gameData->entityManager.activeBooster->pos.y - 1500)
    {
        self->pos.y -= 16;
    }
    else
    {
        // if 'a' is pressed, set the update function to NULL.
        if (self->gameData->btnDownState & PB_A)
        {
            // set the active booster's pause illusion to false.
            self->gameData->endDayChecks
                = self->gameData->endDayChecks & ~(1 << 0); // set the pause illusion bit to false.
            self->updateFunction = NULL;
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

    // draw the tow cables
    node_t* current = gData->towedEntities.first;
    while (current != NULL)
    {
        bb_entity_t* curEntity = (bb_entity_t*)current->val;
        drawLineFast((self->pos.x >> DECIMAL_BITS) - camera->pos.x, (self->pos.y >> DECIMAL_BITS) - camera->pos.y,
                     (curEntity->pos.x >> DECIMAL_BITS) - camera->pos.x,
                     (curEntity->pos.y >> DECIMAL_BITS) - camera->pos.y, c425);
        current = current->next;
    }

    int16_t xOff = (self->pos.x >> DECIMAL_BITS) - entityManager->sprites[self->spriteIndex].originX - camera->pos.x;
    int16_t yOff = (self->pos.y >> DECIMAL_BITS) - entityManager->sprites[self->spriteIndex].originY - camera->pos.y;

    uint8_t frameIndex;
    bool useSimple = true;

    // Determine frameIndex and draw style
    if (gData->yaw.x < -1400)
    {
        frameIndex = 0;
    }
    else if (gData->yaw.x < -400)
    {
        frameIndex = 1;
    }
    else if (gData->yaw.x < 400)
    {
        frameIndex = 2;
    }
    else if (gData->yaw.x < 1400)
    {
        frameIndex = 1;
        useSimple  = false;
    }
    else
    {
        frameIndex = 0;
        useSimple  = false;
    }

    // Draw the sprite
    if (gData->damageEffect > 70 || (gData->damageEffect > 0 && bb_randomInt(0, 1)))
    {
        drawWsgPalette(&entityManager->sprites[self->spriteIndex].frames[frameIndex], xOff, yOff,
                       &self->gameData->damagePalette, !useSimple, false, 0);
    }
    else if (useSimple)
    {
        drawWsgSimple(&entityManager->sprites[self->spriteIndex].frames[frameIndex], xOff, yOff);
    }
    else
    {
        drawWsg(&entityManager->sprites[self->spriteIndex].frames[frameIndex], xOff, yOff, true, false, 0);
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

void bb_drawBasicEmbed(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self)
{
    if (self->gameData->entityManager.playerEntity != NULL)
    {
        vec_t lookup
            = {.x = (self->pos.x >> DECIMAL_BITS) - (self->gameData->entityManager.playerEntity->pos.x >> DECIMAL_BITS)
                    + self->gameData->tilemap.headlampWsg.w,
               .y = (self->pos.y >> DECIMAL_BITS) - (self->gameData->entityManager.playerEntity->pos.y >> DECIMAL_BITS)
                    + self->gameData->tilemap.headlampWsg.h};

        lookup             = divVec2d(lookup, 2);
        uint8_t brightness = 0;
        if (GARBOTNIK_DATA == self->gameData->entityManager.playerEntity->dataType)
        {
            brightness = bb_foregroundLighting(
                &(self->gameData->tilemap.headlampWsg), &lookup,
                &(((bb_garbotnikData_t*)self->gameData->entityManager.playerEntity->data)->yaw.x));
        }
        drawWsgSimple(&entityManager->sprites[self->spriteIndex].frames[brightness],
                      (self->pos.x >> DECIMAL_BITS) - entityManager->sprites[self->spriteIndex].originX - camera->pos.x,
                      (self->pos.y >> DECIMAL_BITS) - entityManager->sprites[self->spriteIndex].originY
                          - camera->pos.y);
    }
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

    drawWsgSimple(&dData->sprite, 0, -dData->offsetY);

    if (dData->curString >= 0 && dData->curString < dData->numStrings)
    {
        paletteColor_t textColor = c525; // garbotnik
        if (strcmp(dData->characters[dData->curString], "Pixel") == 0)
        {
            textColor = c402;
        }
        else if (strcmp(dData->characters[dData->curString], "Pango") == 0)
        {
            textColor = c552;
        }
        else if (strcmp(dData->characters[dData->curString], "Po") == 0)
        {
            textColor = c544;
        }
        if (dData->blinkTimer > 0)
        {
            //The sprites I took from UTT need to be drawn a little more to the left.
            drawWsgSimple(&dData->spriteNext, 254 + (textColor == c525 ? 4 : 0), -dData->offsetY + 186 );
        }
        drawText(&self->gameData->font, textColor, dData->characters[dData->curString], 13, 152);

        int16_t xOff = 13;
        int16_t yOff = 177;
        drawTextWordWrap(&self->gameData->font, textColor, dData->strings[dData->curString], &xOff, &yOff, 253, 230);
    }
}

void bb_drawGameOver(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self)
{
    bb_gameOverData_t* goData = (bb_gameOverData_t*)self->data;
    if (goData->wsgLoaded)
    {
        drawWsgSimple(&goData->fullscreenGraphic, 0, 0);
    }
}

void bb_drawAttachmentArm(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self)
{
    bb_rocketData_t* rData = (bb_rocketData_t*)((bb_attachmentArmData_t*)self->data)->rocket->data;
    drawWsg(&entityManager->sprites[self->spriteIndex].frames[self->currentAnimationFrame],
            (self->pos.x >> DECIMAL_BITS) - entityManager->sprites[self->spriteIndex].originX - camera->pos.x - 17,
            (self->pos.y >> DECIMAL_BITS) - entityManager->sprites[self->spriteIndex].originY - camera->pos.y + 14,
            false, false, (int16_t)rData->armAngle >> DECIMAL_BITS);
}

void bb_drawDeathDumpster(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self)
{
    bb_DeathDumpsterData_t* ddData = (bb_DeathDumpsterData_t*)self->data;
    vec_t shiftedCameraPos         = camera->pos;
    shiftedCameraPos.x             = (shiftedCameraPos.x + 140) << DECIMAL_BITS;
    shiftedCameraPos.y             = (shiftedCameraPos.y + 120) << DECIMAL_BITS;
    if (self->pos.x > shiftedCameraPos.x - 3200 && self->pos.x < shiftedCameraPos.x + 3200
        && self->pos.y > shiftedCameraPos.y - 2880 && self->pos.y < shiftedCameraPos.y + 3520)
    { // if it is close
        if (!ddData->loaded)
        {
            bb_sprite_t* deathDumpsterSprite
                = bb_loadSprite("DeathDumpster", 1, 1, &entityManager->sprites[BB_DEATH_DUMPSTER]);
            deathDumpsterSprite->originX = 138;
            deathDumpsterSprite->originY = 100;

            ddData->loaded = true;
        }
        int16_t xOff
            = (self->pos.x >> DECIMAL_BITS) - entityManager->sprites[self->spriteIndex].originX - camera->pos.x;
        int16_t yOff
            = (self->pos.y >> DECIMAL_BITS) - entityManager->sprites[self->spriteIndex].originY - camera->pos.y;
        drawWsgSimple(&entityManager->sprites[self->spriteIndex].frames[self->currentAnimationFrame], xOff, yOff);
    }
    else
    { // if it is far
        if (ddData->loaded)
        {
            bb_freeSprite(&entityManager->sprites[BB_DEATH_DUMPSTER]);
            ddData->loaded = false;
        }
    }
}

void bb_drawRadarPing(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self)
{
    bb_radarPingData_t* rpData = (bb_radarPingData_t*)self->data;

    for (int reflectionIdx = 0; reflectionIdx < rpData->reflectionIdx; reflectionIdx++)
    {
        drawCircle((rpData->reflections[reflectionIdx].pos.x >> DECIMAL_BITS) - camera->pos.x,
                   (rpData->reflections[reflectionIdx].pos.y >> DECIMAL_BITS) - camera->pos.y,
                   rpData->reflections[reflectionIdx].radius, rpData->color);
    }

    drawCircle((self->pos.x >> DECIMAL_BITS) - camera->pos.x, (self->pos.y >> DECIMAL_BITS) - camera->pos.y,
               rpData->radius, rpData->color);
    drawCircle((self->pos.x >> DECIMAL_BITS) - camera->pos.x, (self->pos.y >> DECIMAL_BITS) - camera->pos.y,
               rpData->radius * 3 / 4, rpData->color);
}

void bb_drawBug(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self)
{
    uint8_t brightness = 5;
    int16_t xOff = (self->pos.x >> DECIMAL_BITS) - entityManager->sprites[self->spriteIndex].originX - camera->pos.x;
    int16_t yOff = (self->pos.y >> DECIMAL_BITS) - entityManager->sprites[self->spriteIndex].originY - camera->pos.y;
    if (entityManager->playerEntity != NULL)
    {
        vec_t lookup = {.x = (self->pos.x >> DECIMAL_BITS) - (entityManager->playerEntity->pos.x >> DECIMAL_BITS)
                             + self->gameData->tilemap.headlampWsg.w,
                        .y = (self->pos.y >> DECIMAL_BITS) - (entityManager->playerEntity->pos.y >> DECIMAL_BITS)
                             + self->gameData->tilemap.headlampWsg.h};

        lookup = divVec2d(lookup, 2);
        if (self->pos.y > 5120)
        {
            if (self->pos.y > 30720)
            {
                brightness = 0;
            }
            else
            {
                brightness = (30720 - self->pos.y) / 5120;
            }
        }

        if (GARBOTNIK_DATA == self->gameData->entityManager.playerEntity->dataType)
        {
            brightness = bb_midgroundLighting(
                &(self->gameData->tilemap.headlampWsg), &lookup,
                &(((bb_garbotnikData_t*)self->gameData->entityManager.playerEntity->data)->yaw.x), brightness);
        }
    }
    bb_buData_t* bData = (bb_buData_t*)self->data;
    if (bData->damageEffect > 70 || (bData->damageEffect > 0 && bb_randomInt(0, 1)))
    {
        drawWsgPalette(&entityManager->sprites[self->spriteIndex].frames[brightness + self->currentAnimationFrame * 6],
                       xOff, yOff, &self->gameData->damagePalette, bData->faceLeft, false,
                       (self->dataType == BU_DATA) * 90 * bData->gravity);
    }
    else
    {
        drawWsg(&entityManager->sprites[self->spriteIndex].frames[brightness + self->currentAnimationFrame * 6], xOff,
                yOff, bData->faceLeft, false, (self->dataType == BU_DATA) * 90 * bData->gravity);
    }
}

void bb_drawRocket(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self)
{
    bb_rocketData_t* rData = (bb_rocketData_t*)self->data;
    drawWsgSimple(&entityManager->sprites[self->spriteIndex].frames[self->currentAnimationFrame],
                  (self->pos.x >> DECIMAL_BITS) - entityManager->sprites[self->spriteIndex].originX - camera->pos.x,
                  (self->pos.y >> DECIMAL_BITS) - entityManager->sprites[self->spriteIndex].originY - camera->pos.y);

    char monitorText[4];
    snprintf(monitorText, sizeof(monitorText), "%02d", rData->numBugs % 100);
    drawText(&self->gameData->tinyNumbersFont, c140, monitorText, (self->pos.x >> DECIMAL_BITS) - camera->pos.x - 4,
             (self->pos.y >> DECIMAL_BITS) - camera->pos.y - 8);
    if (rData->armAngle > 2880) // that is 180 << DECIMAL_BITS
    {
        snprintf(monitorText, sizeof(monitorText), "%02d",
                 (uint8_t)(29 - ((rData->armAngle >> DECIMAL_BITS) - 180) / 6));
        drawText(&self->gameData->tinyNumbersFont, c410, monitorText, (self->pos.x >> DECIMAL_BITS) - camera->pos.x - 4,
                 (self->pos.y >> DECIMAL_BITS) - camera->pos.y - 2);
        if ((rData->armAngle >> 6) % 2 == 0)
        {
            char screenText[30] = "Hang tight!";
            drawText(&self->gameData->font, c500, screenText, 140 - (textWidth(&self->gameData->font, screenText) >> 1),
                     2);
            snprintf(screenText, sizeof(screenText), "pre-flight check in progress");
            drawText(&self->gameData->font, c500, screenText, 140 - (textWidth(&self->gameData->font, screenText) >> 1),
                     215);
        }
    }

    if (self->paused == false)
    {
        // increment the frame counter
        self->animationTimer += 1;
        self->currentAnimationFrame = self->animationTimer / self->gameFramesPerAnimationFrame;
    }
}

void bb_drawCar(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self)
{
    // garbage behind the car
    drawWsgSimple(&entityManager->sprites[self->spriteIndex].frames[0],
                  (self->pos.x >> DECIMAL_BITS) - entityManager->sprites[self->spriteIndex].originX - camera->pos.x + 6,
                  (self->pos.y >> DECIMAL_BITS) - entityManager->sprites[self->spriteIndex].originY - camera->pos.y
                      + 1);
    // the car
    drawWsgSimple(&entityManager->sprites[self->spriteIndex].frames[self->currentAnimationFrame],
                  (self->pos.x >> DECIMAL_BITS) - entityManager->sprites[self->spriteIndex].originX - camera->pos.x,
                  (self->pos.y >> DECIMAL_BITS) - entityManager->sprites[self->spriteIndex].originY - camera->pos.y);

    if (self->updateFunction != &bb_updateCarActive)
    {
        return;
    }
    bb_carData_t* cData = (bb_carData_t*)self->data;
    cData->textTimer -= self->gameData->elapsedUs >> 8;
    if ((cData->textTimer / 3000) % 2 == 0)
    {
        char screenText[30] = "Car Alarm!";
        drawText(&self->gameData->font, c500, screenText, 140 - (textWidth(&self->gameData->font, screenText) >> 1), 2);
        snprintf(screenText, sizeof(screenText), "Kill %d bugs!", self->gameData->carFightState);
        drawText(&self->gameData->font, c500, screenText, 140 - (textWidth(&self->gameData->font, screenText) >> 1),
                 215);
    }
}

void bb_drawSpit(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self)
{
    bb_spitData_t* sData = (bb_spitData_t*)self->data;
    uint16_t rotation    = 0;
    if (abs(sData->vel.x) > abs(sData->vel.y))
    {
        if (sData->vel.x < 0)
        {
            rotation = 270;
        }
        else
        {
            rotation = 90;
        }
    }
    else if (sData->vel.y < 0)
    {
        rotation = 180;
    }
    drawWsgPalette(&entityManager->sprites[BB_FUEL].frames[self->currentAnimationFrame],
                   (self->pos.x >> DECIMAL_BITS) - camera->pos.x, (self->pos.y >> DECIMAL_BITS) - camera->pos.y,
                   &self->gameData->damagePalette, false, false, rotation);
}

void bb_drawHitEffect(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self)
{
    drawWsgPaletteSimple(
        &entityManager->sprites[self->spriteIndex].frames[self->currentAnimationFrame],
        (self->pos.x >> DECIMAL_BITS) - entityManager->sprites[self->spriteIndex].originX - camera->pos.x,
        (self->pos.y >> DECIMAL_BITS) - entityManager->sprites[self->spriteIndex].originY - camera->pos.y,
        &self->gameData->damagePalette);
}

void bb_drawGrabbyHand(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self)
{
    // don't draw the hand if it is fully retracted. Cuts down on overdraw a lot of the time.
    if (self->gameData->entityManager.sprites[BB_GRABBY_HAND].originY > -26)
    {
        drawWsgSimple(&entityManager->sprites[self->spriteIndex].frames[0],
                      (self->pos.x >> DECIMAL_BITS) - entityManager->sprites[self->spriteIndex].originX - camera->pos.x,
                      (self->pos.y >> DECIMAL_BITS) - entityManager->sprites[self->spriteIndex].originY
                          - camera->pos.y);
    }
}

void bb_drawDiveSummary(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self)
{
    // draw a notepad paper
    drawRectFilled((self->pos.x >> DECIMAL_BITS) - camera->pos.x, (self->pos.y >> DECIMAL_BITS) - camera->pos.y,
                   (self->pos.x >> DECIMAL_BITS) - camera->pos.x + 200,
                   (self->pos.y >> DECIMAL_BITS) - camera->pos.y + 230, c554);

    drawRectFilled((self->pos.x >> DECIMAL_BITS) - camera->pos.x, (self->pos.y >> DECIMAL_BITS) - camera->pos.y + 33,
                   (self->pos.x >> DECIMAL_BITS) - camera->pos.x + 200,
                   (self->pos.y >> DECIMAL_BITS) - camera->pos.y + 35, c335);

    for (int blueLine = 4; blueLine < 23; blueLine++)
    {
        drawLineFast((self->pos.x >> DECIMAL_BITS) - camera->pos.x,
                     (self->pos.y >> DECIMAL_BITS) - camera->pos.y + blueLine * 11,
                     (self->pos.x >> DECIMAL_BITS) - camera->pos.x + 199,
                     (self->pos.y >> DECIMAL_BITS) - camera->pos.y + blueLine * 11, c335);
    }

    drawCircleFilled((self->pos.x >> DECIMAL_BITS) - camera->pos.x + 12,
                     (self->pos.y >> DECIMAL_BITS) - camera->pos.y + 50, 5, c000);

    drawCircleFilled((self->pos.x >> DECIMAL_BITS) - camera->pos.x + 12,
                     (self->pos.y >> DECIMAL_BITS) - camera->pos.y + 125, 5, c000);
    drawCircleFilled((self->pos.x >> DECIMAL_BITS) - camera->pos.x + 12,
                     (self->pos.y >> DECIMAL_BITS) - camera->pos.y + 200, 5, c000);

    drawLineFast((self->pos.x >> DECIMAL_BITS) - camera->pos.x + 25, (self->pos.y >> DECIMAL_BITS) - camera->pos.y,
                 (self->pos.x >> DECIMAL_BITS) - camera->pos.x + 25,
                 (self->pos.y >> DECIMAL_BITS) - camera->pos.y + 230, c522);

    drawText(&self->gameData->cgFont, c002, "Dive Summary", (self->pos.x >> DECIMAL_BITS) - camera->pos.x + 30,
             (self->pos.y >> DECIMAL_BITS) - camera->pos.y + 1);

    // snprintf to for a date string based on self->gameData->day
    char date[30];
    // array of days of the week
    char* days[] = {
        "Wednesday", "Thursday", "Friday", "Saturday", "Sunday", "Monday", "Tuesday",
    };
    snprintf(date, sizeof(date), "%s Jan. %d, 2025", days[self->gameData->day % 7], 22 + self->gameData->day);

    drawText(&self->gameData->cgThinFont, c002, date, (self->pos.x >> DECIMAL_BITS) - camera->pos.x + 30,
             (self->pos.y >> DECIMAL_BITS) - camera->pos.y + 20);
    drawText(&self->gameData->cgFont, c500, "A+", (self->pos.x >> DECIMAL_BITS) - camera->pos.x + 100,
             (self->pos.y >> DECIMAL_BITS) - camera->pos.y + 140);
    drawCircle((self->pos.x >> DECIMAL_BITS) - camera->pos.x + 110, (self->pos.y >> DECIMAL_BITS) - camera->pos.y + 150,
               14, c500);

    drawText(&self->gameData->cgThinFont, c002, "Booster Depth:", (self->pos.x >> DECIMAL_BITS) - camera->pos.x + 30,
             (self->pos.y >> DECIMAL_BITS) - camera->pos.y + 33);
    // 0b1000     booster depth flavor text      1 << 3
    if (self->gameData->endDayChecks & (1 << 3))
    {
        drawText(&self->gameData->cgThinFont, c500, "really far", (self->pos.x >> DECIMAL_BITS) - camera->pos.x + 126,
                 (self->pos.y >> DECIMAL_BITS) - camera->pos.y + 33);
    }
    drawText(&self->gameData->cgThinFont, c002, "Trash Pod Depth:", (self->pos.x >> DECIMAL_BITS) - camera->pos.x + 30,
             (self->pos.y >> DECIMAL_BITS) - camera->pos.y + 44);
    drawText(&self->gameData->cgThinFont, c002, "Bugs Killed:", (self->pos.x >> DECIMAL_BITS) - camera->pos.x + 30,
             (self->pos.y >> DECIMAL_BITS) - camera->pos.y + 55);
    // 0b100000   bugs killed flavor text        1 << 4
    if (self->gameData->endDayChecks & (1 << 4))
    {
        drawText(&self->gameData->cgThinFont, c500, "a lot", (self->pos.x >> DECIMAL_BITS) - camera->pos.x + 100,
                 (self->pos.y >> DECIMAL_BITS) - camera->pos.y + 55);
    }

    drawText(&self->gameData->cgThinFont, c002, "Bugs Collected:", (self->pos.x >> DECIMAL_BITS) - camera->pos.x + 30,
             (self->pos.y >> DECIMAL_BITS) - camera->pos.y + 66);
    drawText(&self->gameData->cgThinFont, c002,
             "Places Discovered:", (self->pos.x >> DECIMAL_BITS) - camera->pos.x + 30,
             (self->pos.y >> DECIMAL_BITS) - camera->pos.y + 77);
    // 0b100000   places discoverd flavor text        1 << 5
    if (self->gameData->endDayChecks & (1 << 5))
    {
        drawText(&self->gameData->cgThinFont, c500, "IDK", (self->pos.x >> DECIMAL_BITS) - camera->pos.x + 147,
                 (self->pos.y >> DECIMAL_BITS) - camera->pos.y + 77);
    }
    drawText(&self->gameData->cgThinFont, c002, "Donuts Collected:", (self->pos.x >> DECIMAL_BITS) - camera->pos.x + 30,
             (self->pos.y >> DECIMAL_BITS) - camera->pos.y + 88);
    // 0b1000000  donuts collected flavor text   1 << 6
    if (self->gameData->endDayChecks & (1 << 6))
    {
        drawText(&self->gameData->cgThinFont, c500, "I hungry", (self->pos.x >> DECIMAL_BITS) - camera->pos.x + 140,
                 (self->pos.y >> DECIMAL_BITS) - camera->pos.y + 88);
    }
    drawText(&self->gameData->cgThinFont, c002,
             "Trash Pod upgrades:", (self->pos.x >> DECIMAL_BITS) - camera->pos.x + 30,
             (self->pos.y >> DECIMAL_BITS) - camera->pos.y + 99);
    drawText(&self->gameData->cgThinFont, c002, "Time Spent:", (self->pos.x >> DECIMAL_BITS) - camera->pos.x + 30,
             (self->pos.y >> DECIMAL_BITS) - camera->pos.y + 110);
    // 0b10000000 time spent flavor text         1 << 7
    if (self->gameData->endDayChecks & (1 << 7))
    {
        drawText(&self->gameData->cgThinFont, c500, "one day", (self->pos.x >> DECIMAL_BITS) - camera->pos.x + 110,
                 (self->pos.y >> DECIMAL_BITS) - camera->pos.y + 110);
    }

    if (self->gameData->day % 7 == 1 || self->gameData->day % 7 == 4 || self->gameData->day % 7 == 6)
    {
        drawText(&self->gameData->cgThinFont, c500, "Tomorrow is trash day!",
                 (self->pos.x >> DECIMAL_BITS) - camera->pos.x + 30,
                 (self->pos.y >> DECIMAL_BITS) - camera->pos.y + 176);
        drawText(&self->gameData->cgThinFont, c002, "So expect the landfill",
                 (self->pos.x >> DECIMAL_BITS) - camera->pos.x + 30,
                 (self->pos.y >> DECIMAL_BITS) - camera->pos.y + 186);
        drawText(&self->gameData->cgThinFont, c002, "to be filled in.",
                 (self->pos.x >> DECIMAL_BITS) - camera->pos.x + 30,
                 (self->pos.y >> DECIMAL_BITS) - camera->pos.y + 198);
    }
    else
    {
        drawText(&self->gameData->cgThinFont, c002, "I didn't have enough time",
                 (self->pos.x >> DECIMAL_BITS) - camera->pos.x + 30,
                 (self->pos.y >> DECIMAL_BITS) - camera->pos.y + 176);
        drawText(&self->gameData->cgThinFont, c002, "to fill this out",
                 (self->pos.x >> DECIMAL_BITS) - camera->pos.x + 30,
                 (self->pos.y >> DECIMAL_BITS) - camera->pos.y + 187);
        drawText(&self->gameData->cgThinFont, c002, "but I'd give myself an A+",
                 (self->pos.x >> DECIMAL_BITS) - camera->pos.x + 30,
                 (self->pos.y >> DECIMAL_BITS) - camera->pos.y + 198);
    }
}

void bb_drawFoodCart(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self)
{
    if (self->currentAnimationFrame == 0)
    {
        drawWsgSimple(
            &entityManager->sprites[self->spriteIndex].frames[(self->currentAnimationFrame > 1)],
            (self->pos.x >> DECIMAL_BITS) - entityManager->sprites[self->spriteIndex].originX - camera->pos.x - 1,
            (self->pos.y >> DECIMAL_BITS) - entityManager->sprites[self->spriteIndex].originY - camera->pos.y + 45);
    }
    else
    {
        bb_foodCartData_t* fcData = (bb_foodCartData_t*)self->data;
        if (fcData->damageEffect > 0)
        {
            fcData->damageEffect -= self->gameData->elapsedUs >> 11;
        }
        if (fcData->damageEffect > 70 || (fcData->damageEffect > 0 && bb_randomInt(0, 1)))
        {
            drawWsgPaletteSimple(
                &entityManager->sprites[self->spriteIndex].frames[(self->currentAnimationFrame > 1)],
                (self->pos.x >> DECIMAL_BITS) - entityManager->sprites[self->spriteIndex].originX - camera->pos.x,
                (self->pos.y >> DECIMAL_BITS) - entityManager->sprites[self->spriteIndex].originY - camera->pos.y,
                &self->gameData->damagePalette);
        }
        else
        {
            drawWsgSimple(
                &entityManager->sprites[self->spriteIndex].frames[(self->currentAnimationFrame > 1)],
                (self->pos.x >> DECIMAL_BITS) - entityManager->sprites[self->spriteIndex].originX - camera->pos.x,
                (self->pos.y >> DECIMAL_BITS) - entityManager->sprites[self->spriteIndex].originY - camera->pos.y);
        }
    }
}

// void bb_drawRect(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self)
// {
//     drawRect (((self->pos.x - self->halfWidth) >>DECIMAL_BITS) - camera->pos.x,
//               ((self->pos.y - self->halfHeight)>>DECIMAL_BITS) - camera->pos.y,
//               ((self->pos.x + self->halfWidth) >>DECIMAL_BITS) - camera->pos.x,
//               ((self->pos.y + self->halfHeight)>>DECIMAL_BITS) - camera->pos.y, c500);
// }

void bb_onCollisionHarpoon(bb_entity_t* self, bb_entity_t* other, bb_hitInfo_t* hitInfo)
{
    bb_projectileData_t* pData = (bb_projectileData_t*)self->data;
    if (other->dataType == EGG_DATA)
    {
        // pop that egg
        int32_t tile_i = other->pos.x >> 9; // 4 decimal bits and 5 bitshifts is divide by 32.
        int32_t tile_j = other->pos.y >> 9; // 4 decimal bits and 5 bitshifts is divide by 32.
        self->gameData->tilemap.fgTiles[tile_i][tile_j].health = 0;
        bb_crumbleDirt(other->gameData, 2, tile_i, tile_j, true);
        // destroy this harpoon
        bb_destroyEntity(self, false);
    }
    else
    {
        // pause the harpoon animation as the tip will no longer even be rendered.
        self->paused = true;

        if (other->dataType != PHYSICS_DATA)
        {
            bb_bugData_t* bData = (bb_bugData_t*)other->data;
            // Bug got stabbed
            if (bData->health - 34 <= 0) // bug just died
            {
                // use a bump animation but tweak its graphics
                bb_entity_t* hitEffect
                    = bb_createEntity(&(self->gameData->entityManager), ONESHOT_ANIMATION, false, BUMP_ANIM, 6,
                                      hitInfo->pos.x >> DECIMAL_BITS, hitInfo->pos.y >> DECIMAL_BITS, true, false);
                hitEffect->drawFunction = &bb_drawHitEffect;

                if (self->gameData->carFightState > 0)
                {
                    self->gameData->carFightState--;
                }
                if (self->dataType == BU_DATA)
                {
                    bb_buData_t* buData = (bb_buData_t*)self->data;
                    switch (buData->gravity)
                    {
                        case BB_LEFT:
                            bb_rotateBug(self, -1);
                            break;
                        case BB_UP:
                            bb_rotateBug(self, 2);
                            break;
                        case BB_RIGHT:
                            bb_rotateBug(self, 1);
                            break;
                        default:
                            break;
                    }
                }
                other->drawFunction = NULL;

                midiPlayer_t* sfx = soundGetPlayerSfx();
                midiPlayerReset(sfx);
                soundPlaySfx(&self->gameData->sfxDirt, 0);

                bData->health               = 0;
                other->paused               = true;
                bb_physicsData_t* physData  = heap_caps_calloc(1, sizeof(bb_physicsData_t), MALLOC_CAP_SPIRAM);
                physData->bounceNumerator   = 2; // 66% bounce
                physData->bounceDenominator = 3;
                bb_setData(other, physData, PHYSICS_DATA);
                other->updateFunction = bb_updatePhysicsObject;
            }
            else
            {
                bData->damageEffect = 100;
                bData->health -= 34;
                if (bData->health < 0)
                {
                    bData->health = 0;
                }
            }
        }
        if (other->dataType
            == PHYSICS_DATA) // leave as "if" not else if, because the last block may have converted it to physics data
        {
            bb_physicsData_t* physData = (bb_physicsData_t*)other->data;
            physData->vel              = addVec2d(physData->vel, pData->vel);
        }
        vecFl_t floatVel              = {(float)pData->vel.x, (float)pData->vel.y};
        bb_stuckHarpoonData_t* shData = heap_caps_calloc(1, sizeof(bb_stuckHarpoonData_t), MALLOC_CAP_SPIRAM);
        shData->parent                = other;
        shData->offset                = subVec2d(self->pos, other->pos);
        shData->floatVel              = normVecFl2d(floatVel);
        bb_setData(self, shData, STUCK_HARPOON_DATA);

        bb_clearCollisions(self, false);

        self->updateFunction = bb_updateStuckHarpoon;
        self->drawFunction   = bb_drawStuckHarpoon;
    }
}

void bb_onCollisionSimple(bb_entity_t* self, bb_entity_t* other, bb_hitInfo_t* hitInfo)
{
    other->pos.x = hitInfo->pos.x + hitInfo->normal.x * other->halfWidth;
    other->pos.y = hitInfo->pos.y + hitInfo->normal.y * other->halfHeight;

    if (other->dataType == GARBOTNIK_DATA)
    {
        bb_garbotnikData_t* gData = (bb_garbotnikData_t*)other->data;
        if (hitInfo->normal.x == 0)
        {
            gData->vel.y = 0;
        }
        else
        {
            gData->vel.x = 0;
        }
    }
    else if (other->dataType == PHYSICS_DATA)
    {
        bb_physicsData_t* pData = (bb_physicsData_t*)other->data;
        if (hitInfo->normal.x == 0)
        {
            pData->vel.y = 0;
            if (other->updateFunction == &bb_updateGarbotnikDying && self->spriteIndex == BB_WASHING_MACHINE)
            {
                bb_triggerGameOver(self);
            }
        }
        else
        {
            pData->vel.x = 0;
        }
    }
    else if (other->dataType == BU_DATA || other->dataType == BUGGO_DATA)
    {
        bb_bugData_t* bData = (bb_bugData_t*)other->data;
        bData->faceLeft     = !bData->faceLeft;
    }
}

void bb_onCollisionHeavyFalling(bb_entity_t* self, bb_entity_t* other, bb_hitInfo_t* hitInfo)
{
    bb_onCollisionSimple(self, other, hitInfo);
    if (hitInfo->normal.y == 1)
    {
        bb_hitInfo_t localHitInfo = {0};
        bb_collisionCheck(&self->gameData->tilemap, other, NULL, &localHitInfo);
        if (localHitInfo.hit == true && localHitInfo.normal.y == -1)
        {
            bb_triggerGameOver(self);
        }
    }
}

void bb_onCollisionCarIdle(bb_entity_t* self, bb_entity_t* other, bb_hitInfo_t* hitInfo)
{
    bb_setupMidi();
    unloadMidiFile(&self->gameData->bgm);
    loadMidiFile("sh_revenge.mid", &self->gameData->bgm, true);
    globalMidiPlayerPlaySong(&self->gameData->bgm, MIDI_BGM);

    // close the door and make it not cacheable so bugs don't walk out offscreen.
    node_t* checkEntity = self->gameData->entityManager.cachedEntities->first;
    while (checkEntity != NULL)
    {
        bb_entity_t* cachedEntityVal = (bb_entity_t*)checkEntity->val;
        node_t* next                 = checkEntity->next;
        if (cachedEntityVal != NULL && cachedEntityVal->spriteIndex == BB_DOOR) // it's a door
        {
            if (abs(cachedEntityVal->pos.x - self->pos.x) + abs(cachedEntityVal->pos.y - self->pos.y)
                < 11250) // eh close enough
            {
                bb_entity_t* foundSpot = bb_findInactiveEntity(&self->gameData->entityManager);
                if (foundSpot != NULL)
                {
                    bb_loadSprite("door", 2, 1, &self->gameData->entityManager.sprites[BB_DOOR]);
                    // like a memcopy
                    *foundSpot = *cachedEntityVal;
                    self->gameData->entityManager.activeEntities++;
                    heap_caps_free(removeEntry(self->gameData->entityManager.cachedEntities, checkEntity));
                }
            }
        }
        checkEntity = next;
    }

    for (int checkIdx = 0; checkIdx < MAX_ENTITIES; checkIdx++)
    {
        bb_entity_t* entityPointer = &self->gameData->entityManager.entities[checkIdx];
        if (entityPointer->spriteIndex == BB_DOOR) // it's a door
        {
            if (abs(entityPointer->pos.x - self->pos.x) + abs(entityPointer->pos.y - self->pos.y)
                < 11250) // eh close enough
            {
                entityPointer->cacheable             = false;
                entityPointer->currentAnimationFrame = 1;
                if (entityPointer->collisions == NULL)
                {
                    entityPointer->collisions = heap_caps_calloc(1, sizeof(list_t), MALLOC_CAP_SPIRAM);
                    list_t* others            = heap_caps_calloc(1, sizeof(list_t), MALLOC_CAP_SPIRAM);
                    push(others, (void*)GARBOTNIK_FLYING);
                    push(others, (void*)BU);
                    push(others, (void*)BUG);
                    push(others, (void*)BUGG);
                    push(others, (void*)BUGGO);
                    push(others, (void*)BUGGY);
                    push(others, (void*)BUTT);
                    bb_collision_t* collision = heap_caps_calloc(1, sizeof(bb_collision_t), MALLOC_CAP_SPIRAM);
                    *collision                = (bb_collision_t){others, bb_onCollisionSimple};
                    push(entityPointer->collisions, (void*)collision);
                }
            }
        }
    }

    bb_playCarAlarm(self);
    // make an active car not cacheable so it keeps beeping from off screen.
    self->cacheable = false;

    bb_carData_t* cData = (bb_carData_t*)self->data;

    // number of bugs to fight. More risk at greater depths.
    self->gameData->carFightState = (3 * (self->pos.y >> 9) / 20) + 5;

    // complex way to spawn bugs that adapts to any arena
    for (int bugSpawn = 0; bugSpawn < self->gameData->carFightState; bugSpawn++)
    {
        bool success = false; // when this loop is done, a bug is spawned.
        while (success == false)
        {
            uint8_t spawnPosStartIdx = bb_randomInt(0, 3); // top left, top right, bottom left, bottom right
            uint8_t spawnDirection   = bb_randomInt(0, 2); // down, sideways, up
            // in terms of tile indices...
            vec_t spawnPos = {0};    // Set all fields to 0
            if (self->pos.x > 18944) // if it's on the right half of the map (74/2)<<9
            {
                spawnPos.x = (self->pos.x >> 9) - 2 - (spawnPosStartIdx % 2) * 3;
            }
            else
            {
                spawnPos.x = (self->pos.x >> 9) + 2 + (spawnPosStartIdx % 2) * 3;
            }
            spawnPos.y = (self->pos.y >> 9) - 5 + (spawnPosStartIdx > 1) * 3;
            // make sure there are 3 consecutive blocks of garbage enclosed in garbage.
            uint8_t consecutiveGarbage = 0;
            while (true)
            {
                switch (spawnDirection)
                {
                    case 0: // down
                    {
                        spawnPos.y++;
                        break;
                    }
                    case 1: // sideways
                    {
                        spawnPos.x += ((self->pos.x > 18944) << 1) - 1;
                        break;
                    }
                    default: // case 2://up
                    {
                        spawnPos.y--;
                        break;
                    }
                }
                if (spawnPos.x - 1 >= 0 && spawnPos.y - 1 >= 0 && spawnPos.x + 1 < TILE_FIELD_WIDTH
                    && spawnPos.y + 1 < TILE_FIELD_HEIGHT)
                {
                    if (self->gameData->tilemap.fgTiles[spawnPos.x][spawnPos.y].health > 0
                        && self->gameData->tilemap.fgTiles[spawnPos.x - 1][spawnPos.y].health > 0
                        && self->gameData->tilemap.fgTiles[spawnPos.x][spawnPos.y - 1].health > 0
                        && self->gameData->tilemap.fgTiles[spawnPos.x + 1][spawnPos.y].health > 0
                        && self->gameData->tilemap.fgTiles[spawnPos.x][spawnPos.y + 1].health > 0)
                    {
                        consecutiveGarbage++;
                    }
                    else if (consecutiveGarbage > 0)
                    {
                        consecutiveGarbage = 0;
                    }
                }
                else // invalid
                {
                    break;
                }
                if (consecutiveGarbage == 2)
                {
                    bb_entity_t* jankyBugDig
                        = bb_createEntity(&self->gameData->entityManager, NO_ANIMATION, true, BB_JANKY_BUG_DIG, 1,
                                          (spawnPos.x << 5) + 16, (spawnPos.y << 5) + 16, false, false);

                    if (jankyBugDig == NULL)
                    {
                        return;
                    }

                    bb_entity_t* bug
                        = bb_createEntity(&self->gameData->entityManager, LOOPING_ANIMATION, false, bb_randomInt(8, 13),
                                          1, (spawnPos.x << 5) + 16, (spawnPos.y << 5) + 16, false, false);

                    // spawn a bug
                    if (bug != NULL)
                    {
                        midiPlayer_t* sfx = soundGetPlayerSfx();
                        midiPlayerReset(sfx);
                        soundPlaySfx(&self->gameData->sfxEgg, 0);

                        success        = true;
                        bug->cacheable = false; // car fight bugs don't cache so they may dig in from off screen.

                        uint8_t jankyBugDigIdx = 0;
                        if ((spawnPosStartIdx == 0 || spawnPosStartIdx == 2) && spawnDirection == 0)
                        {
                            ((bb_jankyBugDigData_t*)jankyBugDig->data)->arena = BB_UP;
                        }
                        else if ((spawnPosStartIdx == 1 || spawnPosStartIdx == 3) && spawnDirection == 0)
                        {
                            ((bb_jankyBugDigData_t*)jankyBugDig->data)->arena = BB_UP;
                            jankyBugDigIdx                                    = 1;
                        }
                        else if ((spawnPosStartIdx == 0 || spawnPosStartIdx == 1) && spawnDirection == 1)
                        {
                            ((bb_jankyBugDigData_t*)jankyBugDig->data)->arena
                                = self->pos.x > 18944 ? BB_LEFT : BB_RIGHT;
                            jankyBugDigIdx = 2;
                        }
                        else if ((spawnPosStartIdx == 2 || spawnPosStartIdx == 3) && spawnDirection == 1)
                        {
                            ((bb_jankyBugDigData_t*)jankyBugDig->data)->arena
                                = self->pos.x > 18944 ? BB_LEFT : BB_RIGHT;
                            jankyBugDigIdx = 3;
                        }
                        else if ((spawnPosStartIdx == 0 || spawnPosStartIdx == 2) && spawnDirection == 2)
                        {
                            ((bb_jankyBugDigData_t*)jankyBugDig->data)->arena = BB_DOWN;
                            jankyBugDigIdx                                    = 4;
                        }
                        else if ((spawnPosStartIdx == 1 || spawnPosStartIdx == 3) && spawnDirection == 2)
                        {
                            ((bb_jankyBugDigData_t*)jankyBugDig->data)->arena = BB_DOWN;
                            jankyBugDigIdx                                    = 5;
                        }

                        if (cData->jankyBugDig[jankyBugDigIdx] == NULL)
                        {
                            cData->jankyBugDig[jankyBugDigIdx] = jankyBugDig;
                        }
                        else
                        {
                            bb_destroyEntity(jankyBugDig, false);
                        }
                    }
                    break;
                }
            }
        }
    }
    for (int jankyBugDigIdx = 0; jankyBugDigIdx < 6; jankyBugDigIdx++)
    {
        if (cData->jankyBugDig[jankyBugDigIdx] != NULL)
        {
            self->gameData->tilemap
                .fgTiles[cData->jankyBugDig[jankyBugDigIdx]->pos.x >> 9][cData->jankyBugDig[jankyBugDigIdx]->pos.y >> 9]
                .health
                = 0;
        }
    }
    self->paused         = false;
    self->updateFunction = &bb_updateCarActive;
    bb_clearCollisions(self, false);
}

void bb_onCollisionAttachmentArm(bb_entity_t* self, bb_entity_t* other, bb_hitInfo_t* hitInfo)
{
    bb_rocketData_t* rData = (bb_rocketData_t*)((bb_attachmentArmData_t*)self->data)->rocket->data;
    rData->armAngle += 18;
}

void bb_onCollisionFuel(bb_entity_t* self, bb_entity_t* other, bb_hitInfo_t* hitInfo)
{
    if (other->dataType == PHYSICS_DATA)
    {
        // Turn dying garbotnik back into playable garbotnik.
        vec_t vel                 = ((bb_physicsData_t*)other->data)->vel;
        bb_garbotnikData_t* gData = heap_caps_calloc(1, sizeof(bb_garbotnikData_t), MALLOC_CAP_SPIRAM);
        gData->vel                = vel;
        other->updateFunction     = &bb_updateGarbotnikFlying;
        other->drawFunction       = &bb_drawGarbotnikFlying;
        bb_setData(other, gData, GARBOTNIK_DATA);
    }
    bb_garbotnikData_t* gData = (bb_garbotnikData_t*)other->data;
    gData->fuel += 30000;
    midiPlayer_t* sfx = soundGetPlayerSfx();
    midiPlayerReset(sfx);
    soundPlaySfx(&self->gameData->sfxHealth, 0);
    if (gData->fuel > 180000) // 1 thousand milliseconds in a second. 60 seconds in a minute. 3 minutes. //also set in
                              // bb_createEntity()
    {
        gData->fuel = 180000;
    }
    bb_destroyEntity(self, false);
}

void bb_onCollisionGrabbyHand(bb_entity_t* self, bb_entity_t* other, bb_hitInfo_t* hitInfo)
{
    bb_grabbyHandData_t* ghData = (bb_grabbyHandData_t*)self->data;
    // extend from the booster
    if (ghData->grabbed == NULL && self->gameData->entityManager.sprites[BB_GRABBY_HAND].originY < 28)
    {
        self->gameData->entityManager.sprites[BB_GRABBY_HAND].originY += 4;
    }

    // if nothing grabbed yet and there is something in hand to grab
    if (self->currentAnimationFrame == 0
        && self->pos.y - (self->gameData->entityManager.sprites[BB_GRABBY_HAND].originY << DECIMAL_BITS)
               < other->pos.y - 128)
    {
        self->paused    = false;
        ghData->grabbed = other;
    }
}

void bb_onCollisionJankyBugDig(bb_entity_t* self, bb_entity_t* other, bb_hitInfo_t* hitInfo)
{
    // do a collision check again to verify, because often times another bug has already incremented the position on the
    // same frame.
    if (!bb_boxesCollide(self, other, NULL, NULL))
    {
        return;
    }

    // the purpose of this function is to dig one tile toward the arena and increment position toward the arena.
    bb_jankyBugDigData_t* jData = (bb_jankyBugDigData_t*)self->data;
    vec_t tilePos               = {.x = self->pos.x >> 9, .y = self->pos.y >> 9};
    switch (jData->arena)
    {
        case BB_DOWN:
        {
            tilePos.y++;
            break;
        }
        case BB_LEFT:
        {
            tilePos.x--;
            break;
        }
        case BB_UP:
        {
            tilePos.y--;
            break;
        }
        default: // BB_RIGHT:
        {
            tilePos.x++;
            break;
        }
    }
    if (self->gameData->tilemap.fgTiles[tilePos.x][tilePos.y].health > 0)
    {
        self->gameData->tilemap.fgTiles[tilePos.x][tilePos.y].health = 0;
        bb_crumbleDirt(self->gameData, 2, tilePos.x, tilePos.y, true);
    }
    self->pos.x = (tilePos.x << 9) + (16 << DECIMAL_BITS);
    self->pos.y = (tilePos.y << 9) + (16 << DECIMAL_BITS);

    switch (jData->arena)
    {
        case BB_DOWN:
        {
            self->pos.y += (13 << DECIMAL_BITS);
            break;
        }
        case BB_LEFT:
        {
            self->pos.x -= (13 << DECIMAL_BITS);
            break;
        }
        case BB_UP:
        {
            self->pos.y -= (13 << DECIMAL_BITS);
            break;
        }
        default: // BB_RIGHT
        {
            self->pos.x += (13 << DECIMAL_BITS);
            break;
        }
    }
    jData->numberOfDigs++;
    if (jData->numberOfDigs == 2)
    {
        bb_destroyEntity(self, false);
    }
}

void bb_onCollisionSpit(bb_entity_t* self, bb_entity_t* other, bb_hitInfo_t* hitInfo)
{
    bb_spitData_t* sData = (bb_spitData_t*)self->data;
    if (other->dataType == GARBOTNIK_DATA)
    {
        bb_garbotnikData_t* gData = (bb_garbotnikData_t*)other->data;
        gData->vel                = addVec2d(gData->vel, (vec_t){sData->vel.x << 4, sData->vel.y << 4});
        gData->damageEffect       = 100;
        gData->fuel -= 10000;
        midiPlayer_t* sfx = soundGetPlayerSfx();
        midiPlayerReset(sfx);
        soundPlaySfx(&self->gameData->sfxDamage, 0);
        if (gData->fuel < 0)
        {
            gData->fuel
                = 1; // It'll decrement soon anyways. Keeps more of the game over code on Garbotnik's side of the fence.
        }
    }
    else // PHYSICS_DATA
    {
        bb_physicsData_t* pData = (bb_physicsData_t*)other->data;
        pData->vel              = addVec2d(pData->vel, (vec_t){sData->vel.x << 4, sData->vel.y << 4});
    }
    bb_destroyEntity(self, false);
}

void bb_onCollisionSwadge(bb_entity_t* self, bb_entity_t* other, bb_hitInfo_t* hitInfo)
{
    midiPlayer_t* sfx = soundGetPlayerSfx();
    midiPlayerReset(sfx);
    soundPlaySfx(&self->gameData->sfxCollection, 0);
    bb_destroyEntity(self, false);
    // give a choice of upgrades
    bb_upgradeGarbotnik(self);
    self->gameData->screen = BIGBUG_GARBOTNIK_UPGRADE_SCREEN;
}

void bb_onCollisionFoodCart(bb_entity_t* self, bb_entity_t* other, bb_hitInfo_t* hitInfo)
{
    bb_foodCartData_t* fcData = (bb_foodCartData_t*)self->data;
    if (other->dataType == GARBOTNIK_DATA)
    {
        // Apply bounce back to garbotnik same as digging a tile.
        bb_garbotnikData_t* gData = (bb_garbotnikData_t*)other->data;

        // get hitInfo flipped around and positioned right.
        // hitInfo->normal = mulVec2d(hitInfo->normal, -1);

        if (hitInfo->normal.y == 0)
        {
            other->pos.x = hitInfo->pos.x + hitInfo->normal.x * other->halfWidth;
        }
        else // hitInfo->normal.x == 0
        {
            other->pos.y = hitInfo->pos.y + hitInfo->normal.y * other->halfHeight;
        }

        // Check for digging
        int32_t dot = dotVec2d(gData->vel, hitInfo->normal);
        if (dot < -40)
        { // velocity angle is opposing food cart normal vector. Tweak number for different threshold.
            ////////////////////////////
            // cart digging detected! //
            ////////////////////////////
            bb_entity_t* mainCart = self;
            if (self->currentAnimationFrame == 0)
            {
                mainCart = fcData->partner;
            }
            bb_foodCartData_t* mcData = (bb_foodCartData_t*)mainCart->data;
            // Update the main cart by decrementing it's animation frame. Using animation frame as health to save space.
            mainCart->currentAnimationFrame--;
            if (mainCart->currentAnimationFrame == 1)
            {
                // Destroy the food cart and spawn a reward.
                // free sprites
                freeWsg(&mainCart->gameData->entityManager.sprites[BB_FOOD_CART].frames[0]);
                freeWsg(&mainCart->gameData->entityManager.sprites[BB_FOOD_CART].frames[1]);

                bb_destroyEntity(mcData->partner, false);

                switch (mcData->reward)
                {
                    case BB_DONUT:
                    {
                        // spawn a donut as a reward for completing the fight
                        bb_createEntity(&mainCart->gameData->entityManager, NO_ANIMATION, true, BB_DONUT, 1,
                                        (mainCart->pos.x >> DECIMAL_BITS), (mainCart->pos.y >> DECIMAL_BITS), true,
                                        false);
                        break;
                    }
                    default: // BB_SWADGE
                    {
                        // spawn a swadge as a reward for completing the fight
                        bb_createEntity(&mainCart->gameData->entityManager, LOOPING_ANIMATION, false, BB_SWADGE, 9,
                                        (mainCart->pos.x >> DECIMAL_BITS), (mainCart->pos.y >> DECIMAL_BITS), true,
                                        false);
                        break;
                    }
                }
                bb_destroyEntity(mainCart, false);
                // use a bump animation but tweak its graphics
                bb_entity_t* hitEffect
                    = bb_createEntity(&(mainCart->gameData->entityManager), ONESHOT_ANIMATION, false, BUMP_ANIM, 6,
                                      hitInfo->pos.x >> DECIMAL_BITS, hitInfo->pos.y >> DECIMAL_BITS, true, false);
                hitEffect->drawFunction = &bb_drawHitEffect;
            }
            else
            {
                // Create a bump animation
                bb_createEntity(&(self->gameData->entityManager), ONESHOT_ANIMATION, false, BUMP_ANIM, 4,
                                hitInfo->pos.x >> DECIMAL_BITS, hitInfo->pos.y >> DECIMAL_BITS, true, false);
                mcData->damageEffect = 100;
            }

            ////////////////////////////////
            // Mirror garbotnik's velocity//
            ////////////////////////////////
            int32_t bounceScalar = 3;
            if (sqMagVec2d(gData->vel) * dot < -360000)
            {
                bounceScalar = 2;
            }
            if (sqMagVec2d(gData->vel) * dot < -550000)
            {
                bounceScalar = 1;
            }
            if (self->currentAnimationFrame == 0 && hitInfo->normal.y == -1)
            {
                // The umbrella is extra bouncy upwards.
                bounceScalar *= 4;
            }
            gData->vel
                = mulVec2d(subVec2d(gData->vel, mulVec2d(hitInfo->normal, (2 * dotVec2d(gData->vel, hitInfo->normal)))),
                           bounceScalar);
        }
    }
}

void bb_startGarbotnikIntro(bb_entity_t* self)
{
    bb_entity_t* ovo
        = bb_createEntity(&self->gameData->entityManager, NO_ANIMATION, true, OVO_TALK, 1,
                          self->gameData->camera.camera.pos.x, self->gameData->camera.camera.pos.y, true, true);

    bb_dialogueData_t* dData = bb_createDialogueData(24, "Ovo");

    // longest possible string     " "
    //  bb_setCharacterLine(dData, 0, "A A A A A A A A A A A A A A A A A A A A A A A A A A A A A A A A A A A A A A A A A
    //  A A A A A A A A A A A A A A A A A A A");
    bb_setCharacterLine(dData, 0, "Ovo", "Holy bug farts!"); //
    bb_setCharacterLine(
        dData, 1, "Ovo",
        "After I sold the chilidog car fresheners to MAGFest, Garbotnik Industries' stock went up by 6,969%!");
    bb_setCharacterLine(dData, 2, "Ovo",
                        "I'm going to use my time machine to steal the next big-selling trinket from the future now.");
    bb_setCharacterLine(dData, 3, "Ovo", "That will floor all my stakeholders and make me UNDEFINED money!");
    bb_setCharacterLine(
        dData, 4, "Ovo",
        "With that kind of cash, I can recruit 200 professional bassoon players to the MAGFest Community Orchestra.");
    bb_setCharacterLine(dData, 5, "Ovo", "I'm so hyped to turn on my time machine for the first time!");
    bb_setCharacterLine(dData, 6, "Ovo", "Everything's in order.");
    bb_setCharacterLine(dData, 7, "Ovo", "Even Pango can't stop me!");
    bb_setCharacterLine(dData, 8, "Ovo", "I just have to attach the chaos orb right here.");
    bb_setCharacterLine(dData, 9, "Ovo", "Where did I put that orb?");
    bb_setCharacterLine(dData, 10, "Ovo", "hmmm...");
    bb_setCharacterLine(dData, 11, "Ovo", "What about in the freezer?");
    bb_setCharacterLine(dData, 12, "Ovo", "I've checked every inch of the death dumpster.");
    bb_setCharacterLine(dData, 13, "Ovo", "Glitch my circuits!");
    bb_setCharacterLine(dData, 14, "Ovo", "It must have gone out with the trash last Wednesday.");
    bb_setCharacterLine(dData, 15, "Ovo", "Can I get an F in the chat?");
    bb_setCharacterLine(dData, 16, "Ovo", "...");
    bb_setCharacterLine(dData, 17, "Ovo", "The chaos orb is three times denser than a black hole.");
    bb_setCharacterLine(dData, 18, "Ovo",
                        "Well if Garbotnik Sanitation Industries took it to the landfill, then it is definitely at the "
                        "VERY BOTTOM of the dump.");
    bb_setCharacterLine(dData, 19, "Ovo", "Not a problem.");
    bb_setCharacterLine(dData, 20, "Ovo", "We have the technology to retrieve it.");
    bb_setCharacterLine(
        dData, 21, "Ovo",
        "Safety first. Activate the cloning machine in case I should perish on that nuclear wasteland.");
    bb_setCharacterLine(dData, 22, "Ovo", "fine.");
    bb_setCharacterLine(dData, 23, "Ovo", "Stupid safety rules. YOLO!");

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

    bb_dialogueData_t* dData = bb_createDialogueData(1, "Ovo");

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
            bb_setCharacterLine(dData, 0, "Ovo", "Ah, sweet stench! How I've longed for your orlfactory embrace.");
            break;
        }
        case 1:
        {
            bb_setCharacterLine(dData, 0, "Ovo",
                                "Tonight's special: Beetle Bruschetta with a side of centipede salad!");
            break;
        }
        case 2:
        {
            bb_setCharacterLine(dData, 0, "Ovo", "Another day, another dump full of delectable delights!");
            break;
        }
        case 3:
        {
            bb_setCharacterLine(dData, 0, "Ovo", "Step aside, garbage! The doctor is in!");
            break;
        }
        case 4:
        {
            bb_setCharacterLine(dData, 0, "Ovo", "I must find that chaos orb at all costs!");
            break;
        }
        case 5:
        {
            bb_setCharacterLine(dData, 0, "Ovo", "The Pango pest must be stopped!");
            break;
        }
        case 6:
        {
            bb_setCharacterLine(dData, 0, "Ovo", "I'll get you next time Pango! NEXT TIME!");
            break;
        }
        case 7:
        {
            bb_setCharacterLine(dData, 0, "Ovo", "Would you look at the time... It's GARBAGE DAY!");
            break;
        }
        case 8:
        {
            bb_setCharacterLine(dData, 0, "Ovo", "Did I remember to wax my moustache today?");
            break;
        }
        case 9:
        {
            bb_setCharacterLine(dData, 0, "Ovo", "Is it spelled \"moustache\" or \"mustache?\" Better look it up...");
            break;
        }
        case 10:
        {
            bb_setCharacterLine(dData, 0, "Ovo", "I wonder how my buddy Hank Waddle is holding up...");
            break;
        }
        case 11:
        {
            bb_setCharacterLine(dData, 0, "Ovo", "Man, I sure hope I see some cool bugs today!");
            break;
        }
        case 12:
        {
            bb_setCharacterLine(dData, 0, "Ovo", "I'm here to take in the trash.");
            break;
        }
        case 13:
        {
            bb_setCharacterLine(dData, 0, "Ovo",
                                "I must remember to research what dastardly technology this dump used to ensure new "
                                "arrivals wind up at the bottom...");
            break;
        }
        case 14:
        {
            bb_setCharacterLine(dData, 0, "Ovo",
                                "I promise, this is perfectly sanitary, the Chaos Orb has antimicrobial properties.");
            break;
        }
        case 15:
        {
            bb_setCharacterLine(dData, 0, "Ovo", "Of course you can trust me; I'm a doctor.");
            break;
        }
        case 16:
        {
            bb_setCharacterLine(dData, 0, "Ovo", "Come on and jump and welcome to the dump!");
            break;
        }
        case 17:
        {
            bb_setCharacterLine(dData, 0, "Ovo", "Oh, I'd been looking for that garbage ray!");
            break;
        }
        case 18:
        {
            bb_setCharacterLine(
                dData, 0, "Ovo",
                "If you ask me what I have a degree in one more time, I'm asking the Internet to draw fanart of you.");
            break;
        }
        case 19:
        {
            bb_setCharacterLine(dData, 0, "Ovo", "Oh, I'd been looking for that garbage ray!");
            break;
        }
        case 20:
        {
            bb_setCharacterLine(dData, 0, "Ovo", "Did I remember to turn off the disintegrator before I left the lab?");
            break;
        }
        case 21:
        {
            bb_setCharacterLine(dData, 0, "Ovo", "Oooh piece of candy!");
            break;
        }
        case 22:
        {
            bb_setCharacterLine(dData, 0, "Ovo",
                                "If they beat this game fast enough, does that mean I too have to strip?");
            break;
        }
        case 23:
        {
            bb_setCharacterLine(dData, 0, "Ovo", "Sluuurrrrrrrrrrp");
            break;
        }
        case 24:
        {
            bb_setCharacterLine(dData, 0, "Ovo", "A chili dog? No no, I've already had my... FILL.");
            break;
        }
        case 25:
        {
            bb_setCharacterLine(dData, 0, "Ovo", "CHAOS... CONT.... oh, I appear to have been sent a C&D.");
            break;
        }
        case 26:
        {
            bb_setCharacterLine(dData, 0, "Ovo", "I didn't like Clone #61 much anyway.");
            break;
        }
        case 27:
        {
            bb_setCharacterLine(dData, 0, "Ovo", "Remember, Ovo is pronounced like 'oooh-voo' with a wink at the end.");
            break;
        }
        case 28:
        {
            bb_setCharacterLine(dData, 0, "Ovo", "You don't have enough badge ribbons to train me.");
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

    bb_dialogueData_t* dData = bb_createDialogueData(2, "Ovo"); // 29

    bb_setCharacterLine(dData, 0, "Ovo", "I'm feeling fresh, baby!"); // V longest possible string here
    bb_setCharacterLine(dData, 1, "Ovo",
                        "It was a good move taking those omega3 fish oils before backing up my brain.");

    dData->curString = -1;

    dData->endDialogueCB = &bb_afterGarbotnikIntro;

    bb_setData(ovo, dData, DIALOGUE_DATA);
}

void bb_startGarbotnikEggTutorialTalk(bb_entity_t* self)
{
    bb_entity_t* ovo
        = bb_createEntity(&self->gameData->entityManager, NO_ANIMATION, true, OVO_TALK, 1,
                          self->gameData->camera.camera.pos.x, self->gameData->camera.camera.pos.y, true, true);

    bb_dialogueData_t* dData = bb_createDialogueData(2, "Ovo");

    // Max dialogue string roughly:                                                                         here----V
    bb_setCharacterLine(dData, 0, "Ovo", "Oooey Gooey! Look at that egg sack!");
    bb_setCharacterLine(dData, 1, "Ovo",
                        "I can use the directional buttons on my swadge to fly over there and check it out.");

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

    bb_dialogueData_t* dData = bb_createDialogueData(5, "Ovo");

    // longest possible string     " "
    //  bb_setCharacterLine(dData, 0, "A A A A A A A A A A A A A A A A A A A A A A A A A A A A A A A A A A A A A A A A A
    //  A A A A A A A A A A A A A A A A A A A");
    bb_setCharacterLine(dData, 0, "Ovo",
                        "When I travel away from the booster, I've got to keep an eye on my fuel level at all times.");
    bb_setCharacterLine(dData, 1, "Ovo",
                        "Sit back atop the booster before all the lights around the outside of the swadge turn off.");
    bb_setCharacterLine(dData, 2, "Ovo",
                        "safety first. Once back on the rocket, it takes a stupid long time for the "
                        "launch sequence to initiate.");
    bb_setCharacterLine(dData, 3, "Ovo", "Too many regulations on equipment these days.");
    bb_setCharacterLine(dData, 4, "Ovo", "Let a trashman go to space in peace.");

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
        if (self->gameData->entityManager.boosterEntities[i]->currentAnimationFrame != 41)
        {
            self->gameData->entityManager.activeBooster = self->gameData->entityManager.boosterEntities[i];

            break;
        }
    }

    ((bb_rocketData_t*)self->gameData->entityManager.activeBooster->data)->numDonuts = 20;

    self->gameData->loadoutScreenData = heap_caps_calloc(1, sizeof(bb_loadoutSelectScreenData_t), MALLOC_CAP_SPIRAM);
    self->gameData->screen = BIGBUG_LOADOUT_SELECT_SCREEN;

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
}

void bb_afterGarbotnikLandingTalk(bb_entity_t* self)
{
    bb_setupMidi();
    unloadMidiFile(&self->gameData->bgm);
    loadMidiFile("BigBugExploration.mid", &self->gameData->bgm, true);
    globalMidiPlayerPlaySong(&self->gameData->bgm, MIDI_BGM);

    self->gameData->isPaused = false;

    if (self->gameData->day)
    {
        return;
    }

    self->gameData->isPaused = false;
    // find the tutorial egg on screen
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
            break;
        }
    }
}

void bb_afterLiftoffInteraction(bb_entity_t* self)
{
    // set the pause illusion bit to 0.
    self->gameData->endDayChecks = self->gameData->endDayChecks & ~(1 << 0);
}

void bb_deployBooster(bb_entity_t* self) // separates from the death dumpster in orbit.
{
    self->gameData->endDayChecks = 0;
    // dive summary flavor text gets filled in randomly for the day.
    self->gameData->endDayChecks += bb_randomInt(0, 1) * (1 << 3);
    self->gameData->endDayChecks += bb_randomInt(0, 1) * (1 << 4);
    self->gameData->endDayChecks += bb_randomInt(0, 1) * (1 << 5);
    self->gameData->endDayChecks += bb_randomInt(0, 1) * (1 << 6);
    self->gameData->endDayChecks += bb_randomInt(0, 1) * (1 << 7);
    bb_destroyEntity(self->gameData->entityManager.viewEntity, false);
    self->gameData->entityManager.viewEntity = self->gameData->entityManager.activeBooster;

    bb_rocketData_t* rData = (bb_rocketData_t*)self->gameData->entityManager.activeBooster->data;
    bb_destroyEntity(rData->flame, false);
    rData->flame = NULL;

    self->gameData->entityManager.activeBooster->updateFunction = &bb_updateRocketLanding;
}

void bb_openMap(bb_entity_t* self)
{
    self->gameData->radar.cam.y = (self->gameData->entityManager.playerEntity->pos.y >> DECIMAL_BITS) / 8 - 120;
    self->gameData->screen      = BIGBUG_RADAR_SCREEN;
}

void bb_upgradeRadar(bb_entity_t* self)
{
    self->gameData->radar.playerPingRadius = 0; // just using this as a selection idx to save some space.
    self->gameData->radar.choices[0]       = (int8_t)BIGBUG_REFILL_AMMO; // refill ammo
    self->gameData->radar.choices[1]       = -1;                         // no choice available

    uint8_t zeroCount = 0; // zero count represent the number of upgrades not yet gotten.
    for (int i = 0; i < 7; i++)
    {
        if ((self->gameData->radar.upgrades & (1 << i)) == 0)
        { // Check if the bit at position i is 0
            zeroCount++;
        }
    }

    if (zeroCount > 0)
    {
        while (self->gameData->radar.choices[0] == BIGBUG_REFILL_AMMO)
        {
            enum bb_radarUpgrade_t candidate = (enum bb_radarUpgrade_t)bb_randomInt(0, 6);
            if (((self->gameData->radar.upgrades & (1 << candidate)) >> candidate) == 0)
            {
                self->gameData->radar.choices[0] = (int8_t)candidate;
            }
        }
    }

    if (zeroCount > 1)
    {
        while (self->gameData->radar.choices[1] == -1)
        {
            enum bb_radarUpgrade_t candidate = (enum bb_radarUpgrade_t)bb_randomInt(0, 6);
            if ((candidate != self->gameData->radar.choices[0])
                && (((self->gameData->radar.upgrades & 1 << candidate) >> candidate) == 0))
            {
                self->gameData->radar.choices[1] = (int8_t)candidate;
            }
        }
    }

    self->gameData->screen = BIGBUG_RADAR_UPGRADE_SCREEN;
}

void bb_triggerGameOver(bb_entity_t* self)
{
    // music is home 2, this function was already triggered. early return.
    if (self->gameData->bgm.length == 9715)
    {
        return;
    }
    bb_freeWsgs(&self->gameData->tilemap);
    bb_destroyEntity(self->gameData->entityManager.playerEntity, false);
    self->gameData->entityManager.playerEntity = NULL;

    bb_setupMidi();
    unloadMidiFile(&self->gameData->bgm);
    loadMidiFile("BigBug_Dr.Garbotniks Home2.mid", &self->gameData->bgm, true);
    globalMidiPlayerPlaySong(&self->gameData->bgm, MIDI_BGM);

    bb_createEntity(&self->gameData->entityManager, NO_ANIMATION, true, BB_GAME_OVER, 1,
                    self->gameData->camera.camera.pos.x, self->gameData->camera.camera.pos.y, true, true);

    self->gameData->camera.camera.pos = (vec_t){(self->gameData->entityManager.deathDumpster->pos.x >> DECIMAL_BITS),
                                                (self->gameData->entityManager.deathDumpster->pos.y >> DECIMAL_BITS)};

    self->gameData->entityManager.viewEntity
        = bb_createEntity(&(self->gameData->entityManager), NO_ANIMATION, true, NO_SPRITE_POI, 1,
                          self->gameData->camera.camera.pos.x, self->gameData->camera.camera.pos.y, true, false);
}

void bb_upgradeGarbotnik(bb_entity_t* self)
{
    self->gameData->radar.playerPingRadius      = 0; // just using this as a selection idx to save some space.
    self->gameData->garbotnikUpgrade.choices[0] = (int8_t)GARBOTNIK_MORE_DIGGING_STRENGTH; // default choice
    uint8_t zeroCount                           = 0; // zero count represent the number of upgrades not yet maxed out.
    for (int i = 0; i < 3; i++)
    {
        if ((self->gameData->garbotnikUpgrade.upgrades & (1 << i)) == 0)
        { // Check if the bit at position i is 0
            zeroCount++;
        }
    }
    if (zeroCount > 1)
    {
        enum bb_garbotnikUpgrade_t candidate = (enum bb_garbotnikUpgrade_t)bb_randomInt(0, 2);
        while (((self->gameData->garbotnikUpgrade.upgrades & (1 << candidate)) >> candidate) == 1)
        {
            candidate = (enum bb_garbotnikUpgrade_t)bb_randomInt(0, 2);
        }
        self->gameData->garbotnikUpgrade.choices[0] = (int8_t)candidate;
        candidate                                   = (enum bb_garbotnikUpgrade_t)bb_randomInt(0, 2);
        while (((self->gameData->garbotnikUpgrade.upgrades & (1 << candidate)) >> candidate) == 1
               || candidate == self->gameData->garbotnikUpgrade.choices[0])
        {
            candidate = (enum bb_garbotnikUpgrade_t)bb_randomInt(0, 2);
        }
        self->gameData->garbotnikUpgrade.choices[1] = (int8_t)candidate;
    }
    self->gameData->screen = BIGBUG_GARBOTNIK_UPGRADE_SCREEN;
}

void bb_playCarAlarm(bb_entity_t* self)
{
    bb_carData_t* cData = (bb_carData_t*)self->data;
    if (cData->midiLoaded)
    {
        unloadMidiFile(&cData->alarm);
    }
    switch (bb_randomInt(1, 3))
    {
        case 1:
        {
            loadMidiFile("BigBug - Car 1.mid", &cData->alarm, true);
            break;
        }
        case 2:
        {
            loadMidiFile("BigBug - Car 2.mid", &cData->alarm, true);
            break;
        }
        default: // case 3
        {
            loadMidiFile("BigBug - Car 3.mid", &cData->alarm, true);
            break;
        }
    }
    cData->midiLoaded = true;
    // Play sfx
    midiPlayer_t* sfx = soundGetPlayerSfx();
    midiPlayerReset(sfx);
    soundPlaySfx(&cData->alarm, 0);
}

void bb_crumbleDirt(bb_gameData_t* gameData, uint8_t gameFramesPerAnimationFrame, uint8_t tile_i, uint8_t tile_j,
                    bool zeroHealth)
{
    // Create a crumble animation
    bb_createEntity(&gameData->entityManager, ONESHOT_ANIMATION, false, CRUMBLE_ANIM, gameFramesPerAnimationFrame,
                    tile_i * TILE_SIZE + HALF_TILE, tile_j * TILE_SIZE + HALF_TILE, false, false);

    // Play sfx
    midiPlayer_t* sfx = soundGetPlayerSfx();
    midiPlayerReset(sfx);
    soundPlaySfx(&gameData->sfxBump, 0);

    if (zeroHealth)
    {
        flagNeighbors((bb_midgroundTileInfo_t*)&gameData->tilemap.fgTiles[tile_i][tile_j], gameData);
        switch (gameData->tilemap.fgTiles[tile_i][tile_j].embed)
        {
            case EGG_EMBED:
            {
                vec_t tilePos = {.x = tile_i * TILE_SIZE + HALF_TILE, .y = tile_j * TILE_SIZE + HALF_TILE};
                // create a bug
                bb_entity_t* bug = bb_createEntity(&gameData->entityManager, LOOPING_ANIMATION, false,
                                                   bb_randomInt(8, 13), 1, tilePos.x, tilePos.y, false, false);

                if (bug != NULL)
                {
                    midiPlayerReset(sfx);
                    soundPlaySfx(&gameData->sfxEgg, 0);

                    if (gameData->tilemap.fgTiles[tile_i][tile_j].entity != NULL)
                    {
                        bb_entity_t* egg
                            = ((bb_eggLeavesData_t*)(gameData->tilemap.fgTiles[tile_i][tile_j].entity->data))->egg;
                        if (egg != NULL)
                        {
                            // destroy the egg
                            bb_destroyEntity(egg, false);
                        }
                        // destroy this (eggLeaves)
                        bb_destroyEntity(gameData->tilemap.fgTiles[tile_i][tile_j].entity, false);
                    }
                    gameData->tilemap.fgTiles[tile_i][tile_j].embed = NOTHING_EMBED;
                }
                break;
            }
            case SKELETON_EMBED:
            {
                vec_t tilePos = {.x = tile_i * TILE_SIZE + HALF_TILE, .y = tile_j * TILE_SIZE + HALF_TILE};
                bb_destroyEntity(gameData->tilemap.fgTiles[tile_i][tile_j].entity, false);
                gameData->tilemap.fgTiles[tile_i][tile_j].embed = NOTHING_EMBED;

                // create fuel
                bb_createEntity(&gameData->entityManager, LOOPING_ANIMATION, false, BB_FUEL, 10, tilePos.x, tilePos.y,
                                false, false);
                break;
            }
            default:
            {
                break;
            }
        }
    }
}

bb_dialogueData_t* bb_createDialogueData(uint8_t numStrings, const char* firstCharacter)
{
    bb_dialogueData_t* dData = heap_caps_calloc(1, sizeof(bb_dialogueData_t), MALLOC_CAP_SPIRAM);
    dData->numStrings        = numStrings;
    dData->offsetY           = -240;
    int8_t characterSprite = 0;
    if (strcmp(firstCharacter, "Ovo") == 0)
    {
        characterSprite = bb_randomInt(0, 6);
        loadWsgInplace("dialogue_next.wsg", &dData->spriteNext, true, bb_decodeSpace, bb_hsd); // TODO free
    }
    else if (strcmp(firstCharacter, "Pixel") == 0)
    {
        characterSprite = bb_randomInt(7, 8);
        //borrow sprite from UTT
        loadWsgInplace("pixil_rs.wsg", &dData->spriteNext, true, bb_decodeSpace, bb_hsd);
    }
    else if (strcmp(firstCharacter, "Pango") == 0)
    {
        characterSprite = bb_randomInt(9, 10);
        //borrow sprite from UTT
        loadWsgInplace("hotdog_rs.wsg", &dData->spriteNext, true, bb_decodeSpace, bb_hsd);
    }
    else if (strcmp(firstCharacter, "Po") == 0)
    {
        characterSprite = bb_randomInt(11, 13);
        //borrow sprite from UTT
        loadWsgInplace("hand_rs.wsg", &dData->spriteNext, true, bb_decodeSpace, bb_hsd);
    }
    // Add dr. Ovo indices
    // FINISH ME!!!

    char wsg_name[strlen("ovo_talk") + 9]; // 6 extra characters makes room for up to a 2 digit number + ".wsg" + null
                                           // terminator ('\0')
    snprintf(wsg_name, sizeof(wsg_name), "%s%d.wsg", "ovo_talk", characterSprite);
    loadWsgInplace(wsg_name, &dData->sprite, true, bb_decodeSpace, bb_hsd);

    dData->strings    = heap_caps_calloc(numStrings, sizeof(char*), MALLOC_CAP_SPIRAM);
    dData->characters = heap_caps_calloc(numStrings, sizeof(char*), MALLOC_CAP_SPIRAM);
    return dData;
}

void bb_setCharacterLine(bb_dialogueData_t* dData, uint8_t index, const char* character, const char* str)
{
    dData->strings[index]    = heap_caps_calloc(strlen(str) + 1, sizeof(char), MALLOC_CAP_SPIRAM);
    dData->characters[index] = heap_caps_calloc(strlen(character) + 1, sizeof(char), MALLOC_CAP_SPIRAM);
    strcpy(dData->strings[index], str);
    strcpy(dData->characters[index], character);
}

void bb_freeDialogueData(bb_dialogueData_t* dData)
{
    for (int i = 0; i < dData->numStrings; i++)
    {
        heap_caps_free(dData->strings[i]); // Free each string
    }
    heap_caps_free(dData->strings); // Free the array of string pointers
    heap_caps_free(dData);          // Free the struct itself
}
