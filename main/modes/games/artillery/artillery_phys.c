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
#include "palette.h"
#include "artillery_phys.h"
#include "macros.h"

//==============================================================================
// Defines
//==============================================================================

#define CAMERA_BTN_MOVE_INTERVAL 2
#define CAMERA_MARGIN            64

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

//==============================================================================
// Function Declarations
//==============================================================================

void physSetZoneMaskLine(physSim_t* phys, physLine_t* pl);
void physSetZoneMaskCirc(physSim_t* phys, physCirc_t* pc);
void physUpdateTimestep(physSim_t* phys, int32_t elapsedUs);
void physCheckCollisions(physSim_t* phys);
void physBinaryMoveObjects(physSim_t* phys);
bool physAnyCollision(physSim_t* phys, physCirc_t* c);

bool physCircCircIntersection(physSim_t* phys, physCirc_t* cMoving, circleFl_t* cOther, float* colDist,
                              vecFl_t* reflVec);
void physAdjustCameraTimer(physSim_t* phys);
void checkTurnOver(physSim_t* phys);
void explodeShell(physSim_t* phys, node_t* shellNode);
float deformTerrainPoint(vecFl_t* p, vecFl_t* expPnt, float rSq, float expMin, float expMax);
bool moveTerrainPoints(physSim_t* phys, int32_t elapsedUs);
void updateLineProperties(physSim_t* phys, physLine_t* pl);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initialize a physics simulation with world bounds and gravity.
 * The world is segmented into 32 zones for more efficient object collision detection
 *
 * @param w Physics space width, unit is px
 * @param h Physics space height, unit is px
 * @param gx Gravity in the X direction, unit is px/uS^2
 * @param gy Gravity in the Y direction, unit is px/uS^2
 * @return The initialized simulation
 */
physSim_t* initPhys(float w, float h, float gx, float gy)
{
    // Allocate a simulation
    physSim_t* phys = heap_caps_calloc(1, sizeof(physSim_t), MALLOC_CAP_8BIT);

    // Set gravity
    phys->g.x = gx;
    phys->g.y = gy;

    // Set bounds for the physics sim
    phys->bounds.x = w;
    phys->bounds.y = h;

    // Figure out number of zones in each dimension
    vecFl_t nZones = {
        .x = 1,
        .y = 1,
    };

    // Calculate the zone size
    vecFl_t zoneSize = {
        .x = w,
        .y = h,
    };

    // Keep dividing along the longer axis
    int32_t divs = NUM_ZONES;
    while (divs /= 2)
    {
        if (zoneSize.x > zoneSize.y)
        {
            zoneSize.x /= 2;
            nZones.x *= 2;
        }
        else
        {
            zoneSize.y /= 2;
            nZones.y *= 2;
        }
    }

    // Create the zones
    for (int32_t y = 0; y < nZones.y; y++)
    {
        for (int32_t x = 0; x < nZones.x; x++)
        {
            rectangleFl_t* zone = &phys->zones[(int)(y * nZones.x) + x];
            zone->pos.x         = x * zoneSize.x;
            zone->pos.y         = y * zoneSize.y;
            zone->width         = zoneSize.x;
            zone->height        = zoneSize.y;
        }
    }

    // Add lines for the world bounds
    physAddLine(phys, 0, 0, w, 0, false);
    physAddLine(phys, w, 0, w, h, false);
    physAddLine(phys, w, h, 0, h, false);
    physAddLine(phys, 0, h, 0, 0, false);

    // Return the simulation
    return phys;
}

/**
 * @brief Deinitialize and free a physics simulation
 *
 * @param phys The physics simulation
 */
void deinitPhys(physSim_t* phys)
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

    // Free the simulation
    heap_caps_free(phys);
}

/**
 * @brief Add an immobile line to the physics simulation. This calculates and saves properties like slope and unit
 * normal.
 *
 * @param phys The physics simulation
 * @param x1 The starting X point of the line
 * @param y1 The starting Y point of the line
 * @param x2 The ending X point of the line
 * @param y2 The ending Y point of the line
 * @param isTerrain true if this is terrain, false otherwise
 * @return The line, also saved in the argument phys
 */
