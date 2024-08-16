/**
 * @file T48_logic.c
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief Game Logic for 2048
 * @version 1.0.0
 * @date 2024-08-16
 *
 * @copyright Copyright (c) 2024
 *
 */

#include "T48_logic.h"

int8_t t48SetRandCell(t48_t* t48)
{
    int8_t cell;
    do
    {
        cell = esp_random() % T48_BOARD_SIZE;
    } while (t48->boardArr[cell / T48_GRID_SIZE][cell % T48_GRID_SIZE] != 0);
    int8_t rand = esp_random() % 10;
    if (rand == 0)
    {
        t48->boardArr[cell / T48_GRID_SIZE][cell % T48_GRID_SIZE] = 4; // 10%
    }
    else
    {
        t48->boardArr[cell / T48_GRID_SIZE][cell % T48_GRID_SIZE] = 2; // 90%
    }
    t48->cellState[cell].state = STATIC;
    return cell;
}

void t48BoardUpdate(t48_t* t48, bool wasUpdated, t48Direction_t dir)
{
    if (wasUpdated)
    {
        t48SetRandCell(t48);
        led_t col = t48GetLEDColors(t48);
        t48LightLEDs(t48, dir, col);
        t48ConvertCellState(t48);
    }
    else
    {
        led_t col = {.r = 200, .g = 200, .b = 200};
        t48LightLEDs(t48, ALL, col);
    }
}

void t48MergeSlice(t48_t* t48, Slice_t slice[], bool* updated)
{
    for (uint8_t i = 0; i < T48_GRID_SIZE - 1; i++)
    {
        if (slice[i].sliceVal == 0)
        {
            continue;
        }
        if (slice[i].sliceVal == slice[i + 1].sliceVal)
        {
            for (int8_t j = 0; j < T48_BOARD_SIZE; j++)
            {
                /* if (slice[i + 1].cell.x == t48->cellState[j].end.x && slice[i + 1].cell.y == t48->cellState[j].end.y)
                {
                    t48->cellState[j].end.x = slice[i].cell.x;
                    t48->cellState[j].end.y = slice[i].cell.y;
                } */
            }
            *updated = true;
            slice[i].sliceVal *= 2;
            t48->score += slice[i].sliceVal;
            // Move if merged
            for (uint8_t j = i + 1; j < T48_GRID_SIZE - 1; j++)
            {
                slice[j].sliceVal = slice[j + 1].sliceVal;
            }
            // Add a 0 to end if merged
            slice[T48_GRID_SIZE - 1].sliceVal = 0;
        }
    }
}

// NOTE: All these seem to be backwards.
// All other systems use row, column, but these seem to require column, row which is mirrors the movements along the
// top left to bottom right diagonal.
// I have left the odd behavior here as it's simple to remap keys to different functions rather than compensate in the
// visual code.

void t48SlideDown(t48_t* t48)
{
    t48ResetCellState(t48);
    bool updated = false;
    int8_t idx   = 0;
    for (uint8_t row = 0; row < T48_GRID_SIZE; row++)
    {
        Slice_t slice[T48_GRID_SIZE] = {0};
        for (int8_t col = T48_GRID_SIZE - 1, i = 0; col >= 0; col--)
        {
            if (t48->boardArr[row][col] != 0)
            {
                t48SetCellState(t48, idx++, MOVING, row, col, row, T48_GRID_SIZE - 1 - i, t48->boardArr[row][col]);
                slice[i].cell.x     = row;
                slice[i].cell.y     = col;
                slice[i++].sliceVal = t48->boardArr[row][col];
                if (col != (T48_GRID_SIZE - i))
                {
                    updated = true;
                }
            }
        }
        t48MergeSlice(t48, slice, &updated);
        for (int8_t col = T48_GRID_SIZE - 1, i = 0; col >= 0; col--)
        {
            t48->boardArr[row][col] = slice[i++].sliceVal;
        }
    }
    t48BoardUpdate(t48, updated, DOWN);
}

