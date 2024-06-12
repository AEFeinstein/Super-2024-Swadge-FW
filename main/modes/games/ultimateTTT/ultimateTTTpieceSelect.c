//==============================================================================
// Includes
//==============================================================================

#include "ultimateTTTpieceSelect.h"

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
 * @brief TODO
 *
 */
void tttInputPieceSelect(ultimateTTT_t* ttt, buttonEvt_t* evt)
{
    int16_t piecesUnlocked = NUM_UNLOCKABLE_PIECES;

    if (evt->down)
    {
        switch (evt->button)
        {
            case PB_A:
            {
                // TODO lock in selection
                ttt->activePiece = ttt->selectPieceIdx;
                break;
            }
            case PB_B:
            {
                ttt->ui = TUI_MENU;
                break;
            }
            case PB_LEFT:
            {
                if (0 == ttt->selectPieceIdx)
                {
                    ttt->selectPieceIdx = piecesUnlocked - 1;
                }
                else
                {
                    ttt->selectPieceIdx--;
                }

                ttt->xSelectScrollOffset -= (ttt->pieceWsg[0].blue.large.w + SELECT_MARGIN_X);
                break;
            }
            case PB_RIGHT:
            {
                ttt->selectPieceIdx = (ttt->selectPieceIdx + 1) % piecesUnlocked;
                ttt->xSelectScrollOffset += (ttt->pieceWsg[0].blue.large.w + SELECT_MARGIN_X);
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
void tttDrawPieceSelect(ultimateTTT_t* ttt, int64_t elapsedUs)
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
    int16_t piecesUnlocked = NUM_UNLOCKABLE_PIECES;
    int16_t wsgDim         = ttt->pieceWsg[0].blue.large.h;
    int16_t yOff           = (TFT_HEIGHT - wsgDim) / 2 - 13;
    int16_t xOff           = (TFT_WIDTH - wsgDim) / 2 + ttt->xSelectScrollOffset;
    int16_t pIdx           = ttt->selectPieceIdx;

    // 'Rewind' until it's off screen
    while (xOff > 0)
    {
        xOff -= (wsgDim + SELECT_MARGIN_X);
        pIdx--;
    }

    // Don't use a negative index!
    while (pIdx < 0)
    {
        pIdx += piecesUnlocked;
    }

    // Draw icons until you're off screen
    while (xOff < TFT_WIDTH)
    {
        drawWsgSimple(&ttt->pieceWsg[pIdx].blue.large, xOff, yOff);
        drawWsgSimple(&ttt->pieceWsg[pIdx].red.large, xOff, yOff + SPACING_Y + wsgDim);

        if (pIdx == ttt->activePiece)
        {
            // Draw selection box
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

        drawWsg(&ttt->selectArrow, 0, yOff + wsgDim + (SPACING_Y / 2) - (ttt->selectArrow.h / 2), true, false, 0);
        drawWsg(&ttt->selectArrow, TFT_WIDTH - ttt->selectArrow.w,
                yOff + wsgDim + (SPACING_Y / 2) - (ttt->selectArrow.h / 2), false, false, 0);

        xOff += (wsgDim + SELECT_MARGIN_X);
        pIdx = (pIdx + 1) % piecesUnlocked;
    }
}