physLine_t* physAddLine(physSim_t* phys, float x1, float y1, float x2, float y2, bool isTerrain)
{
    // Allocate the line
    physLine_t* pl = heap_caps_calloc(1, sizeof(physLine_t), MALLOC_CAP_8BIT);

    // Save the points of the line
    pl->l.p1.x = x1;
    pl->l.p1.y = y1;
    pl->l.p2.x = x2;
    pl->l.p2.y = y2;

    // Set physical properties based on the points
    updateLineProperties(phys, pl);

    // Save terrain info
    pl->isTerrain   = isTerrain;
    pl->destination = pl->l;

    // Push line into list in the simulation
    push(&phys->lines, pl);

    // Return the line
    return pl;
}

/**
 * @brief TODO doc
 *
 * @param phys
 * @param pl
 */
void updateLineProperties(physSim_t* phys, physLine_t* pl)
{
    // Find the unit slope
    vecFl_t unitSlope = normVecFl2d(subVecFl2d(pl->l.p2, pl->l.p1));

    // Rotate the unit slope to get the unit normal, making sure it points upwards
    if (unitSlope.x < 0)
    {
        pl->unitNormal.x = -unitSlope.y;
        pl->unitNormal.y = unitSlope.x;
    }
    else
    {
        pl->unitNormal.x = unitSlope.y;
        pl->unitNormal.y = -unitSlope.x;
    }

    // Store what zones this line is in
    physSetZoneMaskLine(phys, pl);
}

/**
 * @brief Add a circle to the physics simulation. It may be mobile or fixed/
 *
 * @param phys The physics simulation
 * @param x1 The X point of the center of the circle
 * @param y1 The Y point of the center of the circle
 * @param r The radius of the circle
 * @param type The type of circle. CT_OBSTACLE is immobile, others are mobile.
 * @return The circle, also saved in the argument phys
 */
physCirc_t* physAddCircle(physSim_t* phys, float x1, float y1, float r, circType_t type)
{
    // Allocate the circle
    physCirc_t* pc = heap_caps_calloc(1, sizeof(physCirc_t), MALLOC_CAP_8BIT);

    // Save the parameters
    pc->c.pos.x  = x1;
    pc->c.pos.y  = y1;
    pc->c.radius = r;
    pc->type     = type;

    // Additional initialization based on type
    switch (type)
    {
        case CT_TANK:
        {
            setBarrelAngle(pc, 0);
            setShotPower(pc, 0.0001f);
            pc->fixed = false;
            break;
        }
        case CT_SHELL:
        {
            pc->fixed           = false;
            pc->explosionRadius = r * 5;
            break;
        }
        default:
        case CT_OBSTACLE:
        {
            pc->fixed = true;
            break;
        }
    }

    // Store what zones the circle is in
    physSetZoneMaskCirc(phys, pc);

    // Push the circle into a list in the simulation
    push(&phys->circles, pc);

    // Return the circle
    return pc;
}

/**
 * @brief Calculate what zones a line is in and save it for that line
 *
 * @param phys The physics simulation
 * @param pl The line to calculate zones for
 */
void physSetZoneMaskLine(physSim_t* phys, physLine_t* pl)
{
    // Clear the zone mask
    pl->zonemask = 0;

    // Check each zone and set the bit if the line is in the zone
    for (int32_t zIdx = 0; zIdx < NUM_ZONES; zIdx++)
    {
        if (rectLineFlIntersection(phys->zones[zIdx], pl->l, NULL))
        {
            pl->zonemask |= (1 << zIdx);
        }
    }
}

/**
 * @brief Calculate what zones a circle is in and save it for that circle
 *
 * @param phys The physics simulation
 * @param pc The circle to calculate zones for
 */
void physSetZoneMaskCirc(physSim_t* phys, physCirc_t* pc)
{
    // Clear the zone mask
    pc->zonemask = 0;

    // Check each zone and set the bit if the circle is in the zone
    for (int32_t zIdx = 0; zIdx < NUM_ZONES; zIdx++)
    {
        if (circleRectFlIntersection(pc->c, phys->zones[zIdx], NULL))
        {
            pc->zonemask |= (1 << zIdx);
        }
    }
}

