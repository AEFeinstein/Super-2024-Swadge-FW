//==============================================================================
// Includes
//==============================================================================

#include "aabb_utils_bigbug.h"
// #include <stdint.h>
#include "fill.h"
#include "typedef_bigbug.h"

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
void bb_drawBox(bb_box_t* box, paletteColor_t* color, bool isFilled)
{
    if (isFilled)
    {
        fillDisplayArea(box->pos.x - box->halfWidth, box->pos.y - box->halfHeight, box->pos.x + box->halfWidth,
                        box->pos.y + box->halfHeight, *color);
    }
    else
    {
        // plotRect(box->pos.x - box->halfWidth, box->pos.y - box->halfHeight,
        //             box->pos.x + box->halfWidth, box->pos.y + box->halfHeight,
        //             *color);
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
bool bb_boxesCollide(bb_box_t* box0, bb_box_t* box1)
{
    return box0->pos.x - box0->halfWidth < box1->pos.x + box1->halfWidth
           && box0->pos.x + box0->halfWidth > box1->pos.x - box1->halfWidth
           && box0->pos.y - box0->halfHeight < box1->pos.y + box1->halfHeight
           && box0->pos.y + box0->halfHeight > box1->pos.y - box1->halfHeight;
}

bool bb_boxesCollideShift(bb_box_t* box0, bb_box_t* box1)
{
    return box0->pos.x - box0->halfWidth < (box1->pos.x >> DECIMAL_BITS) + (box1->halfWidth >> DECIMAL_BITS)
           && box0->pos.x + box0->halfWidth > (box1->pos.x >> DECIMAL_BITS) - (box1->halfWidth >> DECIMAL_BITS)
           && box0->pos.y - box0->halfHeight < (box1->pos.y >> DECIMAL_BITS) + (box1->halfHeight >> DECIMAL_BITS)
           && box0->pos.y + box0->halfHeight > (box1->pos.y >> DECIMAL_BITS) - (box1->halfHeight >> DECIMAL_BITS);
}