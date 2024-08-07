#include <stdlib.h>
#include "heatshrink_helper.h"

#include "pinball_game.h"

#include "pinball_line.h"
#include "pinball_circle.h"
#include "pinball_rectangle.h"
#include "pinball_flipper.h"

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
 * @brief TODO doc
 *
 * @param scene
 */
void jsSceneInit(jsScene_t* scene)
{
    scene->gravity.x = 0;
    scene->gravity.y = 30;
    scene->dt        = 1 / 60.0f;
    scene->score     = 0;
    scene->paused    = true;

    uint32_t decompressedSize = 0;
    uint8_t* tableData        = (uint8_t*)readHeatshrinkFile("pinball.raw", &decompressedSize, true);
    uint32_t dIdx             = 0;

    uint8_t numGroups = readInt8(tableData, &dIdx);
    // TODO alloc groups

    uint16_t linesInFile = readInt16(tableData, &dIdx);
    scene->numLines      = 0;
    for (uint16_t lIdx = 0; lIdx < linesInFile; lIdx++)
    {
        dIdx += readLineFromFile(&tableData[dIdx], scene);
    }

    uint16_t circlesInFile = readInt16(tableData, &dIdx);
    scene->numObstacles    = 0;
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

    free(tableData);

    // TODO load ball position from file
    float radius = 4.0f;
    vecFl_t pos  = {.x = 274.0f, .y = 234.0f};
    // vecFl_t pos     = {.x = 48.0f, .y = 140.0f};
    vecFl_t vel     = {.x = 0.0f, .y = 0.0f};
    scene->numBalls = 0;
    // jsBallInit(&scene->balls[scene->numBalls++], radius, M_PI * radius * radius, pos, vel, 0.2f);

    // pos.x = 160.0f;
    // pos.y = 60.0f;
    // vel.x = 0.0f;
    // vel.y = -20.0f;
    // jsBallInit(&scene->balls[scene->numBalls++], radius, M_PI * radius * radius, pos, vel, 0.2f);
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
