//==============================================================================
// Includes
//==============================================================================

#include <math.h>
#include <float.h>
#include <stdio.h>

#include <esp_heap_caps.h>
#include <esp_log.h>

#include "shapes.h"
#include "palette.h"
#include "artillery_phys.h"
#include "macros.h"

//==============================================================================
// Defines
//==============================================================================

#define PRINT_P1_VEC(label, vec)                                          \
    do                                                                    \
    {                                                                     \
        if (cNode == phys->circles.first)                                 \
        {                                                                 \
            ESP_LOGI("VEC", "%s: %.15f, %.15f", label, (vec).x, (vec.y)); \
        }                                                                 \
    } while (0)

//==============================================================================
// Function Declarations
//==============================================================================

void physSetZoneMaskLine(physSim_t* phys, physLine_t* pl);
void physSetZoneMaskCirc(physSim_t* phys, physCirc_t* pc);
void physMoveObjects(physSim_t* phys, int32_t elapsedUs);
void physCheckCollisions(physSim_t* phys);

bool physCircCircIntersection(physSim_t* phys, physCirc_t* cMoving, circleFl_t* cOther, float* colDist,
                              vecFl_t* colPoint, vecFl_t* reflVec);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief TODO doc
 *
 * @param w Physics space width, unit is px
 * @param h Physics space height, unit is px
 * @param gx Gravity in the X direction, unit is px/uS^2
 * @param gy Gravity in the Y direction, unit is px/uS^2
 * @return physSim_t*
 */
physSim_t* initPhys(float w, float h, float gx, float gy)
{
    physSim_t* phys = heap_caps_calloc(1, sizeof(physSim_t), MALLOC_CAP_8BIT);

    // Set gravity
    phys->g.x  = gx;
    phys->g.y  = gy;
    phys->gMag = magVecFl2d(phys->g);

    // Set bounds for the physics sim
    phys->bounds.x = w;
    phys->bounds.y = h;

    // Figure out zones
    int32_t zonesX, zonesY;
    if (phys->bounds.x > phys->bounds.y)
    {
        zonesX = NUM_ZONES_BIG;
        zonesY = NUM_ZONES_LIL;
    }
    else
    {
        zonesX = NUM_ZONES_LIL;
        zonesY = NUM_ZONES_BIG;
    }

    // Calculate the zone size
    vecFl_t zoneSize = {
        .x = phys->bounds.x / (float)zonesX,
        .y = phys->bounds.y / (float)zonesY,
    };

    // Create the zones
    for (int32_t y = 0; y < zonesY; y++)
    {
        for (int32_t x = 0; x < zonesX; x++)
        {
            rectangleFl_t* zone = &phys->zones[(y * zonesX) + x];
            zone->pos.x         = x * zoneSize.x;
            zone->pos.y         = y * zoneSize.y;
            zone->width         = zoneSize.x;
            zone->height        = zoneSize.y;
        }
    }

    // Add lines for the world bounds
    physAddLine(phys, 0, 0, w, 0);
    physAddLine(phys, w, 0, w, h);
    physAddLine(phys, w, h, 0, h);
    physAddLine(phys, 0, h, 0, 0);

    return phys;
}

/**
 * @brief TODO doc
 *
 * @param phys The physics simulation
 */
void deinitPhys(physSim_t* phys)
{
    while (phys->lines.first)
    {
        heap_caps_free(pop(&phys->lines));
    }

    while (phys->circles.first)
    {
        heap_caps_free(pop(&phys->circles));
    }

    heap_caps_free(phys);
}

/**
 * @brief TODO doc
 *
 * @param phys The physics simulation
 * @param x1
 * @param y1
 * @param x2
 * @param y2
 * @return
 */
physLine_t* physAddLine(physSim_t* phys, float x1, float y1, float x2, float y2)
{
    // Make space for the line
    physLine_t* pl = heap_caps_calloc(1, sizeof(physLine_t), MALLOC_CAP_8BIT);

    // Save the points of the line
    pl->l.p1.x = x1;
    pl->l.p1.y = y1;
    pl->l.p2.x = x2;
    pl->l.p2.y = y2;

    // Calculate unit normal vector (perpendicular to slope)
    float unx        = y1 - y2;
    float uny        = x2 - x1;
    float magnitude  = sqrtf((unx * unx) + (uny * uny));
    pl->unitNormal.x = unx / magnitude;
    pl->unitNormal.y = uny / magnitude;

    // Make sure it points up
    if (pl->unitNormal.y > 0)
    {
        pl->unitNormal.y = -pl->unitNormal.y;
        pl->unitNormal.x = -pl->unitNormal.x;
    }

    // Get the unit slope by rotating the normal vector
    pl->unitSlope.x = pl->unitNormal.y;
    pl->unitSlope.y = -pl->unitNormal.x;

    // Make sure the unit slope points downward
    if (pl->unitSlope.y < 0)
    {
        pl->unitSlope.x = -pl->unitSlope.x;
        pl->unitSlope.y = -pl->unitSlope.y;
    }

    // Store what zones this line is in
    physSetZoneMaskLine(phys, pl);

    // Push line into list
    push(&phys->lines, pl);
    return pl;
}

