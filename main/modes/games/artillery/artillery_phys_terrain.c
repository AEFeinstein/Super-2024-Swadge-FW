//==============================================================================
// Includes
//==============================================================================

#include <math.h>
#include <esp_heap_caps.h>
#include <esp_random.h>
#include "macros.h"
#include "color_utils.h"
#include "geometry.h"
#include "trigonometry.h"
#include "artillery.h"
#include "artillery_phys_terrain.h"
#include "artillery_phys_objs.h"
<<<<<<< HEAD
=======
#include "artillery_game.h"
>>>>>>> origin/main

//==============================================================================
// Function Declarations
//==============================================================================

static float deformTerrainPoint(vecFl_t* p, vecFl_t* expPnt, float rSq, float expMin, float expMax, bool raiseTerrain);
static int32_t moveTerrainPoint(vecFl_t* src, vecFl_t* dst);

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
 * @brief TODO doc
 *
 * TODO transmit over p2p
 *
 * @param phys
 */
void physGenerateClouds(physSim_t* phys)
{
    // Find highest land point
    float highPoint = FLT_MAX;
    node_t* lNode   = phys->lines.first;
    while (lNode)
    {
        physLine_t* line = lNode->val;
        if (line->isTerrain)
        {
            if (line->l.p1.y < highPoint)
            {
                highPoint = line->l.p1.y;
            }
            if (line->l.p2.y < highPoint)
            {
                highPoint = line->l.p2.y;
            }
        }
        lNode = lNode->next;
    }

#define C_PART_RAD_MIN   8
#define C_PART_RAD_MAX   24
#define CLOUD_MAX_RADIUS (2 * C_PART_RAD_MAX)

    vec_t cloudCell = {
        .x = phys->bounds.x / CLOUD_COLS,
        .y = highPoint / CLOUD_ROWS,
    };

    // Generate clouds above land
    for (int y = 0; y < CLOUD_ROWS; y++)
    {
        for (int x = 0; x < CLOUD_COLS; x++)
        {
            // Math the cloud index
            int c = (y * CLOUD_COLS) + x;

            // Find the randomized midpoint in this cloud 'cell'
            vec_t midpoint = {
                .x = (x * cloudCell.x) + (C_PART_RAD_MAX + esp_random() % (cloudCell.x - (2 * CLOUD_MAX_RADIUS))),
                .y = (y * cloudCell.y) + (C_PART_RAD_MAX + esp_random() % (cloudCell.y - (2 * CLOUD_MAX_RADIUS))),
            };

            // Find a random angle to start drawing fluffs at
            int16_t randAngle = esp_random() % 360;

            // For each fluff
            for (int n = 0; n < CIRC_PER_CLOUD; n++)
            {
                // The circle to write
                circle_t* circ = &phys->clouds[(c * CIRC_PER_CLOUD) + n];

                // Randomize radius
                int16_t r = C_PART_RAD_MIN + (esp_random() % (C_PART_RAD_MAX - C_PART_RAD_MIN));

                // Write radius and position
                circ->radius = r;
                circ->pos.x  = midpoint.x + ((r - 2) * getSin1024(randAngle)) / 1024;
                circ->pos.y  = midpoint.y + ((r - 2) * getCos1024(randAngle)) / 1024;

                // Iterate the angle
                randAngle = (randAngle + (360 / CIRC_PER_CLOUD)) % 360;
            }
        }
    }
}

/**
 * @brief Explode a shell and deform terrain
 *
 * @param phys The physics simulation
 * @param shell The shell which exploded
 * @param hitTank The tank that was hit by the shell, may be NULL
 */
