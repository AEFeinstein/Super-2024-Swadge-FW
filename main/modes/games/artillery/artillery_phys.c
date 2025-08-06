//==============================================================================
// Includes
//==============================================================================

#include <math.h>
#include <float.h>
#include <stdio.h>

#include <esp_heap_caps.h>
#include <esp_log.h>
#include <hdw-tft.h>

#include "shapes.h"
#include "artillery.h"
#include "artillery_phys.h"
#include "macros.h"
#include "fill.h"

#include "artillery_phys_objs.h"
#include "artillery_phys_camera.h"
#include "artillery_phys_collisions.h"
#include "artillery_phys_terrain.h"
#include "artillery_phys_bsp.h"

//==============================================================================
// Defines
//==============================================================================

#define PHYS_TIME_STEP (1000000 / 60)

#if 0
    #define PRINT_P1_VEC(label, vec)                                          \
        do                                                                    \
        {                                                                     \
            if (cNode == phys->circles.first)                                 \
            {                                                                 \
                ESP_LOGI("VEC", "%s: %.15f, %.15f", label, (vec).x, (vec.y)); \
            }                                                                 \
        } while (0)
#endif

#define PLAYER_RADIUS 8

//==============================================================================
// Function Declarations
//==============================================================================

static void physFindObjDests(physSim_t* phys, int32_t elapsedUs);
static bool physBinaryMoveObjects(physSim_t* phys);
static void checkTurnOver(physSim_t* phys);
static void physAnimateExplosions(physSim_t* phys, int32_t elapsedUs);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initialize a physics simulation with world bounds and gravity.
 * The world is segmented into 32 zones for more efficient object collision detection
 *
 * @param w Physics space width, unit is px
 * @param h Physics space height, unit is px
 * @param groundLevel The base height of the ground. May be randomized higher or lower
 * @param gx Gravity in the X direction, unit is px/uS^2
 * @param gy Gravity in the Y direction, unit is px/uS^2
 * @return The initialized simulation
 */
physSim_t* initPhys(float w, float h, int32_t groundLevel, float gx, float gy, bool generateTerrain)
{
    // Allocate a simulation
    physSim_t* phys = heap_caps_calloc(1, sizeof(physSim_t), MALLOC_CAP_8BIT);

    // Set gravity
    phys->g.x = gx;
    phys->g.y = gy;

    // Set bounds for the physics sim
    phys->bounds.x = w;
    phys->bounds.y = h;

    // Add lines for the world bounds
    physAddWorldBounds(phys);

    if (generateTerrain)
    {
        // Generate terrain
        physGenerateTerrain(phys, groundLevel);

        // Create BSP zones
        createBspZones(phys);

        phys->isReady = true;
    }
    else
    {
        phys->isReady = false;
    }

    // Return the simulation
    return phys;
}

/**
 * @brief TODO doc
 *
 * @param phys
 */
void physAddWorldBounds(physSim_t* phys)
{
    physAddLine(phys, 0, 0, phys->bounds.x, 0, false);
    physAddLine(phys, phys->bounds.x, 0, phys->bounds.x, phys->bounds.y, false);
    physAddLine(phys, phys->bounds.x, phys->bounds.y, 0, phys->bounds.y, false);
    physAddLine(phys, 0, phys->bounds.y, 0, 0, false);
}

/**
 * @brief TODO doc
 *
 * @param phys
 */
void physRemoveAllObjects(physSim_t* phys)
{
    // Free all lines
    while (phys->lines.first)
    {
        heap_caps_free(pop(&phys->lines));
    }

    // Free all circles
    while (phys->circles.first)
    {
        heap_caps_free(pop(&phys->circles));
    }

    // Free all explosions
    while (phys->explosions.first)
    {
        heap_caps_free(pop(&phys->explosions));
    }

    // Free camera targets, don't need to free elements
    while (phys->cameraTargets.first)
    {
        pop(&phys->cameraTargets);
    }
}

/**
 * @brief Deinitialize and free a physics simulation
 *
 * @param phys The physics simulation
 */
void deinitPhys(physSim_t* phys)
{
    physRemoveAllObjects(phys);

    // Free the simulation
    heap_caps_free(phys);
}

/**
 * @brief Update the entire physics simulation by updating object position, velocity, and acceleration. This checks for
 * object collisions and doesn't allow object to clip into each other.
 *
 * @param phys The physics simulation
 * @param elapsedUs The time elapsed since this was last called
 */