/**
 * @brief Update the entire physics simulation by updating object position, velocity, and acceleration. This checks for
 * object collisions and doesn't allow object to clip into each other.
 *
 * @param phys The physics simulation
 * @param elapsedUs The time elapsed since this was last called
 */
void physStep(physSim_t* phys, int32_t elapsedUs)
{
    physUpdateTimestep(phys, elapsedUs);
    physCheckCollisions(phys);
    physBinaryMoveObjects(phys);
    checkTurnOver(phys);
}

/**
 * @brief Update each object's acceleration, velocity, and desired next position based on elapsedUs. This doesn't move
 * objects yet so objects will not clip into each other.
 *
 * @param phys The physics simulation
 * @param elapsedUs The time elapsed since this was last called
 */
void physUpdateTimestep(physSim_t* phys, int32_t elapsedUs)
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
            physSetZoneMaskCirc(phys, pc);
        }

        // Iterate
        cNode = cNode->next;
    }

    phys->terrainMoving = moveTerrainPoints(phys, elapsedUs);
}

/**
 * @brief Check for collisions between moving objects (circles with current and desired positions) and fixed objects
 * (circles or lines). This may update the velocity of an object after a collision but will not update the position.
 *
 * @param phys The physics simulation
 */
void physCheckCollisions(physSim_t* phys)
{
    // For each circle
    bool shouldRemoveNode = false;
    node_t* cNode         = phys->circles.first;
    while (cNode)
    {
        // Get a convenience pointer
        physCirc_t* pc = (physCirc_t*)cNode->val;

        // If it's not fixed in space
        if (!pc->fixed)
        {
            // Keep track of the collision closest to the starting point of the circle
            float colDist = FLT_MAX;
            // Keep track of the reflection vector in case of collision
            vecFl_t reflVec = {0};
            // Keep track if the object should bounce or not
            bool countBounce = true;

            // Check for collisions with lines
            node_t* oln = phys->lines.first;
            while (oln)
            {
                // Get a convenience pointer
                physLine_t* opl = (physLine_t*)oln->val;

                // Quick check to see if objects are in the same zone
                if (pc->zonemask & opl->zonemask)
                {
                    // Used to determine which component of the line collided
                    float minLineDist      = FLT_MAX;
                    vecFl_t lineUnitNormal = {0}; // Points upward from the line

                    ////////////////////////////////////////////////////////////

                    // Construct the bounding lines around this line (parallel lines, one radius away)
                    lineFl_t bounds[] = {
                        {
                            .p1 = addVecFl2d(opl->l.p1, mulVecFl2d(opl->unitNormal, pc->c.radius)),
                            .p2 = addVecFl2d(opl->l.p2, mulVecFl2d(opl->unitNormal, pc->c.radius)),
                        },
                        {
                            .p1 = addVecFl2d(opl->l.p1, mulVecFl2d(opl->unitNormal, -pc->c.radius)),
                            .p2 = addVecFl2d(opl->l.p2, mulVecFl2d(opl->unitNormal, -pc->c.radius)),
                        },
                    };

                    // Check for intersections between circle's travel line and bounds lines
                    for (int32_t idx = 0; idx < ARRAY_SIZE(bounds); idx++)
                    {
                        vecFl_t intersection = {0};
                        if (lineLineFlIntersection(bounds[idx], pc->travelLine, &intersection))
                        {
                            // There was an intersection, find the distance from the starting point to the intersection
                            float dist = sqMagVecFl2d(subVecFl2d(intersection, pc->travelLine.p1));

                            if (dist < minLineDist)
                            {
                                minLineDist    = dist;
                                lineUnitNormal = mulVecFl2d(opl->unitNormal, (1 == idx) ? -1 : 1);
                            }
                        }
                    }

                    ////////////////////////////////////////////////////////////

                    // Construct bounds around the line endcaps
                    // TODO for polylines and polygons, we could skip every other endcap
                    circleFl_t caps[] = {
                        {
                            .pos    = opl->l.p1,
                            .radius = 0,
                        },
                        {
                            .pos    = opl->l.p2,
                            .radius = 0,
                        },
                    };

                    // Check bounding endcaps
                    for (int32_t cIdx = 0; cIdx < ARRAY_SIZE(caps); cIdx++)
                    {
                        // Check if this intersection is closer than a line segment intersection
                        physCircCircIntersection(phys, pc, &caps[cIdx], &minLineDist, &lineUnitNormal);
                    }

                    ////////////////////////////////////////////////////////////

                    // Now that all the line components are checked, apply the closest one
                    if (minLineDist < colDist)
                    {
                        colDist     = minLineDist;
                        reflVec     = lineUnitNormal;
                        countBounce = true;
                    }
                }

                // Iterate to next line
                oln = oln->next;
            }

            // Check for collisions with circles
            node_t* ocn = phys->circles.first;
            while (ocn)
            {
                // Convenience pointer
                physCirc_t* opc = ocn->val;

                float circColDist      = FLT_MAX;
                vecFl_t circUnitNormal = {0};

                if ((opc != pc) &&                    // Don't collide a circle with itself
                    (opc->zonemask & pc->zonemask) && // make sure they're in the same zone
                    (pc->type != opc->type) &&        // Types should differ (shells don't hit shells, etc.)
                    physCircCircIntersection(phys, pc, &opc->c, &circColDist, &circUnitNormal))
                {
                    // TODO do I care if the other circle isn't fixed in space?
                    if (CT_SHELL == pc->type && CT_TANK == opc->type)
                    {
                        // Shells explode
                        shouldRemoveNode = true;
                    }
                    else
                    {
                        // If this is a closer collision than others
                        if (circColDist < colDist)
                        {
                            // Save it
                            colDist     = circColDist;
                            reflVec     = circUnitNormal;
                            countBounce = (CT_OBSTACLE != opc->type);
                        }
                    }
                }

                // Iterate to the next circle
                ocn = ocn->next;
            }

            // After checking all lines and circles, if there was an intersection
            // (there will only be one, closest to the starting point)
            if (FLT_MAX != colDist)
            {
                // Shells have a limited number of bounces
                if (countBounce && CT_SHELL == pc->type)
                {
                    pc->bounces--;
                    if (0 == pc->bounces)
                    {
                        shouldRemoveNode = true;
                    }
                }

                // Bounce it by reflecting across this vector
                pc->vel = subVecFl2d(pc->vel, mulVecFl2d(reflVec, (2 * dotVecFl2d(pc->vel, reflVec))));

                // Dampen after bounce
                pc->vel = mulVecFl2d(pc->vel, 0.75f);

                // Deadband the velocity if it's small enough
                if (sqMagVecFl2d(pc->vel) < 1e-9f)
                {
                    pc->vel.x = 0;
                    pc->vel.y = 0;
                }

                // If the circle is on top of an object (i.e. reflection vector points upward)
                if (reflVec.y < 0)
                {
                    pc->inContact = true;

                    // Unit slope is the unit normal rotated 90 degrees, pointed downward
                    if (reflVec.x < 0)
                    {
                        pc->slopeVec.x = reflVec.y;
                        pc->slopeVec.y = -reflVec.x;
                    }
                    else
                    {
                        pc->slopeVec.x = -reflVec.y;
                        pc->slopeVec.y = reflVec.x;
                    }

                    // Project the gravity vector onto the unit slope to find the static force moving this
                    // object proj(a onto b) == (b) * ((a dot b) / (b dot b)) But b is a unit vector, so (b dot
                    // b) is just 2
                    pc->staticForce = mulVecFl2d(pc->slopeVec, dotVecFl2d(phys->g, pc->slopeVec) / 2.0f);

                    // Just in case, don't float upwards
                    if (pc->staticForce.y < 0)
                    {
                        pc->staticForce.y = 0;
                    }
                }
            }
            // No collision but the object is still in contact with something
            else if (pc->inContact)
            {
                // Shift it downward a half pixel just to see if it's still in contact
                // This is because the circle will never clip into an object,
                // and we don't apply a constant downward force to keep contact.
                // A constant downward force messes with lateral movement inputs
                pc->c.pos.y += 0.5f;
                pc->inContact = physAnyCollision(phys, pc);
                pc->c.pos.y -= 0.5f;
            }
        }

        // Iterate to next moving circle
        node_t* nextNode = cNode->next;

        // Optionally remove this entry (if a shell exploded)
        if (shouldRemoveNode)
        {
            explodeShell(phys, cNode);
            shouldRemoveNode = false;
        }
        cNode = nextNode;
    }
}

