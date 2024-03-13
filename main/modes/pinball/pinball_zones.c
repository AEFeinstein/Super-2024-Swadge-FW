//==============================================================================
// Includes
//==============================================================================

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
    int32_t splitOffset = (NUM_ZONES >> 1);
    p->zones[0].pos.x   = 0;
    p->zones[0].pos.y   = 0;
    p->zones[0].width   = TO_FX(TFT_WIDTH);
    p->zones[0].height  = TO_FX(TFT_HEIGHT);
    p->zones[0].color   = c505;

    // While more zoning needs to happen
    while (splitOffset)
    {
        // Iterate over current zones, back to front
        for (int32_t i = NUM_ZONES - 1; i >= 0; i--)
        {
            // If this is a real zone
            if (0 < p->zones[i].height)
            {
                // Split it either vertically or horizontally, depending on which is larger
                if (p->zones[i].height > p->zones[i].width)
                {
                    // Split vertically
                    int32_t newHeight_1 = p->zones[i].height / 2;
                    int32_t newHeight_2 = p->zones[i].height - newHeight_1;

                    // Shrink the original zone
                    p->zones[i].height = newHeight_1;

                    // Create the new zone
                    p->zones[i + splitOffset].height = newHeight_2;
                    p->zones[i + splitOffset].pos.y  = p->zones[i].pos.y + p->zones[i].height;

                    p->zones[i + splitOffset].width = p->zones[i].width;
                    p->zones[i + splitOffset].pos.x = p->zones[i].pos.x;
                }
                else
                {
                    // Split horizontally
                    int32_t newWidth_1 = p->zones[i].width / 2;
                    int32_t newWidth_2 = p->zones[i].width - newWidth_1;

                    // Shrink the original zone
                    p->zones[i].width = newWidth_1;

                    // Create the new zone
                    p->zones[i + splitOffset].width = newWidth_2;
                    p->zones[i + splitOffset].pos.x = p->zones[i].pos.x + p->zones[i].width;

                    p->zones[i + splitOffset].height = p->zones[i].height;
                    p->zones[i + splitOffset].pos.y  = p->zones[i].pos.y;
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
uint32_t pinZoneRect(pinball_t* p, pbRect_t r)
{
    uint32_t zoneMask = 0;
    rectangle_t ir    = intRect(r);
    for (int z = 0; z < NUM_ZONES; z++)
    {
        if (rectRectIntersection(intRect(p->zones[z]), ir, NULL))
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
uint32_t pinZoneLine(pinball_t* p, pbLine_t l)
{
    uint32_t zoneMask = 0;
    line_t il         = intLine(l);
    for (int z = 0; z < NUM_ZONES; z++)
    {
        if (rectLineIntersection(intRect(p->zones[z]), il, NULL))
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
uint32_t pinZoneCircle(pinball_t* p, pbCircle_t c)
{
    uint32_t zoneMask = 0;
    circle_t ic       = intCircle(c);
    for (int z = 0; z < NUM_ZONES; z++)
    {
        if (circleRectIntersection(ic, intRect(p->zones[z]), NULL))
        {
            zoneMask |= (1 << z);
        }
    }
    return zoneMask;
}