bool explodeShell(physSim_t* phys, node_t* shellNode, physCirc_t* hitTank)
{
    bool change       = false;
    physCirc_t* shell = shellNode->val;

    int32_t color    = paletteToRGB(shell->baseColor);
    phys->ledColor.r = ((color >> 16) & 0xFF) / 2;
    phys->ledColor.g = ((color >> 8) & 0xFF) / 2;
    phys->ledColor.b = ((color >> 0) & 0xFF) / 2;
    phys->ledTimer   = LED_EXPLOSION_US;

    circleFl_t explosion = {
        .pos    = shell->c.pos,
        .radius = shell->explosionRadius,
    };

    bool raiseTerrain = false;
    bool makeLava     = false;
    if (WALL_MAKER == shell->effect)
    {
        // Raise terrain without an explosion animation
        raiseTerrain = true;
    }
    else if (FLOOR_LAVA == shell->effect)
    {
        makeLava = true;
    }
    else
    {
        // Create an explosion animation
        explosionAnim_t* ea = heap_caps_calloc(1, sizeof(explosionAnim_t), MALLOC_CAP_8BIT);
        ea->circ            = explosion;
        ea->color           = shell->baseColor;
        ea->ttlUs           = 500000;
        ea->expTimeUs       = ea->ttlUs;
        push(&phys->explosions, ea);
    }

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
            if (makeLava)
            {
                if ((expMin <= line->l.p1.x && line->l.p1.x < expMax)
                    || (expMin <= line->l.p2.x && line->l.p2.x < expMax))
                {
                    line->isLava = true;
                }
            }
            else
            {
                // Attempt to deform points
<<<<<<< HEAD
=======
                // TODO this doesn't work great with machine gun b/c it doesn't accumulate deformations
>>>>>>> origin/main
                line->destination.p1.y
                    = deformTerrainPoint(&line->l.p1, &shell->c.pos, rSq, expMin, expMax, raiseTerrain);
                line->destination.p2.y
                    = deformTerrainPoint(&line->l.p2, &shell->c.pos, rSq, expMin, expMax, raiseTerrain);
            }
        }
        // Iterate
        lNode = lNode->next;
    }

    // Iterate through all circles
    node_t* cNode = phys->circles.first;
    while (cNode)
    {
        physCirc_t* circ = cNode->val;
        // If this is a tank that was hit by the shell
        if (cNode != shellNode && CT_TANK == circ->type
            && (hitTank == circ || circleCircleFlIntersection(explosion, circ->c, NULL, NULL)))
        {
            // Add or subtract score depending on who's hit
            if (shell->owner == circ)
            {
                shell->owner->score -= shell->score;
<<<<<<< HEAD
=======

                if (shell->score && artilleryIsMyTurn(getArtilleryData()))
                {
                    trophyUpdate(&artilleryTrophies[AT_HITTING_YOURSELF], true, true);
                }
>>>>>>> origin/main
            }
            else
            {
                shell->owner->score += shell->score;
<<<<<<< HEAD
=======

                if ((SNIPER == shell->effect) && artilleryIsMyTurn(getArtilleryData()))
                {
                    trophyUpdate(&artilleryTrophies[AT_SNIPER], true, true);
                }
            }

            // Set eyes to [X X]
            artilleryData_t* ad = getArtilleryData();
            if (EYES_DEAD != ad->eyeSlot)
            {
                ad->eyeSlot = EYES_DEAD;
                ch32v003SelectBitmap(ad->eyeSlot);
                ad->deadEyeTimer = 3000000;
>>>>>>> origin/main
            }

            // Impart force on hit tanks
            circ->vel = addVecFl2d(circ->vel,
                                   mulVecFl2d(normVecFl2d(subVecFl2d(circ->c.pos, shell->c.pos)), shell->explosionVel));

            // Track the tank after being hit
            push(&phys->cameraTargets, circ);
        }
        cNode = cNode->next;
    }

    if (hitTank && CONFUSION == shell->effect)
    {
        setBarrelAngle(hitTank, (esp_random() % 360) * M_PIf / 180.0f);
        setShotPower(hitTank, esp_random() % MAX_SHOT_POWER);
        change = true;
    }

    // Remove this shell from camera tracking
    removeVal(&phys->cameraTargets, shell);

    phys->terrainMoving = true;
<<<<<<< HEAD
=======

    // Play SFX (explosion)
    midiNoteOn(globalMidiPlayerGet(MIDI_SFX), 9, CRASH_CYMBAL_2, 0x7F);

>>>>>>> origin/main
    return change;
}

/**
 * @brief Check if a single point should be deformed by an explosion
 *
 * @param p A point to check for deformation
 * @param expPnt The point of explosion
 * @param rSq The squared radius of explosion
 * @param expMin The minimum X position which is affected
 * @param expMax The maximum X position which is affected
 * @param raiseTerrain true to raise terrain, false to lower it
 * @return The new Y position of the deformed point
 */
static float deformTerrainPoint(vecFl_t* p, vecFl_t* expPnt, float rSq, float expMin, float expMax, bool raiseTerrain)
{
    if (expMin <= p->x && p->x < expMax)
    {
        if (raiseTerrain)
        {
            // Simply raise terrain, but not out of bounds
<<<<<<< HEAD
            return MAX(p->y - 40, 0);
=======
            return MAX(p->y - 80, 0);
>>>>>>> origin/main
        }

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
 * @return A negative number if the point moved up, a positive number if it moved down, or zero for no movement
 */
static int32_t moveTerrainPoint(vecFl_t* src, vecFl_t* dst)
{
    if (src->y == dst->y)
    {
        return 0;
    }

    int32_t direction = dst->y - src->y;

    if (ABS(dst->y - src->y) < 1)
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

    return direction;
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

    // Keep track of the patch of ground that's moving
    int32_t minMoveX   = INT32_MAX;
    int32_t maxMoveX   = INT32_MIN;
    bool movingUpwards = false;

    // Iterate through all lines
    node_t* lNode = phys->lines.first;
    while (lNode)
    {
        physLine_t* line = lNode->val;
        // If the line is terrain
        if (line->isTerrain)
        {
            // Track if this line is moving or needs a properties update
            int32_t direction = moveTerrainPoint(&line->l.p1, &line->destination.p1)
                                + moveTerrainPoint(&line->l.p2, &line->destination.p2);

            // If a segment is moving upwards
            if (direction < 0)
            {
                // Raise a flag to check tanks for upward movement too
                movingUpwards = true;
            }

            // If the line needs a properties update
            if (direction)
            {
                anyTerrainMoving = true;
                // Update slope and normals and such
                updateLineProperties(phys, line);

                // Update space that gets moved
                minMoveX = MIN(minMoveX, line->l.p1.x);
                maxMoveX = MAX(maxMoveX, line->l.p2.x);
            }
        }
        // Iterate
        lNode = lNode->next;
    }

    // If terrain is moving upwards
    if (movingUpwards)
    {
        // Iterate through tanks and bump them upwards too
        node_t* cNode = phys->circles.first;
        while (cNode)
        {
            physCirc_t* c = cNode->val;
            if (CT_TANK == c->type)
            {
                int32_t cx0 = c->c.pos.x - c->c.radius;
                int32_t cx1 = c->c.pos.x + c->c.radius;

                // If the tank is on upward moving terrain
                if (cx0 <= maxMoveX && cx1 >= minMoveX)
                {
                    // bump the tank up too!
                    c->c.pos.y--;
                }
            }
            cNode = cNode->next;
        }
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
