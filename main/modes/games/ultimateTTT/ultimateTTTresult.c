//==============================================================================
// Includes
//==============================================================================

#include "ultimateTTTresult.h"

//==============================================================================
// Variables
//==============================================================================

static const char winStr[]  = "A winner is you!";
static const char lossStr[] = "You lost :(";
static const char drawStr[] = "It is a draw.";

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief TODO
 *
 */
void tttInputResult(ultimateTTT_t* ttt, buttonEvt_t* evt)
{
    if (evt->down)
    {
        switch (evt->button)
        {
            case PB_A:
            case PB_B:
            {
                ttt->ui = TUI_MENU;
                break;
            }
            default:
            {
                // Some unused button
                break;
            }
        }
    }
}

/**
 * @brief TODO
 *
 * @param ttt
 * @param elapsedUs
 */
void tttDrawResult(ultimateTTT_t* ttt, int64_t elapsedUs)
{
    // Draw the background, a blank menu
    drawMenuMania(ttt->bgMenu, ttt->menuRenderer, elapsedUs);

    const char* resultStr;
    switch (ttt->lastResult)
    {
        case TTT_P1:
        {
            // Won
            resultStr = winStr;
            break;
        }
        case TTT_P2:
        {
            // lost
            resultStr = lossStr;
            break;
        }
        default:
        case TTT_DRAW:
        {
            // Draw
            resultStr = drawStr;
            break;
        }
    }

    char recordStr[64] = {0};
    snprintf(recordStr, sizeof(recordStr) - 1, "W: %" PRId32 ", L: %" PRId32 ", D: %" PRId32, ttt->wins, ttt->losses,
             ttt->draws);

    int32_t yOff = 100;

    uint16_t tWidth = textWidth(&ttt->font_rodin, resultStr);
    drawText(&ttt->font_rodin, c000, resultStr, (TFT_WIDTH - tWidth) / 2, yOff);
    yOff += ttt->font_rodin.height + 8;

    tWidth = textWidth(&ttt->font_rodin, recordStr);
    drawText(&ttt->font_rodin, c000, recordStr, (TFT_WIDTH - tWidth) / 2, yOff);

    // TODO show new piece if unlocked
}
