//==============================================================================
// Includes
//==============================================================================

#include <math.h>
#include <esp_heap_caps.h>
#include <esp_random.h>
#include "macros.h"
#include "artillery_phys_terrain.h"
#include "artillery_phys_objs.h"

//==============================================================================
// Function Declarations
//==============================================================================

static float deformTerrainPoint(vecFl_t* p, vecFl_t* expPnt, float rSq, float expMin, float expMax);
static bool moveTerrainPoint(vecFl_t* src, vecFl_t* dst);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Generate random terrain using midpoint displacement
 *
 * @param phys The physics simulation to generate terrain in
 * @param groundLevel The initial ground level, to be raised (Y value)
 */
void physGenerateTerrain(physSim_t* phys, int32_t groundLevel)
{
    // First remove all terrain
    node_t* lNode = phys->lines.first;
    while (lNode)
    {
        physLine_t* line = (physLine_t*)lNode->val;
        node_t* next     = lNode->next;
        if (line->isTerrain)
        {
            removeEntry(&phys->lines, lNode);
        }
        lNode = next;
    }

    // Build a list of heights
    list_t heights = {0};

    // Start with two points, start and end
    push(&heights, (void*)((intptr_t)groundLevel));
    push(&heights, (void*)((intptr_t)groundLevel));

    // For some number of iterations, divide each terrain segment into two and add randomness to the midpoint
    int32_t numIterations = TERRAIN_ITERATIONS;
    int32_t randBound     = (1 << (numIterations + 1));
    while (numIterations--)
    {
        // Start at the beginning of the list
        node_t* prev = heights.first;
        node_t* next = prev->next;
        while (next)
        {
            // The current height is the average of previous and next heights
            intptr_t prevH = (intptr_t)prev->val;
            intptr_t nextH = (intptr_t)next->val;
            intptr_t h     = (prevH + nextH) / 2;
            // Add randomness to the new height
            h -= (esp_random() % randBound);
            h = CLAMP(h, 0, phys->bounds.y);
            // Insert the new height into the list
            addAfter(&heights, (void*)h, prev);

            // Iterate past the inserted midpoint
            prev = next;
            next = prev->next;
        }
        randBound /= 2;
    }

    // Figure out width of segments
    float x     = 0;
    float xStep = phys->bounds.x / (float)(heights.length - 1);

    // Iterate through heights, adding lines to the simulation
    node_t* prev = heights.first;
    node_t* next = prev->next;
    while (next)
    {
        // Add the line
        physAddLine(phys, x, (intptr_t)prev->val, x + xStep, (intptr_t)next->val, true);

        // Iterate to the next segment
        x += xStep;
        prev = next;
        next = prev->next;
    }

    // Clear out the list
    while (heights.first)
    {
        pop(&heights);
    }
}

/**
 * @brief Flatten the terrain under the player, making a starting platform
 *
 * @param phys The physics simulation
 * @param player The player, represented by a circle
 */
void flattenTerrainUnderPlayer(physSim_t* phys, physCirc_t* player)
{
    // The heights of the first and last points under the player
    float y1 = FLT_MAX;
    float y2 = FLT_MAX;
    // Keep track of if points are under the player while iterating
    bool isUnderPlayer = false;

    // First find all points under the player, keeping track of the start and end height
    node_t* lNode = phys->lines.first;
    while (lNode)
    {
        physLine_t* line = lNode->val;

        float cX1  = player->c.pos.x - player->c.radius;
        float cX2  = player->c.pos.x + player->c.radius;
        vecFl_t p1 = line->l.p1;
        vecFl_t p2 = line->l.p2;

        // First line point
        if (cX1 <= p1.x && p1.x <= cX2)
        {
            if (!isUnderPlayer)
            {
                // Points are now under the player
                isUnderPlayer = true;
                y1            = p1.y;
            }
        }
        else if (isUnderPlayer)
        {
            // Point is no longer under player
            isUnderPlayer = false;
            y2            = p1.y;
            break;
        }

        // Second line point
        if (cX1 <= p2.x && p2.x <= cX2)
        {
            if (!isUnderPlayer)
            {
                // Points are now under the player
                isUnderPlayer = true;
                y1            = p2.y;
            }
        }
        else if (isUnderPlayer)
        {
            // Point is no longer under player
            isUnderPlayer = false;
            y2            = p2.y;
            break;
        }

        // Iterate lines
        lNode = lNode->next;
    }

    // The flat height is the average of the start and end
    float flatHeight = (int32_t)(((y1 + y2) / 2) + 0.5f);

    // The start and end X points for the flattened platform
    float cX1 = player->c.pos.x - (2 * player->c.radius);
    float cX2 = player->c.pos.x + (2 * player->c.radius);

    // Flatten nodes
    isUnderPlayer = false;
    lNode         = phys->lines.first;
    while (lNode)
    {
        // The line and some convenience variables
        physLine_t* line = lNode->val;
        vecFl_t p1       = line->l.p1;
        vecFl_t p2       = line->l.p2;

        bool updateLine = false;
        bool allDone    = false;

        // First line point
        if (cX1 <= p1.x && p1.x <= cX2)
        {
            // Flatten the point
            isUnderPlayer          = true;
            line->l.p1.y           = flatHeight;
            line->destination.p1.y = flatHeight;
            updateLine             = true;
        }
        else if (isUnderPlayer)
        {
            // No longer under player
            allDone = true;
        }

        // Second line point
        if (cX1 <= p2.x && p2.x <= cX2)
        {
            // Flatten the point
            isUnderPlayer          = true;
            line->l.p2.y           = flatHeight;
            line->destination.p2.y = flatHeight;
            updateLine             = true;
        }
        else if (isUnderPlayer)
        {
            // No longer under player
            allDone = true;
        }

        // If the line needs a properties update
        if (updateLine)
        {
            // Update slope and normals and such
            updateLineProperties(phys, line);
        }

        // No need to iterate anymore
        if (allDone)
        {
            break;
        }
        else
        { // Iterate lines
            lNode = lNode->next;
        }
    }
}

