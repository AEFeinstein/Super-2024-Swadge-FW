//==============================================================================
// Includes
//==============================================================================

#include <math.h>
#include "macros.h"
#include "artillery_phys_terrain.h"
#include "artillery_phys_objs.h"

//==============================================================================
// Function Declarations
//==============================================================================

static float deformTerrainPoint(vecFl_t* p, vecFl_t* expPnt, float rSq, float expMin, float expMax);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Explode a shell and deform terrain
 *
 * @param phys The physics simulation
 * @param shell The shell which exploded
 */
void explodeShell(physSim_t* phys, node_t* shellNode)
{
    physCirc_t* shell = shellNode->val;

    // Calculate the X bounds of the explosion and the squared radius
    float expMin = shell->c.pos.x - shell->explosionRadius;
    float expMax = shell->c.pos.x + shell->explosionRadius;
    float rSq    = (shell->explosionRadius * shell->explosionRadius);

    // Iterate through all lines
    node_t* lNode = phys->lines.first;
    while (lNode)
    {
        physLine_t* line = lNode->val;
        // If this is terrain
        if (line->isTerrain)
        {
            // Attempt to deform points
            line->destination.p1.y = deformTerrainPoint(&line->l.p1, &shell->c.pos, rSq, expMin, expMax);
            line->destination.p2.y = deformTerrainPoint(&line->l.p2, &shell->c.pos, rSq, expMin, expMax);
        }
        // Iterate
        lNode = lNode->next;
    }

    // Remove this shell from camera tracking
    removeVal(&phys->cameraTargets, shell);

    phys->terrainMoving = true;
}

/**
 * @brief Check if a single point should be deformed by an explosion
 *
 * @param p A point to check for deformation
 * @param expPnt The point of explosion
 * @param rSq The squared radius of explosion
 * @param expMin The minimum X position which is affected
 * @param expMax The maximum X position which is affected
 * @return The new Y position of the deformed point
 */
static float deformTerrainPoint(vecFl_t* p, vecFl_t* expPnt, float rSq, float expMin, float expMax)
{
    if (expMin <= p->x && p->x < expMax)
    {
        // X distance from shell to point to adjust
        float xDist = expPnt->x - p->x;
        // Explosion Y size at the point to adjust
        float ySz = sqrtf(rSq - (xDist * xDist));
        // Top and bottom points of the explosion
        float expTop    = expPnt->y - ySz;
        float expBottom = expPnt->y + ySz;

        if (p->y < expTop)
        {
            // Explosion entirely underground, add full explosion size
            return p->y + (2 * ySz);
        }
        else if (p->y <= expBottom)
        {
            // Point somewhere in explosion, adjust terrain to point
            return expBottom;
        }
    }
    // Explosion doesn't affect point
    return p->y;
}

/**
 * @brief Move terrain points a bit towards where they should be after shell explosion
 *
 * @param phys The physics simulation
 * @param elapsedUs The time elapsed since this was last called
 * @return true if terrain is actively moving, false otherwise
 */
bool moveTerrainPoints(physSim_t* phys, int32_t elapsedUs)
{
    // Keep track if any terrain is moving anywhere
    bool anyTerrainMoving = false;

    // Iterate through all lines
    node_t* lNode = phys->lines.first;
    while (lNode)
    {
        physLine_t* line = lNode->val;
        // If the line is terrain
        if (line->isTerrain)
        {
            // Track if this line is moving or needs a properties update
            bool updateLine = false;

            if (line->destination.p1.y == line->l.p1.y)
            {
                // Not moving
            }
            else if (ABS(line->destination.p1.y - line->l.p1.y) < 1)
            {
                // Less than one pixel off, clamp it
                line->l.p1.y = line->destination.p1.y;
                updateLine   = true;
            }
            else if (line->destination.p1.y > line->l.p1.y)
            {
                // Move p1 towards destination
                line->l.p1.y++;
                anyTerrainMoving = true;
            }
            else if (line->destination.p1.y < line->l.p1.y)
            {
                // Move p1 towards destination
                line->l.p1.y--;
                anyTerrainMoving = true;
            }

            if (line->destination.p2.y == line->l.p2.y)
            {
                // Do nothing
            }
            else if (ABS(line->destination.p2.y - line->l.p2.y) < 1)
            {
                // Less than one pixel off, clamp it
                line->l.p2.y = line->destination.p2.y;
                updateLine   = true;
            }
            else if (line->destination.p2.y > line->l.p2.y)
            {
                // Move p2 towards destination
                line->l.p2.y++;
                anyTerrainMoving = true;
            }
            else if (line->destination.p2.y < line->l.p2.y)
            {
                // Move p1 towards destination
                line->l.p2.y--;
                anyTerrainMoving = true;
            }

            // If the line needs a properties update
            if (updateLine)
            {
                // Update slope and normals and such
                updateLineProperties(phys, line);
            }
        }
        // Iterate
        lNode = lNode->next;
    }

    // Return if any terrain anywhere is moving
    return anyTerrainMoving;
}