/**
 * @brief TODO
 *
 * @param phys
 * @param shell
 */
void explodeShell(physSim_t* phys, node_t* shellNode)
{
    physCirc_t* shell = shellNode->val;

    // Calculate the X bounds of the explosion and the squared radius
    float expMin = shell->c.pos.x - shell->explosionRadius;
    float expMax = shell->c.pos.x + shell->explosionRadius;
    float rSq    = (shell->explosionRadius * shell->explosionRadius);

    // Iterate through all lines
    node_t* lNode = phys->lines.first;
    while (lNode)
    {
        physLine_t* line = lNode->val;
        // If this is terrain
        if (line->isTerrain)
        {
            // Attempt to deform points
            line->destination.p1.y = deformTerrainPoint(&line->l.p1, &shell->c.pos, rSq, expMin, expMax);
            line->destination.p2.y = deformTerrainPoint(&line->l.p2, &shell->c.pos, rSq, expMin, expMax);
        }
        // Iterate
        lNode = lNode->next;
    }

    // Unset the camera target if it was tracking this shell
    if (shell == phys->cameraTarget)
    {
        phys->cameraTarget = NULL;
    }

    heap_caps_free(removeEntry(&phys->circles, shellNode));
    phys->terrainMoving = true;
}

/**
 * @brief TODO
 *
 * @param p
 * @param expPnt
 * @param rSq
 * @param expMin
 * @param expMax
 * @return float
 */
