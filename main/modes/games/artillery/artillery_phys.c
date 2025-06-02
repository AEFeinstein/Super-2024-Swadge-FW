//==============================================================================
// Includes
//==============================================================================

#include <float.h>
#include "esp_heap_caps.h"
#include "shapes.h"
#include "palette.h"
#include "artillery_phys.h"

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
 * @param phys
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
 * @param phys
 * @param x1
 * @param y1
 * @param x2
 * @param y2
 * @param isFixed
 */
void physAddLine(physSim_t* phys, float x1, float y1, float x2, float y2, bool isFixed)
{
    physLine_t* pl = heap_caps_calloc(1, sizeof(physLine_t), MALLOC_CAP_8BIT);
    pl->l.p1.x     = x1;
    pl->l.p1.y     = y1;
    pl->l.p2.x     = x2;
    pl->l.p2.y     = y2;
    physSetZoneMaskLine(phys, pl);
    push(&phys->lines, pl);
}

/**
 * @brief TODO doc
 *
 * @param phys
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
 * @param phys
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
 * @param phys
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
 * @brief TODO
 *
 * @param phys
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
 * @param phys
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

            // Recompute zones
            physSetZoneMaskCirc(phys, pc);
        }

        // Iterate
        cNode = cNode->next;
    }
}

void physCheckCollisions(physSim_t* phys)
{
    // TODO Check for moving lines?

    // For each circle
    node_t* cNode = phys->circles.first;
    while (cNode)
    {
        physCirc_t* pc = (physCirc_t*)cNode->val;
        // If it's not fixed in space
        if (!pc->fixed)
        {
            /* Pseudocode!
             * 1. Make two parallel projections of the line, each one radius away from the line
             * 2. Intersect travelLine to the two projections, save distance to closest collision if there is one
             * 3. Intersect travelLine to two circles around the endpoints of the line, save distance to closest
             * collision if there is one BONUS: skip identical endpoints?
             * 4. If there was a collision, move back from the closest one along travelLine, plus EPSILON
             * 5. Update velocity for the bounce (reflect around normal vector?)
             */

            // vecFl_t minCDist = {
            //     .x = FLT_MAX,
            //     .y = FLT_MAX,
            // };

            // Check for collisions
            node_t* oln = phys->lines.first;
            while (oln)
            {
                physLine_t* opl = (physLine_t*)oln->val;

                // Quick check to see if objects are in the same zone
                if (pc->zonemask & opl->zonemask)
                {
                    if (circleLineFlIntersection(pc->c, opl->l, true, NULL, NULL))
                    {
                        pc->fixed = true;
                    }
                }

                oln = oln->next;
            }
        }
        // Iterate
        cNode = cNode->next;
    }
}

/**
 * @brief TODO doc
 *
 * @param phys
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
