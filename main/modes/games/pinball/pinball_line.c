#include "shapes.h"
#include "palette.h"

#include "pinball_line.h"

/**
 * @brief TODO doc
 *
 * @param tableData
 * @param scene
 * @return int32_t
 */
int32_t readLineFromFile(uint8_t* tableData, jsScene_t* scene)
{
    jsLine_t* line = &scene->lines[scene->numLines++];
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
    line->isSolid  = readInt8(tableData, &dIdx);
    line->isUp     = true;

    return dIdx;
}

/**
 * @brief TODO doc
 *
 * @param line
 */
void pinballDrawLine(jsLine_t* line, vec_t* cameraOffset)
{
    paletteColor_t color = c555;
    switch (line->type)
    {
        case JS_WALL:
        {
            if (!line->isUp)
            {
                return;
            }
            color = c555;
            break;
        }
        case JS_SLINGSHOT:
        {
            color = c500;
            break;
        }
        case JS_DROP_TARGET:
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
        case JS_STANDUP_TARGET:
        {
            color = c004;
            break;
        }
        case JS_SPINNER:
        {
            color = c123;
            break;
        }
        case JS_SCOOP:
        {
            color = c202;
            break;
        }
    }

    drawLine(line->p1.x - cameraOffset->x, line->p1.y - cameraOffset->y, line->p2.x - cameraOffset->x,
             line->p2.y - cameraOffset->y, color, 0);
}