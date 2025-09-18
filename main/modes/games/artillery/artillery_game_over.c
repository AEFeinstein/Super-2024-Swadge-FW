//==============================================================================
// Includes
//==============================================================================

#include "artillery_game.h"

//==============================================================================
// Static Const Variables
//==============================================================================

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
    font_t* font = ad->mRenderer->menuFont;
    drawText(font, c555, "Game Over", (TFT_WIDTH - textWidth(font, "Game Over")) / 2, (TFT_HEIGHT - font->height) / 2);
}
