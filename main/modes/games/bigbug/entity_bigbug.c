//==============================================================================
// Includes
//==============================================================================
#include <stdlib.h>
#include <esp_log.h>
#include <math.h>

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
// #define TILE_FIELD_WIDTH  32  // matches the level wsg graphic width
// #define TILE_FIELD_HEIGHT 192 // matches the level wsg graphic height

//==============================================================================
// Functions
//==============================================================================
void bb_initializeEntity(bb_entity_t* self, bb_entityManager_t* entityManager, bb_gameData_t* gameData,
                         bb_soundManager_t* soundManager)
{
    self->active        = false;
    self->gameData      = gameData;
    self->soundManager  = soundManager;
}

void bb_setData(bb_entity_t* self, void* data)
{
    if (self->data != NULL)
    {
        FREE_DBG(self->data);
    }
    self->data = data;
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
        if(self->spriteIndex == OVO_TALK)
        {
            bb_dialogueData_t* dData = (bb_dialogueData_t*) self->data;
            for(int i = 0; i<dData->numStrings; i++){
                FREE_DBG(dData->strings[i]);
            }
            FREE_DBG(dData->strings);
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

    if (self->pos.y > -5000 && rData->flame == NULL)
    {
        rData->flame = bb_createEntity(&(self->gameData->entityManager), LOOPING_ANIMATION, false, FLAME_ANIM, 2,
                                       self->pos.x >> DECIMAL_BITS, self->pos.y >> DECIMAL_BITS, false);
    }

    else if (rData->flame != NULL)
    {
        rData->yVel -= 7;
        rData->flame->pos.y = self->pos.y + rData->yVel * self->gameData->elapsedUs / 100000;
        if (rData->yVel <= 0)
        {
            bb_destroyEntity(rData->flame, false);
            bb_setData(self, HEAP_CAPS_CALLOC_DBG(1, sizeof(bb_heavyFallingData_t), MALLOC_CAP_SPIRAM));
            self->updateFunction                     = bb_updateHeavyFallingInit;

            bb_entity_t* arm = bb_createEntity(&self->gameData->entityManager, NO_ANIMATION, true, ATTACHMENT_ARM, 1, self->pos.x >> DECIMAL_BITS, (self->pos.y >> DECIMAL_BITS) - 33, false);
            ((bb_attachmentArmData_t*)arm->data)->rocket = self;
            self->gameData->entityManager.viewEntity = NULL;

            return;
        }
    }
    else if (rData->yVel < 350)
    {
        rData->yVel++;
    }
    self->pos.y += rData->yVel * self->gameData->elapsedUs / 100000;
}

void bb_updateHeavyFallingInit(bb_entity_t* self)
{
    bb_heavyFallingData_t* hfData = (bb_heavyFallingData_t*)self->data;
    hfData->yVel++;

    self->pos.y += hfData->yVel * (self->gameData->elapsedUs >> 14);

    bb_hitInfo_t hitInfo = {0};
    bb_collisionCheck(&self->gameData->tilemap, self, NULL, &hitInfo);
    if (hitInfo.hit == false)
    {
        return;
    }

    self->pos.y = hitInfo.pos.y - self->halfHeight;
    if (hfData->yVel < 160)
    {
        hfData->yVel         = 0;
        self->updateFunction = bb_updateGarbotnikDeploy;
        self->paused         = false;
    }
    else
    {
        hfData->yVel -= 160;
        // Update the dirt to air.
        self->gameData->tilemap.fgTiles[hitInfo.tile_i][hitInfo.tile_j].health = 0;
        // Create a crumble animation
        bb_createEntity(&(self->gameData->entityManager), ONESHOT_ANIMATION, false, CRUMBLE_ANIM, 1,
                        hitInfo.tile_i * TILE_SIZE + TILE_SIZE, hitInfo.tile_j * TILE_SIZE + TILE_SIZE, true);
    }
    return;
}

void bb_updateHeavyFalling(bb_entity_t* self)
{
    bb_heavyFallingData_t* hfData = (bb_heavyFallingData_t*)self->data;
    hfData->yVel++;
    self->pos.y += hfData->yVel * (self->gameData->elapsedUs >> 14);

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
    if (hfData->yVel < 28)
    {
        hfData->yVel = 0;
    }
    else
    {
        hfData->yVel -= 28;
        // Update the dirt to air.
        self->gameData->tilemap.fgTiles[hitInfo.tile_i][hitInfo.tile_j].health = 0;
        // Create a crumble animation
        bb_createEntity(&(self->gameData->entityManager), ONESHOT_ANIMATION, false, CRUMBLE_ANIM, 1,
                        hitInfo.tile_i * TILE_SIZE + HALF_TILE, hitInfo.tile_j * TILE_SIZE + HALF_TILE, true);
    }
    return;
}

void bb_updatePhysicsObject(bb_entity_t* self)
{
    bb_physicsData_t* pData = (bb_physicsData_t*)self->data;
    pData->vel.y++;
    self->pos = addVec2d(self->pos, mulVec2d(pData->vel, (self->gameData->elapsedUs >> 14)));

    bb_hitInfo_t hitInfo = {0};
    bb_collisionCheck(&self->gameData->tilemap, self, NULL, &hitInfo);
    if (hitInfo.hit == false)
    {
        return;
    }
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
    if (self->currentAnimationFrame == self->gameData->entityManager.sprites[self->spriteIndex].numFrames - 1)
    {
        self->paused = true;
        // deploy garbotnik!!!
        bb_entity_t* garbotnik
            = bb_createEntity(&(self->gameData->entityManager), NO_ANIMATION, true, GARBOTNIK_FLYING, 1,
                              self->pos.x >> DECIMAL_BITS, (self->pos.y >> DECIMAL_BITS) - 50, true);
        self->gameData->entityManager.viewEntity = garbotnik;
        self->updateFunction                     = bb_updateHeavyFalling;
    }
}

void bb_updateGarbotnikFlying(bb_entity_t* self)
{
    bb_garbotnikData_t* gData = (bb_garbotnikData_t*)self->data;

    gData->fuel
        -= self->gameData->elapsedUs >> 10; // Fuel decrements with time. Right shifting by 10 is fairly close to
                                            // converting microseconds to milliseconds without requiring division.
    if (gData->fuel < 0)
    {
        gData->fuel = 0;
    }

    // touchpad stuff
    gData->fire     = gData->touching;
    gData->touching = getTouchJoystick(&gData->phi, &gData->r, &gData->intensity);
    gData->fire     = gData->fire && !gData->touching; // is true for one frame upon touchpad release.
    if (gData->fire && gData->numHarpoons > 0)
    {
        // Create a harpoon
        bb_entity_t* harpoon = bb_createEntity(&(self->gameData->entityManager), LOOPING_ANIMATION, false, HARPOON, 1,
                                               self->pos.x >> DECIMAL_BITS, self->pos.y >> DECIMAL_BITS, false);
        if (harpoon != NULL)
        {
            gData->numHarpoons -= 1;
            bb_projectileData_t* pData = (bb_projectileData_t*)harpoon->data;
            int32_t x;
            int32_t y;
            getTouchCartesian(gData->phi, gData->r, &x, &y);
            // Set harpoon's velocity
            pData->vel.x = (x - 512) / 10;
            pData->vel.y = (-y + 512) / 10;
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
            accel.y = -50;
            break;
        case 0b1101:
            accel.y = -50;
            break;

        // down
        case 0b0010:
            accel.y = 50;
            break;
        case 0b1110:
            accel.y = 50;
            break;

        // left
        case 0b0100:
            accel.x = -50;
            break;
        case 0b0111:
            accel.x = -50;
            break;

        // right
        case 0b1000:
            accel.x = 50;
            break;
        case 0b1011:
            accel.x = 50;
            break;

        // up,left
        case 0b0101:
            accel.x = -35; // magnitude is sqrt(1/2) * 100000
            accel.y = -35;
            break;

        // up,right
        case 0b1001:
            accel.x = 35; // 35 707 7035
            accel.y = -35;
            break;

        // down,right
        case 0b1010:
            accel.x = 35;
            accel.y = 35;
            break;

        // down,left
        case 0b0110:
            accel.x = -35;
            accel.y = 35;
            break;
        default:
            break;
    }

    // printf("accel x: %d\n", accel.x);
    // printf("elapsed: %d", (int32_t) elapsedUs);
    // printf("offender: %d\n", (int32_t) elapsedUs / 100000);
    // printf("now   x: %d\n", mulVec2d(accel, elapsedUs) / 100000).x);

    gData->accel = divVec2d(mulVec2d(accel, self->gameData->elapsedUs), 100000);

    // physics
    gData->yaw.y += gData->accel.x;
    if (gData->yaw.x < 0)
    {
        gData->yaw.y -= 5.0 * self->gameData->elapsedUs / 100000;
    }
    else
    {
        gData->yaw.y += 5.0 * self->gameData->elapsedUs / 100000;
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
    int32_t sqMagVel = sqMagVec2d(gData->vel);
    int32_t speed    = sqrt(sqMagVel);
    int32_t drag     = sqMagVel / 500; // smaller denominator for bigger drag.

    if (drag > speed * 0.9)
    {
        drag = speed * 0.9;
    }
    if (drag < 5)
    {
        drag = 5.0;
    }
    // printf("speed: %d\n", speed);
    // printf("drag: %d\n", drag);
    if (speed > 0)
    {
        gData->accel.x += (gData->vel.x / (double)speed) * -drag * self->gameData->elapsedUs / 100000;
        gData->accel.y += (gData->vel.y / (double)speed) * -drag * self->gameData->elapsedUs / 100000;
        // bigbug->garbotnikAccel = addVec2d(bigbug->garbotnikAccel, mulVec2d(divVec2d(bigbug->garbotnikVel, speed),
        // -drag * elapsedUs / 100000));
    }

    // Update garbotnik's velocity
    gData->vel.x += gData->accel.x;
    gData->vel.y += gData->accel.y;

    // Update garbotnik's position
    self->pos.x += gData->vel.x * self->gameData->elapsedUs / 100000;
    self->pos.y += gData->vel.y * self->gameData->elapsedUs / 100000;

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

    // printf("dot product: %d\n",dotVec2d(bigbug->garbotnikVel, normal));
    if (dotVec2d(gData->vel, hitInfo.normal) < -95)
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

        if (self->gameData->tilemap.fgTiles[hitInfo.tile_i][hitInfo.tile_j].health == 0
            && self->gameData->tilemap.fgTiles[hitInfo.tile_i][hitInfo.tile_j].embed == EGG_EMBED
            && self->gameData->tilemap.fgTiles[hitInfo.tile_i][hitInfo.tile_j].entity != NULL)
        {
            vec_t tilePos = {.x = hitInfo.tile_i * TILE_SIZE + HALF_TILE, .y = hitInfo.tile_j * TILE_SIZE + HALF_TILE};
            // create a bug
            bb_entity_t* bug = bb_createEntity(&self->gameData->entityManager, LOOPING_ANIMATION, false,
                                               bb_randomInt(8, 13), 1, tilePos.x, tilePos.y, false);
            if (bug != NULL)
            {
                // destroy the egg
                bb_destroyEntity(((bb_eggLeavesData_t*)(self->gameData->tilemap.fgTiles[hitInfo.tile_i][hitInfo.tile_j]
                                                            .entity->data))
                                     ->egg,
                                 false);
                // destroy this (eggLeaves)
                bb_destroyEntity(self->gameData->tilemap.fgTiles[hitInfo.tile_i][hitInfo.tile_j].entity, false);
            }
        }

        if (self->gameData->tilemap.fgTiles[hitInfo.tile_i][hitInfo.tile_j].health == 0
            || self->gameData->tilemap.fgTiles[hitInfo.tile_i][hitInfo.tile_j].health == 1
            || self->gameData->tilemap.fgTiles[hitInfo.tile_i][hitInfo.tile_j].health == 4)
        {
            // Create a crumble animation
            bb_createEntity(&(self->gameData->entityManager), ONESHOT_ANIMATION, false, CRUMBLE_ANIM, 1,
                            hitInfo.tile_i * TILE_SIZE + HALF_TILE, hitInfo.tile_j * TILE_SIZE + HALF_TILE, true);
        }
        else
        {
            // Create a bump animation
            bb_createEntity(&(self->gameData->entityManager), ONESHOT_ANIMATION, false, BUMP_ANIM, 1,
                            hitInfo.pos.x >> DECIMAL_BITS, hitInfo.pos.y >> DECIMAL_BITS, true);
        }

        ////////////////////////////////
        // Mirror garbotnik's velocity//
        ////////////////////////////////
        // Reflect the velocity vector along the normal
        // See http://www.sunshine2k.de/articles/coding/vectorreflection/vectorreflection.html
        // printf("hit squared speed: %" PRId32 "\n", sqMagVec2d(gData->vel));
        int32_t bounceScalar = sqMagVec2d(gData->vel) / -11075 + 3;
        if (bounceScalar > 3)
        {
            bounceScalar = 3;
        }
        else if (bounceScalar < 1)
        {
            bounceScalar = 1;
        }
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
    self->pos = addVec2d(self->pos, mulVec2d(pData->vel, (self->gameData->elapsedUs >> 14)));

    bb_hitInfo_t hitInfo = {0};
    bb_collisionCheck(&self->gameData->tilemap, self, NULL, &hitInfo);
    if (hitInfo.hit && pData->prevFrameInAir && pData->vel.y > 0)
    {
        vecFl_t floatVel              = {(float)pData->vel.x, (float)pData->vel.y};
        bb_stuckHarpoonData_t* shData = HEAP_CAPS_CALLOC_DBG(1, sizeof(bb_stuckHarpoonData_t), MALLOC_CAP_SPIRAM);
        shData->floatVel              = normVecFl2d(floatVel);
        bb_setData(self, shData);

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
        else
        {
            elData->brightness = bb_foregroundLighting(
                &(self->gameData->tilemap.headlampWsg), &lookup,
                &(((bb_garbotnikData_t*)self->gameData->entityManager.playerEntity->data)->yaw.x));
        }

        bb_eggData_t* eData = (bb_eggData_t*)elData->egg->data;
        eData->stimulation += elData->brightness * 2;
        if (eData->stimulation > 0)
        {
            eData->stimulation -= 2;
        }
        if (eData->stimulation > 399)
        {
            ((bb_entity_t*)elData->egg)->pos.x += bb_randomInt(-1, 1);
            ((bb_entity_t*)elData->egg)->pos.y += bb_randomInt(-1, 1);
        }
        if (eData->stimulation > 599)
        {
            eData->stimulation = 599;
            // transform "hatch" into a bug...
            self->pos = elData->egg->pos; // sets what will be the bug to the egg position, because eggs tend to wiggle
                                          // about before hatching.

            // create a bug
            bb_entity_t* bug
                = bb_createEntity(&self->gameData->entityManager, LOOPING_ANIMATION, false, bb_randomInt(8, 13), 1,
                                  elData->egg->pos.x >> DECIMAL_BITS, elData->egg->pos.y >> DECIMAL_BITS, false);

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
                    // Create a crumble animation
                    bb_createEntity(&(self->gameData->entityManager), ONESHOT_ANIMATION, false, CRUMBLE_ANIM, 1,
                                    hitInfo.tile_i * TILE_SIZE + TILE_SIZE, hitInfo.tile_j * TILE_SIZE + TILE_SIZE,
                                    false);
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
    if((self->pos.y >> DECIMAL_BITS) < self->gameData->camera.camera.pos.y)
    {
        bb_destroyEntity(self, false);
    }
}

void bb_updateMenuBug(bb_entity_t* self)
{
    bb_menuBugData_t* mbData = (bb_menuBugData_t*)self->data;
    self->pos.x += (mbData->xVel - 3) << DECIMAL_BITS;
    if (mbData->firstTrip && self->pos.x < self->gameData->entityManager.viewEntity->pos.x - (130 << DECIMAL_BITS))
    {
        mbData->firstTrip = false;
        mbData->xVel      = bb_randomInt(-5, 5);
        mbData->xVel
            = mbData->xVel == 3 ? mbData->xVel - 1 : mbData->xVel; // So as not to match the treadmill speed exactly.
        self->gameFramesPerAnimationFrame = abs(6 - mbData->xVel);
        if (mbData->xVel == 0)
        {
            self->gameFramesPerAnimationFrame = 255;
        }
    }
}

void bb_updateMoveLeft(bb_entity_t* self)
{
    self->pos.x -= 3 << DECIMAL_BITS;
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
            tData->tracking = bb_createEntity(&self->gameData->entityManager, NO_ANIMATION, true, BB_DEATH_DUMPSTER, 1,
                                              self->pos.x >> DECIMAL_BITS, (self->pos.y >> DECIMAL_BITS) + 586, true);
            tData->midPointSqDist = sqMagVec2d(
                divVec2d(subVec2d(tData->tracking->pos, self->gameData->entityManager.viewEntity->pos), 2));

            self->gameData->entityManager.viewEntity->updateFunction = &bb_updatePOI;

            for(int rocketIdx = 0; rocketIdx < 3; rocketIdx++){
                self->gameData->entityManager.boosterEntities[rocketIdx] = bb_createEntity(&self->gameData->entityManager, NO_ANIMATION, true, ROCKET_ANIM, 3,
                    (self->pos.x >> DECIMAL_BITS) - 96 + 96 * rocketIdx, (self->pos.y >> DECIMAL_BITS) + 561, true);
            
                self->gameData->entityManager.boosterEntities[rocketIdx]->updateFunction = NULL;

                bb_rocketData_t* rData = (bb_rocketData_t*)self->gameData->entityManager.boosterEntities[rocketIdx]->data;

                rData->flame = bb_createEntity(&(self->gameData->entityManager), LOOPING_ANIMATION, false, FLAME_ANIM, 2,
                    self->gameData->entityManager.boosterEntities[rocketIdx]->pos.x >> DECIMAL_BITS, self->gameData->entityManager.boosterEntities[rocketIdx]->pos.y >> DECIMAL_BITS, true);

                rData->flame->updateFunction = &bb_updateFlame;
            }

            self->updateFunction = NULL;
            return;
        }
        else if(mData->selectionIdx == 1)
        {
            //exit the game
            self->gameData->exit = true;
        }
    }


    mData->cursor->pos.y = self->pos.y - (209<<DECIMAL_BITS) + mData->selectionIdx * (22<<DECIMAL_BITS);

    if (self->gameData->menuBug == NULL || self->gameData->menuBug->active == false)
    {
        self->gameData->menuBug
            = bb_createEntity(&self->gameData->entityManager, LOOPING_ANIMATION, false, bb_randomInt(8, 13), 1,
                              (self->pos.x >> DECIMAL_BITS) + 135, (self->pos.y >> DECIMAL_BITS) - 172, true);
        self->gameData->menuBug->cacheable = false;
        self->gameData->menuBug->drawFunction      = &bb_drawMenuBug;
        self->gameData->menuBug->updateFunction    = &bb_updateMenuBug;
        self->gameData->menuBug->updateFarFunction = &bb_updateFarDestroy;
        bb_menuBugData_t* mbData                   = HEAP_CAPS_CALLOC_DBG(1, sizeof(bb_menuBugData_t), MALLOC_CAP_SPIRAM);
        mbData->xVel                               = bb_randomInt(-5, 5);
        mbData->xVel
            = mbData->xVel == 3 ? mbData->xVel - 1 : mbData->xVel; // So as not to match the treadmill speed exactly.
        mbData->firstTrip                                    = true;
        FREE_DBG(self->gameData->menuBug->data);
        self->gameData->menuBug->data                        = mbData;
        self->gameData->menuBug->gameFramesPerAnimationFrame = abs(6 - mbData->xVel);
        if (mbData->xVel == 0)
        {
            self->gameData->menuBug->gameFramesPerAnimationFrame = 255;
        }
    }

    if (bb_randomInt(0, 1) < 1) //%50
    {
        bb_entity_t* treadmillDust
            = bb_createEntity(&self->gameData->entityManager, NO_ANIMATION, true, NO_SPRITE_STAR, 1,
                              (self->pos.x >> DECIMAL_BITS) + 140, (self->pos.y >> DECIMAL_BITS) - 164 + bb_randomInt(-10, 10), false);
        treadmillDust->updateFunction = &bb_updateMoveLeft;
    }
}



void bb_updatePOI(bb_entity_t* self)
{
    bb_goToData* tData = (bb_goToData*)self->data;
    if (tData != NULL)
    {
        vec_t ToFrom = subVec2d(tData->tracking->pos, self->pos);
        if (sqMagVec2d(ToFrom) > tData->midPointSqDist)
        {
            tData->speed++;
        }
        else
        {
            tData->speed--;
        }
        if (tData->speed == 0)
        {
            tData->executeOnArrival(self);
            self->updateFunction = NULL;

            return;
        }
        fastNormVec(&ToFrom.x, &ToFrom.y);
        ToFrom    = mulVec2d(ToFrom, tData->speed);
        ToFrom.x  = ToFrom.x >> 8;
        ToFrom.y  = ToFrom.y >> 8;
        self->pos = addVec2d(self->pos, ToFrom);
    }
}

void bb_updateFlame(bb_entity_t* self)
{
    if(self->currentAnimationFrame > 2)
    {
        if(bb_randomInt(0,1))
        {
            int newFrame = 0;
            while(bb_randomInt(0,1) && newFrame < self->currentAnimationFrame - 1)
            {
                newFrame++;
            }
            self->animationTimer = newFrame;
            self->currentAnimationFrame = newFrame;
        }
    }
}

void bb_updateCharacterTalk(bb_entity_t* self)
{
    bb_dialogueData_t* dData = (bb_dialogueData_t*) self->data;

    if(self->gameData->entityManager.sprites[self->spriteIndex].originY < -30 && dData->curString < dData->numStrings)
    {
        self->gameData->entityManager.sprites[self->spriteIndex].originY += 10;
    }
    else if(dData->curString >= dData->numStrings)
    {
        self->gameData->entityManager.sprites[self->spriteIndex].originY -= 10;
        if(self->gameData->entityManager.sprites[self->spriteIndex].originY <= -240)
        {
            dData->endDialogueCB(self);
            bb_destroyEntity(self, false);
            return;
        }
    }
    else
    {

        if(dData->curString < 0)
        {
            dData->curString = 0;
        }

        if(self->gameData->btnDownState & PB_A)
        {
            dData->curString++;
            if(dData->curString < dData->numStrings)
            {
                self->currentAnimationFrame = bb_randomInt(0,7);
            }
        }
    }
}

void bb_updateAttachmentArm(bb_entity_t* self)
{
    bb_attachmentArmData_t* aData = (bb_attachmentArmData_t*) self->data;
    self->pos = aData->rocket->pos;
    if(aData->angle > 180)
    {
        aData->angle--;
    }
    printf("angle %d\n", aData->angle);
}

void bb_drawGarbotnikFlying(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self)
{
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
    angle += 135;
    while (angle < 0)
    {
        angle += 360;
    }
    while (angle > 359)
    {
        angle -= 360;
    }

    floatVel = normVecFl2d(floatVel);

    drawLineFast(xOff, yOff - 1, xOff - floatVel.x * 20, yOff - floatVel.y * 20 - 1, c344);
    drawLineFast(xOff, yOff, xOff - floatVel.x * 20, yOff - floatVel.y * 20, c223);
    drawLineFast(xOff, yOff + 1, xOff - floatVel.x * 20, yOff - floatVel.y * 20 + 1, c000);


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

    drawWsgSimple(&entityManager->sprites[self->spriteIndex].frames[((bb_eggData_t*)self->data)->stimulation / 100],
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
    bb_dialogueData_t* dData = (bb_dialogueData_t*) self->data;

    drawWsgSimple(
                    &entityManager->sprites[self->spriteIndex].frames[self->currentAnimationFrame],
                    (self->pos.x >> DECIMAL_BITS)
                        - entityManager->sprites[self->spriteIndex].originX - camera->pos.x,
                    (self->pos.y >> DECIMAL_BITS)
                        - entityManager->sprites[self->spriteIndex].originY - camera->pos.y);
    
    
    if(dData->curString >= 0 && dData->curString < dData->numStrings)
    {
        drawText(&self->gameData->font, c344, dData->character, 33, 184);

        uint8_t end = 28;
        if(end > strlen(dData->strings[dData->curString]))
        {
            end = strlen(dData->strings[dData->curString]);
        }
        else
        {
            while(dData->strings[dData->curString][end-1] != ' ')
            {
                end--;
                if(end == 0)
                {
                    end = strlen(dData->strings[dData->curString]);
                    break;
                }
            }
        }
        char substring[29];
        strncpy(substring, dData->strings[dData->curString], end);
        substring[28] = '\0';
        drawText(&self->gameData->font, c344, substring, 33, 202);
        if(strlen(dData->strings[dData->curString]) > 28)
        {
            uint8_t end2 = 28;
            end2 = end2 > strlen(dData->strings[dData->curString]) - end ? strlen(dData->strings[dData->curString]) - end : end2;
            char substring2[29];
            strncpy(substring2, dData->strings[dData->curString] + end, end2);
            substring2[28] = '\0';
            drawText(&self->gameData->font, c344, substring2, 33, 220);
        }
    }
}

void bb_drawAttachmentArm(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self)
{
    bb_attachmentArmData_t* aData = (bb_attachmentArmData_t*) self->data;
    drawWsg(
                &entityManager->sprites[self->spriteIndex].frames[self->currentAnimationFrame],
                (self->pos.x >> DECIMAL_BITS)
                    - entityManager->sprites[self->spriteIndex].originX - camera->pos.x,
                (self->pos.y >> DECIMAL_BITS)
                    - entityManager->sprites[self->spriteIndex].originY - camera->pos.y + 33,
                    false, false, aData->angle);
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
        bData->health               = 0;
        other->paused               = true;
        bb_physicsData_t* physData  = HEAP_CAPS_CALLOC_DBG(1, sizeof(bb_physicsData_t), MALLOC_CAP_SPIRAM);
        physData->vel               = divVec2d(pData->vel, 2);
        physData->bounceNumerator   = 2; // 66% bounce
        physData->bounceDenominator = 3;
        bb_setData(other, physData);
        other->updateFunction = bb_updatePhysicsObject;
    }
    vecFl_t floatVel              = {(float)pData->vel.x, (float)pData->vel.y};
    bb_stuckHarpoonData_t* shData = HEAP_CAPS_CALLOC_DBG(1, sizeof(bb_stuckHarpoonData_t), MALLOC_CAP_SPIRAM);
    shData->parent                = other;
    shData->offset                = subVec2d(self->pos, other->pos);
    shData->floatVel              = normVecFl2d(floatVel);
    bb_setData(self, shData);

    bb_clearCollisions(self, false);

    self->updateFunction = bb_updateStuckHarpoon;
    self->drawFunction   = bb_drawStuckHarpoon;
}

void bb_onCollisionRocket(bb_entity_t* self, bb_entity_t* other, bb_hitInfo_t* hitInfo)
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
    if (hitInfo->normal.y == 1)
    {
        gData->gettingCrushed = true;
    }
}

void bb_startGarbotnikIntro(bb_entity_t* self)
{
        bb_entity_t* ovo = bb_createEntity(&self->gameData->entityManager, NO_ANIMATION, true, OVO_TALK, 1, self->gameData->camera.camera.pos.x, self->gameData->camera.camera.pos.y, true);

        bb_dialogueData_t* dData = bb_createDialogueData(34);//34

        strncpy(dData->character, "Dr. Ovo", sizeof(dData->character) - 1);
        dData->character[sizeof(dData->character) - 1] = '\0';

        bb_setCharacterLine(dData, 0,  "Holy bug farts!");
        bb_setCharacterLine(dData, 1,  "After I marketed the");
        bb_setCharacterLine(dData, 2,  "chilidog car freshener at MAGFest,");
        bb_setCharacterLine(dData, 3,  "Garbotnik Industries' stock went up by 6,969%!");
        bb_setCharacterLine(dData, 4,  "I'm going to use my time machine to steal the");
        bb_setCharacterLine(dData, 5,  "next big-selling trinket from the future now.");
        bb_setCharacterLine(dData, 6,  "That will floor all my stakeholders and make me");
        bb_setCharacterLine(dData, 7,  "UNDEFINED money!");
        bb_setCharacterLine(dData, 8,  "With that kind of cash,");
        bb_setCharacterLine(dData, 9,  "I can recruit 200 professional bassoon players");
        bb_setCharacterLine(dData, 10, "to the MAGFest Community Orchestra.");
        bb_setCharacterLine(dData, 11, "I'm so hyped");
        bb_setCharacterLine(dData, 12, "to turn on my time machine for the first time!");
        bb_setCharacterLine(dData, 13, "Everything's in order.");
        bb_setCharacterLine(dData, 14, "Even Pango can't stop me!");
        bb_setCharacterLine(dData, 15, "I just have to attach the chaos core right here.");
        bb_setCharacterLine(dData, 16, "Where did I put that core?");
        bb_setCharacterLine(dData, 17, "hmmm...");
        bb_setCharacterLine(dData, 18, "What about in the freezer?");
        bb_setCharacterLine(dData, 19, "I've checked every inch of the death dumpster.");
        bb_setCharacterLine(dData, 20, "Glitch my circuits!");
        bb_setCharacterLine(dData, 21, "It must have gone out with the trash last Wednesday.");
        bb_setCharacterLine(dData, 22, "Can I get an F in the chat?");
        bb_setCharacterLine(dData, 23, "...");
        bb_setCharacterLine(dData, 24, "The chaos core is three times denser");
        bb_setCharacterLine(dData, 25, "than a black hole.");
        bb_setCharacterLine(dData, 26, "Well if  Waste Management took it to the landfill,");
        bb_setCharacterLine(dData, 27, "then it is definitely at the VERY BOTTOM of the dump.");
        bb_setCharacterLine(dData, 28, "Not a problem.");
        bb_setCharacterLine(dData, 29, "We have the technology to retrieve it.");
        bb_setCharacterLine(dData, 30, "Safety first.");
        bb_setCharacterLine(dData, 31, "I've activated my cloning machine up here in case I");
        bb_setCharacterLine(dData, 32, "should perish on that nuclear wasteland.");
        bb_setCharacterLine(dData, 33, "YOLO!");

        dData->curString = -1;

        dData->endDialogueCB = &bb_afterGarbotnikIntro;

        bb_setData(ovo, dData);
}

void bb_afterGarbotnikIntro(bb_entity_t* self)
{
    for(int i = 0; i < 3; i++)
    {
        if(self->gameData->entityManager.boosterEntities[i] != NULL)
        {
            self->gameData->entityManager.activeBooster = self->gameData->entityManager.boosterEntities[i];

            bb_goToData* tData = (bb_goToData*)self->gameData->entityManager.viewEntity->data;
            tData->tracking = self->gameData->entityManager.activeBooster;
            tData->midPointSqDist = sqMagVec2d(
                divVec2d(subVec2d(tData->tracking->pos, self->gameData->entityManager.viewEntity->pos), 2));

            tData->executeOnArrival = &bb_deployBooster;

            self->gameData->entityManager.viewEntity->updateFunction = &bb_updatePOI;

            return;
        }
    }
}

void bb_deployBooster(bb_entity_t* self) //separates from the death dumpster in orbit.
{
        self->gameData->entityManager.viewEntity = self->gameData->entityManager.activeBooster;

        bb_rocketData_t* rData = (bb_rocketData_t*) self->gameData->entityManager.activeBooster->data;
        bb_destroyEntity(rData->flame, false);
        rData->flame = NULL;

        self->gameData->entityManager.activeBooster->updateFunction = &bb_updateRocketLanding;
}

bb_dialogueData_t* bb_createDialogueData(int numStrings)
{
    bb_dialogueData_t* dData = HEAP_CAPS_CALLOC_DBG(1, sizeof(bb_dialogueData_t), MALLOC_CAP_SPIRAM);
    dData->numStrings = numStrings;
    dData->strings = HEAP_CAPS_CALLOC_DBG(numStrings, sizeof(char*), MALLOC_CAP_SPIRAM);
    return dData;
}

void bb_setCharacterLine(bb_dialogueData_t* dData, int index, const char* str)
{
    dData->strings[index] = HEAP_CAPS_CALLOC_DBG(1, strlen(str) + 1, MALLOC_CAP_SPIRAM);
    strcpy(dData->strings[index], str);
}

void bb_freeDialogueData(bb_dialogueData_t* dData)
{
    for (int i = 0; i < dData->numStrings; i++) {
        FREE_DBG(dData->strings[i]);  // Free each string
    }
    FREE_DBG(dData->strings);         // Free the array of string pointers
    FREE_DBG(dData);                  // Free the struct itself
}