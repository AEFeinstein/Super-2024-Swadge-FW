/**
 * @file artillery_game_over.c
 * @author gelakinetic (gelakinetic@gmail.com)
 * @brief A screen that shows Game Over with scores
 * @date 2025-11-26
 */

//==============================================================================
// Includes
//==============================================================================

#include <math.h>
#include "artillery_game.h"
#include "artillery_game_over.h"
#include "artillery_paint.h"

//==============================================================================
// Static Const Variables
//==============================================================================

static const char str_gameOver[] = "Game Over";
static const char str_gg[]       = "Good Game!";
static const char str_win[]      = "You win!";
static const char str_lose[]     = "Tough loss";
static const char str_draw[]     = "Call it a draw";

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Process button input for the Game Over screen
 *
 * @param ad All the artillery mode data
 * @param evt The button event to process
 */
void artilleryGameOverInput(artilleryData_t* ad, buttonEvt_t* evt)
{
    // Everything returns to the main menu
    if (evt->down)
    {
        ad->mState = AMS_MENU;
    }
}

/**
 * @brief Draw the game over screen with two tanks and their scores
 *
 * @param ad All the artillery mode data
 * @param elapsedUs The time since this function was last called
 */
void artilleryGameOverLoop(artilleryData_t* ad, uint32_t elapsedUs)
{
    // Set title and draw background
    ad->blankMenu->title = str_gameOver;
    drawMenuMega(ad->blankMenu, ad->mRenderer, elapsedUs);

    // Draw a top string
    const char* title = str_gg;
    switch (ad->gameType)
    {
        default:
        case AG_PASS_AND_PLAY:
        {
            title = str_gg;
            break;
        }
        case AG_CPU_PRACTICE:
        case AG_WIRELESS:
        {
            int32_t thisScore  = 0;
            int32_t otherScore = 0;
            if (ad->gameOverData[0].isPlayer)
            {
                thisScore  = ad->gameOverData[0].score;
                otherScore = ad->gameOverData[1].score;
            }
            else
            {
                thisScore  = ad->gameOverData[1].score;
                otherScore = ad->gameOverData[0].score;
            }

            if (thisScore > otherScore)
            {
                title = str_win;
            }
            else if (thisScore < otherScore)
            {
                title = str_lose;
            }
            else
            {
                title = str_draw;
            }
            break;
        }
    }
    font_t* font = ad->mRenderer->menuFont;

    drawTextShadow(font, COLOR_TEXT, COLOR_TEXT_SHADOW, title, (TFT_WIDTH - textWidth(font, title)) / 2, 66);

    // Set parameters to draw tanks
    int16_t tankR  = 30;
    int16_t margin = (TFT_WIDTH - (4 * tankR)) / 3;
    int16_t tankY  = 141;

    vecFl_t wheelOffVert = {
        .x = 0,
        .y = 1,
    };

    vecFl_t relBarrelTip = {
        .x = sinf(M_PIf / 4) * tankR * 2,
        .y = -cosf(M_PIf / 4) * tankR * 2,
    };

    // Draw tanks
    drawTank(margin + tankR, tankY, tankR, ad->gameOverData[0].baseColor, ad->gameOverData[0].accentColor, 5,
             wheelOffVert, relBarrelTip);
    drawTank(TFT_WIDTH - (margin + tankR), tankY, tankR, ad->gameOverData[1].baseColor, ad->gameOverData[1].accentColor,
             5, wheelOffVert, relBarrelTip);

    // Set parameters to draw scores
    char scoreStr[64] = {0};
    int scoreY        = 185;

    // Draw scores
    snprintf(scoreStr, sizeof(scoreStr) - 1, "%" PRId32, ad->gameOverData[0].score);
    drawTextShadow(font, COLOR_TEXT, COLOR_TEXT_SHADOW, scoreStr, margin + tankR - textWidth(font, scoreStr) / 2,
                   scoreY);

    snprintf(scoreStr, sizeof(scoreStr) - 1, "%" PRId32, ad->gameOverData[1].score);
    drawTextShadow(font, COLOR_TEXT, COLOR_TEXT_SHADOW, scoreStr,
                   TFT_WIDTH - (margin + tankR) - textWidth(font, scoreStr) / 2, scoreY);
}
