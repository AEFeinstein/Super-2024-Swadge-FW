//==============================================================================
// Includes
//==============================================================================

#include <math.h>
#include <float.h>
#include <stdio.h>
#include "esp_heap_caps.h"
#include "shapes.h"
#include "palette.h"
#include "artillery_phys.h"
#include "macros.h"

//==============================================================================
// Function Declarations
//==============================================================================

void physSetZoneMaskLine(physSim_t* phys, physLine_t* pl);
void physSetZoneMaskCirc(physSim_t* phys, physCirc_t* pc);
void physMoveObjects(physSim_t* phys, int32_t elapsedUs);
void physCheckCollisions(physSim_t* phys);

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
    phys->g.x = gx;
    phys->g.y = gy;

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
 */
void physAddLine(physSim_t* phys, float x1, float y1, float x2, float y2)
{
    // Make space for the line
    physLine_t* pl = heap_caps_calloc(1, sizeof(physLine_t), MALLOC_CAP_8BIT);

    // Save the points of the line
    pl->l.p1.x = x1;
    pl->l.p1.y = y1;
    pl->l.p2.x = x2;
    pl->l.p2.y = y2;

    // Calculate unit normal vector (rotated 90 deg)
    float unx        = y1 - y2;
    float uny        = x2 - x1;
    float magnitude  = sqrtf((unx * unx) + (uny * uny));
    pl->unitNormal.x = unx / magnitude;
    pl->unitNormal.y = uny / magnitude;

    // Store what zones this line is in
    physSetZoneMaskLine(phys, pl);

    // Push line into list
    push(&phys->lines, pl);
}

/**
 * @brief TODO doc
 *
 * @param phys The physics simulation
 * @param x1
 * @param y1
 * @param r
 * @param isFixed
 */
void physAddCircle(physSim_t* phys, float x1, float y1, float r, bool isFixed)
{
    physCirc_t* pc = heap_caps_calloc(1, sizeof(physCirc_t), MALLOC_CAP_8BIT);
    pc->c.pos.x    = x1;
    pc->c.pos.y    = y1;
    pc->c.radius   = r;
    pc->fixed      = isFixed;
    physSetZoneMaskCirc(phys, pc);
    push(&phys->circles, pc);
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

            // Calculate new velocity
            pc->vel = addVecFl2d(pc->vel, mulVecFl2d(addVecFl2d(pc->acc, phys->g), elapsedUs));
            // Calculate new position
            vecFl_t dp = mulVecFl2d(pc->vel, elapsedUs);
            pc->c.pos  = addVecFl2d(pc->c.pos, dp);

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
    node_t* cNode = phys->circles.first;
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

            // Check for collisions with lines
            node_t* oln = phys->lines.first;
            while (oln)
            {
                // Get a convenience pointer
                physLine_t* opl = (physLine_t*)oln->val;

                // Quick check to see if objects are in the same zone
                if (pc->zonemask & opl->zonemask)
                {
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

                            // If this is the closest point
                            if (dist < colDist)
                            {
                                // Save the distance and point
                                colDist  = dist;
                                colPoint = intersection;

                                // Save the reflection vector for this line
                                reflVec = opl->unitNormal;
                                if (1 == idx)
                                {
                                    // Flip depending on which side of the line collided
                                    reflVec.x = -reflVec.x;
                                    reflVec.y = -reflVec.y;
                                }
                            }
                        }
                    }

                    // Construct bounds around the line endcaps
                    // TODO for polylines and polygons, we could skip every other endcap
                    circleFl_t caps[] = {
                        {
                            .pos    = opl->l.p1,
                            .radius = pc->c.radius,
                        },
                        {
                            .pos    = opl->l.p2,
                            .radius = pc->c.radius,
                        },
                    };

                    // Check bounding endcaps
                    for (int32_t cIdx = 0; cIdx < ARRAY_SIZE(caps); cIdx++)
                    {
                        // A line intersecting with a circle may intersect up to two places
                        vecFl_t intersections[2];
                        int16_t iCnt = circleLineSegFlIntersection( //
                            caps[cIdx], pc->travelLine, pc->travelLineBB, intersections);

                        // For each intersection
                        for (int16_t iIdx = 0; iIdx < iCnt; iIdx++)
                        {
                            // find the distance from the starting point to the intersection
                            float dist = sqMagVecFl2d(subVecFl2d(intersections[iIdx], pc->travelLine.p1));

                            // If this is the closest point
                            if (dist < colDist)
                            {
                                // Save the distance and the point
                                colDist  = dist;
                                colPoint = intersections[iIdx];

                                // Save the reflection vector for this line
                                reflVec = normVecFl2d(subVecFl2d(pc->c.pos, caps[cIdx].pos));
                            }
                        }
                    }
                }

                // Iterate to next line
                oln = oln->next;
            }

            // After checking all lines, if there was an intersection
            // (there will only be one, closest to the starting point)
            if (FLT_MAX != colDist)
            {
                // Move back EPSILON from the collision point
                vecFl_t travelNorm = normVecFl2d(subVecFl2d(pc->travelLine.p1, pc->travelLine.p2));
                pc->c.pos          = addVecFl2d(colPoint, mulVecFl2d(travelNorm, EPSILON));

                // Bounce it by reflecting across the collision normal
                pc->vel = subVecFl2d(pc->vel, mulVecFl2d(reflVec, (2 * dotVecFl2d(pc->vel, reflVec))));

                // Dampen after bounce
                pc->vel = mulVecFl2d(pc->vel, 0.9f);
            }
        }

        // Iterate to next circle
        cNode = cNode->next;
    }
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
        drawLine(pl->l.p1.x, pl->l.p1.y, pl->l.p2.x, pl->l.p2.y, c555, 0);
        lNode = lNode->next;
    }

    node_t* cNode = phys->circles.first;
    while (cNode)
    {
        physCirc_t* pc = (physCirc_t*)cNode->val;
        drawCircle(pc->c.pos.x, pc->c.pos.y, pc->c.radius, c555);
        cNode = cNode->next;
    }
}
