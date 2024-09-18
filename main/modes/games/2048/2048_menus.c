/**
 * @file 2048_menus.c
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief Handles the initialization and display of non-game screens
 * @version 1.0.0
 * @date 2024-09-17
 *
 * @copyright Copyright (c) 2024
 *
 */

//==============================================================================
// Includes
//==============================================================================

#include "2048_menus.h"

//==============================================================================
// Const Variables
//==============================================================================

static const char highScore[] = "You got a high score!";
static const char pressAB[]   = "Press A or B to reset the game";

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Draws the screen showing all the high scores, player score, and prompts them to continue
 *
 * @param t48 Game Data
 * @param score Player score
 * @param pc Color of test to display
 */
void t48DrawGameOverScreen(t48_t* t48, int32_t score, paletteColor_t pc)
{
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);
    static char textBuffer[32];
    snprintf(textBuffer, sizeof(textBuffer) - 1, "Final score: %" PRIu64, score);
    drawText(&t48->titleFont, c550, textBuffer, 16, 48);
    for (int8_t i = 0; i < T48_HS_COUNT; i++)
    {
        static char initBuff[20];
        static paletteColor_t color;
        if (score == t48->highScore[i])
        {
            int16_t x = 16;
            int16_t y = 80;
            drawTextWordWrap(&t48->titleFont, pc, highScore, &x, &y, TFT_WIDTH - 16, y + 120);
            color = c500;
        }
        else
        {
            color = c444;
        }
        snprintf(initBuff, sizeof(initBuff) - 1, "%d: %d - ", i + 1, (int)t48->highScore[i]);
        strcat(initBuff, t48->hsInitials[i]);
        drawText(&t48->font, color, initBuff, 16, TFT_HEIGHT - (98 - (16 * i)));
    }
    drawText(&t48->font, c444, pressAB, 18, TFT_HEIGHT - 16);
}