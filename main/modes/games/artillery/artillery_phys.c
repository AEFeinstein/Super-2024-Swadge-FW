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

static void physFindObjDests(physSim_t* phys, float elapsedS);
static bool physBinaryMoveObjects(physSim_t* phys);
static void checkTurnOver(physSim_t* phys);
static void physAnimateExplosions(physSim_t* phys, int32_t elapsedUs);
static void findSurfacePoints(int x0, int y0, int x1, int y1, paletteColor_t color, int16_t* surfacePoints);

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
 * @param generateTerrain true to generate terrain, false to receive it from a packet later
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
 * @brief Add world bounds lines to the simulation
 *
 * @param phys The physics simulation
 */
void physAddWorldBounds(physSim_t* phys)
{
    physAddLine(phys, 0, 0, phys->bounds.x, 0, false);
    physAddLine(phys, phys->bounds.x, 0, phys->bounds.x, phys->bounds.y, false);
    physAddLine(phys, phys->bounds.x, phys->bounds.y, 0, phys->bounds.y, false);
    physAddLine(phys, 0, phys->bounds.y, 0, 0, false);
}

/**
 * @brief Remove all lines and circles from the physics simulation
 *
 * @param phys The physics simulation
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
    if (phys)
    {
        physRemoveAllObjects(phys);

        // Free the simulation
        heap_caps_free(phys);
    }
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
        RUN_TIMER_EVERY(phys->frameTimer, PHYS_TIME_STEP_US, elapsedUs, {
            physFindObjDests(phys, PHYS_TIME_STEP_S);
            phys->terrainMoving = moveTerrainLines(phys, PHYS_TIME_STEP_US);
            physCheckCollisions(phys);
            change |= physBinaryMoveObjects(phys);
            change |= physAdjustCameraTimer(phys);
            physAnimateExplosions(phys, PHYS_TIME_STEP_US);
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
 * @param elapsedS The time elapsed since this was last called
 */
static void physFindObjDests(physSim_t* phys, float elapsedS)
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
            if (pc->inContact)
            {
                // Accelerate in the direction of the surface the object is on
                totalForce = addVecFl2d(pc->staticForce, pc->g);

                // Static friction opposes the static force
                vecFl_t staticFriction = mulVecFl2d(pc->slopeVec, -phys->g.y / 4.0f);

                // If the static friction overcomes the total force
                if (sqMagVecFl2d(staticFriction) >= sqMagVecFl2d(totalForce))
                {
                    // don't move
                    totalForce.x = 0;
                    totalForce.y = 0;
                }
                else
                {
                    // Subtract the static friction from the total force
                    totalForce = addVecFl2d(totalForce, staticFriction);
                }

                // If the object is moving
                if (pc->moving)
                {
                    // Add something to the velocity (not acceleration)
                    if ((PB_LEFT == pc->moving && pc->slopeVec.x < 0) || //
                        (PB_RIGHT == pc->moving && pc->slopeVec.x > 0))
                    {
                        totalForce = addVecFl2d(totalForce, mulVecFl2d(pc->slopeVec, 98));
                    }
                    else
                    {
                        totalForce = addVecFl2d(totalForce, mulVecFl2d(pc->slopeVec, -98));
                    }
                }
            }
            else
            {
                // If not in contact, force is world and object gravity
                totalForce = addVecFl2d(pc->g, phys->g);
            }

            // Calculate new velocity
            pc->vel = addVecFl2d(pc->vel, mulVecFl2d(totalForce, elapsedS));

            // Set ending point
            pc->travelLine.p2 = addVecFl2d(pc->c.pos, mulVecFl2d(pc->vel, elapsedS));

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
 * @param players
 * @param font
 * @param moveTimeLeftUs
 */