/**
 * @brief TODO doc
 *
 * @param phys The physics simulation
 * @param x1
 * @param y1
 * @param r
 * @param type
 * @return
 */
physCirc_t* physAddCircle(physSim_t* phys, float x1, float y1, float r, circType_t type)
{
    physCirc_t* pc = heap_caps_calloc(1, sizeof(physCirc_t), MALLOC_CAP_8BIT);
    pc->c.pos.x    = x1;
    pc->c.pos.y    = y1;
    pc->c.radius   = r;
    pc->type       = type;
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
            pc->fixed = false;
            break;
        }
        default:
        case CT_OBSTACLE:
        {
            pc->fixed = true;
            break;
        }
    }
    physSetZoneMaskCirc(phys, pc);
    push(&phys->circles, pc);
    return pc;
}

/**
 * @brief TODO doc
 *
 * @param phys The physics simulation
 * @param line
 */
void physSetZoneMaskLine(physSim_t* phys, physLine_t* pl)
{
    pl->zonemask = 0;
    for (int32_t zIdx = 0; zIdx < NUM_ZONES; zIdx++)
    {
        if (rectLineFlIntersection(phys->zones[zIdx], pl->l, NULL))
        {
            pl->zonemask |= (1 << zIdx);
        }
    }
}

/**
 * @brief TODO doc
 *
 * @param phys The physics simulation
 * @param pc
 */
void physSetZoneMaskCirc(physSim_t* phys, physCirc_t* pc)
{
    pc->zonemask = 0;
    for (int32_t zIdx = 0; zIdx < NUM_ZONES; zIdx++)
    {
        if (circleRectFlIntersection(pc->c, phys->zones[zIdx], NULL))
        {
            pc->zonemask |= (1 << zIdx);
        }
    }
}

/**
 * @brief TODO doc
 *
 * @param phys The physics simulation
 * @param elapsedUs
 */
void physStep(physSim_t* phys, int32_t elapsedUs)
{
    physMoveObjects(phys, elapsedUs);
    physCheckCollisions(phys);
}

/**
 * @brief TODO doc
 *
 * @param phys The physics simulation
 * @param elapsedUs
 */
void physMoveObjects(physSim_t* phys, int32_t elapsedUs)
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

            // If the object is moving
            vecFl_t movingAcc = {0};
            if (pc->moving)
            {
                if (PB_LEFT == pc->moving)
                {
                    // TODO move left along static force vector, or just closest one?
                    movingAcc.x = -0.000000001f;
                }
                else if (PB_RIGHT == pc->moving)
                {
                    // TODO move right along static force vector, or just closest one?
                    movingAcc.x = 0.000000001f;
                }
            }

            // Pick which forces are acting on the object
            vecFl_t totalForce;
            if (pc->inContact)
            {
                // Touching something, so static forces
                totalForce = addVecFl2d(pc->g, pc->staticForce);
                // Only apply movement when in contact with something else
                totalForce = addVecFl2d(totalForce, movingAcc);
            }
            else
            {
                // Not touching anything, so world gravity
                totalForce = addVecFl2d(pc->g, phys->g);
            }

            // Calculate new velocity
            pc->vel = addVecFl2d(pc->vel, mulVecFl2d(totalForce, elapsedUs));

            // Calculate new position
            pc->c.pos = addVecFl2d(pc->c.pos, mulVecFl2d(pc->vel, elapsedUs));

            // Set ending point
            pc->travelLine.p2 = pc->c.pos;

            // Set bounding box of travel line, for later checks
            pc->travelLineBB = getLineBoundingBox(pc->travelLine);

            // Recompute zones
            physSetZoneMaskCirc(phys, pc);
        }

        // Iterate
        cNode = cNode->next;
    }
}

