//==============================================================================
// Includes
//==============================================================================

#include "dn_characterSelect.h"
#include "dn_game.h"

//==============================================================================
// Defines
//==============================================================================

//#define SPACING_Y       4
//#define RECT_STROKE     4
//#define CHECKER_MARGIN  (SPACING_Y / 2)

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Handle a button input when marker selection is being shown
 *
 * @param gameData The entire game state
 * @param evt The button event
 */
void dn_InputCharacterSelect(dn_gameData_t* gameData, buttonEvt_t* evt)
{
    // If the button was pressed down
    if (evt->down)
    {
        switch (evt->button)
        {
            case PB_A:
            {
                bool exitAfterSelect = (-1 == gameData->activeMarkerIdx);
                // If the index hasn't changed assume it's 0
                if (-1 == gameData->selectMarkerIdx)
                {
                    gameData->selectMarkerIdx = 0;
                }
                // Select marker
                gameData->activeMarkerIdx = gameData->selectMarkerIdx;
                // Save to NVS
                writeNvs32(dnCharacterKey, gameData->activeMarkerIdx);
                if (exitAfterSelect)
                {
                    // Go to the main menu if a marker was selected for the first time
                    dn_ShowUi(UI_MENU);
                }
                break;
            }
            case PB_B:
            {
                if (-1 != gameData->activeMarkerIdx)
                {
                    // Go back to the main menu if a marker was selected
                    dn_ShowUi(UI_MENU);
                }
                break;
            }
            case PB_LEFT:
            {
                // Scroll to the left
                if (0 == gameData->selectMarkerIdx)
                {
                    gameData->selectMarkerIdx = NUM_CHARACTERS - 1;
                }
                else
                {
                    gameData->selectMarkerIdx--;
                }

                // Decrement the offset to scroll smoothly
                gameData->xSelectScrollOffset -= gameData->sprites.groundTile.w * 5;
                break;
            }
            case PB_RIGHT:
            {
                // Scroll to the right
                gameData->selectMarkerIdx = (gameData->selectMarkerIdx + 1) % NUM_CHARACTERS;
                // Increment the offset to scroll smoothly
                gameData->xSelectScrollOffset += gameData->sprites.groundTile.w * 5;
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
 * @param gameData The entire game state
 * @param elapsedUs The time elapsed since this was last called
 */
void dn_DrawCharacterSelect(dn_gameData_t* gameData, int64_t elapsedUs)
{
    // Scroll the offset if it's not centered yet
    gameData->xSelectScrollTimer += elapsedUs;
    while (gameData->xSelectScrollTimer >= 3000)
    {
        gameData->xSelectScrollTimer -= 3000;
        if (gameData->xSelectScrollOffset > 0)
        {
            gameData->xSelectScrollOffset--;
        }
        else if (gameData->xSelectScrollOffset < 0)
        {
            gameData->xSelectScrollOffset++;
        }
    }

    // Draw the background, a blank menu
    drawMenuMania(gameData->bgMenu, gameData->menuRenderer, elapsedUs);

    // Set up variables for drawing
    int16_t yOff   = MANIA_TITLE_HEIGHT + (MANIA_BODY_HEIGHT >> 2 );
    int16_t xOff   = ((TFT_WIDTH - gameData->sprites.groundTile.w) >> 1) + gameData->xSelectScrollOffset;
    int8_t pIdx   = gameData->selectMarkerIdx;

    // 'Rewind' markers until they're off screen
    while (xOff > 0)
    {
        xOff -= gameData->sprites.groundTile.w * 5;
        pIdx--;
    }

    // Don't use a negative index!
    while (pIdx < 0)
    {
        pIdx += NUM_CHARACTERS;
    }

    // Draw markers until you're off screen (sort of)
    while (xOff < TFT_WIDTH + ((gameData->sprites.groundTile.w * 5)>>1))
    {
        // Draw down on top, up on bottom
        drawWsgSimple(&gameData->characterAssets[pIdx].pawnDown, xOff, yOff - 50);
        drawWsgSimple(&gameData->characterAssets[pIdx].pawnDown, xOff + (gameData->sprites.groundTile.w >> 1) * 1, yOff - 50 + (gameData->sprites.groundTile.h >> 1) * 1);
        drawWsgSimple(&gameData->characterAssets[pIdx].kingDown, xOff + (gameData->sprites.groundTile.w >> 1) * 2, yOff - 50 + (gameData->sprites.groundTile.h >> 1) * 2);
        drawWsgSimple(&gameData->characterAssets[pIdx].pawnDown, xOff + (gameData->sprites.groundTile.w >> 1) * 3, yOff - 50 + (gameData->sprites.groundTile.h >> 1) * 3);
        drawWsgSimple(&gameData->characterAssets[pIdx].pawnDown, xOff + (gameData->sprites.groundTile.w >> 1) * 4, yOff - 50 + (gameData->sprites.groundTile.h >> 1) * 4);
        
        drawWsgSimple(&gameData->characterAssets[pIdx].pawnUp, xOff - (gameData->sprites.groundTile.w >> 1) * 4, yOff - 50 + (gameData->sprites.groundTile.h >> 1) * 4);
        drawWsgSimple(&gameData->characterAssets[pIdx].pawnUp, xOff - (gameData->sprites.groundTile.w >> 1) * 3, yOff - 50 + (gameData->sprites.groundTile.h >> 1) * 5);
        drawWsgSimple(&gameData->characterAssets[pIdx].kingUp, xOff - (gameData->sprites.groundTile.w >> 1) * 2, yOff - 50 + (gameData->sprites.groundTile.h >> 1) * 6);
        drawWsgSimple(&gameData->characterAssets[pIdx].pawnUp, xOff - (gameData->sprites.groundTile.w >> 1) * 1, yOff - 50 + (gameData->sprites.groundTile.h >> 1) * 7);
        drawWsgSimple(&gameData->characterAssets[pIdx].pawnUp, xOff, yOff - 50 + (gameData->sprites.groundTile.h >> 1) * 8);
        // If this is the active maker, draw a box around it
        //TBD

        // Increment X offset
        xOff += gameData->sprites.groundTile.w * 5;
        // Increment marker index
        pIdx = (pIdx + 1) % NUM_CHARACTERS;
    }

    // Draw arrows to indicate this can be scrolled
    // Blink the arrows
    gameData->generalTimer += elapsedUs >> 12;

    if (gameData->generalTimer > 127)
    {
        int16_t arrowY = (TFT_HEIGHT >> 1) - (gameData->font_rodin.height >> 1);

        // Draw arrows to indicate this can be scrolled
        drawText(&gameData->font_rodin, c000, "<", 3, arrowY);
        drawText(&gameData->font_rodin, c000, ">", TFT_WIDTH - 3 - textWidth(&gameData->font_rodin, ">"), arrowY);
    }
}