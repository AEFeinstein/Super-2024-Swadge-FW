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

static void t48StartTileMoving(t48_t* t48, int8_t idx, t48Cell_t* start, t48Cell_t* end)
{
    t48->mvTiles[idx].start = *start;
    t48->mvTiles[idx].end = *end;
    t48->mvTiles[idx].progress = 0;
    t48->mvTiles[idx].speed = (end->x - start->x + end->y - start->y) * (T48_CELL_SIZE + T48_LINE_WEIGHT) / T48_MAX_SEQ;
}

void t48ResetAnim(t48_t* t48)
{
    // Reset animation timer
    t48->globalAnim = 0;
    for (int i = 0; i < T48_BOARD_SIZE; i++)
    {
        t48->board[i].state = STATIC;
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

void t48InitMovingTiles(t48_t* t48)
{
    for (int i = 0; i < T48_GRID_SIZE; i++)
    {
        if (t48->sliceData.src[i] == t48->sliceData.dest[i] || t48->sliceData.src[i] == -1)
        {
            // Static
            continue;
        }
        // FIXME: Convert 1d coords to 2d coords
        t48Cell_t start, end;
        start.val = 4;
        start.x = 0;
        start.y = 3;
        end.x = 0;
        end.y = 0;
        t48StartTileMoving(t48, 0, &start, &end);
    }
}

/* void t48ResetCellState(t48_t* t48)
{
    for (int8_t i = 0; i < T48_BOARD_SIZE; i++)
    {
        t48->board[i].state      = STATIC;
    }
    for (int8_t i = 0; i < T48_MAX_MERGES; i++)
    {
        t48->slidingTiles[i].speed    = 0;
        t48->slidingTiles[i].value    = 0;
        t48->slidingTiles[i].sequence = 0;
    }
}

void t48SetCellState(t48_t* t48, int8_t idx, t48CellStateEnum_t st, int8_t startX, int8_t startY, int8_t endX,
                     int8_t endY, int32_t value)
{
    t48->prevBoard[idx].x = startX;
    t48->prevBoard[idx].y = startY;
    t48->board[idx].x      = endX;
    t48->board[idx].y      = endY;
    t48->board[idx].state      = st;
    t48->board[idx].val      = value;
}

void t48UpdateCells(t48_t* t48)
{
    int8_t idx      = 0;
    t48->globalAnim = 0;
    for (int8_t i = 0; i < 12; i++)
    {
        switch (t48->board[i].state)
        {
            case MOVING:
                t48SetSlidingTile(t48, idx++, t48->prevBoard[i], t48->board[i],
                                  t48->board[i].val);
                t48->board[t48->board[i].x * T48_GRID_SIZE + t48->board[i].y].state = MOVED;
                break;
            case MERGED:
                // FIXME: Move this to where the final cell is merged
                // t48->cellState[t48->cellState[i].end.x * T48_GRID_SIZE + t48->cellState[i].end.y].state = MERGED;
                break;
            case NEW:
                break;
            default:
                break;
        }
    }
}

void t48SetSlidingTile(t48_t* t48, int8_t idx, t48Cell_t start, t48Cell_t end, int32_t value)
{
    t48->slidingTiles[idx].value  = value;
    t48->slidingTiles[idx].gridStart.x = start.x;
    t48->slidingTiles[idx].gridStart.y = start.y;
    t48->slidingTiles[idx].speed
        = (end.x - start.x + end.y - start.y) * (T48_CELL_SIZE + T48_LINE_WEIGHT) / T48_MAX_SEQ;
    t48->slidingTiles[idx].horizontal = (end.x != start.x);
} */