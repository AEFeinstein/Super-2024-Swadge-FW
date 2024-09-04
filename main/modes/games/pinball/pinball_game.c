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
list_t* addToGroup(jsScene_t* scene, void* obj, uint8_t groupId)
{
    push(&scene->groups[groupId], obj);
    return &scene->groups[groupId];
}

/**
 * @brief TODO doc
 *
 * @param scene
 */
void jsSceneInit(jsScene_t* scene)
{
    scene->gravity.x = 0;
    scene->gravity.y = 180;
    scene->score     = 0;
    scene->paused    = true;

    uint32_t decompressedSize = 0;
    uint8_t* tableData        = (uint8_t*)readHeatshrinkFile("pinball.raw", &decompressedSize, true);
    uint32_t dIdx             = 0;

    // Allocate groups
    scene->numGroups = readInt8(tableData, &dIdx) + 1;
    scene->groups    = (list_t*)calloc(scene->numGroups, sizeof(list_t));

    uint16_t linesInFile = readInt16(tableData, &dIdx);
    scene->numLines      = 0;
    for (uint16_t lIdx = 0; lIdx < linesInFile; lIdx++)
    {
        dIdx += readLineFromFile(&tableData[dIdx], scene);
        jsLine_t* newLine = &scene->lines[scene->numLines - 1];

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

    // Launch tube door is group 1
    node_t* wallNode = scene->groups[1].first;
    while (wallNode)
    {
        // Lower the door for launch
        ((jsLine_t*)wallNode->val)->isSolid = false;
        ((jsLine_t*)wallNode->val)->isUp    = false;
        wallNode                            = wallNode->next;
    }
    // Mark the door as open
    scene->launchTubeClosed = false;

    // Clear loop history
    memset(scene->loopHistory, 0, sizeof(scene->loopHistory));

    uint16_t circlesInFile = readInt16(tableData, &dIdx);
    scene->numCircles      = 0;
    for (uint16_t cIdx = 0; cIdx < circlesInFile; cIdx++)
    {
        dIdx += readCircleFromFile(&tableData[dIdx], scene);
    }

    uint16_t rectanglesInFile = readInt16(tableData, &dIdx);
    scene->numLaunchers       = 0;
    for (uint16_t rIdx = 0; rIdx < rectanglesInFile; rIdx++)
    {
        dIdx += readRectangleFromFile(&tableData[dIdx], scene);
    }

    uint16_t flippersInFile = readInt16(tableData, &dIdx);
    scene->numFlippers      = 0;
    for (uint16_t fIdx = 0; fIdx < flippersInFile; fIdx++)
    {
        dIdx += readFlipperFromFile(&tableData[dIdx], scene);
    }

    uint16_t trianglesInFile = readInt16(tableData, &dIdx);
    scene->numTriangles      = 0;
    for (uint16_t tIdx = 0; tIdx < trianglesInFile; tIdx++)
    {
        dIdx += readTriangleFromFile(&tableData[dIdx], scene);
    }

    uint16_t pointsInFile = readInt16(tableData, &dIdx);
    scene->numPoints      = 0;
    scene->numBalls       = 0;
    for (uint16_t pIdx = 0; pIdx < pointsInFile; pIdx++)
    {
        dIdx += readPointFromFile(&tableData[dIdx], scene);

        if (JS_BALL_SPAWN == scene->points[pIdx].type)
        {
            jsBall_t* ball    = &scene->balls[scene->numBalls++];
            ball->pos         = scene->points[pIdx].pos;
            ball->vel.x       = 0;
            ball->vel.y       = 0;
            ball->radius      = PINBALL_RADIUS;
            ball->mass        = M_PI * 4.0f * 4.0f;
            ball->restitution = 0.2f;
        }
    }

    free(tableData);

    // Reset the camera
    scene->cameraOffset.x = 0;
    scene->cameraOffset.y = 0;
}

/**
 * @brief TODO
 *
 * @param scene
 */
void jsSceneDestroy(jsScene_t* scene)
{
    if (scene->groups)
    {
        // Free the rest of the state
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
void jsButtonPressed(jsScene_t* scene, buttonEvt_t* event)
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
