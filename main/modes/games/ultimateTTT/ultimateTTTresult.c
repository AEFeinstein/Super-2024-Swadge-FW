//==============================================================================
// Includes
//==============================================================================

#include "ultimateTTTresult.h"

//==============================================================================
// Variables
//==============================================================================

static const char winStr[]          = "A winner is you!";
static const char lossStr[]         = "You lost :(";
static const char drawStr[]         = "It is a draw.";
static const char disconnectedStr[] = "Disconnected";

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Handle a button input when results are being shown
 *
 * @param ttt The entire game state
 * @param evt The button event
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
                // Return to the main menu
                tttShowUi(TUI_MENU);
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
 * @brief Draw the result UI to the display
 *
 * @param ttt The entire game state
 * @param elapsedUs The time elapsed since this was last called
 */
void tttDrawResult(ultimateTTT_t* ttt, int64_t elapsedUs)
{
    // Draw the background, a blank menu
    drawMenuMania(ttt->bgMenu, ttt->menuRenderer, elapsedUs);

    // Get the string based on the result
    const char* resultStr;
    switch (ttt->lastResult)
    {
        case TTR_WIN:
        {
            resultStr = winStr;
            break;
        }
        case TTR_LOSE:
        {
            resultStr = lossStr;
            break;
        }
        case TTR_DRAW:
        {
            resultStr = drawStr;
            break;
        }
        default:
        case TTR_DISCONNECT:
        {
            resultStr = disconnectedStr;
            break;
        }
    }

    // Build a string with the player's overall record
    // TODO show this somewhere else too?
    char recordStr[64] = {0};
    snprintf(recordStr, sizeof(recordStr) - 1, "W: %" PRId32 ", L: %" PRId32 ", D: %" PRId32, ttt->wins, ttt->losses,
             ttt->draws);

    // Y offset to start drawing
    int32_t yOff = 100;

    // Draw the result string, centered
    uint16_t tWidth = textWidth(&ttt->font_rodin, resultStr);
    drawText(&ttt->font_rodin, c000, resultStr, (TFT_WIDTH - tWidth) / 2, yOff);
    yOff += ttt->font_rodin.height + 8;

    // Draw the record string, centered
    tWidth = textWidth(&ttt->font_rodin, recordStr);
    drawText(&ttt->font_rodin, c000, recordStr, (TFT_WIDTH - tWidth) / 2, yOff);

    // TODO show new marker if unlocked
}