/**
 * @brief Check for physics collisions, move objects to not clip into each other, update velocity vectors based on
 * collisions
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
            float colDist    = FLT_MAX;
            vecFl_t colPoint = {0};
            vecFl_t reflVec  = {0};

            // Zero the normal forces before checking for collisions
            pc->staticForce.x = 0;
            pc->staticForce.y = 0;
            pc->inContact     = false;

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
                    vecFl_t lineColPoint   = {0}; // The position of the circle when the collision with the line occurs
                    vecFl_t lineUnitNormal = {0}; // Points outward from the line
                    vecFl_t lineUnitSlope  = {0}; // Points downward along the slope of the line

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
                                lineColPoint   = intersection;
                                lineUnitNormal = mulVecFl2d(opl->unitNormal, (1 == idx) ? -1 : 1);
                                lineUnitSlope  = opl->unitSlope;
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
                        // If this intersection is closer than a line segment intersection
                        if (physCircCircIntersection(phys, pc, &caps[cIdx], &minLineDist, &lineColPoint,
                                                     &lineUnitNormal))
                        {
                            if (lineUnitNormal.x < 0)
                            {
                                lineUnitSlope.x = lineUnitNormal.y;
                                lineUnitSlope.y = -lineUnitNormal.x;
                            }
                            else
                            {
                                lineUnitSlope.x = -lineUnitNormal.y;
                                lineUnitSlope.y = lineUnitNormal.x;
                            }
                        }
                    }

                    ////////////////////////////////////////////////////////////

                    // Now that all the segments are checked, apply the closest one
                    if (minLineDist < colDist)
                    {
                        colDist  = minLineDist;
                        colPoint = lineColPoint;
                        reflVec  = addVecFl2d(reflVec, lineUnitNormal);

                        // If the point of collision is above the circle's position
                        // i.e. the circle moved downward, then collided, so the intersection point moves the circle
                        // back upward
                        if (lineColPoint.y < pc->c.pos.y)
                        {
                            // Project the gravity vector onto the unit slope to find the static force moving this
                            // object proj(a onto b) == (b) * ((a dot b) / (b dot b)) But b is a unit vector, so (b dot
                            // b) is just 2
                            vecFl_t projected = mulVecFl2d(lineUnitSlope, dotVecFl2d(phys->g, lineUnitSlope) / 2.0f);

                            // Sum the static force
                            pc->staticForce = addVecFl2d(pc->staticForce, projected);
                            pc->inContact   = true;
                        }
                    }
                }

                // Iterate to next line
                oln = oln->next;
            }

            // Check for collisions with circles
            node_t* ocn = phys->circles.first;
            while (ocn)
            {
                physCirc_t* opc = ocn->val;

                float circColDist      = FLT_MAX;
                vecFl_t circColPoint   = {0};
                vecFl_t circUnitNormal = {0};

                if ((opc != pc) &&                    // Don't collide a circle with itself
                    (opc->zonemask & pc->zonemask) && // make sure they're in the same zone
                    (pc->type != opc->type) &&        // Types should differ (shells don't hit shells, etc.)
                    physCircCircIntersection(phys, pc, &opc->c, &circColDist, &circColPoint, &circUnitNormal))
                {
                    // TODO do I care if the other circle isn't fixed in space?
                    if (CT_SHELL == pc->type && CT_TANK == opc->type)
                    {
                        // Shells explode
                        shouldRemoveNode = true;
                    }
                    else
                    {
                        // Now that all the segments are checked, apply the closet one
                        if (circColDist < colDist)
                        {
                            colDist  = circColDist;
                            colPoint = circColPoint;
                            reflVec  = addVecFl2d(reflVec, circUnitNormal);

                            // If the point of collision is above the circle's position
                            // i.e. the circle moved downward, then collided, so the intersection point moves the circle
                            // back upward
                            if (circColPoint.y < pc->c.pos.y)
                            {
                                vecFl_t circUnitSlope;
                                if (circUnitNormal.x < 0)
                                {
                                    circUnitSlope.x = circUnitNormal.y;
                                    circUnitSlope.y = -circUnitNormal.x;
                                }
                                else
                                {
                                    circUnitSlope.x = -circUnitNormal.y;
                                    circUnitSlope.y = circUnitNormal.x;
                                }

                                // Project the gravity vector onto the unit slope to find the static force moving this
                                // object proj(a onto b) == (b) * ((a dot b) / (b dot b)) But b is a unit vector, so (b
                                // dot b) is just 2
                                vecFl_t projected
                                    = mulVecFl2d(circUnitSlope, dotVecFl2d(phys->g, circUnitSlope) / 2.0f);

                                // Sum the static force
                                pc->staticForce = addVecFl2d(pc->staticForce, projected);
                                pc->inContact   = true;
                            }
                        }
                    }
                }
                ocn = ocn->next;
            }

            // After checking all lines, if there was an intersection
            // (there will only be one, closest to the starting point)
            if (FLT_MAX != colDist)
            {
                // Move back EPSILON from the collision point
                vecFl_t travelNorm = normVecFl2d(subVecFl2d(pc->travelLine.p1, pc->travelLine.p2));
                pc->c.pos          = addVecFl2d(colPoint, mulVecFl2d(travelNorm, EPSILON));

                // Bounce it by reflecting across this vector
                // This may be the sum of multiple collisions, so normalize it
                reflVec = normVecFl2d(reflVec);
                pc->vel = subVecFl2d(pc->vel, mulVecFl2d(reflVec, (2 * dotVecFl2d(pc->vel, reflVec))));

                // Dampen after bounce
                pc->vel = mulVecFl2d(pc->vel, 0.75f);

                // Just in case, don't float upwards
                if (pc->staticForce.y < 0)
                {
                    pc->staticForce.y = 0;
                }
            }
        }

        node_t* nextNode = cNode->next;
        if (shouldRemoveNode)
        {
            heap_caps_free(removeEntry(&phys->circles, cNode));
        }
        cNode            = nextNode;
        shouldRemoveNode = false;
    }
}

/**
 * @brief Check for intersections between a moving circle, represented by a line segment, and a fixed circle. The
 * fixed circle may have radius 0, which indicates a point
 *
 * @param cMoving [IN] The moving circle
 * @param cOther [IN] The fixed circle
 * @param colDist [IN/OUT] The distance between the moving circle's position and the collision point
 * @param colPoint [IN/OUT] The position of the moving circle when the circles collide
 * @param reflVec [OUT] The vector to reflect the moving circle's velocity across if the two collided
 * @return true if there was a collision, false if there was not
 */
