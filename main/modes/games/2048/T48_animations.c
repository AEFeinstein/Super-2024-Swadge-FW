/**
 * @file T48_animations.c
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief Animation controller for 2048
 * @version 1.0.0
 * @date 2024-09-07
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "T48_animations.h"

void t48ResetAnim(t48_t* t48)
{
    for (int i = 0; i < T48_BOARD_SIZE; i++)
    {
        t48->board[i].state = STATIC;
    }
    for (int i = 0; i < T48_MAX_MOVES; i++)
    {
        t48Cell_t cell        = {0};
        t48->mvTiles[i].start = cell;
        t48->mvTiles[i].end   = cell;
        t48->mvTiles[i].speed = 0;
    }
}

void t48InitSparkles(t48_t* t48, int8_t idx, int8_t x, int8_t y, wsg_t spr)
{
    // Side to side speed
    int8_t sideSpeed      = esp_random() % 17;
    t48->sparks[idx].xSpd = sideSpeed - 9; // Centers the direction
    // Initial vertical speed
    t48->sparks[idx].ySpd = -10;
    // Convert cell coords to pixel space
    t48->sparks[idx].x = x * (T48_CELL_SIZE + T48_LINE_WEIGHT) + T48_SIDE_MARGIN + T48_LINE_WEIGHT + T48_CELL_SIZE / 2;
    t48->sparks[idx].y = y * (T48_CELL_SIZE + T48_LINE_WEIGHT) + T48_TOP_MARGIN + T48_LINE_WEIGHT + T48_CELL_SIZE / 2;
    // Set image
    t48->sparks[idx].img = spr;
    // set active
    t48->sparks[idx].active = true;
}

void t48InitMovingTilesVert(t48_t* t48)
{
    for (int i = 0; i < T48_GRID_SIZE; i++)
    {
        if (t48->sliceData.src[i] == -1 || t48->sliceData.startVals[i] == 0)
        {
            // Static
        }
        // FIXME: Need better glue logic between the math and the shuffly bits.
        t48Cell_t start, end;
        start.val                        = t48->sliceData.startVals[i];
        start.x                          = t48->sliceData.src[i];
        start.y                          = t48->sliceData.lockedCoord;
        end.x                            = i;
        end.y                            = t48->sliceData.lockedCoord;
        t48->mvTiles[t48->tileIdx].start = start;
        t48->mvTiles[t48->tileIdx].end   = end;
        t48->mvTiles[t48->tileIdx++].speed
            = (end.x - start.x) * (T48_CELL_SIZE + T48_LINE_WEIGHT) / T48_MAX_SEQ;

    }
}
