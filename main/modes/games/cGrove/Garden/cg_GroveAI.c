/**
 * @file cg_GroveAI.c
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief Chowa AI in the garden
 * @version 0.1
 * @date 2024-10-13
 *
 * @copyright Copyright (c) 2024
 *
 */

//==============================================================================
// Includes
//==============================================================================

#include "cg_GroveAI.h"
#include <esp_random.h>
#include <math.h>

//==============================================================================
// Function Declarations
//==============================================================================

/**
 * @brief Gets a new target position to wander to
 * 
 * @param cg Game Data
 * @param c Pointer to CHowa to calc position for
 */
static void cg_GroveGetRandMovePoint(cGrove_t* cg, cgGroveChowa_t* c);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief 
 * 
 * @param cg Game data
 * @param chowa The chowa to run AI on
 * @param elapsedUs Time since last frame
 */
void cg_GroveAI(cGrove_t* cg, cgGroveChowa_t* chowa, int64_t elapsedUs)
{
    // Basic behaviors:
    // - Stand in place (includes other things like singing)
    // - Go to place/item
    // - Use item (Must be holding item)
    // - Join other Chowa at activity
    // - Struggle when held
    // - Chase cursor 

    // Update timer
    chowa->timer += elapsedUs;

    // The MONOLITH
    if (true)
    {
    }
}

//==============================================================================
// Static Functions
//==============================================================================

static void cg_GroveGetRandMovePoint(cGrove_t* cg, cgGroveChowa_t* c)
{
    // Get a random point inside the bounds of the play area
    // - Cannot be on stump/in tree/too close to edge
    rectangle_t targetPos = {.height = 32, .width = 32};
    vec_t colVec;

    // Check if inside an object
    do
    {
        targetPos.pos.x = 32 + esp_random() % (cg->grove.groveBG.w - 64);
        targetPos.pos.y = 32 + esp_random() % (cg->grove.groveBG.h - 64);
    } while (rectRectIntersection(targetPos, cg->grove.boundaries[CG_TREE], &colVec)
             || rectRectIntersection(targetPos, cg->grove.boundaries[CG_STUMP], &colVec));

    // Once a position is found, calculate angle and distance to point
    c->targetPos = targetPos.pos;
}