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

#include "artillery_game.h"
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

const artilleryAmmoAttrib_t ammoAttributes[] = {
    {
        .name       = "Normal",
        .color      = c315,
        .radius     = 4,
        .numBounces = 1,
        .numSpread  = 1,
        .numConsec  = 1,
        .score      = 100,
        .expVel     = 100,
        .expRadius  = 20,
        .effect     = NO_EFFECT,
    },
    {
        .name       = "Three Shot",
        .color      = c341,
        .radius     = 3,
        .numBounces = 1,
        .numSpread  = 3,
        .numConsec  = 1,
        .score      = 100,
        .expVel     = 80,
        .expRadius  = 15,
        .effect     = NO_EFFECT,
    },
    {
        .name       = "Five Shot",
        .color      = c314,
        .radius     = 2,
        .numBounces = 1,
        .numSpread  = 5,
        .numConsec  = 1,
        .score      = 100,
        .expVel     = 60,
        .expRadius  = 10,
        .effect     = NO_EFFECT,
    },
    {
        .name       = "Bouncy",
        .color      = c224,
        .radius     = 4,
        .numBounces = 5,
        .numSpread  = 1,
        .numConsec  = 1,
        .score      = 75,
        .expVel     = 90,
        .expRadius  = 20,
        .effect     = NO_EFFECT,
    },
    {
        .name       = "Sniper",
        .color      = c222,
        .radius     = 1,
        .numBounces = 1,
        .numSpread  = 1,
        .numConsec  = 1,
        .score      = 300,
        .expVel     = 0,
        .expRadius  = 1,
        .effect     = NO_EFFECT,
    },
    {
        .name       = "Big Shot",
        .color      = c243,
        .radius     = 6,
        .numBounces = 1,
        .numSpread  = 1,
        .numConsec  = 1,
        .score      = 40,
        .expVel     = 200,
        .expRadius  = 40,
        .effect     = NO_EFFECT,
    },
    {
        .name       = "Rocket Jump",
        .color      = c421,
        .radius     = 12,
        .numBounces = 1,
        .numSpread  = 1,
        .numConsec  = 1,
        .score      = 0,
        .expVel     = 200,
        .expRadius  = 12,
        .effect     = NO_EFFECT,
    },
    {
        .name       = "Wallmaker",
        .color      = c242,
        .radius     = 3,
        .numBounces = 1,
        .numSpread  = 1,
        .numConsec  = 1,
        .score      = 0,
        .expVel     = 0,
        .expRadius  = 20,
        .effect     = WALL_MAKER,
    },
    {
        .name       = "Laser Bolt",
        .color      = c331,
        .radius     = 2,
        .numBounces = 1,
        .numSpread  = 1,
        .numConsec  = 1,
        .score      = 150,
        .expVel     = 75,
        .expRadius  = 15,
        .effect     = LASER,
    },
    {
        .name       = "Homing Shot",
        .color      = c223,
        .radius     = 4,
        .numBounces = 1,
        .numSpread  = 1,
        .numConsec  = 1,
        .score      = 50,
        .expVel     = 100,
        .expRadius  = 20,
        .effect     = HOMING_MISSILE,
    },
    {
        .name       = "Confuser",
        .color      = c244,
        .radius     = 4,
        .numBounces = 1,
        .numSpread  = 1,
        .numConsec  = 1,
        .score      = 50,
        .expVel     = 100,
        .expRadius  = 20,
        .effect     = CONFUSION,
    },
    {
        .name       = "Machine Gun",
        .color      = c321,
        .radius     = 2,
        .numBounces = 1,
        .numSpread  = 1,
        .numConsec  = 5,
        .score      = 100,
        .expVel     = 60,
        .expRadius  = 10,
        .effect     = NO_EFFECT,
    },
    {
        .name       = "Floor is Lava",
        .color      = c412,
        .radius     = 3,
        .numBounces = 1,
        .numSpread  = 1,
        .numConsec  = 1,
        .score      = 100,
        .expVel     = 100,
        .expRadius  = 20,
        .effect     = FLOOR_LAVA,
    },
};

const artilleryAmmoAttrib_t* getAmmoAttributes(uint16_t* numAttributes)
{
    *numAttributes = ARRAY_SIZE(ammoAttributes);
    return ammoAttributes;
}

const artilleryAmmoAttrib_t* getAmmoAttribute(uint16_t idx)
{
    return &ammoAttributes[idx];
}

//==============================================================================
// Function Declarations
//==============================================================================

