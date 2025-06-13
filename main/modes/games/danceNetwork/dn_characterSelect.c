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
    int16_t yOff   = MANIA_TITLE_HEIGHT + 20;
    int16_t xOff   = ((TFT_WIDTH - gameData->sprites.groundTile.w) >> 1) + gameData->xSelectScrollOffset;
    int8_t pIdx   = gameData->selectMarkerIdx;

    // 'Rewind' characters until they're off screen
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

    //Draw floor tiles
    for(int16_t y = 0; y < 9; y++)
    {
        for(int16_t x = 0; x < 15; x++)
        {
            int16_t drawX = xOff + x * gameData->sprites.groundTile.w + ((gameData->sprites.groundTile.w >> 1) * (y % 2));
            int16_t drawY = yOff + y * (gameData->sprites.groundTile.h >> 1);
            if(drawX >= -gameData->sprites.groundTile.w &&
                drawX <= TFT_WIDTH)
            {
                drawWsgSimple(&gameData->sprites.groundTile, drawX, drawY);
            }
        }
    }

    // Draw characters until you're off screen (sort of)
    while (xOff < TFT_WIDTH + ((gameData->sprites.groundTile.w * 5)>>1))
    {
        // Draw down on top, up on bottom
        drawWsgSimple(&gameData->characterAssets[pIdx].pawnDown.sprite, xOff + gameData->characterAssets[pIdx].pawnDown.xOff, yOff + gameData->characterAssets[pIdx].pawnDown.yOff);
        drawWsgSimple(&gameData->characterAssets[pIdx].pawnDown.sprite, xOff + (gameData->sprites.groundTile.w >> 1) * 1 + gameData->characterAssets[pIdx].pawnDown.xOff, yOff + (gameData->sprites.groundTile.h >> 1) * 1 + gameData->characterAssets[pIdx].pawnDown.yOff);
        drawWsgSimple(&gameData->characterAssets[pIdx].kingDown.sprite, xOff + (gameData->sprites.groundTile.w >> 1) * 2 + gameData->characterAssets[pIdx].kingDown.xOff, yOff + (gameData->sprites.groundTile.h >> 1) * 2 + gameData->characterAssets[pIdx].kingDown.yOff);
        drawWsgSimple(&gameData->characterAssets[pIdx].pawnDown.sprite, xOff + (gameData->sprites.groundTile.w >> 1) * 3 + gameData->characterAssets[pIdx].pawnDown.xOff, yOff + (gameData->sprites.groundTile.h >> 1) * 3 + gameData->characterAssets[pIdx].pawnDown.yOff);
        drawWsgSimple(&gameData->characterAssets[pIdx].pawnDown.sprite, xOff + (gameData->sprites.groundTile.w >> 1) * 4 + gameData->characterAssets[pIdx].pawnDown.xOff, yOff + (gameData->sprites.groundTile.h >> 1) * 4 + gameData->characterAssets[pIdx].pawnDown.yOff);
        
        drawWsgSimple(&gameData->characterAssets[pIdx].pawnUp.sprite, xOff - (gameData->sprites.groundTile.w >> 1) * 4 + gameData->characterAssets[pIdx].pawnUp.xOff, yOff + (gameData->sprites.groundTile.h >> 1) * 4 + gameData->characterAssets[pIdx].pawnUp.yOff);
        drawWsgSimple(&gameData->characterAssets[pIdx].pawnUp.sprite, xOff - (gameData->sprites.groundTile.w >> 1) * 3 + gameData->characterAssets[pIdx].pawnUp.xOff, yOff + (gameData->sprites.groundTile.h >> 1) * 5 + gameData->characterAssets[pIdx].pawnUp.yOff);
        drawWsgSimple(&gameData->characterAssets[pIdx].kingUp.sprite, xOff - (gameData->sprites.groundTile.w >> 1) * 2 + gameData->characterAssets[pIdx].kingUp.xOff, yOff + (gameData->sprites.groundTile.h >> 1) * 6 + gameData->characterAssets[pIdx].kingUp.yOff);
        drawWsgSimple(&gameData->characterAssets[pIdx].pawnUp.sprite, xOff - (gameData->sprites.groundTile.w >> 1) * 1 + gameData->characterAssets[pIdx].pawnUp.xOff, yOff + (gameData->sprites.groundTile.h >> 1) * 7 + gameData->characterAssets[pIdx].pawnUp.yOff);
        drawWsgSimple(&gameData->characterAssets[pIdx].pawnUp.sprite, xOff + gameData->characterAssets[pIdx].pawnUp.xOff, yOff + (gameData->sprites.groundTile.h >> 1) * 8 + gameData->characterAssets[pIdx].pawnUp.yOff);
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
        // Draw arrows to indicate this can be scrolled
        drawText(&gameData->font_rodin, c000, "<", 3, 53);
        drawText(&gameData->font_rodin, c000, ">", TFT_WIDTH - 3 - textWidth(&gameData->font_rodin, ">"), 53);
    }
}