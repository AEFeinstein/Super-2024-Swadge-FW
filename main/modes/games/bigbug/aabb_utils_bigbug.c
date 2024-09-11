//==============================================================================
// Includes
//==============================================================================

#include "aabb_utils_bigbug.h"
// #include <stdint.h>
#include "fill.h"

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Draw a box
 *
 * @param box The box to draw
 * @param color The color of the box to draw
 * @param isFilled true to draw a filled box, false to draw an outline
 * @param scalingFactor The scaling factor to apply before drawing the box
 */
void bb_drawBox(bb_box_t box, paletteColor_t color, bool isFilled, int32_t scalingFactor)
{
    if (isFilled)
    {
        fillDisplayArea(box.x0 >> scalingFactor, box.y0 >> scalingFactor, box.x1 >> scalingFactor,
                        box.y1 >> scalingFactor, color);
    }
    else
    {
        /*plotRect(box.x0 >> scalingFactor,
                 box.y0 >> scalingFactor,
                 box.x1 >> scalingFactor,
                 box.y1 >> scalingFactor,
                 color);*/
    }
}

/**
 * @brief
 *
 * @param box0 A box to check for collision
 * @param box1 The other box to check for collision
 * @param scalingFactor The factor to scale the boxes by before checking for collision
 * @return true if the boxes collide, false if they do not
 */
bool bb_boxesCollide(bb_box_t box0, bb_box_t box1, int32_t scalingFactor)
{
    return (box0.x0 >> scalingFactor) < (box1.x1 >> scalingFactor)
           && (box0.x1 >> scalingFactor) > (box1.x0 >> scalingFactor)
           && (box0.y0 >> scalingFactor) < (box1.y1 >> scalingFactor)
           && (box0.y1 >> scalingFactor) > (box1.y0 >> scalingFactor);
}