bool physCircCircIntersection(physSim_t* phys, physCirc_t* cMoving, circleFl_t* cOther, float* colDist,
                              vecFl_t* colPoint, vecFl_t* reflVec)
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
            *colDist  = dist;
            *colPoint = intersections[iIdx];

            // Sum the reflection vector for this line
            *reflVec = normVecFl2d(subVecFl2d(cMoving->c.pos, cOther->pos));

            return true;
        }
    }
    return false;
}

/**
 * @brief TODO doc
 *
 * @param phys The physics simulation
 */
void drawPhysOutline(physSim_t* phys)
{
    node_t* lNode = phys->lines.first;
    while (lNode)
    {
        physLine_t* pl = (physLine_t*)lNode->val;
        drawLine(pl->l.p1.x, pl->l.p1.y, pl->l.p2.x, pl->l.p2.y, c522, 0);
        lNode = lNode->next;
    }

    node_t* cNode = phys->circles.first;
    while (cNode)
    {
        physCirc_t* pc      = (physCirc_t*)cNode->val;
        paletteColor_t cCol = c522;
        if (CT_TANK == pc->type)
        {
            cCol = c335;
        }
        else if (CT_SHELL == pc->type)
        {
            cCol = c550;
        }
        drawCircle(pc->c.pos.x, pc->c.pos.y, pc->c.radius, cCol);
        if (CT_TANK == pc->type)
        {
            vecFl_t absBarrelTip = addVecFl2d(pc->c.pos, pc->relBarrelTip);
            drawLineFast(pc->c.pos.x, pc->c.pos.y, absBarrelTip.x, absBarrelTip.y, c335);
        }
        cNode = cNode->next;
    }
}

/**
 * @brief TODO
 *
 * @param circ
 * @param angle
 */
void setBarrelAngle(physCirc_t* circ, float angle)
{
    circ->barrelAngle = angle;

    circ->relBarrelTip.x = sinf(angle) * circ->c.radius * 2;
    circ->relBarrelTip.y = -cosf(angle) * circ->c.radius * 2;
}

/**
 * @brief TODO
 *
 * @param circ
 * @param power
 */
void setShotPower(physCirc_t* circ, float power)
{
    circ->shotPower = power;
}

/**
 * @brief TODO
 *
 * @param phys
 * @param circ
 * @return physCirc_t*
 */
physCirc_t* fireShot(physSim_t* phys, physCirc_t* circ)
{
    vecFl_t absBarrelTip = addVecFl2d(circ->c.pos, circ->relBarrelTip);
    physCirc_t* shell    = physAddCircle(phys, absBarrelTip.x, absBarrelTip.y, 4, CT_SHELL);

    shell->vel.x = sinf(circ->barrelAngle) * circ->shotPower;
    shell->vel.y = -cosf(circ->barrelAngle) * circ->shotPower;

    return shell;
}