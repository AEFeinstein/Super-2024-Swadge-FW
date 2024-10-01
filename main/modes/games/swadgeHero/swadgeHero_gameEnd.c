//==============================================================================
// Includes
//==============================================================================

#include "swadgeHero_gameEnd.h"

//==============================================================================
// Defines
//==============================================================================

//==============================================================================
// Const Variables
//==============================================================================

//==============================================================================
// Function Declarations
//==============================================================================

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief TODO
 *
 * @param sh
 * @param evt
 */
void shGameEndInput(shVars_t* sh, buttonEvt_t* evt)
{
    if (evt->down)
    {
        switch (evt->button)
        {
            case PB_A:
            {
                shChangeScreen(sh, SH_MENU);
                break;
            }
            default:
            {
                break;
            }
        }
    }
}

/**
 * @brief TODO
 *
 * @param sh
 * @param elapsedUs
 */
void shGameEndDraw(shVars_t* sh, int32_t elapsedUs)
{
    clearPxTft();

    char scoreStr[32];
    snprintf(scoreStr, sizeof(scoreStr) - 1, "%" PRId32, sh->score);
    int16_t tWidth = textWidth(&sh->rodin, scoreStr);
    drawText(&sh->rodin, c555, scoreStr, (TFT_WIDTH - tWidth) / 2, (TFT_HEIGHT - sh->rodin.height) / 2);
}
