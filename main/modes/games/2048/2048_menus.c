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

const char mode[]         = "2048";
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
    // Set random x and y coordinates for all blocks
    /* for (uint8_t i = 0; i < T48_TILE_COUNT; i++)
    {
        t48->fb[i].image  = &t48->tiles[i];
        t48->fb[i].pos[0] = (esp_random() % (TFT_WIDTH + (2 * T48_CELL_SIZE))) - T48_CELL_SIZE;
        t48->fb[i].pos[1] = (esp_random() % (TFT_HEIGHT + (2 * T48_CELL_SIZE))) - T48_CELL_SIZE;
        t48->fb[i].spd    = (esp_random() % 2) + 1;
        t48->fb[i].dir    = (esp_random() % 3) - 1; // -1 for left, 0 for down, 1 for right
    } */
}

/**
 * @brief Draws the
 *
 * @param t48
 * @param color
 */
void t48_drawStartScreen(t48_t* t48, paletteColor_t color)
{
    // Blank
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);

    // Draw random blocks

    /* for (uint8_t i = 0; i < T48_TILE_COUNT; i++)
    {
        // Move block
        t48->fb[i].pos[1] += t48->fb[i].spd;
        t48->fb[i].pos[0] += t48->fb[i].spd * t48->fb[i].dir;
        // Wrap block if outside visual area
        if (t48->fb[i].pos[0] >= TFT_WIDTH + T48_CELL_SIZE)
        { // Wraps from right to left
            t48->fb[i].pos[0] = -T48_CELL_SIZE;
        }
        if (t48->fb[i].pos[0] <= -T48_CELL_SIZE)
        { // Wraps from left to right
            t48->fb[i].pos[0] = TFT_WIDTH + T48_CELL_SIZE;
        }
        if (t48->fb[i].pos[1] >= TFT_HEIGHT + T48_CELL_SIZE)
        { // Wraps from bottom ot top
            t48->fb[i].pos[1] = -T48_CELL_SIZE;
            t48->fb[i].pos[0] = (esp_random() % (TFT_WIDTH - T48_CELL_SIZE));
        }
        // Draw block
        drawWsgSimple(t48->fb[i].image, t48->fb[i].pos[0], t48->fb[i].pos[1]);
    } */

    // Title
    drawText(&t48->titleFont, color, mode, (TFT_WIDTH - textWidth(&t48->titleFont, mode)) / 2,
             TFT_HEIGHT / 2 - 12);
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
    // CLear display
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