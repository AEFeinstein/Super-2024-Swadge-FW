//==============================================================================
// Includes
//==============================================================================

#include <stddef.h>
#include "esp_heap_caps.h"
#include "macros.h"
#include "artillery_phys_collisions.h"
#include "artillery_phys_terrain.h"

//==============================================================================
// Function Declarations
//==============================================================================

static bool physCircLineIntersection(physSim_t* phys, physCirc_t* pc, physLine_t* opl, float* colDist,
                                     vecFl_t* reflVec);
static bool physCircCircIntersection(physSim_t* phys, physCirc_t* cMoving, circleFl_t* cOther, float* colDist,
                                     vecFl_t* reflVec);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Check for collisions between moving objects (circles with current and desired positions) and fixed objects
 * (circles or lines). This may update the velocity of an object after a collision but will not update the position.
 *
 * @param phys The physics simulation
 */
bool physCheckCollisions(physSim_t* phys)
{
    bool change = false;

    // For each circle
    bool shouldRemoveNode = false;
    physCirc_t* hitTank   = NULL;
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

            // Remove out-of-bounds non-tanks objects. Removing tanks can cause crashes
            if (CT_TANK != pc->type)
            {
                // Check if out of bounds
                if (pc->c.pos.x < 0 ||               //
                    pc->c.pos.x >= phys->bounds.x || //
                    pc->c.pos.y < 0 ||               //
                    pc->c.pos.y > phys->bounds.y)
                {
                    // Out of bounds, remove circle
                    shouldRemoveNode = true;
                }
                else // In bounds, check if it's underground
                {
                    int16_t screenX = pc->c.pos.x - phys->camera.x;
                    if (0 <= screenX && screenX < TFT_WIDTH)
                    {
                        int16_t screenY = pc->c.pos.y - phys->camera.y;
                        if (screenY > phys->surfacePoints[screenX])
                        {
                            // Out of bounds, remove circle
                            shouldRemoveNode = true;
                        }
                    }
                }
            }

            // Check for collisions with lines
            node_t* oln = phys->lines.first;
            while (oln)
            {
                // Check for collision
                physCircLineIntersection(phys, pc, (physLine_t*)oln->val, &colDist, &reflVec);

                // Iterate to next line
                oln = oln->next;
            }

            // Check for collisions with circles
            node_t* ocn = phys->circles.first;
            while (ocn)
            {
                // Convenience pointer
                physCirc_t* opc = ocn->val;

                if ((opc != pc) &&                    // Don't collide a circle with itself
                    (opc->zonemask & pc->zonemask) && // make sure they're in the same zone
                    (pc->type != opc->type) &&        // Types should differ (shells don't hit shells, etc.)
                    physCircCircIntersection(phys, pc, &opc->c, &colDist, &reflVec))
                {
                    // If a shell
                    if (CT_SHELL == pc->type)
                    {
                        // hits a tank
                        if (CT_TANK == opc->type)
                        {
                            // it explodes
                            shouldRemoveNode = true;
                            hitTank          = opc;
                        }

                        // Shells always bounce off obstacles
                        countBounce = (CT_OBSTACLE != opc->type);
                    }
                }

                // Iterate to the next circle
                ocn = ocn->next;
            }

            // Save last contact normal for wheel hysteresis
            pc->lastContactNorm = pc->contactNorm;

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
                        // Ran out of bounces, so it explodes
                        shouldRemoveNode = true;
                    }
                }

                // If object isn't marked for removal
                if (!shouldRemoveNode)
                {
                    // Bounce it by reflecting across this vector
                    pc->vel = subVecFl2d(pc->vel, mulVecFl2d(reflVec, (2 * dotVecFl2d(pc->vel, reflVec))));

                    // Dampen after bounce
                    pc->vel = mulVecFl2d(pc->vel, pc->bounciness);

                    // Deadband the velocity if it's small enough
                    if (sqMagVecFl2d(pc->vel) < 300.0f)
                    {
                        pc->vel.x = 0;
                        pc->vel.y = 0;
                    }

                    // If the circle is on top of an object (i.e. reflection vector points upward)
                    if (reflVec.y < 0)
                    {
                        pc->inContact = true;

                        // Save the contact normal
                        pc->contactNorm = reflVec;

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

        // Optionally remove this entry
        if (shouldRemoveNode)
        {
            // Shells explode
            if (pc->type == CT_SHELL)
            {
                change |= explodeShell(phys, cNode, hitTank);
            }

            // Remove the node
            heap_caps_free(removeEntry(&phys->circles, cNode));
            shouldRemoveNode = false;
        }
        cNode = nextNode;
    }

    return change;
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
 * @brief TODO doc
 *
 * @param phys The physics simulation
 * @param pc [IN] the moving circle
 * @param opl [IN] the fixed line
 * @param colDist [IN/OUT] The distance between the moving circle's position and the collision point
 * @param reflVec [OUT] The vector to reflect the moving circle's velocity across if the two collided
 * @return true if there was a collision, false if there was not
 */
static bool physCircLineIntersection(physSim_t* phys, physCirc_t* pc, physLine_t* opl, float* colDist, vecFl_t* reflVec)
{
    bool collision = false;

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

                if (dist < *colDist)
                {
                    *colDist  = dist;
                    *reflVec  = mulVecFl2d(opl->unitNormal, (1 == idx) ? -1 : 1);
                    collision = true;
                }
            }
        }

        // Construct bounds around the line endcaps
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
            collision |= physCircCircIntersection(phys, pc, &caps[cIdx], colDist, reflVec);
        }
    }

    return collision;
}

/**
 * @brief Check for intersections between a moving circle, represented by a line segment, and a fixed circle. The
 * fixed circle may have radius 0, which indicates a point
 *
 * @param phys The physics simulation
 * @param cMoving [IN] The moving circle
 * @param cOther [IN] The fixed circle
 * @param colDist [IN/OUT] The distance between the moving circle's position and the collision point
 * @param reflVec [OUT] The vector to reflect the moving circle's velocity across if the two collided
 * @return true if there was a collision, false if there was not
 */
static bool physCircCircIntersection(physSim_t* phys, physCirc_t* cMoving, circleFl_t* cOther, float* colDist,
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