bool physStep(physSim_t* phys, int32_t elapsedUs)
{
    bool change = false;
    if (phys->isReady)
    {
        // Calculate physics frames at a very regular PHYS_TIME_STEP
        RUN_TIMER_EVERY(phys->frameTimer, PHYS_TIME_STEP, elapsedUs, {
            physFindObjDests(phys, PHYS_TIME_STEP);
            phys->terrainMoving = moveTerrainPoints(phys, PHYS_TIME_STEP);
            physCheckCollisions(phys);
            change |= physBinaryMoveObjects(phys);
            physAnimateExplosions(phys, PHYS_TIME_STEP);
            checkTurnOver(phys);
        });
    }
    return change;
}

/**
 * @brief Update each object's acceleration, velocity, and desired next position.
 * This doesn't move objects yet so objects will not clip into each other.
 *
 * @param phys The physics simulation
 * @param elapsedUs The time elapsed since this was last called
 */
static void physFindObjDests(physSim_t* phys, int32_t elapsedUs)
{
    // For each circle
    node_t* cNode = phys->circles.first;
    while (cNode)
    {
        physCirc_t* pc = (physCirc_t*)cNode->val;
        // If it's not fixed in space
        if (!pc->fixed)
        {
            // Set starting point
            pc->travelLine.p1 = pc->c.pos;

            vecFl_t totalForce = {0};
            vecFl_t moveVel    = {0};
            if (pc->inContact)
            {
                // Accelerate in the direction of the surface the object is on
                totalForce = addVecFl2d(pc->staticForce, pc->g);

                // If the object is moving
                if (pc->moving)
                {
                    // Add something to the velocity (not acceleration)
                    if ((PB_LEFT == pc->moving && pc->slopeVec.x < 0) || //
                        (PB_RIGHT == pc->moving && pc->slopeVec.x > 0))
                    {
                        moveVel = mulVecFl2d(pc->slopeVec, 0.0001f);
                    }
                    else
                    {
                        moveVel = mulVecFl2d(pc->slopeVec, -0.0001f);
                    }
                }
            }
            else
            {
                // If not in contact, force is world and object gravity
                totalForce = addVecFl2d(pc->g, phys->g);
            }

            // Calculate new velocity
            pc->vel = addVecFl2d(pc->vel, mulVecFl2d(totalForce, elapsedUs));

            // Set ending point
            pc->travelLine.p2 = addVecFl2d(pc->c.pos, mulVecFl2d(addVecFl2d(pc->vel, moveVel), elapsedUs));

            // Set bounding box of travel line, for later checks
            pc->travelLineBB = getLineBoundingBox(pc->travelLine);

            // Recompute zones
            updateCircleProperties(phys, pc);
        }

        // Iterate
        cNode = cNode->next;
    }
}

/**
 * @brief Update the position of all moving objects in the simulation. This will move objects from their current to
 * their desired positions as best as possible, without clipping objects. Binary search is used to find the best final
 * destination.
 *
 * @param phys The physics simulation
 */
static bool physBinaryMoveObjects(physSim_t* phys)
{
    bool playerMoved = false;

    // For each circle
    node_t* cNode = phys->circles.first;
    while (cNode)
    {
        // Convenience pointer
        physCirc_t* pc = (physCirc_t*)cNode->val;

        // If it's not fixed in space
        if (!pc->fixed)
        {
            // Save old position
            vecFl_t oldPos = pc->c.pos;

            // Test at the final destination
            pc->c.pos = pc->travelLine.p2;
            updateCircleProperties(phys, pc);

            // If there is a collision, binary search. Otherwise move on.
            if (physAnyCollision(phys, pc))
            {
                // If the final destination isn't valid, binary search
                bool collision = true;

                // TODO pick a better number?
                int32_t numIter = 5;
                while (numIter)
                {
                    numIter--;

                    // Set circle to the midpoint of the travel line
                    pc->c.pos = divVecFl2d(addVecFl2d(pc->travelLine.p1, pc->travelLine.p2), 2);
                    updateCircleProperties(phys, pc);

                    // Test for collision
                    collision = physAnyCollision(phys, pc);
                    if (collision)
                    {
                        // Move towards the start of the travel line
                        pc->travelLine.p2 = pc->c.pos;
                    }
                    else
                    {
                        // Move towards the end of the travel line
                        pc->travelLine.p1 = pc->c.pos;
                    }
                }

                // If there's still a collision after binary search
                if (collision)
                {
                    // Return to the starting point
                    pc->c.pos = pc->travelLine.p1;
                    updateCircleProperties(phys, pc);
                }
            }

            // Mark if a player moved an integer amount
            if (CT_TANK == pc->type && ((int)pc->c.pos.x != (int)oldPos.x || (int)pc->c.pos.y != (int)oldPos.y))
            {
                playerMoved = true;
            }
        }

        // Iterate
        cNode = cNode->next;
    }

    return playerMoved;
}