void t48SlideUp(t48_t* t48)
{
    t48ResetCellState(t48);
    bool updated = false;
    int8_t idx   = 0;
    for (uint8_t row = 0; row < T48_GRID_SIZE; row++)
    {
        Slice_t slice[T48_GRID_SIZE] = {0};
        for (int8_t col = 0, i = 0; col <= T48_GRID_SIZE - 1; col++)
        {
            if (t48->boardArr[row][col] != 0)
            {
                if (col != i)
                {
                    updated = true;
                }
                t48SetCellState(t48, idx++, MOVING, row, col, row, i, t48->boardArr[row][col]);
                slice[i].cell.x     = row;
                slice[i].cell.y     = col;
                slice[i++].sliceVal = t48->boardArr[row][col];
            }
        }
        t48MergeSlice(t48, slice, &updated);
        for (int8_t col = 0, i = 0; col <= T48_GRID_SIZE - 1; col++)
        {
            t48->boardArr[row][col] = slice[i++].sliceVal;
        }
    }
    t48BoardUpdate(t48, updated, UP);
}

void t48SlideRight(t48_t* t48)
{
    t48ResetCellState(t48);
    bool updated = false;
    int8_t idx   = 0;
    for (uint8_t col = 0; col < T48_GRID_SIZE; col++)
    {
        Slice_t slice[T48_GRID_SIZE] = {0};
        for (int8_t row = T48_GRID_SIZE - 1, i = 0; row >= 0; row--)
        {
            if (t48->boardArr[row][col] != 0)
            {
                t48SetCellState(t48, idx++, MOVING, row, col, T48_GRID_SIZE - 1 - i, col, t48->boardArr[row][col]);
                slice[i].cell.x     = row;
                slice[i].cell.y     = col;
                slice[i++].sliceVal = t48->boardArr[row][col];
                if (row != (T48_GRID_SIZE - i))
                {
                    updated = true;
                }
            }
        }
        t48MergeSlice(t48, slice, &updated);
        for (int8_t row = T48_GRID_SIZE - 1, i = 0; row >= 0; row--)
        {
            t48->boardArr[row][col] = slice[i++].sliceVal;
        }
    }
    t48BoardUpdate(t48, updated, RIGHT);
}

void t48SlideLeft(t48_t* t48)
{
    t48ResetCellState(t48);
    bool updated = false;
    int8_t idx   = 0;
    for (uint8_t col = 0; col < T48_GRID_SIZE; col++)
    {
        Slice_t slice[T48_GRID_SIZE] = {0};
        for (int8_t row = 0, i = 0; row <= T48_GRID_SIZE - 1; row++)
        {
            if (t48->boardArr[row][col] != 0)
            {
                if (row != i)
                {
                    updated = true;
                }
                t48SetCellState(t48, idx++, MOVING, row, col, i, col, t48->boardArr[row][col]);
                slice[i].cell.x     = row;
                slice[i].cell.y     = col;
                slice[i++].sliceVal = t48->boardArr[row][col];
            }
        }
        t48MergeSlice(t48, slice, &updated);
        for (int8_t row = 0, i = 0; row <= T48_GRID_SIZE - 1; row++)
        {
            t48->boardArr[row][col] = slice[i++].sliceVal;
        }
    }
    t48BoardUpdate(t48, updated, LEFT);
}

void t48StartGame(t48_t* t48)
{
    // clear the board
    for (uint8_t i = 0; i < T48_BOARD_SIZE; i++)
    {
        t48->boardArr[i / T48_GRID_SIZE][i % T48_GRID_SIZE] = 0;
    }
    t48ResetCellState(t48);
    t48->alreadyWon = false;
    t48->score      = 0;
    // Get random places to start
    t48SetRandCell(t48);
    t48SetRandCell(t48);
}

bool t48CheckWin(t48_t* t48)
{
    if (t48->alreadyWon)
    {
        return false;
    }
    for (uint8_t i = 0; i < T48_BOARD_SIZE; i++)
    {
        if (t48->boardArr[i / T48_GRID_SIZE][i % T48_GRID_SIZE] == 2048)
        {
            t48->alreadyWon = true;
            return true;
        }
    }
    return false;
}

