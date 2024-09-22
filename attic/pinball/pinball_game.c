#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "heatshrink_helper.h"

#include "pinball_game.h"

#include "pinball_line.h"
#include "pinball_circle.h"
#include "pinball_rectangle.h"
#include "pinball_flipper.h"
#include "pinball_triangle.h"
#include "pinball_point.h"

/**
 * @brief TODO doc
 *
 * @param data
 * @param idx
 * @return uint8_t
 */
uint8_t readInt8(uint8_t* data, uint32_t* idx)
{
    return data[(*idx)++];
}

/**
 * @brief TODO doc
 *
 * @param data
 * @param idx
 * @return uint16_t
 */
uint16_t readInt16(uint8_t* data, uint32_t* idx)
{
    int16_t ret = (data[*idx] << 8) | (data[(*idx) + 1]);
    (*idx) += 2;
    return ret;
}

/**
 * @brief TODO
 *
 * @param scene
 * @param obj
 * @param groupId
 * @return list_t*
 */
list_t* addToGroup(pbScene_t* scene, void* obj, uint8_t groupId)
{
    push(&scene->groups[groupId], obj);
    return &scene->groups[groupId];
}

/**
 * @brief TODO doc
 *
 * @param scene
 */
void pbSceneInit(pbScene_t* scene)
{
    scene->gravity.x = 0;
    scene->gravity.y = 180;
    scene->score     = 0;
    scene->paused    = false;

    uint32_t decompressedSize = 0;
    uint8_t* tableData        = (uint8_t*)readHeatshrinkFile("pinball.raw", &decompressedSize, true);
    uint32_t dIdx             = 0;

    // Allocate groups
    scene->numGroups = readInt8(tableData, &dIdx) + 1;
    scene->groups    = (list_t*)calloc(scene->numGroups, sizeof(list_t));

    uint16_t linesInFile = readInt16(tableData, &dIdx);
    scene->lines         = calloc(linesInFile, sizeof(pbLine_t));
    scene->numLines      = 0;
    for (uint16_t lIdx = 0; lIdx < linesInFile; lIdx++)
    {
        dIdx += readLineFromFile(&tableData[dIdx], scene);
        pbLine_t* newLine = &scene->lines[scene->numLines - 1];

        // Record the table dimension
        float maxX = MAX(newLine->p1.x, newLine->p2.x);
        float maxY = MAX(newLine->p1.y, newLine->p2.y);

        if (maxX > scene->tableDim.x)
        {
            scene->tableDim.x = maxX;
        }
        if (maxY > scene->tableDim.y)
        {
            scene->tableDim.y = maxY;
        }
    }

    uint16_t circlesInFile = readInt16(tableData, &dIdx);
    scene->circles         = calloc(circlesInFile, sizeof(pbCircle_t));
    scene->numCircles      = 0;
    for (uint16_t cIdx = 0; cIdx < circlesInFile; cIdx++)
    {
        dIdx += readCircleFromFile(&tableData[dIdx], scene);
    }

    uint16_t rectanglesInFile = readInt16(tableData, &dIdx);
    scene->launchers          = calloc(1, sizeof(pbLauncher_t));
    scene->numLaunchers       = 0;
    for (uint16_t rIdx = 0; rIdx < rectanglesInFile; rIdx++)
    {
        dIdx += readRectangleFromFile(&tableData[dIdx], scene);
    }

    uint16_t flippersInFile = readInt16(tableData, &dIdx);
    scene->flippers         = calloc(flippersInFile, sizeof(pbFlipper_t));
    scene->numFlippers      = 0;
    for (uint16_t fIdx = 0; fIdx < flippersInFile; fIdx++)
    {
        dIdx += readFlipperFromFile(&tableData[dIdx], scene);
    }

    uint16_t trianglesInFile = readInt16(tableData, &dIdx);
    scene->triangles         = calloc(trianglesInFile, sizeof(pbTriangle_t));
    scene->numTriangles      = 0;
    for (uint16_t tIdx = 0; tIdx < trianglesInFile; tIdx++)
    {
        dIdx += readTriangleFromFile(&tableData[dIdx], scene);
    }

    uint16_t pointsInFile = readInt16(tableData, &dIdx);
    scene->points         = calloc(pointsInFile, sizeof(pbPoint_t));
    scene->numPoints      = 0;
    for (uint16_t pIdx = 0; pIdx < pointsInFile; pIdx++)
    {
        dIdx += readPointFromFile(&tableData[dIdx], scene);
    }

    free(tableData);

    // Reset the camera
    scene->cameraOffset.x = 0;
    scene->cameraOffset.y = 0;

    // Start with three balls
    scene->ballCount = 3;
}

