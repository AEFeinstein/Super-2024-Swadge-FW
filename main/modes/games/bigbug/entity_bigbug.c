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
    self->entityManager = entityManager;
}

void bb_setData(bb_entity_t* self, void* data)
{
    if (self->data != NULL)
    {
        free(self->data);
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
            free(shiftedCollision->checkOthers);
            free(shiftedCollision);
        }
        free(self->collisions);
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
        free(self->data);
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

    self->entityManager->activeEntities--;
    // printf("%d/%d entities v\n", self->entityManager->activeEntities, MAX_ENTITIES);
}

void bb_updateRocketLanding(bb_entity_t* self)
{
    bb_rocketData_t* rData = (bb_rocketData_t*)self->data;

    if (self->pos.y > -3640 && rData->flame == NULL)
    {
        rData->flame = bb_createEntity(&(self->gameData->entityManager), LOOPING_ANIMATION, false, FLAME_ANIM, 2,
                                       self->pos.x >> DECIMAL_BITS, self->pos.y >> DECIMAL_BITS, false);
    }

    else if (rData->flame != NULL)
    {
        rData->yVel -= 5;
        rData->flame->pos.y = self->pos.y + rData->yVel * self->gameData->elapsedUs / 100000;
        if (rData->yVel <= 0)
        {
            bb_destroyEntity(rData->flame, false);
            bb_setData(self, heap_caps_calloc(1, sizeof(bb_heavyFallingData_t), MALLOC_CAP_SPIRAM));
            self->updateFunction                     = bb_updateHeavyFallingInit;
            self->gameData->entityManager.viewEntity = NULL;

            return;
        }
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
    if (self->currentAnimationFrame == self->entityManager->sprites[self->spriteIndex].numFrames - 1)
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
        //  uint32_t* val = heap_caps_calloc(2,sizeof(uint32_t), MALLOC_CAP_SPIRAM);
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
        //         uint32_t* val = heap_caps_calloc(4, sizeof(uint32_t), MALLOC_CAP_SPIRAM);
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
        bb_stuckHarpoonData_t* shData = heap_caps_calloc(1, sizeof(bb_stuckHarpoonData_t), MALLOC_CAP_SPIRAM);
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

void bb_updateMenuBug(bb_entity_t* self)
{
    bb_menuBugData_t* mbData = (bb_menuBugData_t*)self->data;
    self->pos.x += (mbData->xVel - 3) << DECIMAL_BITS;
    if (mbData->firstTrip && self->pos.x < self->entityManager->viewEntity->pos.x - (130 << DECIMAL_BITS))
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
            bb_goToData* tData = heap_caps_calloc(1, sizeof(bb_goToData), MALLOC_CAP_SPIRAM);
            tData->tracking = bb_createEntity(&self->gameData->entityManager, NO_ANIMATION, true, BB_DEATH_DUMPSTER, 1,
                                              self->pos.x >> DECIMAL_BITS, (self->pos.y >> DECIMAL_BITS) + 700, true);
            tData->midPointSqDist = sqMagVec2d(
                divVec2d(subVec2d(tData->tracking->pos, self->gameData->entityManager.viewEntity->pos), 2));

            self->gameData->entityManager.viewEntity->data = (void*)tData;
            self->gameData->entityManager.viewEntity->updateFunction = &bb_updatePOI;

            for(int rocketIdx = 0; rocketIdx < 3; rocketIdx++){
                bb_entity_t* rocket = bb_createEntity(&self->gameData->entityManager, NO_ANIMATION, true, ROCKET_ANIM, 3,
                    (self->pos.x >> DECIMAL_BITS) - 80 + 80 * rocketIdx, (self->pos.y >> DECIMAL_BITS) + 675, true);
            
                rocket->updateFunction = NULL;

                bb_rocketData_t* rData = (bb_rocketData_t*)rocket->data;

                rData->flame = bb_createEntity(&(self->gameData->entityManager), LOOPING_ANIMATION, false, FLAME_ANIM, 2,
                    rocket->pos.x >> DECIMAL_BITS, rocket->pos.y >> DECIMAL_BITS, true);

                rData->flame->updateFunction = &bb_updateFlame;
            }

            self->updateFunction = NULL;
            return;
        }
    }


    mData->cursor->pos.y = self->pos.y + (143<<DECIMAL_BITS) + mData->selectionIdx * (24<<DECIMAL_BITS);

    if (self->gameData->menuBug == NULL || self->gameData->menuBug->active == false)
    {
        self->gameData->menuBug
            = bb_createEntity(&self->gameData->entityManager, LOOPING_ANIMATION, false, bb_randomInt(8, 13), 1,
                              (self->pos.x >> DECIMAL_BITS) + 135, (self->pos.y >> DECIMAL_BITS) + 182, true);
        self->gameData->menuBug->drawFunction      = &bb_drawMenuBug;
        self->gameData->menuBug->updateFunction    = &bb_updateMenuBug;
        self->gameData->menuBug->updateFarFunction = &bb_updateFarDestroy;
        bb_menuBugData_t* mbData                   = heap_caps_calloc(1, sizeof(bb_menuBugData_t), MALLOC_CAP_SPIRAM);
        mbData->xVel                               = bb_randomInt(-5, 5);
        mbData->xVel
            = mbData->xVel == 3 ? mbData->xVel - 1 : mbData->xVel; // So as not to match the treadmill speed exactly.
        mbData->firstTrip                                    = true;
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
                              (self->pos.x >> DECIMAL_BITS) + 140, (self->pos.y >> DECIMAL_BITS) + 190 + bb_randomInt(-10, 10), false);
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
            free(tData);
            self->data           = NULL;
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
    // drawLineFast(xOff, yOff + 1, xOff - floatVel.x * 20, yOff - floatVel.y * 20 + 1, c000);

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
        bb_physicsData_t* physData  = heap_caps_calloc(1, sizeof(bb_physicsData_t), MALLOC_CAP_SPIRAM);
        physData->vel               = divVec2d(pData->vel, 2);
        physData->bounceNumerator   = 2; // 66% bounce
        physData->bounceDenominator = 3;
        bb_setData(other, physData);
        other->updateFunction = bb_updatePhysicsObject;
    }
    vecFl_t floatVel              = {(float)pData->vel.x, (float)pData->vel.y};
    bb_stuckHarpoonData_t* shData = heap_caps_calloc(1, sizeof(bb_stuckHarpoonData_t), MALLOC_CAP_SPIRAM);
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