static void physFindObjDests(physSim_t* phys, float elapsedS);
static bool physBinaryMoveObjects(physSim_t* phys);
static void checkTurnOver(physSim_t* phys);
static void physRunAnimateTimers(physSim_t* phys, int32_t elapsedUs);
static void findSurfacePoints(int x0, int y0, int x1, int y1, int16_t* surfacePoints);

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

        // Generate clouds
        physGenerateClouds(phys);

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
        physCirc_t* circ = pop(&phys->circles);
        while (circ->availableAmmo.first)
        {
            pop(&circ->availableAmmo);
        }
        heap_caps_free(circ);
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
 * @brief Update any moving terrain in the background, before the background draw callback
 *
 * @param phys The physics simulation to move terrain in
 */
void physStepBackground(physSim_t* phys)
{
    if (phys->isReady)
    {
        // Move the terrain
        phys->terrainMoving = moveTerrainLines(phys, PHYS_TIME_STEP_US);

        // Clear surface points
        memset(phys->surfacePoints, 0xFF, sizeof(phys->surfacePoints));

        // Find all surface pixels in screen space
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
                                  phys->surfacePoints);

                // Assign ground color
                int16_t minScreenX = CLAMP(pl->l.p1.x - phys->camera.x, 0, TFT_WIDTH);
                int16_t maxScreenX = CLAMP(pl->l.p2.x - phys->camera.x, 0, TFT_WIDTH);
                memset(&phys->surfaceColors[minScreenX], pl->isLava ? c200 : c020, maxScreenX - minScreenX);
            }

            // Iterate
            lNode = lNode->next;
        }

        // Raise a flag to update the foreground as well
        phys->shouldStepForeground = true;
    }
}

/**
 * @brief Update the entire physics simulation by updating object position, velocity, and acceleration. This checks for
 * object collisions and doesn't allow object to clip into each other.
 *
 * @param phys The physics simulation
 * @param elapsedUs The time elapsed since this was last called
 */
