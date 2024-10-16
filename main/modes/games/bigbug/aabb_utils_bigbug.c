//==============================================================================
// Includes
//==============================================================================

#include "aabb_utils_bigbug.h"
#include "entity_bigbug.h"
// #include <stdint.h>
#include "fill.h"

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief
 *
 * @param box0 A box to check for collision
 * @param box1 The other box to check for collision
 * @param scalingFactor The factor to scale the boxes by before checking for collision
 * @return true if the boxes collide, false if they do not
 */

bool bb_boxesCollide(bb_entity_t* unyielding, bb_entity_t* yielding, bb_hitInfo_t* hitInfo, vec_t* previousPos)
{
    // AABB-AABB collision detection begins here
    // https://tutorialedge.net/gamedev/aabb-collision-detection-tutorial/
    if (unyielding->pos.x - unyielding->halfWidth
            < yielding->pos.x + yielding->halfWidth
        && unyielding->pos.x + unyielding->halfWidth
                > yielding->pos.x - yielding->halfWidth
        && unyielding->pos.y - unyielding->halfHeight
                < yielding->pos.y + yielding->halfHeight
        && unyielding->pos.y + unyielding->halfHeight
                > yielding->pos.y - yielding->halfHeight)
    {
        /////////////////////////
        // Collision detected! //
        /////////////////////////
        if(hitInfo == NULL)
        {
            //The caller simply wants a simple true or false.
            return true;
        }

        hitInfo->hit    = true;

        if (previousPos != NULL)
        {
            // More accurate collision resolution if previousPos provided.
            // Used by entities that need to bounce around or move quickly.

            // generate hitInfo based on position from previous frame.
            hitInfo->normal = subVec2d(*previousPos, unyielding->pos);
        }
        else
        {
            // Worse collision resolution
            // for entities that don't care to store their previousPos.
            hitInfo->normal = subVec2d(*previousPos, unyielding->pos);
        }
        // Snap the offset to an orthogonal direction.
        if ((hitInfo->normal.x < 0 ? -hitInfo->normal.x : hitInfo->normal.x)
            > (hitInfo->normal.y < 0 ? -hitInfo->normal.y : hitInfo->normal.y))
        {
            if (hitInfo->normal.x > 0)
            {
                hitInfo->normal.x = 1;
                hitInfo->normal.y = 0;
                hitInfo->pos.x    = unyielding->pos.x + unyielding->halfWidth;
                hitInfo->pos.y    = yielding->pos.y;
            }
            else
            {
                hitInfo->normal.x = -1;
                hitInfo->normal.y = 0;
                hitInfo->pos.x    = unyielding->pos.x - unyielding->halfWidth;
                hitInfo->pos.y    = yielding->pos.y;
            }
        }
        else
        {
            if (hitInfo->normal.y > 0)
            {
                hitInfo->normal.x = 0;
                hitInfo->normal.y = 1;
                hitInfo->pos.x    = yielding->pos.x;
                hitInfo->pos.y    = unyielding->pos.y + unyielding->halfHeight;
            }
            else
            {
                hitInfo->normal.x = 0;
                hitInfo->normal.y = -1;
                hitInfo->pos.x    = yielding->pos.x;
                hitInfo->pos.y    = unyielding->pos.y - unyielding->halfHeight;
            }
        }
        return true;
    }
    return false;
}

