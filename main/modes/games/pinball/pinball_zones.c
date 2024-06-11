//==============================================================================
// Includes
//==============================================================================

#include <math.h>
#include "pinball_zones.h"
#include "pinball_physics.h"

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Split a table up into zones. Each object is assigned to one or more zones for a very quick first-pass
 * collision check.
 *
 * @param p The pinball state
 */
void createTableZones(pinball_t* p)
{
    // Split the space into zones. Start with one big rectangle
    int32_t splitOffset  = (NUM_ZONES >> 1);
    p->zones[0].r.pos.x  = 0;
    p->zones[0].r.pos.y  = 0;
    p->zones[0].r.width  = (TFT_WIDTH);
    p->zones[0].r.height = (TFT_HEIGHT);
    p->zones[0].color    = c505;

    // While more zoning needs to happen
    while (splitOffset)
    {
        // Iterate over current zones, back to front
        for (int32_t i = NUM_ZONES - 1; i >= 0; i--)
        {
            // If this is a real zone
            if (0 < p->zones[i].r.height)
            {
                // Split it either vertically or horizontally, depending on which is larger
                if (p->zones[i].r.height > p->zones[i].r.width)
                {
                    // Split vertically
                    int32_t newHeight_1 = p->zones[i].r.height / 2;
                    int32_t newHeight_2 = p->zones[i].r.height - newHeight_1;

                    // Shrink the original zone
                    p->zones[i].r.height = newHeight_1;

                    // Create the new zone
                    p->zones[i + splitOffset].r.height = newHeight_2;
                    p->zones[i + splitOffset].r.pos.y  = p->zones[i].r.pos.y + p->zones[i].r.height;

                    p->zones[i + splitOffset].r.width = p->zones[i].r.width;
                    p->zones[i + splitOffset].r.pos.x = p->zones[i].r.pos.x;
                }
                else
                {
                    // Split horizontally
                    int32_t newWidth_1 = p->zones[i].r.width / 2;
                    int32_t newWidth_2 = p->zones[i].r.width - newWidth_1;

                    // Shrink the original zone
                    p->zones[i].r.width = newWidth_1;

                    // Create the new zone
                    p->zones[i + splitOffset].r.width = newWidth_2;
                    p->zones[i + splitOffset].r.pos.x = p->zones[i].r.pos.x + p->zones[i].r.width;

                    p->zones[i + splitOffset].r.height = p->zones[i].r.height;
                    p->zones[i + splitOffset].r.pos.y  = p->zones[i].r.pos.y;
                }

                // Give it a random color, just because
                p->zones[i + splitOffset].color = esp_random() % cTransparent;
            }
        }

        // Half the split offset
        splitOffset /= 2;
    }
}

/**
 * @brief Determine which table zones a rectangle is in
 *
 * @param p The pinball state
 * @param r The rectangle to zone
 * @return A bitmask of the zones the rectangle is in
 */
uint32_t pinZoneRect(pinball_t* p, pbRect_t rect)
{
    uint32_t zoneMask = 0;
    for (int16_t z = 0; z < NUM_ZONES; z++)
    {
        if (rectRectFlIntersection(p->zones[z].r, rect.r, NULL))
        {
            zoneMask |= (1 << z);
        }
    }
    return zoneMask;
}

/**
 * @brief Determine which table zones a line is in
 *
 * @param p The pinball state
 * @param l The line to zone
 * @return A bitmask of the zones the line is in
 */
uint32_t pinZoneLine(pinball_t* p, pbLine_t line)
{
    uint32_t zoneMask = 0;
    for (int16_t z = 0; z < NUM_ZONES; z++)
    {
        if (rectLineFlIntersection(p->zones[z].r, line.l, NULL))
        {
            zoneMask |= (1 << z);
        }
    }
    return zoneMask;
}

/**
 * @brief Determine which table zones a circle is in
 *
 * @param p The pinball state
 * @param r The circle to zone
 * @return A bitmask of the zones the circle is in
 */
uint32_t pinZoneCircle(pinball_t* p, pbCircle_t circ)
{
    uint32_t zoneMask = 0;
    for (int16_t z = 0; z < NUM_ZONES; z++)
    {
        if (circleRectFlIntersection(circ.c, p->zones[z].r, NULL))
        {
            zoneMask |= (1 << z);
        }
    }
    return zoneMask;
}

/**
 * @brief Determine which table zones a flipper is in. Note, this function will modify the flipper's angle
 *
 * @param p The pinball state
 * @param f The flipper to zone
 * @return A bitmask of the zones the circle is in
 */
uint32_t pinZoneFlipper(pinball_t* p, pbFlipper_t* f)
{
    pbRect_t boundingBox = {0};
    if (f->facingRight)
    {
        // Record the X position
        boundingBox.r.pos.x = (f->cPivot.c.pos.x - f->cPivot.c.radius);
    }
    else
    {
        // Record the X position
        boundingBox.r.pos.x = (f->cPivot.c.pos.x - f->length - f->cTip.c.radius);
    }

    // Width is the same when facing left and right
    boundingBox.r.width = (f->length + f->cPivot.c.radius + f->cTip.c.radius);

    // Height is the same too. Move the flipper up and record the Y start
    f->angle = M_PI_2 - FLIPPER_UP_ANGLE;
    updateFlipperPos(f);
    boundingBox.r.pos.y = (f->cTip.c.pos.y - f->cTip.c.radius);

    // Move the flipper down and record the Y end
    f->angle = M_PI_2 + FLIPPER_DOWN_ANGLE;
    updateFlipperPos(f);
    boundingBox.r.height = (f->cTip.c.pos.y + f->cTip.c.radius) - boundingBox.r.pos.y;

    // Return the zones of the bounding box
    return pinZoneRect(p, boundingBox);
}