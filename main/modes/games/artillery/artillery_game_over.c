//==============================================================================
// Includes
//==============================================================================

#include "artillery_game.h"
#include "artillery_paint.h"

//==============================================================================
// Static Const Variables
//==============================================================================

const char gameOverTitle[] = "Game Over";
const char gg[]            = "Good Game!";

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief TODO doc
 *
 * @param ad
 * @param evt
 */
void artilleryGameOverInput(artilleryData_t* ad, buttonEvt_t* evt)
{
    if (evt->down)
    {
        ad->mState = AMS_MENU;
    }
}

/**
 * @brief TODO doc
 *
 * @param ad
 * @param elapsedUs
 */
void artilleryGameOverLoop(artilleryData_t* ad, uint32_t elapsedUs)
{
    // Set title and draw background
    ad->paintMenu->title = gameOverTitle;
    drawMenuMega(ad->paintMenu, ad->mRenderer, elapsedUs);

    // Draw a top string
    // TODO adjust for CPU & P2P win/loss, but not pass-and-play
    font_t* font = ad->mRenderer->menuFont;
    drawTextShadow(font, c555, c000, gg, (TFT_WIDTH - textWidth(font, gg)) / 2, 66);

    // Set parameters to draw tanks
    int16_t tankR  = 30;
    int16_t margin = (TFT_WIDTH - (4 * tankR)) / 3;
    int16_t tankY  = 141;

    // Draw tanks
    drawTank(margin + tankR, tankY, tankR, ad->gameOverData[0].baseColor, ad->gameOverData[0].accentColor, 5);
    drawTank(TFT_WIDTH - (margin + tankR), tankY, tankR, ad->gameOverData[1].baseColor, ad->gameOverData[1].accentColor,
             5);

    // Set parameters to draw scores
    char scoreStr[64] = {0};
    int scoreY        = 185;

    // Draw scores
    snprintf(scoreStr, sizeof(scoreStr) - 1, "%d", ad->gameOverData[0].score);
    drawTextShadow(font, c555, c000, scoreStr, margin + tankR - textWidth(font, scoreStr) / 2, scoreY);

    snprintf(scoreStr, sizeof(scoreStr) - 1, "%d", ad->gameOverData[1].score);
    drawTextShadow(font, c555, c000, scoreStr, TFT_WIDTH - (margin + tankR) - textWidth(font, scoreStr) / 2, scoreY);
}
