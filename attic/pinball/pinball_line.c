#include "shapes.h"
#include "palette.h"

#include "pinball_line.h"
#include "pinball_physics.h"

/**
 * @brief TODO doc
 *
 * @param tableData
 * @param scene
 * @return int32_t
 */
int32_t readLineFromFile(uint8_t* tableData, pbScene_t* scene)
{
    pbLine_t* line = &scene->lines[scene->numLines++];
    uint32_t dIdx  = 0;
    line->id       = readInt16(tableData, &dIdx);
    line->groupId  = readInt8(tableData, &dIdx);
    line->group    = addToGroup(scene, line, line->groupId);
    line->p1.x     = readInt16(tableData, &dIdx);
    line->p1.y     = readInt16(tableData, &dIdx);
    line->p2.x     = readInt16(tableData, &dIdx);
    line->p2.y     = readInt16(tableData, &dIdx);
    line->type     = readInt8(tableData, &dIdx);
    line->pushVel  = readInt8(tableData, &dIdx);
    line->isUp     = readInt8(tableData, &dIdx);

    return dIdx;
}

/**
 * @brief TODO doc
 *
 * @param line
 */
void pinballDrawLine(pbLine_t* line, vec_t* cameraOffset)
{
    paletteColor_t color = c555;
    switch (line->type)
    {
        case PB_WALL:
        case PB_BALL_LOST:
        case PB_LAUNCH_DOOR:
        {
            if (!line->isUp)
            {
                return;
            }
            color = c555;
            break;
        }
        case PB_SLINGSHOT:
        {
            color = line->litTimer > 0 ? c500 : c300;
            break;
        }
        case PB_DROP_TARGET:
        {
            if (line->isUp)
            {
                color = c050;
            }
            else
            {
                color = c010;
            }
            break;
        }
        case PB_STANDUP_TARGET:
        {
            color = line->litTimer > 0 ? c004 : c002;
            break;
        }
        case PB_SPINNER:
        {
            color = c123;
            break;
        }
        case PB_SCOOP:
        {
            color = c202;
            break;
        }
    }

    drawLine(line->p1.x - cameraOffset->x, line->p1.y - cameraOffset->y, line->p2.x - cameraOffset->x,
             line->p2.y - cameraOffset->y, color, 0);
}

/**
 * @brief TODO
 *
 * @param line
 * @param elapsedUs
 */
void pbLineTimer(pbLine_t* line, int32_t elapsedUs, pbScene_t* scene)
{
    // Decrement the lit timer
    if (line->litTimer > 0)
    {
        line->litTimer -= elapsedUs;
    }

    // Decrement the reset timer
    if (line->resetTimer > 0)
    {
        line->resetTimer -= elapsedUs;

        if (line->resetTimer <= 0)
        {
            // Make sure the line isn't intersecting a ball before popping up
            bool intersecting = false;

            node_t* bNode = scene->balls.first;
            while (bNode)
            {
                pbBall_t* ball = bNode->val;
                if (ballLineIntersection(ball, line))
                {
                    intersecting = true;
                    break;
                }
                bNode = bNode->next;
            }

            // If there are no intersections
            if (!intersecting)
            {
                // Raise the target
                line->isUp = true;
            }
            else
            {
                // Try next frame
                line->resetTimer = 1;
            }
        }
    }
}
