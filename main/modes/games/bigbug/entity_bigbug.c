//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include "entity_bigbug.h"
#include "entityManager_bigbug.h"
#include "gameData_bigbug.h"
#include "soundFuncs.h"
#include "hdw-btn.h"
#include "esp_random.h"
#include "aabb_utils_bigbug.h"
#include "trigonometry.h"
#include <esp_log.h>
#include <math.h>

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
    self->active               = false;
    self->gameData             = gameData;
    self->soundManager         = soundManager;
    self->entityManager        = entityManager;
}

void bb_destroyEntity(bb_entity_t* self, bool respawn)
{
    if(self->data != NULL){
        free(self->data);
        self->data = NULL;
    }
    self->entityManager->activeEntities--;
    self->active = false;
}

void bb_updateRocketLanding(bb_entity_t* self){
    bb_rocketData_t* rData = (bb_rocketData_t*)self->data;

    if(self->pos.y > -3640 && rData->flame == NULL){
        rData->flame = bb_createEntity(&(self->gameData->entityManager), LOOPING_ANIMATION, false, FLAME_ANIM, 2,
        self->pos.x>>DECIMAL_BITS,
        self->pos.y>>DECIMAL_BITS);
    }

    else if(rData->flame != NULL){
        rData->yVel -= 5;
        if(rData->yVel <= 0){
            bb_destroyEntity(rData->flame, false);
            free(self->data);
            self->data = calloc(1,sizeof(bb_heavyFallingData_t));
            self->updateFunction = bb_updateHeavyFallingInit;
            self->gameData->entityManager.viewEntity = NULL;
            return;
        }
    }
    self->pos.y += rData->yVel * self->gameData->elapsedUs / 100000;
}

void bb_updateHeavyFallingInit(bb_entity_t* self){
    bb_heavyFallingData_t* hfData = (bb_heavyFallingData_t*)self->data;
    hfData->yVel += 6;

    self->pos.y += hfData->yVel * self->gameData->elapsedUs / 100000;

    bb_hitInfo_t* hitInfo = bb_collisionCheck(&self->gameData->tilemap, self, NULL);
    if(hitInfo->hit == false){
        free(hitInfo);
        return;
    }
    
    self->pos.y = hitInfo->pos.y - self->halfHeight;
    if(hfData->yVel < 160){
        hfData->yVel = 0;
        self->updateFunction = bb_updateGarbotnikDeploy;
        self->paused = false;
    }
    else{
        hfData->yVel -= 160;
        // Update the dirt to air.
        self->gameData->tilemap.fgTiles[hitInfo->tile_i][hitInfo->tile_j] = 0;
        // Create a crumble animation
        bb_createEntity(&(self->gameData->entityManager), ONESHOT_ANIMATION, false, CRUMBLE_ANIM, 1,
                        hitInfo->tile_i * TILE_SIZE + TILE_SIZE,
                        hitInfo->tile_j * TILE_SIZE + TILE_SIZE);
    }
    free(hitInfo);
    return;
}

void bb_updateHeavyFalling(bb_entity_t* self){
    bb_heavyFallingData_t* hfData = (bb_heavyFallingData_t*)self->data;
    hfData->yVel += 6;
    self->pos.y += hfData->yVel * self->gameData->elapsedUs / 100000;

    printf("tilemap addr: %p\n", &self->gameData->tilemap);
    printf("self    addr: %p\n", self);
    bb_hitInfo_t* hitInfo = bb_collisionCheck(&self->gameData->tilemap, self, NULL);
    if(hitInfo->hit == false){
        free(hitInfo);
        return;
    }
    
    // self->pos.y -= hfData->yVel * self->gameData->elapsedUs / 100000;
    self->pos.y = hitInfo->pos.y - self->halfHeight;
    if(hfData->yVel < 160){
        hfData->yVel = 0;
    }
    else{
        hfData->yVel -= 160;
        // Update the dirt to air.
        self->gameData->tilemap.fgTiles[hitInfo->tile_i][hitInfo->tile_j] = 0;
        // Create a crumble animation
        bb_createEntity(&(self->gameData->entityManager), ONESHOT_ANIMATION, false, CRUMBLE_ANIM, 1,
                        hitInfo->tile_i * TILE_SIZE + HALF_TILE,
                        hitInfo->tile_j * TILE_SIZE + HALF_TILE);
    }
    free(hitInfo);
    return;
}

void bb_updateGarbotnikDeploy(bb_entity_t* self){
    if(self->currentAnimationFrame == self->entityManager->sprites[self->spriteIndex].numFrames - 1){
        self->paused = true;
        //deploy garbotnik!!!
        bb_entity_t* garbotnik = bb_createEntity(&(self->gameData->entityManager),
            NO_ANIMATION, true, GARBOTNIK_FLYING, 1,
            self->pos.x >> DECIMAL_BITS,
            (self->pos.y >> DECIMAL_BITS) - 50);
        self->gameData->entityManager.viewEntity = garbotnik;
        self->updateFunction = bb_updateHeavyFalling;
    }
}

void bb_updateFlame(bb_entity_t* self){
    self->pos.y = self->gameData->entityManager.entities[0].pos.y;//It's nasty, but that is the rocket.
}

