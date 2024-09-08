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

/**
 * @brief Sets an empty cell to 2 or 4 on 50/50 basis
 *
 * @param t48 Main Game Object
 * @return int8_t Cell used
 */
static int8_t t48SetRandCell(t48_t* t48)
{
    int8_t cell;
    do
    {
        cell = esp_random() % T48_BOARD_SIZE;
    } while (t48->board[cell].val != 0);
    int8_t rand = esp_random() % 10;
    if (rand == 0)
    {
        t48->board[cell].val = 4; // 10%
    }
    else
    {
        t48->board[cell].val = 2; // 90%
    }
    t48->board[cell].state = NEW;
    return cell;
}

/**
 * @brief Updates the LEDs based on a direction and adds a new tile if valid
 *
 * @param t48 Main Game Object
 * @param wasUpdated If the board has changed to a new config
 * @param dir Direction used to update board
 */
static void t48BoardUpdate(t48_t* t48, bool wasUpdated, t48Direction_t dir)
{
    if (wasUpdated)
    {
        t48SetRandCell(t48);
        led_t col = t48GetLEDColors(t48);
        t48LightLEDs(t48, dir, col);
        // FIXME: t48UpdateCells(t48);
    }
    else
    {
        led_t col = {.r = 200, .g = 200, .b = 200};
        t48LightLEDs(t48, ALL, col);
    }
}

/**
 * @brief Merge a single row or column. Cells merged from CELL_SIZE-1 -> 0.
 *
 * @param t48 Main Game Object
 * @param updated If board was already updated
 */
static void t48MergeSlice(t48_t* t48, bool* updated)
{
    for (uint8_t i = 0; i < T48_GRID_SIZE - 1; i++)
    {
        if (t48->sliceData.slice[i].val == 0)
        {
            continue;
        }
        if (t48->sliceData.slice[i].val == t48->sliceData.slice[i + 1].val)
        {
            *updated = true;
            // Animate
            t48->sliceData.dest[i + 1] = i;
            t48->sliceData.endVals[i] *= 2;
            t48->sliceData.endVals[i + 1] = 0;

            // Other
            t48->sliceData.slice[i].state = MERGED;
            // Math
            t48->sliceData.slice[i].val *= 2;
            t48->score += t48->sliceData.slice[i].val;
            // Move if merged
            for (uint8_t j = i + 1; j < T48_GRID_SIZE - 1; j++)
            {
                t48->sliceData.endVals[j]   = t48->sliceData.endVals[j + 1];
                t48->sliceData.slice[j].val = t48->sliceData.slice[j + 1].val;
            }
            // Add a 0 to end if merged
            t48->sliceData.slice[T48_GRID_SIZE - 1].val   = 0;
            t48->sliceData.slice[T48_GRID_SIZE - 1].state = STATIC;
        }
    }
}

void t48SlideDown(t48_t* t48)
{
    // Reset animation timer
    t48->globalAnim = 0;
    t48ResetAnim(t48);
    bool updated = false;
    for (uint8_t row = 0; row < T48_GRID_SIZE; row++)
    {
        for (int i = 0; i < T48_GRID_SIZE; i++)
        {
            t48->sliceData.slice[i].val   = 0;
            t48->sliceData.slice[i].x     = -1;
            t48->sliceData.slice[i].y     = -1;
            t48->sliceData.slice[i].state = STATIC;
        }
        for (int8_t col = T48_GRID_SIZE - 1, i = 0; col >= 0; col--)
        {
            if (t48->board[(row * 4) + col].val != 0)
            {
                t48->sliceData.slice[i].x     = row;
                t48->sliceData.slice[i].y     = col;
                t48->sliceData.slice[i++].val = t48->board[(row * 4) + col].val;
                if (col != (T48_GRID_SIZE - i))
                {
                    updated = true;
                }
            }
        }
        t48MergeSlice(t48, &updated);
        for (int8_t col = T48_GRID_SIZE - 1, i = 0; col >= 0; col--)
        {
            t48->board[(row * 4) + col].val   = t48->sliceData.slice[i].val;
            t48->board[(row * 4) + col].state = t48->sliceData.slice[i++].state;
        }
    }
    t48BoardUpdate(t48, updated, DOWN);
}