float deformTerrainPoint(vecFl_t* p, vecFl_t* expPnt, float rSq, float expMin, float expMax)
{
    if (expMin <= p->x && p->x < expMax)
    {
        // X distance from shell to point to adjust
        float xDist = expPnt->x - p->x;
        // Explosion Y size at the point to adjust
        float ySz = sqrtf(rSq - (xDist * xDist));
        // Top and bottom points of the explosion
        float expTop    = expPnt->y - ySz;
        float expBottom = expPnt->y + ySz;

        if (p->y < expTop)
        {
            // Explosion entirely underground, add full explosion size
            return p->y + (2 * ySz);
        }
        else if (p->y <= expBottom)
        {
            // Point somewhere in explosion, adjust terrain to point
            return expBottom;
        }
    }
    // Explosion doesn't affect point
    return p->y;
}

/**
 * @brief TODO
 *
 * @param phys
 * @param elapsedUs TODO use elapsedUs
 * @return true
 * @return false
 */
bool moveTerrainPoints(physSim_t* phys, int32_t elapsedUs)
{
    // Keep track if any terrain is moving anywhere
    bool terrainMoving = false;

    // Iterate through all lines
    node_t* lNode = phys->lines.first;
    while (lNode)
    {
        physLine_t* line = lNode->val;
        // If the line is terrain
        if (line->isTerrain)
        {
            // Track if this line is moving or needs a properties update
            bool lineIsMoving = false;
            bool updateLine   = false;

            if (line->destination.p1.y == line->l.p1.y)
            {
                // Do nothing
            }
            else if (ABS(line->destination.p1.y - line->l.p1.y) < 1)
            {
                // Less than one pixel off, clamp it
                line->l.p1.y = line->destination.p1.y;
                updateLine   = true;
            }
            else if (line->destination.p1.y > line->l.p1.y)
            {
                // Move p1 towards destination
                line->l.p1.y++;
                terrainMoving = true;
                lineIsMoving  = true;
            }
            else if (line->destination.p1.y < line->l.p1.y)
            {
                // Move p1 towards destination
                line->l.p1.y--;
                terrainMoving = true;
                lineIsMoving  = true;
            }

            if (line->destination.p2.y == line->l.p2.y)
            {
                // Do nothing
            }
            else if (ABS(line->destination.p2.y - line->l.p2.y) < 1)
            {
                // Less than one pixel off, clamp it
                line->l.p2.y = line->destination.p2.y;
                updateLine   = true;
            }
            else if (line->destination.p2.y > line->l.p2.y)
            {
                // Move p2 towards destination
                line->l.p2.y++;
                terrainMoving = true;
                lineIsMoving  = true;
            }
            else if (line->destination.p2.y < line->l.p2.y)
            {
                // Move p1 towards destination
                line->l.p2.y--;
                terrainMoving = true;
                lineIsMoving  = true;
            }

            // If the line needs a properties update and has finished moving both points
            if (updateLine && !lineIsMoving)
            {
                // Update slope and normals and such
                updateLineProperties(phys, line);
            }
        }
        // Iterate
        lNode = lNode->next;
    }

    // Return if any terrain anywhere is moving
    return terrainMoving;
}