void drawPhysOutline(physSim_t* phys, physCirc_t** players, font_t* font, int32_t moveTimeLeftUs)
{
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

    // Clear surface points
    memset(phys->surfacePoints, 0xFF, sizeof(phys->surfacePoints));

    // Draw all lines
    node_t* lNode = phys->lines.first;
    while (lNode)
    {
        physLine_t* pl = (physLine_t*)lNode->val;
        if (pl->isTerrain)
        {
            // Draw surface lines differently to track the ground fill
            findSurfacePoints(pl->l.p1.x - phys->camera.x, //
                              pl->l.p1.y - phys->camera.y, //
                              pl->l.p2.x - phys->camera.x, //
                              pl->l.p2.y - phys->camera.y, //
                              c555, phys->surfacePoints);
        }

        // Iterate
        lNode = lNode->next;
    }

    // Find the minimum and maximum surface points
    int16_t minSurf = INT16_MAX;
    int16_t maxSurf = INT16_MIN;
    for (int idx = 0; idx < TFT_WIDTH; idx++)
    {
        if (phys->surfacePoints[idx] < minSurf && phys->surfacePoints[idx] >= 0)
        {
            minSurf = phys->surfacePoints[idx];
        }
        if (phys->surfacePoints[idx] > maxSurf)
        {
            maxSurf = phys->surfacePoints[idx];
        }
    }

    // Check if a non-negative minimum surface point was found
    if (INT16_MAX == minSurf)
    {
        minSurf = -1;
    }

    // Clamp to screen space for fills and draws
    minSurf = CLAMP(minSurf, 0, TFT_WIDTH);
    maxSurf = CLAMP(maxSurf, 0, TFT_HEIGHT);

    // Fill sky and ground contiguous regions
    fillDisplayArea(-phys->camera.x,                 //
                    -phys->camera.y,                 //
                    phys->bounds.x - phys->camera.x, //
                    minSurf,                         //
                    c002);
    fillDisplayArea(-phys->camera.x,                 //
                    maxSurf,                         //
                    phys->bounds.x - phys->camera.x, //
                    phys->bounds.y - phys->camera.y, //
                    c020);

    // Fill in boundary between sky and ground
    SETUP_FOR_TURBO();
    int16_t startX = CLAMP(-phys->camera.x, 0, TFT_WIDTH);
    int16_t endX   = CLAMP(phys->bounds.x - phys->camera.x, 0, TFT_WIDTH);
    for (int y = minSurf; y < maxSurf; y++)
    {
        for (int16_t x = startX; x < endX; x++)
        {
            if (y < phys->surfacePoints[x])
            {
                TURBO_SET_PIXEL(x, y, c002);
            }
            else
            {
                TURBO_SET_PIXEL(x, y, c020);
            }
        }
    }

    // Draw all circles
    node_t* cNode = phys->circles.first;
    while (cNode)
    {
        physCirc_t* pc = (physCirc_t*)cNode->val;

        // Get color from object
        paletteColor_t bCol = pc->baseColor;
        paletteColor_t aCol = pc->accentColor;

        // Draw a gun barrel for tanks
        if (CT_TANK == pc->type)
        {
            vecFl_t absBarrelTip = addVecFl2d(pc->c.pos, pc->relBarrelTip);
            drawLineFast(pc->c.pos.x - phys->camera.x,    //
                         pc->c.pos.y - phys->camera.y,    //
                         absBarrelTip.x - phys->camera.x, //
                         absBarrelTip.y - phys->camera.y, //
                         aCol);
        }

        // Draw main circle
        drawCircleFilled(pc->c.pos.x - phys->camera.x, //
                         pc->c.pos.y - phys->camera.y, //
                         pc->c.radius, bCol);

        // Draw wheels for tanks too
        if (CT_TANK == pc->type)
        {
            // and some wheels too
            float wheelR = pc->c.radius / 2.0f;
            float wheelY = pc->c.radius - wheelR;

            // Find the vector pointing from the center of the tank to the floor
            vecFl_t wheelOffVert = mulVecFl2d(normVecFl2d(addVecFl2d(pc->contactNorm, pc->lastContactNorm)), -1);

            // Rotate by 90 deg, doesn't matter which way
            vecFl_t wheelOffHorz = {
                .x = wheelOffVert.y,
                .y = -wheelOffVert.x,
            };

            // Scale vectors to place the wheels
            vecFl_t treadOff = mulVecFl2d(wheelOffVert, wheelR);
            wheelOffVert     = mulVecFl2d(wheelOffVert, wheelY);
            wheelOffHorz     = mulVecFl2d(wheelOffHorz, pc->c.radius);

            // Draw first wheel
            vecFl_t w1 = addVecFl2d(pc->c.pos, addVecFl2d(wheelOffVert, wheelOffHorz));
            drawCircleFilled(w1.x - phys->camera.x, w1.y - phys->camera.y, wheelR, aCol);

            // Draw second wheel
            vecFl_t w2 = addVecFl2d(pc->c.pos, subVecFl2d(wheelOffVert, wheelOffHorz));
            drawCircleFilled(w2.x - phys->camera.x, w2.y - phys->camera.y, wheelR, aCol);

            // Draw top tread
            drawLineFast(w1.x + treadOff.x - phys->camera.x, //
                         w1.y + treadOff.y - phys->camera.y, //
                         w2.x + treadOff.x - phys->camera.x, //
                         w2.y + treadOff.y - phys->camera.y, //
                         aCol);
            drawLineFast(w1.x - treadOff.x - phys->camera.x, //
                         w1.y - treadOff.y - phys->camera.y, //
                         w2.x - treadOff.x - phys->camera.x, //
                         w2.y - treadOff.y - phys->camera.y, //
                         aCol);
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

    // Draw gas gauge
    fillDisplayArea(0, 0, (TFT_WIDTH * moveTimeLeftUs) / TANK_MOVE_TIME_US, 16, c222);

    // Draw score if players are set
    if (players[0])
    {
        char scoreStr[32] = {0};
        snprintf(scoreStr, sizeof(scoreStr) - 1, "%" PRId32, players[0]->score);
        drawText(font, c555, scoreStr, 20, 20);

        snprintf(scoreStr, sizeof(scoreStr) - 1, "%" PRId32, players[1]->score);
        drawText(font, c555, scoreStr, TFT_WIDTH - textWidth(font, scoreStr) - 20, 20);
    }
}

/**
 * @brief Bresenham's line algorithm which also tracks surface points
 *
 * @param x0 The X coordinate to start the line at
 * @param y0 The Y coordinate to start the line at
 * @param x1 The X coordinate to end the line at
 * @param y1 The Y coordinate to end the line at
 * @param color The color to draw the line
 * @param surfacePoints [OUT] An array where the highest surface point is written to
 */
static void findSurfacePoints(int x0, int y0, int x1, int y1, paletteColor_t color, int16_t* surfacePoints)
{
    // X is ordered and these lines would definitely be off-screen
    if (x1 < 0 || x0 >= TFT_WIDTH)
    {
        return;
    }

    int dx  = ABS(x1 - x0);
    int sx  = x0 < x1 ? 1 : -1;
    int dy  = -ABS(y1 - y0);
    int sy  = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    int e2; /* error value e_xy */

    /* loop */
    for (;;)
    {
        if (0 <= x0 && x0 < TFT_WIDTH)
        {
            if (y0 > surfacePoints[x0])
            {
                surfacePoints[x0] = y0;
            }
        }

        e2 = 2 * err;
        if (e2 >= dy)
        {
            /* e_xy+e_x > 0 */
            if (x0 == x1)
            {
                break;
            }
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx)
        {
            /* e_xy+e_y < 0 */
            if (y0 == y1)
            {
                break;
            }
            err += dx;
            y0 += sy;
        }
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

    // Make sure it's in the range of 0 to 2*pi
    while (circ->barrelAngle < 0)
    {
        circ->barrelAngle += (2 * M_PI);
    }
    while (circ->barrelAngle >= (2 * M_PI))
    {
        circ->barrelAngle -= (2 * M_PI);
    }

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
 * @brief Calculate a 'perfect' shot for the CPU to hit the target
 *
 * @param phys The physics simulation
 * @param cpu The CPU firing a shot
 * @param target The target
 */
void adjustCpuShot(physSim_t* phys, physCirc_t* cpu, physCirc_t* target)
{
    // Arc the shot between the terrain peak and ceiling
    // First find the terrain peak
    float minY    = phys->bounds.y;
    node_t* lNode = phys->lines.first;
    while (lNode)
    {
        physLine_t* line = lNode->val;
        if (line->isTerrain)
        {
            if (line->l.p1.y < minY)
            {
                minY = line->l.p1.y;
            }
            if (line->l.p2.y < minY)
            {
                minY = line->l.p2.y;
            }
        }
        lNode = lNode->next;
    }
    // Scale it closer towards the ceiling
    minY /= 4.0f;

    // Starting position
    vecFl_t absBarrelTip = addVecFl2d(cpu->c.pos, cpu->relBarrelTip);

    // We want to shoot the shell to minY height, so give it that initial velocity
    vecFl_t v0 = {
        .x = 0,
        .y = -sqrtf(2 * phys->g.y * (absBarrelTip.y - minY)),
    };

    // Calculate the shell's total flight time, up and down to the target
    float tUp    = -v0.y / phys->g.y;
    float tDown  = sqrtf(2 * (target->c.pos.y - minY) / phys->g.y);
    float tTotal = tUp + tDown;

    // Calculate the horizontal distance to travel
    float deltaX = target->c.pos.x - absBarrelTip.x;
    // Find the horizontal velocity with distance and time
    v0.x = deltaX / tTotal;

    // Use the initial velocity to find the barrel angle and magnitude
    cpu->targetBarrelAngle = atan2f(v0.x, -v0.y);
    while (cpu->targetBarrelAngle < 0)
    {
        cpu->targetBarrelAngle += (2 * M_PI);
    }
    setShotPower(cpu, magVecFl2d(v0));
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
        physCirc_t* shell = physAddCircle(phys, absBarrelTip.x, absBarrelTip.y, radius, CT_SHELL, c440, c000);
        // Set the owner of the shell as the firing tank
        shell->owner = circ;

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
 * @brief TODO doc
 *
 * @param phys
 * @param numPlayers
 * @param players
 * @param colors
 */
void physSpawnPlayers(physSim_t* phys, int32_t numPlayers, physCirc_t* players[], paletteColor_t* colors)
{
    float margin  = phys->bounds.x / 8;
    float spacing = (phys->bounds.x - (2 * margin)) / (numPlayers - 1);

    float x = margin;
    for (int32_t p = 0; p < numPlayers; p++)
    {
        paletteColor_t base   = *(colors++);
        paletteColor_t accent = *(colors++);
        // Create a new player
        players[p] = physAddCircle(phys, x, 1 + PLAYER_RADIUS, PLAYER_RADIUS, CT_TANK, base, accent);

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
physCirc_t* physAddPlayer(physSim_t* phys, vecFl_t pos, float barrelAngle, paletteColor_t baseColor,
                          paletteColor_t accentColor)
{
    physCirc_t* pc = physAddCircle(phys, pos.x, pos.y, PLAYER_RADIUS, CT_TANK, baseColor, accentColor);
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