bool t48CheckOver(t48_t* t48)
{
    // Check if any cells are open
    for (uint8_t i = 0; i < T48_BOARD_SIZE; i++)
    {
        if (t48->boardArr[i / T48_GRID_SIZE][i % T48_GRID_SIZE] == 0)
        {
            return false;
        }
    }
    // Check if any two consecutive block match vertically
    for (uint8_t row = 0; row < T48_GRID_SIZE; row++)
    {
        for (uint8_t col = 0; col < T48_GRID_SIZE - 1; col++)
        { // -1 to account for comparison
            if (t48->boardArr[row][col] == t48->boardArr[row + 1][col])
            {
                return false;
            }
        }
    }
    // Check if any two consecutive block match horizontally
    for (uint8_t row = 0; row < T48_GRID_SIZE - 1; row++)
    { // -1 to account for comparison
        for (uint8_t col = 0; col < T48_GRID_SIZE - 1; col++)
        {
            if (t48->boardArr[row][col] == t48->boardArr[row][col + 1])
            {
                return false;
            }
        }
    }
    // Game is over
    return true;
}

void t48SortHighScores(t48_t* t48)
{
    // 5th place needs to compare to the score
    if (t48->highScore[T48_HS_COUNT - 1] < t48->score)
    {
        t48->highScore[T48_HS_COUNT - 1] = t48->score;
        strcpy(t48->hsInitials[T48_HS_COUNT - 1], t48->playerInitials);
    }
    else
    {
        // Scores *should* be sorted already. Save cycles.
        return;
    }
    for (int8_t i = T48_HS_COUNT - 2; i >= 0; i--)
    {
        if (t48->highScore[i] < t48->highScore[i + 1])
        {
            // Swap
            int32_t swap          = t48->highScore[i];
            t48->highScore[i]     = t48->highScore[i + 1];
            t48->highScore[i + 1] = swap;
            char swapI[4];
            strcpy(swapI, t48->hsInitials[i]);
            strcpy(t48->hsInitials[i], t48->hsInitials[i + 1]);
            strcpy(t48->hsInitials[i + 1], swapI);
        }
    }
    // Save out the new scores
    for (int8_t i = 0; i < T48_HS_COUNT; i++)
    {
        writeNvs32(highScoreKey[i], t48->highScore[i]);
        writeNvsBlob(highScoreInitialsKey[i], &t48->hsInitials[i], 4);
    }
}

void t48ResetCellState(t48_t* t48)
{
    for (int8_t i = 0; i < T48_BOARD_SIZE; i++)
    {
        t48->cellState[i].incoming.x = 0;
        t48->cellState[i].incoming.y = 0;
        t48->cellState[i].end.x      = 0;
        t48->cellState[i].end.y      = 0;
        t48->cellState[i].state      = STATIC;
        t48->cellState[i].value      = 0;
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
    t48->cellState[idx].incoming.x = startX;
    t48->cellState[idx].incoming.y = startY;
    t48->cellState[idx].end.x      = endX;
    t48->cellState[idx].end.y      = endY;
    t48->cellState[idx].state      = st;
    t48->cellState[idx].value      = value;
}

void t48ConvertCellState(t48_t* t48)
{
    int8_t idx      = 0;
    t48->globalAnim = 0;
    for (int8_t i = 0; i < T48_BOARD_SIZE; i++)
    {
        switch (t48->cellState[i].state)
        {
            case MOVING:
                t48SetSlidingTile(t48, idx++, t48->cellState[i].incoming, t48->cellState[i].end,
                                  t48->cellState[i].value);
                t48->cellState[t48->cellState[i].end.x * T48_GRID_SIZE + t48->cellState[i].end.y].state = MOVED;
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

void t48SetSlidingTile(t48_t* t48, int8_t idx, t48CellCoors_t start, t48CellCoors_t end, int32_t value)
{
    t48->slidingTiles[idx].value  = value;
    t48->slidingTiles[idx].grid.x = start.x;
    t48->slidingTiles[idx].grid.y = start.y;
    t48->slidingTiles[idx].speed
        = (end.x - start.x + end.y - start.y) * (T48_CELL_SIZE + T48_LINE_WEIGHT) / T48_MAX_SEQ;
    t48->slidingTiles[idx].horizontal = (end.x != start.x);
}