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
#include "cg_GroveItems.h"
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

/**
 * @brief Sets a new task for the Chowa
 *
 * @param cg Game Data
 * @param c Chowa Data
 */
static cgChowaStateGarden_t cg_getNewTask(cGrove_t* cg, cgGroveChowa_t* c);

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
    // - Go to place/item
    // - Use item (Must be holding item)

    // Abort if not active
    if (!c->chowa->active)
    {
        return;
    }

    // Update timer
    c->timeLeft -= elapsedUs;

    // Update Age
    if (c->chowa->age <= 70)
    {
        c->ageTimer += elapsedUs;
        if (c->ageTimer >= SECOND * 60)
        {
            c->chowa->age += 1;
        }
    }

    // Update mood
    c->moodTimer += elapsedUs;
    if (c->moodTimer >= 60 * SECOND)
    {
        c->moodTimer = 0;
        switch (esp_random() % 4)
        {
            case 0:
                c->chowa->mood = CG_HAPPY;
                break;
            case 1:
                c->chowa->mood = CG_SAD;
                break;
            case 2:
                c->chowa->mood = CG_ANGRY;
                break;
            case 3:
                c->chowa->mood = CG_CONFUSED;
                break;
        }
    }

    // The MONOLITH
    switch (c->gState)
    {
        case CHOWA_IDLE:
        {
            // Chowa is essentially unset. Look for new behavior
            c->nextState = CHOWA_IDLE;
            c->gState    = cg_getNewTask(cg, c);
            c->animFrame = 0; // Reset animation frame to avoid displaying garbage data
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
            if (c->targetPos.x == 0 || c->targetPos.y == 0)
            {
                cg_GroveGetRandMovePoint(cg, c);
                return;
            }
            vec_t difference = {.x = c->targetPos.x - c->aabb.pos.x, .y = c->targetPos.y - c->aabb.pos.y};
            if (sqMagVec2d(difference) < pow(c->precision, 2))
            {
                c->timeLeft = c->nextTimeLeft;
                c->gState   = c->nextState;
                if (c->chowa->stats[CG_STAMINA] >= 255)
                {
                    c->chowa->stats[CG_STAMINA] += 1;
                }
                if (c->chowa->stats[CG_SPEED] >= 255)
                {
                    c->chowa->stats[CG_SPEED] += 1;
                }
            }
            fastNormVec(&difference.x, &difference.y);
            int16_t angle = getAtan2(difference.y, difference.x);
            c->angle      = angle;
            c->aabb.pos.x += difference.x / 128;
            c->aabb.pos.y += difference.y / 128;
            break;
        }
        case CHOWA_CHASE:
        {
            // Calculate the distance to target
            c->targetPos     = addVec2d(cg->grove.cursor.pos, cg->grove.camera.pos);
            vec_t difference = {.x = c->targetPos.x - c->aabb.pos.x, .y = c->targetPos.y - c->aabb.pos.y};
            if (sqMagVec2d(difference) < pow(c->precision, 2))
            {
                c->timeLeft = c->nextTimeLeft;
                c->gState   = c->nextState;
                // Update stamina stat
                if (c->chowa->stats[CG_STAMINA] < 255)
                {
                    c->chowa->stats[CG_STAMINA] += 1;
                }
            }
            fastNormVec(&difference.x, &difference.y);
            int16_t angle = getAtan2(difference.y, difference.x);
            c->angle      = angle;
            c->aabb.pos.x += difference.x / 128;
            c->aabb.pos.y += difference.y / 128;
            break;
        }
        case CHOWA_GRAB_ITEM:
        {
            // Grab an item
            if (c->heldItem->active)
            {
                c->heldItem->active = false; // disables this item for others
            }
            else
            {
                c->heldItem = NULL;
            }
            c->gState = CHOWA_IDLE;
            break;
        }
        case CHOWA_DROP_ITEM:
        {
            c->heldItem->active   = true;
            c->heldItem->aabb.pos = c->aabb.pos;
            c->heldItem           = NULL;
            c->nextState          = CHOWA_IDLE;
            c->gState             = CHOWA_IDLE;
            break;
        }
        case CHOWA_USE_ITEM:
        {
            // Wait until item is done being used
            if (c->timeLeft <= 0)
            {
                // Check if ball, and spawn if so
                if (strcmp(c->heldItem->name, shopMenuItems[5]) == 0)
                {
                    c->heldItem->active    = true;
                    c->heldItem->aabb.pos  = c->aabb.pos;
                    c->ballInAir           = true;
                    c->ballFlip            = c->flip;
                    c->ballAnimFrame       = 0;
                    c->ySpd                = -64;
                    c->heldItem->numOfUses = 2; // Never delete the ball
                }
                // Update stats
                if (strcmp(c->heldItem->name, shopMenuItems[0]) == 0)
                {
                    if (c->chowa->stats[CG_AGILITY] < 255)
                    {
                        c->chowa->stats[CG_AGILITY] += 1;
                    }
                }
                else if (strcmp(c->heldItem->name, shopMenuItems[1]) == 0)
                {
                    if (c->chowa->stats[CG_CHARISMA] < 255)
                    {
                        c->chowa->stats[CG_CHARISMA] += 1;
                    }
                }
                else if (strcmp(c->heldItem->name, shopMenuItems[2]) == 0)
                {
                    if (c->chowa->stats[CG_STRENGTH] < 255)
                    {
                        c->chowa->stats[CG_STRENGTH] += 1;
                    }
                }
                else if (strcmp(c->heldItem->name, shopMenuItems[3]) == 0)
                {
                    if (c->chowa->stats[CG_STAMINA] < 255)
                    {
                        c->chowa->stats[CG_STAMINA] += 1;
                    }
                }
                if (strcmp(c->heldItem->name, shopMenuItems[4]) == 0)
                {
                    if (c->chowa->stats[CG_SPEED] < 255)
                    {
                        c->chowa->stats[CG_SPEED] += 1;
                    }
                }
                else if (strcmp(c->heldItem->name, shopMenuItems[9]) == 0)
                {
                    if (c->chowa->stats[CG_HEALTH] < 255)
                    {
                        c->chowa->stats[CG_HEALTH] += 1;
                    }
                }
                else if (strcmp(c->heldItem->name, shopMenuItems[10]) == 0)
                {
                    if (c->chowa->stats[CG_HEALTH] < 252)
                    {
                        c->chowa->stats[CG_HEALTH] += 3;
                    }
                }
                // Remove item once used
                if (c->heldItem->numOfUses <= 0)
                {
                    strcpy(c->heldItem->name, "");
                    c->heldItem = NULL;
                }
                else
                {
                    c->heldItem->numOfUses -= 1;
                }
                c->gState = CHOWA_IDLE;
            }
            break;
        }
        case CHOWA_BOX:
        {
            // Cycle between attack animations
            if (c->timeLeft <= 0)
            {
                c->gState = CHOWA_IDLE;
                if (c->chowa->stats[CG_STRENGTH] < 255)
                {
                    c->chowa->stats[CG_STRENGTH] += 1;
                }
                if (c->chowa->stats[CG_AGILITY] < 255)
                {
                    c->chowa->stats[CG_AGILITY] += 1;
                }
            }
            break;
        }
        case CHOWA_SING:
        {
            // stand in place and sing
            if (c->timeLeft <= 0)
            {
                c->gState = CHOWA_IDLE;
                if (c->chowa->stats[CG_CHARISMA] < 255)
                {
                    c->chowa->stats[CG_CHARISMA] += 1;
                }
            }
            break;
        }
        case CHOWA_DANCE:
        {
            // stand in place and sing
            if (c->timeLeft <= 0)
            {
                c->gState = CHOWA_IDLE;
                if (c->chowa->stats[CG_CHARISMA] < 255)
                {
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
                if (c->chowa->stats[CG_CHARISMA] < 255)
                {
                    c->chowa->stats[CG_CHARISMA] += 1;
                }
            }
            break;
        }
        case CHOWA_HELD:
        {
            // Picked up by player
            if (c->timeLeft <= 0)
            {
                if (cg->grove.heldChowa == c)
                {
                    if (c->chowa->playerAffinity > 0)
                    {
                        c->chowa->playerAffinity -= 1;
                    }
                    c->timeLeft = 2 * SECOND;
                }
                else
                {
                    c->gState = CHOWA_IDLE;
                }
            }
            break;
        }
        case CHOWA_GIFT:
        {
            // stand in place with arms raised
            if (c->timeLeft <= 0)
            {
                c->gState = CHOWA_IDLE;
            }
            break;
        }
        case CHOWA_PET:
        {
            // Chowa is being pet
            if (c->timeLeft <= 0)
            {
                c->gState = CHOWA_IDLE;
                // Update affinity;
                if (c->chowa->playerAffinity < 255)
                {
                    c->chowa->playerAffinity += 1;
                }
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
        targetPos.pos.x = 32 + (esp_random() % (cg->grove.groveBG.w - 64));
        targetPos.pos.y = 32 + (esp_random() % (cg->grove.groveBG.h - 64));
    } while (rectRectIntersection(targetPos, cg->grove.boundaries[CG_TREE], &colVec)
             || rectRectIntersection(targetPos, cg->grove.boundaries[CG_STUMP], &colVec));

    // Once a position is found, calculate angle and distance to point
    c->targetPos = targetPos.pos;
}

static cgChowaStateGarden_t cg_getNewTask(cGrove_t* cg, cgGroveChowa_t* c)
{
    // If in water, continue moving until outside the water
    vec_t temp;
    if (rectRectIntersection(c->aabb, cg->grove.boundaries[CG_WATER], &temp))
    {
        cg_GroveGetRandMovePoint(cg, c);
        c->precision = 10.0f;
        c->frameTimer += esp_random() % SECOND; // Offset frame timer
        c->nextState = CHOWA_IDLE;
        return CHOWA_WALK;
    }
    // Check other behaviors
    for (int32_t idx = 0; idx < CG_MAX_CHOWA + CG_GROVE_MAX_GUEST_CHOWA; idx++)
    {
        if ((cg->grove.chowa[idx].gState == CHOWA_SING || cg->grove.chowa[idx].gState == CHOWA_DANCE)
            && (esp_random() % 6 == 0) && !(c->chowa->mood == CG_ANGRY || c->chowa->mood == CG_SAD))
        {
            // Get the other Chowa's position
            c->targetPos = cg->grove.chowa[idx].aabb.pos;
            c->precision = 32.0f;
            if (esp_random() % 2 == 0)
            {
                c->nextState = CHOWA_SING;
            }
            else
            {
                c->nextState = CHOWA_DANCE;
            }
            cg->grove.chowa[idx].timeLeft += SECOND;
            c->nextTimeLeft = cg->grove.chowa[idx].timeLeft;
            return CHOWA_WALK;
        }
        else if (cg->grove.chowa[idx].gState == CHOWA_BOX && esp_random() % 6 == 0 && !cg->grove.chowa[idx].hasPartner)
        {
            cg->grove.chowa[idx].hasPartner = true;
            c->targetPos.x                  = cg->grove.chowa[idx].aabb.pos.x + 24;
            c->targetPos.y                  = cg->grove.chowa[idx].aabb.pos.y;
            c->precision                    = 10.0f;
            c->nextState                    = CHOWA_BOX;
            c->flip                         = true;
            c->hasPartner                   = true;
            cg->grove.chowa[idx].timeLeft += SECOND;
            c->nextTimeLeft = cg->grove.chowa[idx].timeLeft;
            return CHOWA_WALK;
        }
        else if (cg->grove.chowa[idx].gState == CHOWA_TALK && esp_random() % 6 == 0 && !cg->grove.chowa[idx].hasPartner)
        {
            cg->grove.chowa[idx].hasPartner = true;
            c->targetPos.x                  = cg->grove.chowa[idx].aabb.pos.x + 24;
            c->targetPos.y                  = cg->grove.chowa[idx].aabb.pos.y;
            c->precision                    = 10.0f;
            c->nextState                    = CHOWA_TALK;
            c->flip                         = true;
            c->hasPartner                   = true;
            cg->grove.chowa[idx].timeLeft += SECOND;
            c->nextTimeLeft = cg->grove.chowa[idx].timeLeft;
            return CHOWA_WALK;
        }
    }
    // Check mood
    if (!(c->chowa->mood == CG_ANGRY || c->chowa->mood == CG_SAD) && (esp_random() % 6 == 0))
    {
        c->timeLeft = SECOND * (10);
        if (esp_random() % 2 == 0)
        {
            return CHOWA_SING;
        }
        return CHOWA_DANCE;
    }
    if (c->chowa->mood != CG_HAPPY && esp_random() % 6 == 0)
    {
        c->timeLeft   = SECOND * (10);
        c->animIdx    = 0;
        c->flip       = false;
        c->hasPartner = false;
        return CHOWA_BOX;
    }
    // Check player affinity
    if (c->chowa->playerAffinity > 100 && esp_random() % 6 == 0)
    {
        c->precision    = 16.0f;
        c->nextTimeLeft = SECOND * (1 + (esp_random() % 3));
        c->nextState    = CHOWA_GIFT;
        return CHOWA_CHASE;
    }
    // Check for held items
    if (c->heldItem != NULL && esp_random() % 6 == 0)
    {
        if (esp_random() % 3 == 0)
        {
            return CHOWA_DROP_ITEM;
        }
        c->timeLeft = SECOND * (5 + (esp_random() % 6));
        if (strcmp(c->heldItem->name, shopMenuItems[5]) == 0)
        {
            c->timeLeft = SECOND;
            c->flip     = esp_random() % 2 == 0; // Pick direction
        }
        return CHOWA_USE_ITEM;
    }
    // Check if items present on map
    if (c->heldItem == NULL)
    {
        for (int idx = 0; idx < CG_GROVE_MAX_ITEMS; idx++)
        {
            if (cg->grove.items[idx].active && 0 != strcmp(shopMenuItems[11], cg->grove.items[idx].name)
                && !c->ballInAir)
            {
                c->targetPos = cg->grove.items[idx].aabb.pos;
                c->heldItem  = &cg->grove.items[idx];
                c->precision = 20.0f;
                c->nextState = CHOWA_GRAB_ITEM;
                return CHOWA_WALK;
            }
        }
    }
    // Otherwise, choose randomly
    switch (esp_random() % 3)
    {
        case 0:
        {
            cg_GroveGetRandMovePoint(cg, c);
            c->precision = 10.0f;
            c->frameTimer += esp_random() % SECOND; // Offset frame timer
            c->nextState = CHOWA_IDLE;
            return CHOWA_WALK;
        }
        case 1:
        default:
        {
            c->timeLeft = SECOND * (1 + (esp_random() % 10));
            return CHOWA_STATIC;
        }
        case 2:
        {
            c->timeLeft   = SECOND * (10);
            c->flip       = false;
            c->hasPartner = false;
            return CHOWA_TALK;
        }
    }
}