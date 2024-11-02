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
#include "trigonometry.h"
#include <esp_random.h>
#include <math.h>

//==============================================================================
// Defines
//==============================================================================

#define SECOND 1000000

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
void cg_GroveAI(cGrove_t* cg, cgGroveChowa_t* c, int64_t elapsedUs)
{
    // Basic behaviors:
    // - Stand in place (includes other things like singing)
    // - Go to place/item
    // - Use item (Must be holding item)
    // - Join other Chowa at activity
    // - Struggle when held
    // - Chase cursor

    // Abort if not active
    if (!c->chowa->active)
    {
        return;
    }

    // Update timer
    c->timeLeft -= elapsedUs;
    c->statUpdate += elapsedUs;

    // The MONOLITH
    switch (c->gState)
    {
        case CHOWA_IDLE:
        {
            // Chowa is essentially unset. Look for new behavior
            // Check other behaviors list
            // Check mood
            // Check stats

            // Wander
            // TODO: Move to decision function
            cg_GroveGetRandMovePoint(cg, c);
            c->gState    = CHOWA_WALK;
            c->nextState = CHOWA_IDLE;
            break;
        }
        case CHOWA_STATIC:
        {
            // Chowa is standing around doing nothing
            if (c->timeLeft <= 0)
            {
                c->gState = CHOWA_IDLE;
            }
            break;
        }
        case CHOWA_WALK:
        {
            // Calculate the distance to target
            vec_t difference = {.x = c->targetPos.x - c->aabb.pos.x, .y = c->targetPos.y - c->aabb.pos.y};

            float dist = sqrt(pow(difference.x, 2) + pow(difference.y, 2));
            if (dist < 10.0f)
            {
                c->gState = c->nextState;
            }
            int16_t angle = getAtan2(difference.y, difference.x);
            float xFrac = 2 * getCos1024(angle)/1024.0f;
            float yFrac = 2 * getSin1024(angle)/1024.0f;
            vec_t moveVec = {.x = (int)xFrac, .y = (int)yFrac};
            c->moveLine = moveVec;
            c->aabb.pos.x += xFrac;
            c->aabb.pos.y += yFrac;
            break;
        }
        case CHOWA_CHASE:
        {
            // Chowa updates target location each frame
            break;
        }
        case CHOWA_USE_ITEM:
        {
            // Wait until item is done being used
            break;
        }
        case CHOWA_BOX:
        {
            // Cycle between attack animations
            if (c->timeLeft <= 0)
            {
                c->gState = CHOWA_IDLE;
            }
            else
            {
                if (c->statUpdate >= SECOND)
                {
                    c->statUpdate = 0;
                    c->chowa->stats[CG_STAMINA] += 1;
                }
            }
            break;
        }
        case CHOWA_SING:
        {
            // stand in place and sing
            if (c->timeLeft)
            {
                c->gState = CHOWA_IDLE;
            }
            else
            {
                if (c->statUpdate >= SECOND)
                {
                    c->statUpdate = 0;
                    c->chowa->stats[CG_CHARISMA] += 1;
                }
            }
            break;
        }
        case CHOWA_TALK:
        {
            // talk to another chowa
            if (c->timeLeft <= 0)
            {
                c->gState = CHOWA_IDLE;
            }
            else
            {
                if (c->statUpdate >= SECOND)
                {
                    c->statUpdate = 0;
                    c->chowa->stats[CG_CHARISMA] += 1;
                }
            }
            break;
        }
        case CHOWA_HELD:
        {
            // Picked up by player
            // If held for too long, start losing affinity
            if (c->statUpdate >= 5 * SECOND)
            {
                c->statUpdate = 0;
                c->chowa->playerAffinity -= 1;
            }
            break;
        }
        default:
        {
            break;
        }
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