/**
 * @brief Check if a shot is done (i.e. all shells have been removed)
 *
 * @param phys The physics simulation
 */
static void checkTurnOver(physSim_t* phys)
{
    // If there is a shot in progress and the turn isn't over yet
    if (phys->shotFired && !phys->playerSwapTimerUs)
    {
        // Check for any shells
        bool anyShells = false;

        // Iterate through all circles, looking for CT_SHELL
        node_t* shNode = phys->circles.first;
        while (shNode)
        {
            // Get a convenience pointer
            physCirc_t* shell = (physCirc_t*)shNode->val;

            // If a shell was found, the shot is still in progress
            if (CT_SHELL == shell->type)
            {
                // Shell found
                anyShells = true;
                break;
            }

            // Iterate
            shNode = shNode->next;
        }

        // Turn is over if there are no shells and no moving terrain
        if (false == anyShells && false == phys->terrainMoving)
        {
            phys->playerSwapTimerUs = 3000000;
        }
    }
}

/**
 * @brief Draw the outlines of physics simulation objects
 *
 * @param phys The physics simulation
 * @param moveTimeLeftUs TODO
 */
void drawPhysOutline(physSim_t* phys, int32_t moveTimeLeftUs)
{
    // Draw gas gauge
    fillDisplayArea(0, 0, (TFT_WIDTH * moveTimeLeftUs) / TANK_MOVE_TIME_US, 16, c313);

    // Draw zones
    // for (int32_t z = 0; z < NUM_ZONES; z++)
    // {
    //     drawRect(                                                          //
    //         phys->zones[z].pos.x - phys->camera.x,                         //
    //         phys->zones[z].pos.y - phys->camera.y,                         //
    //         phys->zones[z].pos.x + phys->zones[z].width - phys->camera.x,  //
    //         phys->zones[z].pos.y + phys->zones[z].height - phys->camera.y, //
    //         c112);
    // }

    // Draw all lines
    node_t* lNode = phys->lines.first;
    while (lNode)
    {
        physLine_t* pl = (physLine_t*)lNode->val;
        drawLineFast(pl->l.p1.x - phys->camera.x, //
                     pl->l.p1.y - phys->camera.y, //
                     pl->l.p2.x - phys->camera.x, //
                     pl->l.p2.y - phys->camera.y, //
                     c522);

        // Iterate
        lNode = lNode->next;
    }

    // Draw all circles
    node_t* cNode = phys->circles.first;
    while (cNode)
    {
        physCirc_t* pc = (physCirc_t*)cNode->val;

        // Pick color based on type
        paletteColor_t cCol = c522;
        if (CT_TANK == pc->type)
        {
            cCol = c335;
        }
        else if (CT_SHELL == pc->type)
        {
            cCol = c550;
        }
        drawCircle(pc->c.pos.x - phys->camera.x, //
                   pc->c.pos.y - phys->camera.y, //
                   pc->c.radius, cCol);

        // Draw a gun barrel for tanks
        if (CT_TANK == pc->type)
        {
            vecFl_t absBarrelTip = addVecFl2d(pc->c.pos, pc->relBarrelTip);
            drawLineFast(pc->c.pos.x - phys->camera.x,    //
                         pc->c.pos.y - phys->camera.y,    //
                         absBarrelTip.x - phys->camera.x, //
                         absBarrelTip.y - phys->camera.y, //
                         c335);
        }

        // Iterate
        cNode = cNode->next;
    }

    // Draw explosions
    node_t* eNode = phys->explosions.first;
    while (eNode)
    {
        explosionAnim_t* exp = eNode->val;
        float radius         = (exp->circ.radius * exp->ttlUs) / exp->expTimeUs;
        drawCircleFilled(                     //
            exp->circ.pos.x - phys->camera.x, //
            exp->circ.pos.y - phys->camera.y, //
            radius,                           //
            exp->color);
        eNode = eNode->next;
    }
}

/**
 * @brief Set the barrel angle for a tank
 *
 * @param circ A circle of type CT_TANK
 * @param angle The angle to set the barrel at, in radians
 */
void setBarrelAngle(physCirc_t* circ, float angle)
{
    circ->barrelAngle = angle;

    circ->relBarrelTip.x = sinf(angle) * circ->c.radius * 2;
    circ->relBarrelTip.y = -cosf(angle) * circ->c.radius * 2;
}

/**
 * @brief Set the shot power for a tank
 *
 * @param circ A circle of type CT_TANK
 * @param power The power to set the shot
 */