/**
 * @brief Check if a shot is done (i.e. all shells have been removed)
 *
 * @param phys The physics simulation
 */
void checkTurnOver(physSim_t* phys)
{
    // If there is a shot in progress and the turn isn't over yet
    if (phys->shotFired && !phys->turnOver)
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
        phys->turnOver = (false == anyShells && false == phys->terrainMoving);

        // TODO set timer to switch turn
    }
}

/**
 * @brief Update the position of all moving objects in the simulation. This will move objects from their current to
 * their desired positions as best as possible, without clipping objects. Binary search is used to find the best final
 * destination.
 *
 * @param phys The physics simulation
 */
void physBinaryMoveObjects(physSim_t* phys)
{
    // For each circle
    node_t* cNode = phys->circles.first;
    while (cNode)
    {
        // Convenience pointer
        physCirc_t* pc = (physCirc_t*)cNode->val;

        // If it's not fixed in space
        if (!pc->fixed)
        {
            // Test at the final destination
            pc->c.pos = pc->travelLine.p2;
            physSetZoneMaskCirc(phys, pc);

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
                    physSetZoneMaskCirc(phys, pc);

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
                    physSetZoneMaskCirc(phys, pc);
                }
            }
        }

        // Iterate
        cNode = cNode->next;
    }
}

/**
 * @brief Test if a given circle is colliding with any object
 *
 * @param phys The physics simulation
 * @param c The circle to check collisions for
 * @return true if the circle is colliding with anything, false otherwise
 */
bool physAnyCollision(physSim_t* phys, physCirc_t* c)
{
    // Check against all other circles
    node_t* cNode = phys->circles.first;
    while (cNode)
    {
        physCirc_t* cOther = (physCirc_t*)cNode->val;

        if (cOther != c &&                                           // Don't collide with itself
            cOther->type != c->type &&                               // Don't collide if the type matches
            cOther->zonemask & c->zonemask &&                        // Don't collide if not in the same zone
            circleCircleFlIntersection(c->c, cOther->c, NULL, NULL)) // Collision check
        {
            return true;
        }
        cNode = cNode->next;
    }

    node_t* lNode = phys->lines.first;
    while (lNode)
    {
        physLine_t* lOther = (physLine_t*)lNode->val;
        if (lOther->zonemask & c->zonemask &&                            // Don't collide if not in the same zone
            circleLineFlIntersection(c->c, lOther->l, true, NULL, NULL)) // Collision check
        {
            return true;
        }
        lNode = lNode->next;
    }
    return false;
}

/**
 * @brief Check for intersections between a moving circle, represented by a line segment, and a fixed circle. The
 * fixed circle may have radius 0, which indicates a point
 *
 * @param cMoving [IN] The moving circle
 * @param cOther [IN] The fixed circle
 * @param colDist [IN/OUT] The distance between the moving circle's position and the collision point
 * @param reflVec [OUT] The vector to reflect the moving circle's velocity across if the two collided
 * @return true if there was a collision, false if there was not
 */
bool physCircCircIntersection(physSim_t* phys, physCirc_t* cMoving, circleFl_t* cOther, float* colDist,
                              vecFl_t* reflVec)
{
    // A line intersecting with a circle may intersect up to two places
    vecFl_t intersections[2];

    // This circle is where collisions may occur
    circleFl_t boundaryCircle = {
        .pos    = cOther->pos,
        .radius = cOther->radius + cMoving->c.radius,
    };

    // Check for intersections between the boundary circle and travel line
    int16_t iCnt
        = circleLineSegFlIntersection(boundaryCircle, cMoving->travelLine, cMoving->travelLineBB, intersections);

    // For each intersection
    for (int16_t iIdx = 0; iIdx < iCnt; iIdx++)
    {
        // find the distance from the starting point to the intersection
        float dist = sqMagVecFl2d(subVecFl2d(intersections[iIdx], cMoving->travelLine.p1));

        // If this is the closest point
        if (dist < *colDist)
        {
            // Save the distance and the point
            *colDist = dist;

            // Set the reflection vector for this line
            *reflVec = normVecFl2d(subVecFl2d(cMoving->c.pos, cOther->pos));

            // Better collision detected
            return true;
        }
    }

    // No collision
    return false;
}