void bb_updateGarbotnikFlying(bb_entity_t* self){
    bb_garbotnikData* gData = (bb_garbotnikData*)self->data;

    // record the previous frame's position before any logic.
    gData->previousPos = self->pos;

    vec_t accel = {.x = 0,
                   .y = 0};

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

    //physics
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

    bb_hitInfo_t* hitInfo = bb_collisionCheck(&self->gameData->tilemap, self, &gData->previousPos);

    if(hitInfo->hit == false){
        free(hitInfo);
        return;
    }

    self->pos.x = hitInfo->pos.x + hitInfo->normal.x * self->halfWidth;
    self->pos.y = hitInfo->pos.y + hitInfo->normal.y * self->halfHeight;

    // printf("dot product: %d\n",dotVec2d(bigbug->garbotnikVel, normal));
    if (dotVec2d(gData->vel, hitInfo->normal)< -95)
    {   // velocity angle is opposing garbage normal vector. Tweak number for different threshold.
        ///////////////////////
        // digging detected! //
        ///////////////////////

        // crumble test
        //  uint32_t* val = calloc(2,sizeof(uint32_t));
        //  val[0] = 5;
        //  val[1] = 3;
        //  push(self->gameData->unsupported, (void*)val);

        // Update the dirt by decrementing it.
        self->gameData->tilemap.fgTiles[hitInfo->tile_i][hitInfo->tile_j] -= 1;

        if(self->gameData->tilemap.fgTiles[hitInfo->tile_i][hitInfo->tile_j] == 0 ||
            self->gameData->tilemap.fgTiles[hitInfo->tile_i][hitInfo->tile_j] == 1 ||
            self->gameData->tilemap.fgTiles[hitInfo->tile_i][hitInfo->tile_j] == 4){
            // Create a crumble animation
            bb_createEntity(&(self->gameData->entityManager), ONESHOT_ANIMATION, false, CRUMBLE_ANIM, 1,
                            hitInfo->tile_i * TILE_SIZE + HALF_TILE,
                            hitInfo->tile_j * TILE_SIZE + HALF_TILE);
        }
        else{
            // Create a bump animation
            bb_createEntity(&(self->gameData->entityManager), ONESHOT_ANIMATION, false, BUMP_ANIM, 1,
                            hitInfo->pos.x >> DECIMAL_BITS,
                            hitInfo->pos.y >> DECIMAL_BITS);
        }

        ////////////////////////////////
        // Mirror garbotnik's velocity//
        ////////////////////////////////
        // Reflect the velocity vector along the normal
        // See http://www.sunshine2k.de/articles/coding/vectorreflection/vectorreflection.html
        printf("hit squared speed: %" PRId32 "\n", sqMagVec2d(gData->vel));
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
            subVec2d(gData->vel, mulVec2d(hitInfo->normal, (2 * dotVec2d(gData->vel, hitInfo->normal)))),
            bounceScalar);

        
        /////////////////////////////////
        // check neighbors for stability//
        /////////////////////////////////
        // for(uint8_t neighborIdx = 0; neighborIdx < 4; neighborIdx++)
        // {
        //     uint32_t check_x = hitInfo->tile_i + self->gameData->neighbors[neighborIdx][0];
        //     uint32_t check_y = hitInfo->tile_j + self->gameData->neighbors[neighborIdx][1];
        //     //Check if neighbor is in bounds of map (also not on left, right, or bottom, perimiter) and if it
        //     is dirt. if(check_x > 0 && check_x < TILE_FIELD_WIDTH - 1 && check_y > 0 && check_y <
        //     TILE_FIELD_HEIGHT - 1 && bigbug->tilemap.fgTiles[check_x][check_y] > 0)
        //     {
        //         uint32_t* val = calloc(4, sizeof(uint32_t));
        //         val[0] = check_x;
        //         val[1] = check_y;
        //         val[2] = 1; //1 is for foreground. 0 is midground.
        //         val[3] = 0; //f value used in pathfinding.
        //         push(self->gameData->pleaseCheck, (void*)val);
        //     }
        // }
        free(hitInfo);
    }
}

void bb_drawGarbotnikFlying(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self){
    bb_garbotnikData* gData = (bb_garbotnikData*)self->data;

    // Draw garbotnik
    if (gData->yaw.x < -1400)
    {
        drawWsgSimple(&entityManager->sprites[self->spriteIndex].frames[0],
            (self->pos.x >> DECIMAL_BITS)
                    - entityManager->sprites[self->spriteIndex].originX - camera->pos.x,
            (self->pos.y >> DECIMAL_BITS)
                    - entityManager->sprites[self->spriteIndex].originY - camera->pos.y);
    }
    else if (gData->yaw.x < -400)
    {
        drawWsgSimple(&entityManager->sprites[self->spriteIndex].frames[1],
            (self->pos.x >> DECIMAL_BITS)
                    - entityManager->sprites[self->spriteIndex].originX - camera->pos.x,
            (self->pos.y >> DECIMAL_BITS)
                    - entityManager->sprites[self->spriteIndex].originY - camera->pos.y);
    }
    else if (gData->yaw.x < 400)
    {
        drawWsgSimple(&entityManager->sprites[self->spriteIndex].frames[2],
            (self->pos.x >> DECIMAL_BITS)
                    - entityManager->sprites[self->spriteIndex].originX - camera->pos.x,
            (self->pos.y >> DECIMAL_BITS)
                    - entityManager->sprites[self->spriteIndex].originY - camera->pos.y);
    }
    else if (gData->yaw.x < 1400)
    {
        drawWsg(&entityManager->sprites[self->spriteIndex].frames[1],
            (self->pos.x >> DECIMAL_BITS)
                    - entityManager->sprites[self->spriteIndex].originX - camera->pos.x,
            (self->pos.y >> DECIMAL_BITS)
                    - entityManager->sprites[self->spriteIndex].originY - camera->pos.y,
            true, false, 0);
    }
    else
    {
        drawWsg(&entityManager->sprites[self->spriteIndex].frames[0],
            (self->pos.x >> DECIMAL_BITS)
                    - entityManager->sprites[self->spriteIndex].originX - camera->pos.x,
            (self->pos.y >> DECIMAL_BITS)
                    - entityManager->sprites[self->spriteIndex].originY - camera->pos.y,
            true, false, 0);
    }
}

