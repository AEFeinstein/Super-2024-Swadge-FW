//==============================================================================
// Includes
//==============================================================================

#include "ultimateTTTresult.h"

//==============================================================================
// Variables
//==============================================================================

static const char redStr[]          = "Red wins!";
static const char blueStr[]         = "Blue wins!";
static const char winStr[]          = "A winner is you!";
static const char lossStr[]         = "You lost :(";
static const char drawStr[]         = "It is a draw.";
static const char disconnectedStr[] = "Disconnected";
static const char unlockStr[]       = "New marker unlocked!";

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
    // Get the string based on the result
    const char* resultStr;
    switch (ttt->lastResult)
    {
        case TTR_WIN:
        {
            if (ttt->game.singleSystem)
            {
                if (GOING_FIRST == ttt->game.singlePlayerPlayOrder)
                {
                    resultStr                         = redStr;
                    ttt->menuRenderer->baseLedColor.r = 0xFF;
                    ttt->menuRenderer->baseLedColor.g = 0x00;
                    ttt->menuRenderer->baseLedColor.b = 0x00;
                }
                else
                {
                    resultStr                         = blueStr;
                    ttt->menuRenderer->baseLedColor.r = 0x00;
                    ttt->menuRenderer->baseLedColor.g = 0x00;
                    ttt->menuRenderer->baseLedColor.b = 0xFF;
                }
            }
            else
            {
                resultStr = winStr;
            }
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
        case TTR_DISCONNECT:
        {
            resultStr = disconnectedStr;
            break;
        }
        default:
        case TTR_RECORDS:
        {
            resultStr = NULL;
        }
    }

    // Get assets if a new marker was unlocked
    tttMarkerColorAssets_t* unlockedMarker = NULL;
    if (TTR_WIN == ttt->lastResult)
    {
        // Check for unlocked markers
        for (int16_t mIdx = 0; mIdx < NUM_UNLOCKABLE_MARKERS; mIdx++)
        {
            // If the player got the required number of wins
            if (0 != ttt->wins && markersUnlockedAtWins[mIdx] == ttt->wins)
            {
                // Get the asset
                unlockedMarker = &ttt->markerWsg[mIdx];
                break;
            }
        }
    }

    // Spacing between lines
    int16_t ySpacing = 8;

    // Draw the background
    drawMenuMania(ttt->bgMenu, ttt->menuRenderer, elapsedUs);

    // Draw a result string, if there is one
    uint16_t tWidth;
    if (resultStr)
    {
        // Always show the W/L/D record
        int16_t tHeight = ttt->font_rodin.height;

        // Add height if a results string is shown
        if (NULL != resultStr)
        {
            tHeight += (ySpacing + ttt->font_rodin.height);
        }

        // Add height if an unlock is shown
        if (NULL != unlockedMarker)
        {
            tHeight += (ySpacing + ttt->font_rodin.height);
            tHeight += (ySpacing + ttt->markerWsg[0].blue.large.h);
        }

        // Center the text, vertically
        int16_t yOff = MANIA_TITLE_HEIGHT + (MANIA_BODY_HEIGHT - tHeight) / 2;

        // Center it
        tWidth = textWidth(&ttt->font_rodin, resultStr);
        drawText(&ttt->font_rodin, c000, resultStr, (TFT_WIDTH - tWidth) / 2, yOff);
        yOff += ttt->font_rodin.height + ySpacing;

        // Build a string with the player's overall record
        char recordStr[64] = {0};
        snprintf(recordStr, sizeof(recordStr) - 1, "W: %" PRId32 ", L: %" PRId32 ", D: %" PRId32, ttt->wins,
                 ttt->losses, ttt->draws);

        // Draw the record string, centered
        tWidth = textWidth(&ttt->font_rodin, recordStr);
        drawText(&ttt->font_rodin, c000, recordStr, (TFT_WIDTH - tWidth) / 2, yOff);
        yOff += ttt->font_rodin.height + ySpacing;

        // Show new marker if unlocked
        if (NULL != unlockedMarker)
        {
            tWidth = textWidth(&ttt->font_rodin, unlockStr);
            drawText(&ttt->font_rodin, c000, unlockStr, (TFT_WIDTH - tWidth) / 2, yOff);
            yOff += ttt->font_rodin.height + ySpacing;

            drawWsgSimple(&unlockedMarker->red.large, (TFT_WIDTH / 2) - (unlockedMarker->red.large.w + 4), yOff);
            drawWsgSimple(&unlockedMarker->blue.large, (TFT_WIDTH / 2) + 4, yOff);
        }
    }
    else
    {
        // Always show the W/L/D record
        int16_t tHeight = (ttt->font_rodin.height * 3) + (ySpacing * 2);

        // Center the text, vertically
        int16_t yOff = MANIA_TITLE_HEIGHT + (MANIA_BODY_HEIGHT - tHeight) / 2;

        char recordStr[64] = {0};

        sprintf(recordStr, "Wins: %" PRId32, ttt->wins);
        // Draw the record string, centered
        tWidth = textWidth(&ttt->font_rodin, recordStr);
        drawText(&ttt->font_rodin, c000, recordStr, (TFT_WIDTH - tWidth) / 2, yOff);
        yOff += ttt->font_rodin.height + ySpacing;

        sprintf(recordStr, "Losses: %" PRId32, ttt->losses);
        // Draw the record string, centered
        tWidth = textWidth(&ttt->font_rodin, recordStr);
        drawText(&ttt->font_rodin, c000, recordStr, (TFT_WIDTH - tWidth) / 2, yOff);
        yOff += ttt->font_rodin.height + ySpacing;

        sprintf(recordStr, "Draws: %" PRId32, ttt->draws);
        // Draw the record string, centered
        tWidth = textWidth(&ttt->font_rodin, recordStr);
        drawText(&ttt->font_rodin, c000, recordStr, (TFT_WIDTH - tWidth) / 2, yOff);
        yOff += ttt->font_rodin.height + ySpacing;
    }
}