void t48SlideUp(t48_t* t48)
{
    // Reset animation timer
    t48->globalAnim = 0;
    t48ResetAnim(t48);
    bool updated = false;
    for (uint8_t row = 0; row < T48_GRID_SIZE; row++)
    {
        for (int i = 0; i < T48_GRID_SIZE; i++)
        {
            // Animation
            t48->sliceData.src[i]       = -1;
            t48->sliceData.dest[i]      = i;
            t48->sliceData.startVals[i] = 0;
            t48->sliceData.endVals[i]   = 0;
            t48->sliceData.lockedCoord  = row;
            // Math
            t48->sliceData.slice[i].val   = 0;
            t48->sliceData.slice[i].x     = -1;
            t48->sliceData.slice[i].y     = -1;
            t48->sliceData.slice[i].state = STATIC;
        }
        for (int8_t col = 0, i = 0; col <= T48_GRID_SIZE - 1; col++)
        {
            t48->sliceData.src[i]         = col;
            t48->sliceData.startVals[col] = t48->board[(row * 4) + col].val;
            if (t48->board[(row * 4) + col].val != 0)
            {
                // Animation slice code
                t48->sliceData.dest[i]    = i;
                t48->sliceData.endVals[i] = t48->board[(row * 4) + col].val;

                // Main slice code
                if (col != i)
                {
                    updated = true;
                }
                t48->sliceData.slice[i].x     = row;
                t48->sliceData.slice[i].y     = col;
                t48->sliceData.slice[i++].val = t48->board[(row * 4) + col].val;
            }
        }
        t48MergeSlice(t48, &updated);
        t48InitMovingTilesVert(t48);
        for (int8_t col = 0, i = 0; col <= T48_GRID_SIZE - 1; col++)
        {
            t48->board[(row * 4) + col].val   = t48->sliceData.slice[i].val;
            t48->board[(row * 4) + col].state = t48->sliceData.slice[i++].state;
        }
    }
    t48BoardUpdate(t48, updated, UP);
}

void t48SlideRight(t48_t* t48)
{
    // Reset animation timer
    t48->globalAnim = 0;
    t48ResetAnim(t48);
    bool updated = false;
    for (uint8_t col = 0; col < T48_GRID_SIZE; col++)
    {
        for (int i = 0; i < T48_GRID_SIZE; i++)
        {
            t48->sliceData.slice[i].val   = 0;
            t48->sliceData.slice[i].x     = -1;
            t48->sliceData.slice[i].y     = -1;
            t48->sliceData.slice[i].state = STATIC;
        }
        for (int8_t row = T48_GRID_SIZE - 1, i = 0; row >= 0; row--)
        {
            if (t48->board[(row * 4) + col].val != 0)
            {
                t48->sliceData.slice[i].x     = row;
                t48->sliceData.slice[i].y     = col;
                t48->sliceData.slice[i++].val = t48->board[(row * 4) + col].val;
                if (row != (T48_GRID_SIZE - i))
                {
                    updated = true;
                }
            }
        }
        t48MergeSlice(t48, &updated);
        for (int8_t row = T48_GRID_SIZE - 1, i = 0; row >= 0; row--)
        {
            t48->board[(row * 4) + col].val   = t48->sliceData.slice[i].val;
            t48->board[(row * 4) + col].state = t48->sliceData.slice[i++].state;
        }
    }
    t48BoardUpdate(t48, updated, RIGHT);
}

void t48SlideLeft(t48_t* t48)
{
    // Reset animation timer
    t48->globalAnim = 0;
    t48ResetAnim(t48);
    bool updated = false;
    for (uint8_t col = 0; col < T48_GRID_SIZE; col++)
    {
        for (int i = 0; i < T48_GRID_SIZE; i++)
        {
            t48->sliceData.slice[i].val   = 0;
            t48->sliceData.slice[i].x     = -1;
            t48->sliceData.slice[i].y     = -1;
            t48->sliceData.slice[i].state = STATIC;
        }
        for (int8_t row = 0, i = 0; row <= T48_GRID_SIZE - 1; row++)
        {
            if (t48->board[(row * 4) + col].val != 0)
            {
                if (row != i)
                {
                    updated = true;
                }
                t48->sliceData.slice[i].x     = row;
                t48->sliceData.slice[i].y     = col;
                t48->sliceData.slice[i++].val = t48->board[(row * 4) + col].val;
            }
        }
        t48MergeSlice(t48, &updated);
        for (int8_t row = 0, i = 0; row <= T48_GRID_SIZE - 1; row++)
        {
            t48->board[(row * 4) + col].val   = t48->sliceData.slice[i].val;
            t48->board[(row * 4) + col].state = t48->sliceData.slice[i++].state;
        }
    }
    t48BoardUpdate(t48, updated, LEFT);
}

void t48StartGame(t48_t* t48)
{
    // clear the board
    for (uint8_t i = 0; i < T48_BOARD_SIZE; i++)
    {
        t48->board[i].val = 0;
    }
    // FIXME t48ResetCellState(t48);
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
        if (t48->board[i].val == 2048)
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
        if (t48->board[i].val == 0)
        {
            return false;
        }
    }
    // Check if any two consecutive block match vertically
    for (uint8_t row = 0; row < T48_GRID_SIZE; row++)
    {
        for (uint8_t col = 0; col < T48_GRID_SIZE - 1; col++)
        { // -1 to account for comparison
            if (t48->board[(row * 4) + col].val == t48->board[((row + 1) * 4) + col].val)
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
            if (t48->board[(row * 4) + col].val == t48->board[(row * 4) + col + 1].val)
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