/**
 * @brief Draw the outlines of physics simulation objects
 *
 * @param phys The physics simulation
 */
void drawPhysOutline(physSim_t* phys)
{
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

        // Camera track the middle shell
        if (shellCount == numShells / 2)
        {
            phys->cameraTarget = shell;
        }
    }
}

/**
 * @brief Save the button state in order to pan the camera
 *
 * @param phys The physics simulation to pan
 * @param btn The current button state
 */
void physSetCameraButton(physSim_t* phys, buttonBit_t btn)
{
    phys->cameraBtn = btn;
}

/**
 * @brief Move the camera according to input buttons or object tracking
 *
 * TODO make this not suck when firing multiple shells
 *
 * @param phys The physics simulation to pan
 */
void physAdjustCameraTimer(physSim_t* phys)
{
    if (NULL == phys->cameraTarget)
    {
        // If there's no camera target, move according to button input, vertically
        if (PB_UP & phys->cameraBtn)
        {
            phys->camera.y -= CAMERA_BTN_MOVE_INTERVAL;
        }
        else if (PB_DOWN & phys->cameraBtn)
        {
            phys->camera.y += CAMERA_BTN_MOVE_INTERVAL;
        }

        // Horizontally
        if (PB_LEFT & phys->cameraBtn)
        {
            phys->camera.x -= CAMERA_BTN_MOVE_INTERVAL;
        }
        else if (PB_RIGHT & phys->cameraBtn)
        {
            phys->camera.x += CAMERA_BTN_MOVE_INTERVAL;
        }
    }
    else
    {
        // Move the camera to track a target
        // This is where the target would be displayed
        vec_t targetDispAt = {
            .x = phys->cameraTarget->c.pos.x - phys->camera.x,
            .y = phys->cameraTarget->c.pos.y - phys->camera.y,
        };

        // This is where we want the camera to be, may be adjusted
        vec_t desiredCamera = phys->camera;

        // Make sure the target is in the viewbox horizontally
        if (targetDispAt.x < CAMERA_MARGIN)
        {
            desiredCamera.x = phys->cameraTarget->c.pos.x - CAMERA_MARGIN;
        }
        else if (targetDispAt.x > TFT_WIDTH - CAMERA_MARGIN)
        {
            desiredCamera.x = (phys->cameraTarget->c.pos.x) - (TFT_WIDTH - CAMERA_MARGIN);
        }

        // Make sure the target is in the viewbox vertically
        if (targetDispAt.y < CAMERA_MARGIN)
        {
            desiredCamera.y = phys->cameraTarget->c.pos.y - CAMERA_MARGIN;
        }
        else if (targetDispAt.y > TFT_HEIGHT - CAMERA_MARGIN)
        {
            desiredCamera.y = (phys->cameraTarget->c.pos.y) - (TFT_HEIGHT - CAMERA_MARGIN);
        }

        // Move camera a third of the way to desired camera
        phys->camera = addVec2d(phys->camera, divVec2d(subVec2d(desiredCamera, phys->camera), 3));
    }

    // Bound the camera to the world
    // phys->camera.x = CLAMP(phys->camera.x, 0, (phys->bounds.x - TFT_WIDTH));
    // phys->camera.y = CLAMP(phys->camera.y, 0, (phys->bounds.y - TFT_HEIGHT));
}

/**
 * @brief Run a timer to adjust the camera, either tracking an object or reacting to button presses
 *
 * @param phys The physics simulation to pan
 * @param elapsedUs The elapsed time
 */
void physAdjustCamera(physSim_t* phys, uint32_t elapsedUs)
{
    RUN_TIMER_EVERY(phys->cameraTimer, 1000000 / 60, elapsedUs, physAdjustCameraTimer(phys););
}