void setShotPower(physCirc_t* circ, float power)
{
    circ->shotPower = power;
}

/**
 * @brief Fire a shot from a tank. This creates a circle of type CT_SHELL at the barrel tip with the tank's barrel angle
 * and shot power
 *
 * @param phys The physics simulation
 * @param circ A circle of type CT_TANK
 */
void fireShot(physSim_t* phys, physCirc_t* circ)
{
    int32_t radius    = 4;
    int32_t numShells = 1;
    int32_t bounces   = 1;

    switch (circ->ammo)
    {
        case AMMO_NORMAL:
        {
            /* todo */
            break;
        }
        case AMMO_BIG_EXPLODE:
        {
            /* todo */
            radius = 8;
            break;
        }
        case AMMO_THREE:
        {
            /* todo */
            numShells = 3;
            break;
        }
        case AMMO_FIVE:
        {
            /* todo */
            numShells = 5;
            break;
        }
        case AMMO_SNIPER:
        {
            /* todo */
            break;
        }
        case AMMO_MACHINE_GUN:
        {
            /* todo */
            break;
        }
        case AMMO_BOUNCY:
        {
            /* todo */
            bounces = 5;
            break;
        }
        case AMMO_JACKHAMMER:
        {
            /* todo */
            break;
        }
        case AMMO_HILL_MAKER:
        {
            /* todo */
            break;
        }
        case AMMO_JUMP:
        {
            /* todo */
            break;
        }
    }

    // For multiple shells, calculate angle spread and starting angle
    const float spread = ((2 * M_PI) / 180.0f);
    float angStart     = circ->barrelAngle - (numShells / 2) * spread;

    // This is where shells get spawned
    vecFl_t absBarrelTip = addVecFl2d(circ->c.pos, circ->relBarrelTip);

    // Track shells, not players
    clear(&phys->cameraTargets);

    // Create each shell
    for (int32_t shellCount = 0; shellCount < numShells; shellCount++)
    {
        // Create the shell at the tip of the barrel
        physCirc_t* shell = physAddCircle(phys, absBarrelTip.x, absBarrelTip.y, radius, CT_SHELL);

        // Give it some initial velocity
        shell->vel.x = sinf(angStart) * circ->shotPower;
        shell->vel.y = -cosf(angStart) * circ->shotPower;
        angStart += spread;

        // Some shells are bouncy, some aren't
        shell->bounces = bounces;

        // Camera tracks all shells
        push(&phys->cameraTargets, shell);
    }
}

/**
 * @brief TODO
 *
 * @param phys
 * @param players
 * @param numPlayers
 */
void physSpawnPlayers(physSim_t* phys, physCirc_t* players[], int32_t numPlayers)
{
    float margin  = phys->bounds.x / 8;
    float spacing = (phys->bounds.x - (2 * margin)) / (numPlayers - 1);

    float x = margin;
    for (int32_t p = 0; p < numPlayers; p++)
    {
        // Create a new player
        players[p] = physAddCircle(phys, x, 1 + PLAYER_RADIUS, PLAYER_RADIUS, CT_TANK);

        // Adjust X for the net player
        x += spacing;
    }

    for (int32_t p = 0; p < numPlayers; p++)
    {
        flattenTerrainUnderPlayer(phys, players[p]);
    }

    for (int32_t p = 0; p < numPlayers; p++)
    {
        // Move downward until you almost hit the ground
        while (!physAnyCollision(phys, players[p]))
        {
            players[p]->c.pos.y += (PLAYER_RADIUS / 2);
            updateCircleProperties(phys, players[p]);
        }
        players[p]->c.pos.y -= PLAYER_RADIUS;
    }
}

/**
 * @brief TODO
 *
 * @param phys
 * @param pos
 * @param barrelAngle
 * @return physCirc_t*
 */
physCirc_t* physAddPlayer(physSim_t* phys, vecFl_t pos, float barrelAngle)
{
    physCirc_t* pc = physAddCircle(phys, pos.x, pos.y, PLAYER_RADIUS, CT_TANK);
    setBarrelAngle(pc, barrelAngle);
    return pc;
}

/**
 * @brief TODO
 *
 * @param phys
 * @param elapsedUs
 */
static void physAnimateExplosions(physSim_t* phys, int32_t elapsedUs)
{
    node_t* eNode = phys->explosions.first;
    while (eNode)
    {
        explosionAnim_t* exp = eNode->val;
        node_t* eNodeNext    = eNode->next;

        exp->ttlUs -= elapsedUs;
        if (exp->ttlUs <= 0)
        {
            heap_caps_free(removeEntry(&phys->explosions, eNode));
        }

        eNode = eNodeNext;
    }
}