/**
 * @brief Explode a shell and deform terrain
 *
 * @param phys The physics simulation
 * @param shell The shell which exploded
 */
void explodeShell(physSim_t* phys, node_t* shellNode)
{
    physCirc_t* shell = shellNode->val;

    // Create an explosion animation
    explosionAnim_t* ea = heap_caps_calloc(1, sizeof(explosionAnim_t), MALLOC_CAP_8BIT);
    ea->circ            = shell->c;
    ea->circ.radius     = shell->explosionRadius;
    ea->color           = c500;
    ea->ttlUs           = 500000;
    ea->expTimeUs       = ea->ttlUs;
    push(&phys->explosions, ea);

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

    if (shell->explosionVel)
    {
        // Iterate through all circles
        node_t* cNode = phys->circles.first;
        while (cNode)
        {
            physCirc_t* circ = cNode->val;
            // If this is a tank that was hit by the shell
            if (cNode != shellNode && CT_TANK == circ->type
                && circleCircleFlIntersection(ea->circ, circ->c, NULL, NULL))
            {
                // Add or subtract score depending on who's hit
                if (shell->owner == circ)
                {
                    shell->owner->score -= shell->score;
                }
                else
                {
                    shell->owner->score += shell->score;
                }

                // Impart force on hit tanks
                circ->vel = addVecFl2d(
                    circ->vel, mulVecFl2d(normVecFl2d(subVecFl2d(circ->c.pos, shell->c.pos)), shell->explosionVel));
            }
            cNode = cNode->next;
        }
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
            return (int32_t)(p->y + (2 * ySz) + 0.5f);
        }
        else if (p->y <= expBottom)
        {
            // Point somewhere in explosion, adjust terrain to point
            return (int32_t)(expBottom + 0.5f);
        }
    }
    // Explosion doesn't affect point
    return p->y;
}

/**
 * @brief Move a single point towards a destination
 *
 * @param src The point's current position
 * @param dst The point's desired position
 * @return true if the point moved, false otherwise
 */
static bool moveTerrainPoint(vecFl_t* src, vecFl_t* dst)
{
    if (src->y == dst->y)
    {
        return false;
    }
    else if (ABS(dst->y - src->y) < 1)
    {
        // Less than one pixel off, clamp it
        src->y = dst->y;
    }
    else if (dst->y > src->y)
    {
        // Move p1 towards destination
        src->y++;
    }
    else if (dst->y < src->y)
    {
        // Move p1 towards destination
        src->y--;
    }

    return true;
}

/**
 * @brief Move terrain lines a bit towards where they should be after shell explosion
 *
 * TODO use elapsedUs
 *
 * @param phys The physics simulation
 * @param elapsedUs The time elapsed since this was last called
 * @return true if terrain is actively moving, false otherwise
 */
bool moveTerrainLines(physSim_t* phys, int32_t elapsedUs)
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
            bool updateLine = moveTerrainPoint(&line->l.p1, &line->destination.p1)
                              | moveTerrainPoint(&line->l.p2, &line->destination.p2);

            // If the line needs a properties update
            if (updateLine)
            {
                anyTerrainMoving = true;
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

/**
 * @brief Add terrain points from a received packet
 *
 * @param phys The physics simulation to add terrain to
 * @param tIdx The index of the terrain to start at
 * @param terrainPoints The Y values of the terrain points
 * @param numTerrainPoints The number of received terrain points
 */
void physAddTerrainPoints(physSim_t* phys, uint16_t tIdx, const uint16_t* terrainPoints, uint16_t numTerrainPoints)
{
    // Calculate the X step and starting X location
    float xStep = phys->bounds.x / (float)(NUM_TERRAIN_POINTS - 1);
    float x     = xStep * tIdx;

    // Pick the starting Y location
    int16_t prevY    = terrainPoints[0];
    int32_t startIdx = 1;

    // Some terrain already added, so start where we left off
    if (0 != tIdx)
    {
        prevY    = ((physLine_t*)phys->lines.last->val)->l.p2.y;
        startIdx = 0;
    }

    // Create each line
    for (int32_t pIdx = startIdx; pIdx < numTerrainPoints; pIdx++)
    {
        physAddLine(phys, x, prevY, x + xStep, terrainPoints[pIdx], true);
        x += xStep;
        prevY = terrainPoints[pIdx];
    }
}
