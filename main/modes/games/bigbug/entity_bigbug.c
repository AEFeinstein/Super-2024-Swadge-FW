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
#define TILE_FIELD_WIDTH  32  // matches the level wsg graphic width
#define TILE_FIELD_HEIGHT 192 // matches the level wsg graphic height

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
    }
    self->entityManager->activeEntities--;
    self->active = false;
}

void bb_updateGarbotnikFlying(bb_entity_t* self){
    // record the previous frame's position before any logic.
    ((bb_garbotnikData*)self->data)->previousPos = self->pos;

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

    ((bb_garbotnikData*)self->data)->accel = divVec2d(mulVec2d(accel, self->gameData->elapsedUs), 100000);

    //physics
    ((bb_garbotnikData*)self->data)->yaw.y += ((bb_garbotnikData*)self->data)->accel.x;
    if (((bb_garbotnikData*)self->data)->yaw.x < 0)
    {
        ((bb_garbotnikData*)self->data)->yaw.y -= 5.0 * self->gameData->elapsedUs / 100000;
    }
    else
    {
        ((bb_garbotnikData*)self->data)->yaw.y += 5.0 * self->gameData->elapsedUs / 100000;
    }
    ((bb_garbotnikData*)self->data)->yaw.x += ((bb_garbotnikData*)self->data)->yaw.y;
    if (((bb_garbotnikData*)self->data)->yaw.x < -1440)
    {
        ((bb_garbotnikData*)self->data)->yaw.x = -1440;
        ((bb_garbotnikData*)self->data)->yaw.y = 0;
    }
    else if (((bb_garbotnikData*)self->data)->yaw.x > 1440)
    {
        ((bb_garbotnikData*)self->data)->yaw.x = 1440;
        ((bb_garbotnikData*)self->data)->yaw.y = 0;
    }

    // Apply garbotnik's drag
    int32_t sqMagVel = sqMagVec2d(((bb_garbotnikData*)self->data)->vel);
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
        ((bb_garbotnikData*)self->data)->accel.x += (((bb_garbotnikData*)self->data)->vel.x / (double)speed) * -drag * self->gameData->elapsedUs / 100000;
        ((bb_garbotnikData*)self->data)->accel.y += (((bb_garbotnikData*)self->data)->vel.y / (double)speed) * -drag * self->gameData->elapsedUs / 100000;
        // bigbug->garbotnikAccel = addVec2d(bigbug->garbotnikAccel, mulVec2d(divVec2d(bigbug->garbotnikVel, speed),
        // -drag * elapsedUs / 100000));
    }

    // Update garbotnik's velocity
    ((bb_garbotnikData*)self->data)->vel.x += ((bb_garbotnikData*)self->data)->accel.x;
    ((bb_garbotnikData*)self->data)->vel.y += ((bb_garbotnikData*)self->data)->accel.y;

    // Update garbotnik's position
    self->pos.x += ((bb_garbotnikData*)self->data)->vel.x * self->gameData->elapsedUs / 100000;
    self->pos.y += ((bb_garbotnikData*)self->data)->vel.y * self->gameData->elapsedUs / 100000;

    // Look up 4 nearest tiles for collision checks
    // a tile's width is 16 pixels << 4 = 512. half width is 256.
    int32_t xIdx = (self->pos.x - BITSHIFT_HALF_TILE) / BITSHIFT_TILE_SIZE
                   - (self->pos.x < 0); // the x index
    int32_t yIdx = (self->pos.y - BITSHIFT_HALF_TILE) / BITSHIFT_TILE_SIZE
                   - (self->pos.y < 0); // the y index

    int32_t best_i        = -1; // negative means no worthy candidates found.
    int32_t best_j        = -1;
    int32_t closestSqDist = 1063842; //(307.35+724.077)^2 if it's further than this, there's no way it's a collision.
    for (int32_t i = xIdx; i <= xIdx + 1; i++)
    {
        for (int32_t j = yIdx; j <= yIdx + 1; j++)
        {
            if (i >= 0 && i < TILE_FIELD_WIDTH && j >= 0 && j < TILE_FIELD_HEIGHT)
            {
                if (self->gameData->tilemap.fgTiles[i][j] >= 1)
                {
                    // Initial circle check for preselecting the closest dirt tile
                    int32_t sqDist = sqMagVec2d(
                        subVec2d(self->pos, (vec_t){i * BITSHIFT_TILE_SIZE + BITSHIFT_HALF_TILE,
                                                    j * BITSHIFT_TILE_SIZE + BITSHIFT_HALF_TILE}));
                    if (sqDist < closestSqDist)
                    {
                        // Good candidate found!
                        best_i        = i;
                        best_j        = j;
                        closestSqDist = sqDist;
                    }
                }
            }
        }
    }
    if (best_i > -1)
    {
        vec_t tilePos = {best_i * BITSHIFT_TILE_SIZE + BITSHIFT_HALF_TILE,
                         best_j * BITSHIFT_TILE_SIZE + BITSHIFT_HALF_TILE};
        // AABB-AABB collision detection begins here
        // https://tutorialedge.net/gamedev/aabb-collision-detection-tutorial/
        if (self->pos.x + 192 > tilePos.x - BITSHIFT_HALF_TILE
            && self->pos.x - 192 < tilePos.x + BITSHIFT_HALF_TILE
            && self->pos.y + 192 > tilePos.y - BITSHIFT_HALF_TILE
            && self->pos.y - 192 < tilePos.y + BITSHIFT_HALF_TILE)
        {
            /////////////////////////
            // Collision detected! //
            /////////////////////////
            // Resolve garbotnik's position somewhat based on his position previously.
            vec_t normal = subVec2d(((bb_garbotnikData*)self->data)->previousPos, tilePos);
            // Snap the previous frame offset to an orthogonal direction.
            if ((normal.x < 0 ? -normal.x : normal.x) > (normal.y < 0 ? -normal.y : normal.y))
            {
                if (normal.x > 0)
                {
                    normal.x               = 1;
                    normal.y               = 0;
                    self->pos.x = tilePos.x + 192 + BITSHIFT_HALF_TILE;
                }
                else
                {
                    normal.x               = -1;
                    normal.y               = 0;
                    self->pos.x = tilePos.x - 192 - BITSHIFT_HALF_TILE;
                }
            }
            else
            {
                if (normal.y > 0)
                {
                    normal.x               = 0;
                    normal.y               = 1;
                    self->pos.y = tilePos.y + 192 + BITSHIFT_HALF_TILE;
                }
                else
                {
                    normal.x               = 0;
                    normal.y               = -1;
                    self->pos.y = tilePos.y - 192 - BITSHIFT_HALF_TILE;
                }
            }

            // printf("dot product: %d\n",dotVec2d(bigbug->garbotnikVel, normal));
            if (dotVec2d(((bb_garbotnikData*)self->data)->vel, normal)
                < -95) // velocity angle is opposing garbage normal vector. Tweak number for different threshold.
            {
                ///////////////////////
                // digging detected! //
                ///////////////////////

                // crumble test
                //  uint32_t* val = calloc(2,sizeof(uint32_t));
                //  val[0] = 5;
                //  val[1] = 3;
                //  push(self->gameData->unsupported, (void*)val);

                // Update the dirt by decrementing it.
                self->gameData->tilemap.fgTiles[best_i][best_j] -= 1;

                if(self->gameData->tilemap.fgTiles[best_i][best_j] == 0 ||
                    self->gameData->tilemap.fgTiles[best_i][best_j] == 1 ||
                    self->gameData->tilemap.fgTiles[best_i][best_j] == 4){
                    // Create a crumble animation
                    bb_createEntity(&(self->gameData->entityManager), ONESHOT_ANIMATION, false, CRUMBLE_ANIM, 1,
                                    tilePos.x >> DECIMAL_BITS,
                                    tilePos.y >> DECIMAL_BITS);
                }
                else{
                    // Create a bump animation
                    bb_createEntity(&(self->gameData->entityManager), ONESHOT_ANIMATION, false, BUMP_ANIM, 1,
                                    ((self->pos.x + tilePos.x)/2) >> DECIMAL_BITS,
                                    ((self->pos.y + tilePos.y)/2) >> DECIMAL_BITS);
                }


                

                ////////////////////////////////
                // Mirror garbotnik's velocity//
                ////////////////////////////////
                // Reflect the velocity vector along the normal
                // See http://www.sunshine2k.de/articles/coding/vectorreflection/vectorreflection.html
                printf("hit squared speed: %" PRId32 "\n", sqMagVec2d(((bb_garbotnikData*)self->data)->vel));
                int32_t bounceScalar = sqMagVec2d(((bb_garbotnikData*)self->data)->vel) / -11075 + 3;
                if (bounceScalar > 3)
                {
                    bounceScalar = 3;
                }
                else if (bounceScalar < 1)
                {
                    bounceScalar = 1;
                }
                ((bb_garbotnikData*)self->data)->vel = mulVec2d(
                    subVec2d(((bb_garbotnikData*)self->data)->vel, mulVec2d(normal, (2 * dotVec2d(((bb_garbotnikData*)self->data)->vel, normal)))),
                    bounceScalar);

                /////////////////////////////////
                // check neighbors for stability//
                /////////////////////////////////
                // for(uint8_t neighborIdx = 0; neighborIdx < 4; neighborIdx++)
                // {
                //     uint32_t check_x = best_i + self->gameData->neighbors[neighborIdx][0];
                //     uint32_t check_y = best_j + self->gameData->neighbors[neighborIdx][1];
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
            }
        }
    }

}