//==============================================================================
// Includes
//==============================================================================

#include <stddef.h>
#include "macros.h"
#include "esp_heap_caps.h"
#include "artillery_phys_objs.h"

static void physSetZoneMaskLine(physSim_t* phys, physLine_t* pl);
static void physSetZoneMaskCirc(physSim_t* phys, physCirc_t* pc);

//==============================================================================
// Functions
//==============================================================================

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
physLine_t* physAddLine(physSim_t* phys, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, bool isTerrain)
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
 * @brief Update a line's properties like unit slope, unit normal, and zone mask
 *
 * @param phys The physics simulation
 * @param pl The line to update
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

    // Set axis aligned bounding box
    pl->aabb.x0 = MIN(pl->l.p1.x, pl->l.p2.x);
    pl->aabb.x1 = MAX(pl->l.p1.x, pl->l.p2.x);
    pl->aabb.y0 = MIN(pl->l.p1.y, pl->l.p2.y);
    pl->aabb.y1 = MAX(pl->l.p1.y, pl->l.p2.y);

    // Store what zones this line is in
    physSetZoneMaskLine(phys, pl);
}

/**
 * @brief Add a circle to the physics simulation. It may be mobile or fixed.
 *
 * @param phys The physics simulation
 * @param x1 The X point of the center of the circle
 * @param y1 The Y point of the center of the circle
 * @param r The radius of the circle
 * @param type The type of circle. CT_OBSTACLE is immobile, others are mobile.
 * @param baseColor
 * @param accentColor
 * @return The circle, also saved in the argument phys
 */
physCirc_t* physAddCircle(physSim_t* phys, uint16_t x1, uint16_t y1, uint16_t r, circType_t type,
                          paletteColor_t baseColor, paletteColor_t accentColor)
{
    // Allocate the circle
    physCirc_t* pc = heap_caps_calloc(1, sizeof(physCirc_t), MALLOC_CAP_8BIT);

    // Save the parameters
    pc->c.pos.x  = x1;
    pc->c.pos.y  = y1;
    pc->c.radius = r;
    pc->type     = type;

    pc->baseColor   = baseColor;
    pc->accentColor = accentColor;

    // Additional initialization based on type
    switch (type)
    {
        case CT_TANK:
        {
            setBarrelAngle(pc, 0);
            setShotPower(pc, 225.0f);
            pc->fixed      = false;
            pc->bounciness = 0.25f;
            break;
        }
        case CT_SHELL:
        {
            pc->fixed      = false;
            pc->bounciness = 0.75f;
            // Other shell parameters set in fireShot()
            break;
        }
        default:
        case CT_OBSTACLE:
        {
            pc->fixed      = true;
            pc->bounciness = 1.0f;
            break;
        }
    }

    updateCircleProperties(phys, pc);

    // Push the circle into a list in the simulation
    push(&phys->circles, pc);

    // Return the circle
    return pc;
}

/**
 * @brief Update a circle's properties like aabb and zone
 *
 * @param phys The physics simulation
 * @param pc The circle to update
 */
void updateCircleProperties(physSim_t* phys, physCirc_t* pc)
{
    // Set axis aligned bounding box
    pc->aabb.x0 = pc->c.pos.x - pc->c.radius;
    pc->aabb.x1 = pc->c.pos.x + pc->c.radius;
    pc->aabb.y0 = pc->c.pos.y - pc->c.radius;
    pc->aabb.y1 = pc->c.pos.y + pc->c.radius;

    // Store what zones the circle is in
    physSetZoneMaskCirc(phys, pc);
}

/**
 * @brief Calculate what zones a line is in and save it for that line
 *
 * @param phys The physics simulation
 * @param pl The line to calculate zones for
 */
static void physSetZoneMaskLine(physSim_t* phys, physLine_t* pl)
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
static void physSetZoneMaskCirc(physSim_t* phys, physCirc_t* pc)
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
