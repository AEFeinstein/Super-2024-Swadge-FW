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

const char mode[]             = "2048";
static const char pressKey[]  = "Press any key to play";
static const char highScore[] = "You got a high score!";
static const char pressAB[]   = "Press A or B to reset the game";

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initializes all the blocks for the start screen
 *
 * @param t48 Game data
 */
void t48_initStartScreen(t48_t* t48)
{
    for (uint8_t i = 0; i < T48_START_SCREEN_BLOCKS; i++)
    {
        t48->ssBlocks[i].pos.x   = esp_random() % (TFT_WIDTH - 50);  // -50 because tile width
        t48->ssBlocks[i].pos.y   = esp_random() % (TFT_HEIGHT - 50); // +25 to re-center
        t48->ssBlocks[i].speed.x = -5 + (esp_random() % 11);         // -7 to center
        t48->ssBlocks[i].speed.y = -5 + (esp_random() % 11);         // 13 to get entire range
    }
}

/**
 * @brief Draws the Start screen
 *
 * @param t48 Game data
 * @param color Color of the title text
 */
void t48_drawStartScreen(t48_t* t48, paletteColor_t color)
{
    // Blank
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);

    // Draw random blocks

    for (uint8_t i = 0; i < T48_START_SCREEN_BLOCKS; i++)
    {
        // Move
        t48->ssBlocks[i].pos.x += t48->ssBlocks[i].speed.x;
        t48->ssBlocks[i].pos.y += t48->ssBlocks[i].speed.y;

        // Update coordinates if out of bounds
        if (t48->ssBlocks[i].pos.x < -50)
        {
            t48->ssBlocks[i].pos.x = TFT_WIDTH;
        }
        else if (t48->ssBlocks[i].pos.x > TFT_WIDTH)
        {
            t48->ssBlocks[i].pos.x = -50;
        }
        if (t48->ssBlocks[i].pos.y < -50)
        {
            t48->ssBlocks[i].pos.y = TFT_HEIGHT;
        }
        else if (t48->ssBlocks[i].pos.y > TFT_HEIGHT)
        {
            t48->ssBlocks[i].pos.y = -50;
        }

        // Draw
        drawWsgSimple(&t48->tiles[i], t48->ssBlocks[i].pos.x, t48->ssBlocks[i].pos.y);
    }

    // Title
    drawText(&t48->titleFont, color, mode, (TFT_WIDTH - textWidth(&t48->titleFont, mode)) / 2, TFT_HEIGHT / 2 - 12);
    drawText(&t48->titleFontOutline, c555, mode, (TFT_WIDTH - textWidth(&t48->titleFont, mode)) / 2,
             TFT_HEIGHT / 2 - 12);

    // Draw current High Score
    static char textBuffer[20];
    snprintf(textBuffer, sizeof(textBuffer) - 1, "High score: %" PRIu32, t48->highScore[0]);
    drawText(&t48->font, c444, textBuffer, (TFT_WIDTH - textWidth(&t48->font, textBuffer)) / 2, TFT_HEIGHT - 32);

    // Press any key...
    drawText(&t48->font, c555, pressKey, (TFT_WIDTH - textWidth(&t48->font, pressKey)) / 2, TFT_HEIGHT - 64);
}

/**
 * @brief Draws the screen showing all the high scores, player score, and prompts them to continue
 *
 * @param t48 Game Data
 * @param score Player score
 * @param pc Color of test to display
 */
void t48_drawGameOverScreen(t48_t* t48, int64_t score, paletteColor_t pc)
{
    // Clear display
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);

    // Draw player score
    static char textBuffer[32];
    snprintf(textBuffer, sizeof(textBuffer) - 1, "Final score: %" PRIu64, score);
    drawText(&t48->titleFont, c550, textBuffer, 16, 48);

    // Draw high scores
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

    // Prompt player to continue
    drawText(&t48->font, c444, pressAB, 18, TFT_HEIGHT - 16);
}