void physStep(physSim_t* phys, int32_t elapsedUs, bool menuShowing, bool* playerMoved, bool* cameraMoved)
{
    *playerMoved = false;
    *cameraMoved = false;
    if (phys->isReady && phys->shouldStepForeground)
    {
        phys->shouldStepForeground = false;

        // Calculate physics frames at a very regular PHYS_TIME_STEP
        physFindObjDests(phys, PHYS_TIME_STEP_S);
        physCheckCollisions(phys);
        *playerMoved = physBinaryMoveObjects(phys);
        *cameraMoved = physAdjustCameraTimer(phys, menuShowing);
        physRunAnimateTimers(phys, PHYS_TIME_STEP_US);
        checkTurnOver(phys);
    }
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
                // If there is a homing target
                if (pc->homingTarget && LASER != pc->effect)
                {
                    // Check if the shell is close enough to the target
                    vecFl_t toTarget = subVecFl2d(pc->homingTarget->c.pos, pc->c.pos);
                    if (sqMagVecFl2d(toTarget) < (140 * 140))
                    {
                        vecFl_t targetNormal = normVecFl2d(toTarget);

                        // Negate world gravity and point at the target
                        pc->g      = addVecFl2d(mulVecFl2d(phys->g, -1), mulVecFl2d(targetNormal, 98));
                        pc->effect = LASER;

                        // Point the velocity right at the target, but don't change magnitude
                        float velMag = magVecFl2d(pc->vel);
                        pc->vel      = mulVecFl2d(normVecFl2d(toTarget), velMag);
                    }
                }

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
 * @brief Draw the background of the simulation (land, sky, and void)
 *
 * @param phys The physics simulation to draw
 */
void drawPhysBackground(physSim_t* phys, int16_t x0, int16_t y0, int16_t w, int16_t h)
{
    // Get a pointer to the framebuffer for this background update
    paletteColor_t* fb = &getPxTftFramebuffer()[y0 * TFT_WIDTH + x0];

    // Fill black to start
    memset(fb, c000, w * h);

    // Find the Y bounds to draw ground and sky between
    int16_t minY = CLAMP(-phys->camera.y, y0, y0 + h);
    int16_t maxY = CLAMP(phys->bounds.y - phys->camera.y, y0, y0 + h);

    // Move the pointer past any blank space on the top
    fb += ((minY - y0) * TFT_WIDTH);

    // For each row to draw
    for (int16_t y = minY; y < maxY; y++)
    {
        // Find the X bounds to draw ground and sky between
        int16_t minX = CLAMP(-phys->camera.x, 0, TFT_WIDTH);
        int16_t maxX = CLAMP(phys->bounds.x - phys->camera.x, 0, TFT_WIDTH);

        // Move the pointer past any blank space on the left
        fb += minX;

        // Draw and iterate pixels
        for (int16_t x = minX; x < maxX; x++)
        {
            if (y < phys->surfacePoints[x])
            {
                // Blue for sky
                *fb = c002;
            }
            else
            {
                // Ground is variable
                *fb = phys->surfaceColors[x];
            }

            // Iterate pointer
            fb++;
        }

        // Move the pointer past any blank space on the right
        fb += (TFT_WIDTH - maxX);
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
void drawPhysOutline(physSim_t* phys, physCirc_t** players, font_t* font, int32_t moveTimeLeftUs, int32_t turn)
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

    // Draw clouds behind everything else
    for (int32_t cIdx = 0; cIdx < ARRAY_SIZE(phys->clouds); cIdx++)
    {
        circle_t* cloud = &phys->clouds[cIdx];
        drawCircleFilled(cloud->pos.x - phys->camera.x, cloud->pos.y - phys->camera.y, cloud->radius, c555);
    }

    // Draw all circles
    node_t* cNode = phys->circles.first;
    while (cNode)
    {
        physCirc_t* pc = (physCirc_t*)cNode->val;

        // Get color from object
        paletteColor_t bCol = pc->baseColor;
        paletteColor_t aCol = pc->accentColor;

        if (pc->lavaAnimTimer)
        {
            if (pc->lavaAnimTimer % LAVA_ANIM_PERIOD < (LAVA_ANIM_PERIOD / 2))
            {
                bCol = c200;
                aCol = c200;
            }
        }

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

#define GAS_GAUGE_HEIGHT 16
#define TEXT_Y           (GAS_GAUGE_HEIGHT + 4)
#define TEXT_X_MARGIN    20

    // Draw gas gauge
    fillDisplayArea(0, 0, (TFT_WIDTH * moveTimeLeftUs) / TANK_MOVE_TIME_US, GAS_GAUGE_HEIGHT, c222);

    // Draw turns
    char turnStr[32] = {0};
    snprintf(turnStr, sizeof(turnStr) - 1, "Turn %" PRId32 "/%d", turn, MAX_TURNS);
    drawTextShadow(font, c444, c000, turnStr, (TFT_WIDTH - textWidth(font, turnStr)) / 2, TEXT_Y);

    // Draw score if players are set
    if (players[0])
    {
        char scoreStr[32] = {0};
        snprintf(scoreStr, sizeof(scoreStr) - 1, "%" PRId32, players[0]->score);
        drawTextShadow(font, c444, c000, scoreStr, TEXT_X_MARGIN, TEXT_Y);

        snprintf(scoreStr, sizeof(scoreStr) - 1, "%" PRId32, players[1]->score);
        drawTextShadow(font, c444, c000, scoreStr, TFT_WIDTH - textWidth(font, scoreStr) - TEXT_X_MARGIN, TEXT_Y);
    }
}

/**
 * @brief Bresenham's line algorithm which also tracks surface points
 *
 * @param x0 The X coordinate to start the line at
 * @param y0 The Y coordinate to start the line at
 * @param x1 The X coordinate to end the line at
 * @param y1 The Y coordinate to end the line at
 * @param surfacePoints [OUT] An array where the highest surface point is written to
 */
static void findSurfacePoints(int x0, int y0, int x1, int y1, int16_t* surfacePoints)
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
void setBarrelAngle(physCirc_t* circ, int16_t angle)
{
    circ->barrelAngle = angle;

    // Make sure it's in the range of 0 to 360
    while (circ->barrelAngle < 0)
    {
        circ->barrelAngle += 360;
    }
    while (circ->barrelAngle >= 360)
    {
        circ->barrelAngle -= 360;
    }

    circ->relBarrelTip.x = (getSin1024(circ->barrelAngle) * circ->c.radius) / 512.0f;
    circ->relBarrelTip.y = (-getCos1024(circ->barrelAngle) * circ->c.radius) / 512.0f;
}

/**
 * @brief Set the shot power for a tank
 *
 * @param circ A circle of type CT_TANK
 * @param power The power to set the shot
 */
void setShotPower(physCirc_t* circ, float power)
{
    circ->shotPower = CLAMP(power, 0, MAX_SHOT_POWER);
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
    float tba = atan2f(v0.x, -v0.y);
    while (tba < 0)
    {
        tba += (2 * M_PIf);
    }
    cpu->targetBarrelAngle = ((180 * tba) / M_PIf) + 0.5f;
    setShotPower(cpu, magVecFl2d(v0));
}

/**
 * @brief Fire a shot from a tank. This creates a circle of type CT_SHELL at the barrel tip with the tank's barrel angle
 * and shot power
 *
 * @param phys The physics simulation
 * @param player The tank which fired the shot
 * @param opponent The opposing tank
 */
void fireShot(physSim_t* phys, physCirc_t* player, physCirc_t* opponent, bool firstShot)
{
    const artilleryAmmoAttrib_t* aa = getAmmoAttribute(player->ammoIdx);

    // Multiple shells are fired three degrees apart. Calculate the starting angle
    const float spread = (3 * M_PIf) / 180.0f;
    float angStart     = (M_PIf * player->barrelAngle / 180.0f) - (aa->numSpread / 2) * spread;

    // This is where shells get spawned
    vecFl_t absBarrelTip = addVecFl2d(player->c.pos, player->relBarrelTip);

    // Calculate individual shell score
    int32_t shellScore = aa->score / (aa->numConsec * aa->numSpread);

    // If this is the first shot, set up consecutive shots
    if (firstShot)
    {
        // Remove fired ammo from the available list
        removeVal(&player->availableAmmo, (void*)((intptr_t)player->ammoIdx));
        // TODO since the IDX changes, show the ammo menu first next round?
        player->ammoIdx = (intptr_t)player->availableAmmo.first->val;

        player->shotsRemaining = aa->numConsec - 1;
        player->shotTimer      = 0;

        // Track shells, not players
        clear(&phys->cameraTargets);
    }

    // Create each shell
    for (int32_t shellCount = 0; shellCount < aa->numSpread; shellCount++)
    {
        // Create the shell at the tip of the barrel
        physCirc_t* shell = physAddCircle(phys, absBarrelTip.x, absBarrelTip.y, aa->radius, CT_SHELL, c440, c000);

        // Set the owner of the shell as the firing tank
        shell->owner = player;

        // Give it some initial velocity
        shell->vel.x = sinf(angStart) * player->shotPower;
        shell->vel.y = -cosf(angStart) * player->shotPower;
        angStart += spread;

        // Set shell parameters
        shell->bounces         = aa->numBounces;
        shell->explosionRadius = aa->expRadius;
        shell->explosionVel    = aa->expVel;
        shell->score           = shellScore;
        shell->effect          = aa->effect;

        // Negate gravity for the laser and make it faster
        if (LASER == shell->effect)
        {
            shell->g   = mulVecFl2d(phys->g, -1);
            shell->vel = mulVecFl2d(shell->vel, 2);
        }
        else if (HOMING_MISSILE == shell->effect)
        {
            shell->homingTarget = opponent;
        }

        // Set color
        shell->baseColor   = aa->color;
        shell->accentColor = c000;

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
    float margin  = phys->bounds.x / 4;
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
physCirc_t* physAddPlayer(physSim_t* phys, vecFl_t pos, int16_t barrelAngle, paletteColor_t baseColor,
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
static void physRunAnimateTimers(physSim_t* phys, int32_t elapsedUs)
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

    node_t* cNode = phys->circles.first;
    while (cNode)
    {
        // Get a convenience pointer
        physCirc_t* pc = (physCirc_t*)cNode->val;
        if (CT_TANK == pc->type)
        {
            if (pc->lavaAnimTimer)
            {
                pc->lavaAnimTimer -= elapsedUs;
                if (pc->lavaAnimTimer < 0)
                {
                    pc->lavaAnimTimer = 0;
                }
            }
        }
        cNode = cNode->next;
    }

    led_t allLeds[CONFIG_NUM_LEDS] = {0};
    if (phys->ledTimer > 0)
    {
        led_t led = {
            .r = (phys->ledColor.r * phys->ledTimer) / LED_EXPLOSION_US,
            .g = (phys->ledColor.g * phys->ledTimer) / LED_EXPLOSION_US,
            .b = (phys->ledColor.b * phys->ledTimer) / LED_EXPLOSION_US,
        };

        phys->ledTimer -= elapsedUs;

        for (int32_t lIdx = 0; lIdx < CONFIG_NUM_LEDS; lIdx++)
        {
            allLeds[lIdx] = led;
        }
    }
    setLeds(allLeds, CONFIG_NUM_LEDS);
}