/**
 * @brief TODO
 *
 * @param scene
 */
void pbStartBall(pbScene_t* scene)
{
    // Set the state
    pbSetState(scene, PBS_WAIT_TO_LAUNCH);

    // Clear loop history
    memset(scene->loopHistory, 0, sizeof(scene->loopHistory));

    // Reset targets
    for (uint16_t lIdx = 0; lIdx < scene->numLines; lIdx++)
    {
        pbLine_t* line = &scene->lines[lIdx];
        if (PB_DROP_TARGET == line->type)
        {
            line->isUp = true;
        }
    }

    // Open the launch tube
    pbOpenLaunchTube(scene, true);

    clear(&scene->balls);
    for (uint16_t pIdx = 0; pIdx < scene->numPoints; pIdx++)
    {
        if (PB_BALL_SPAWN == scene->points[pIdx].type)
        {
            pbBall_t* ball    = calloc(1, sizeof(pbBall_t));
            ball->pos         = scene->points[pIdx].pos;
            ball->vel.x       = 0;
            ball->vel.y       = 0;
            ball->radius      = PINBALL_RADIUS;
            ball->mass        = M_PI * 4.0f * 4.0f;
            ball->restitution = 0.2f;
            push(&scene->balls, ball);
            return;
        }
    }
}

/**
 * @brief
 *
 * @param scene
 */
void pbStartMultiball(pbScene_t* scene)
{
    // Don't start multiball if there are already three balls
    if (3 == scene->balls.length)
    {
        return;
    }

    // Ignore the first spawn point (tube)
    bool ignoreFirst = true;

    // For each point
    for (uint16_t pIdx = 0; pIdx < scene->numPoints; pIdx++)
    {
        // If this is a spawn point
        if (PB_BALL_SPAWN == scene->points[pIdx].type)
        {
            // Ignore the first
            if (ignoreFirst)
            {
                ignoreFirst = false;
            }
            else
            {
                // Spawn a ball here
                // TODO check if space is empty first
                pbBall_t* ball    = calloc(1, sizeof(pbBall_t));
                ball->pos         = scene->points[pIdx].pos;
                ball->vel.x       = 0;
                ball->vel.y       = 0;
                ball->radius      = PINBALL_RADIUS;
                ball->mass        = M_PI * 4.0f * 4.0f;
                ball->restitution = 0.2f;
                push(&scene->balls, ball);

                // All balls spawned
                if (3 == scene->balls.length)
                {
                    return;
                }
            }
        }
    }
}

/**
 * @brief TODO
 *
 * @param scene
 */
void pbSceneDestroy(pbScene_t* scene)
{
    if (scene->groups)
    {
        // Free the rest of the state
        free(scene->lines);
        free(scene->circles);
        free(scene->launchers);
        free(scene->flippers);
        free(scene->triangles);
        free(scene->points);

        node_t* bNode = scene->balls.first;
        while (bNode)
        {
            free(bNode->val);
            bNode = bNode->next;
        }
        clear(&scene->balls);

        for (int32_t gIdx = 0; gIdx < scene->numGroups; gIdx++)
        {
            clear(&scene->groups[gIdx]);
        }
        free(scene->groups);
        scene->groups = NULL;
    }
}

// ------------------------ user interaction ---------------------------

/**
 * @brief TODO doc
 *
 * @param scene
 * @param event
 */
