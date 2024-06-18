//==============================================================================
// Includes
//==============================================================================

#include "ultimateTTTmarkerSelect.h"

//==============================================================================
// Defines
//==============================================================================

#define SELECT_MARGIN_X 16
#define SELECT_MARGIN_Y 16
#define SPACING_Y       4
#define RECT_STROKE     4

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Handle a button input when marker selection is being shown
 *
 * @param ttt The entire game state
 * @param evt The button event
 */
void tttInputMarkerSelect(ultimateTTT_t* ttt, buttonEvt_t* evt)
{
    // Get the number of markers unlocked
    // TODO unlock markers at a different rate?
    int16_t markersUnlocked = MIN(2 + ttt->wins, NUM_UNLOCKABLE_MARKERS);

    // If the button was pressed down
    if (evt->down)
    {
        switch (evt->button)
        {
            case PB_A:
            {
                bool exitAfterSelect = (-1 == ttt->activeMarkerIdx);
                // Select marker
                ttt->activeMarkerIdx = ttt->selectMarkerIdx;
                // Save to NVS
                writeNvs32(tttMarkerKey, ttt->activeMarkerIdx);
                if (exitAfterSelect)
                {
                    // Go to the main menu if a marker was selected for the first time
                    tttShowUi(TUI_MENU);
                }
                break;
            }
            case PB_B:
            {
                if (-1 != ttt->activeMarkerIdx)
                {
                    // Go back to the main menu if a marker was selected
                    tttShowUi(TUI_MENU);
                }
                break;
            }
            case PB_LEFT:
            {
                // Scroll to the left
                if (0 == ttt->selectMarkerIdx)
                {
                    ttt->selectMarkerIdx = markersUnlocked - 1;
                }
                else
                {
                    ttt->selectMarkerIdx--;
                }

                // Decrement the offset to scroll smoothly
                ttt->xSelectScrollOffset -= (ttt->markerWsg[0].blue.large.w + SELECT_MARGIN_X);
                break;
            }
            case PB_RIGHT:
            {
                // Scroll to the right
                ttt->selectMarkerIdx = (ttt->selectMarkerIdx + 1) % markersUnlocked;
                // Increment the offset to scroll smoothly
                ttt->xSelectScrollOffset += (ttt->markerWsg[0].blue.large.w + SELECT_MARGIN_X);
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
 * @brief Draw the marker selection UI
 *
 * @param ttt The entire game state
 * @param elapsedUs The time elapsed since this was last called
 */
void tttDrawMarkerSelect(ultimateTTT_t* ttt, int64_t elapsedUs)
{
    // Scroll the offset if it's not centered yet
    ttt->xSelectScrollTimer += elapsedUs;
    while (ttt->xSelectScrollTimer >= 3000)
    {
        ttt->xSelectScrollTimer -= 3000;
        if (ttt->xSelectScrollOffset > 0)
        {
            ttt->xSelectScrollOffset--;
        }
        else if (ttt->xSelectScrollOffset < 0)
        {
            ttt->xSelectScrollOffset++;
        }
    }

    // Draw the background, a blank menu
    drawMenuMania(ttt->bgMenu, ttt->menuRenderer, elapsedUs);

    // Set up variables for drawing
    int16_t markersUnlocked = MIN(2 + ttt->wins, NUM_UNLOCKABLE_MARKERS);
    int16_t wsgDim          = ttt->markerWsg[0].blue.large.h;
    int16_t yOff            = (TFT_HEIGHT - wsgDim) / 2 - 13;
    int16_t xOff            = (TFT_WIDTH - wsgDim) / 2 + ttt->xSelectScrollOffset;
    int16_t pIdx            = ttt->selectMarkerIdx;

    // 'Rewind' markers until they're off screen
    while (xOff > 0)
    {
        xOff -= (wsgDim + SELECT_MARGIN_X);
        pIdx--;
    }

    // Don't use a negative index!
    while (pIdx < 0)
    {
        pIdx += markersUnlocked;
    }

    // Draw markers until you're off screen
    while (xOff < TFT_WIDTH)
    {
        // Draw red on top, blue on bottom
        drawWsgSimple(&ttt->markerWsg[pIdx].red.large, xOff, yOff);
        drawWsgSimple(&ttt->markerWsg[pIdx].blue.large, xOff, yOff + SPACING_Y + wsgDim);

        // If this is the active maker, draw a box around it
        if (pIdx == ttt->activeMarkerIdx)
        {
            // Left
            fillDisplayArea(xOff - (SELECT_MARGIN_X + RECT_STROKE) / 2,                            //
                            yOff - (SELECT_MARGIN_Y - RECT_STROKE) / 2,                            //
                            xOff - (SELECT_MARGIN_X - RECT_STROKE) / 2,                            //
                            yOff + (2 * wsgDim) + SPACING_Y + (SELECT_MARGIN_Y - RECT_STROKE) / 2, //
                            c000);
            // Right
            fillDisplayArea(xOff + wsgDim + (SELECT_MARGIN_X - RECT_STROKE) / 2,                   //
                            yOff - (SELECT_MARGIN_Y - RECT_STROKE) / 2,                            //
                            xOff + wsgDim + (SELECT_MARGIN_X + RECT_STROKE) / 2,                   //
                            yOff + (2 * wsgDim) + SPACING_Y + (SELECT_MARGIN_Y - RECT_STROKE) / 2, //
                            c000);
            // Up
            fillDisplayArea(xOff - (SELECT_MARGIN_X + RECT_STROKE) / 2,          //
                            yOff - (SELECT_MARGIN_Y + RECT_STROKE) / 2,          //
                            xOff + wsgDim + (SELECT_MARGIN_X + RECT_STROKE) / 2, //
                            yOff - (SELECT_MARGIN_Y - RECT_STROKE) / 2,          //
                            c000);

            // Down
            fillDisplayArea(xOff - (SELECT_MARGIN_X + RECT_STROKE) / 2,                            //
                            yOff + (2 * wsgDim) + SPACING_Y + (SELECT_MARGIN_Y - RECT_STROKE) / 2, //
                            xOff + wsgDim + (SELECT_MARGIN_X + RECT_STROKE) / 2,                   //
                            yOff + (2 * wsgDim) + SPACING_Y + (SELECT_MARGIN_Y + RECT_STROKE) / 2, //
                            c000);
        }

        // Increment X offset
        xOff += (wsgDim + SELECT_MARGIN_X);
        // Increment marker index
        pIdx = (pIdx + 1) % markersUnlocked;
    }

    // Draw arrows to indicate this can be scrolled
    // Blink the arrows
    ttt->arrowBlinkTimer += elapsedUs;
    while (ttt->arrowBlinkTimer >= ARROW_BLINK_PERIOD)
    {
        ttt->arrowBlinkTimer -= ARROW_BLINK_PERIOD;
    }

    if (ttt->arrowBlinkTimer < (ARROW_BLINK_PERIOD / 2))
    {
        int16_t arrowY = yOff + wsgDim + (SPACING_Y / 2) - (ttt->font_rodin.height / 2);

        // Draw arrows to indicate this can be scrolled
        drawText(&ttt->font_rodin, c000, "<", 0, arrowY);
        drawText(&ttt->font_rodin, c000, ">", TFT_WIDTH - textWidth(&ttt->font_rodin, ">"), arrowY);
    }
}
