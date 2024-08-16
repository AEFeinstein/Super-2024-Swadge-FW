/**
 * @file T48_draw.c
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief Functions to draw the game board
 * @version 1.0.0
 * @date 2024-08-16
 *
 * @copyright Copyright (c) 2024
 *
 */

#include "T48_draw.h"
#include "T48_logic.h"

void t48Draw(t48_t* t48)
{
    // Blank
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);

    // Draw grid lines
    for (uint8_t i = 0; i < T48_GRID_SIZE + 1; i++)
    {
        int16_t left = i * (T48_CELL_SIZE + T48_LINE_WEIGHT);
        fillDisplayArea(T48_SIDE_MARGIN + left, T48_TOP_MARGIN, T48_SIDE_MARGIN + left + T48_LINE_WEIGHT, TFT_HEIGHT,
                        c111);
        int16_t top = i * (T48_CELL_SIZE + T48_LINE_WEIGHT);
        fillDisplayArea(T48_SIDE_MARGIN, top + T48_TOP_MARGIN, TFT_WIDTH - T48_SIDE_MARGIN,
                        top + T48_TOP_MARGIN + T48_LINE_WEIGHT, c111);
    }

    // Score
    static char textBuffer[32];
    snprintf(textBuffer, sizeof(textBuffer) - 1, "Score: %" PRIu32, t48->score);
    strcpy(t48->scoreStr, textBuffer);
    drawText(&t48->font, c555, t48->scoreStr, T48_SIDE_MARGIN, 4);

    // Draw Tiles
    t48DrawCellState(t48);
}

static uint8_t getTileSprIndex(uint32_t val)
{
    switch (val)
    {
        case 2:
            return 5;
        case 4:
            return 8;
        case 8:
            return 2;
        case 16:
            return 12;
        case 32:
            return 0;
        case 64:
            return 15;
        case 128:
            return 1;
        case 256:
            return 7;
        case 512:
            return 10;
        case 1024:
            return 9;
        case 2048:
            return 14;
        case 4096:
            return 11;
        case 8192:
            return 6;
        case 16384:
            return 13;
        case 32768:
            return 4;
        case 65535:
            return 3;
        case 131072:
            return 0; // Probably never seen
        default:
            return 0;
    }
}

static void t48DrawTileOnGrid(t48_t* t48, wsg_t* img, int8_t row, int8_t col, int16_t xOff, int16_t yOff, int32_t val)
{
    // Bail if 0
    if (val == 0)
        return;
    // Sprite
    uint16_t x_cell_offset = row * (T48_CELL_SIZE + T48_LINE_WEIGHT) + T48_SIDE_MARGIN + T48_LINE_WEIGHT + xOff;
    uint16_t y_cell_offset = col * (T48_CELL_SIZE + T48_LINE_WEIGHT) + T48_TOP_MARGIN + T48_LINE_WEIGHT + yOff;
    drawWsgSimple(img, x_cell_offset, y_cell_offset);
    // Text
    static char buffer[16];
    snprintf(buffer, sizeof(buffer) - 1, "%" PRIu32, val);
    uint16_t text_center = (textWidth(&t48->font, buffer)) / 2;
    drawText(&t48->font, c555, buffer, x_cell_offset - text_center + T48_CELL_SIZE / 2,
             y_cell_offset - 4 + T48_CELL_SIZE / 2);
}

static void t48DrawCellState(t48_t* t48)
{
    if (t48->globalAnim <= T48_MAX_SEQ)
    {
        t48->globalAnim++;
    }
    else
    {
        t48ResetCellState(t48);
    }

    // TODO: Ordering of the drawing

    t48DrawSlidingTiles(t48);
    for (int8_t i = 0; i < T48_BOARD_SIZE; i++)
    {
        int8_t row = i / T48_GRID_SIZE;
        int8_t col = i % T48_GRID_SIZE;
        switch (t48->cellState[i].state)
        {
            case STATIC:

                t48DrawTileOnGrid(t48, &t48->tiles[getTileSprIndex(t48->boardArr[row][col])], row, col, 0, 0,
                                  t48->boardArr[row][col]); // FIXME: Convert to CellState vars
                break;
            case MOVED:
                if (t48->globalAnim > T48_MAX_SEQ)
                {
                    t48DrawTileOnGrid(t48, &t48->tiles[getTileSprIndex(t48->boardArr[row][col])], row, col, 0, 0,
                                      t48->boardArr[row][col]); // FIXME: Convert to CellState vars
                }
                break;
            case MERGED:
                // FIXME: Draw new sprite once the gems combine
                if (t48->globalAnim > T48_MAX_SEQ)
                {
                    t48DrawTileOnGrid(t48, &t48->tiles[getTileSprIndex(t48->boardArr[row][col])], row, col, 0, 0,
                                      t48->boardArr[row][col]); // FIXME: Convert to CellState vars
                }
                break;
            case NEW:
                break;
            default:
                break;
        }
    }
}

static void t48DrawSlidingTiles(t48_t* t48)
{
    for (int8_t idx = 0; idx < T48_MAX_MERGES; idx++)
    {
        if (t48->slidingTiles[idx].value != 0)
        {
            int16_t xVal = 0;
            int16_t yVal = 0;
            if (t48->slidingTiles[idx].horizontal)
            {
                xVal = t48->globalAnim * t48->slidingTiles[idx].speed;
            }
            else
            {
                yVal = t48->globalAnim * t48->slidingTiles[idx].speed;
            }
            t48DrawTileOnGrid(t48, &t48->tiles[getTileSprIndex(t48->slidingTiles[idx].value)],
                              t48->slidingTiles[idx].grid.x, t48->slidingTiles[idx].grid.y, xVal, yVal,
                              t48->slidingTiles[idx].value);
        }
    }
}