void pbButtonPressed(pbScene_t* scene, buttonEvt_t* event)
{
    if (event->down)
    {
        switch (event->button)
        {
            case PB_LEFT:
            {
                for (int32_t fIdx = 0; fIdx < scene->numFlippers; fIdx++)
                {
                    if (scene->flippers[fIdx].facingRight)
                    {
                        scene->flippers[fIdx].buttonHeld = true;
                    }
                }
                break;
            }
            case PB_RIGHT:
            {
                for (int32_t fIdx = 0; fIdx < scene->numFlippers; fIdx++)
                {
                    if (!scene->flippers[fIdx].facingRight)
                    {
                        scene->flippers[fIdx].buttonHeld = true;
                    }
                }
                for (int32_t rIdx = 0; rIdx < scene->numLaunchers; rIdx++)
                {
                    scene->launchers[rIdx].buttonHeld = true;
                }
                break;
            }
            default:
            {
                break;
            }
        }
    }
    else
    {
        switch (event->button)
        {
            case PB_LEFT:
            {
                for (int32_t fIdx = 0; fIdx < scene->numFlippers; fIdx++)
                {
                    if (scene->flippers[fIdx].facingRight)
                    {
                        scene->flippers[fIdx].buttonHeld = false;
                    }
                }
                break;
            }
            case PB_RIGHT:
            {
                for (int32_t fIdx = 0; fIdx < scene->numFlippers; fIdx++)
                {
                    if (!scene->flippers[fIdx].facingRight)
                    {
                        scene->flippers[fIdx].buttonHeld = false;
                    }
                }
                for (int32_t rIdx = 0; rIdx < scene->numLaunchers; rIdx++)
                {
                    scene->launchers[rIdx].buttonHeld = false;
                }
                break;
            }
            default:
            {
                break;
            }
        }
    }
}

/**
 * @brief
 *
 * @param ball
 * @param scene
 */
void pbRemoveBall(pbBall_t* ball, pbScene_t* scene)
{
    // Clear loop history
    memset(scene->loopHistory, 0, sizeof(scene->loopHistory));

    // If the save timer is running
    if (scene->saveTimer > 0 && 1 == scene->balls.length)
    {
        // Save the ball by scooping it back
        printf("Ball Saved\n");
        ball->scoopTimer = 2000000;
    }
    else
    {
        // Find the ball in the list
        node_t* bNode = scene->balls.first;
        while (bNode)
        {
            if (ball == bNode->val)
            {
                // Remove the ball from the list
                free(bNode->val);
                removeEntry(&scene->balls, bNode);
                break;
            }
            bNode = bNode->next;
        }

        // If there are no active balls left
        if (0 == scene->balls.length)
        {
            // Decrement the overall ball count
            scene->ballCount--;

            // If there are balls left
            if (0 < scene->ballCount)
            {
                pbSetState(scene, PBS_BALL_OVER);
                // TODO show bonus set up for next ball, etc.
                pbStartBall(scene);
            }
            else
            {
                // No balls left
                pbSetState(scene, PBS_GAME_OVER);
            }
        }
    }
}

/**
 * @brief TODO
 *
 * @param scene
 * @param elapsedUs
 */
void pbGameTimers(pbScene_t* scene, int32_t elapsedUs)
{
    if (scene->saveTimer > 0)
    {
        scene->saveTimer -= elapsedUs;
    }
}

/**
 * @brief TODO
 *
 * @param scene
 * @param open
 */
void pbOpenLaunchTube(pbScene_t* scene, bool open)
{
    if (open != scene->launchTubeClosed)
    {
        scene->launchTubeClosed = open;

        if (!open)
        {
            // Start a 15s timer to save the ball when the door closes
            scene->saveTimer = 15000000;
            pbSetState(scene, PBS_GAME_NO_EVENT);
        }

        for (int32_t lIdx = 0; lIdx < scene->numLines; lIdx++)
        {
            pbLine_t* line = &scene->lines[lIdx];
            if (PB_LAUNCH_DOOR == line->type)
            {
                line->isUp = !open;
            }
        }
    }
}

/**
 * @brief TODO
 *
 * @param scene
 * @param state
 */
void pbSetState(pbScene_t* scene, pbGameState_t state)
{
    if (scene->state != state)
    {
        scene->state = state;
        switch (state)
        {
            case PBS_WAIT_TO_LAUNCH:
            {
                printf("Ball Start\n");
                break;
            }
            case PBS_GAME_NO_EVENT:
            {
                printf("Event Finished\n");
                break;
            }
            case PBS_GAME_EVENT:
            {
                printf("Event Started\n");
                break;
            }
            case PBS_BALL_OVER:
            {
                printf("Ball Lost\n");
                break;
            }
            case PBS_GAME_OVER:
            {
                printf("Game Over\n");
                break;
            }
        }
    }
}
