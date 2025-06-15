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
// Variables
//==============================================================================
const bool selectDiamondShape[] = {
    false, false, true, false, false,
    false, true, true, false, false,
    false, true, true, true, false,
    true, true, true, true, false,
    true, true, true, true, true,
    true, true, true, true, false,
    false, true, true, true, false,
    false, true, true, false, false,
    false, false, true, false, false,
};

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
        // Draw tiles until you're off screen
        while (xOff < TFT_WIDTH + ((gameData->sprites.groundTile.w * 5)>>1))
        {
            for(int16_t x = -2; x < 3; x++)
            {
                int16_t drawX = xOff + x * gameData->sprites.groundTile.w + ((gameData->sprites.groundTile.w >> 1) * (y % 2));
                int16_t drawY = yOff + y * (gameData->sprites.groundTile.h >> 1);
                if(drawX >= -gameData->sprites.groundTile.w &&
                    drawX <= TFT_WIDTH)
                {
                    // If this is the active maker, draw swapped pallete
                    if (pIdx == gameData->activeMarkerIdx && selectDiamondShape[y * 5 + x+2])
                    {
                        drawWsgPaletteSimple(&gameData->sprites.groundTile, drawX, drawY, &gameData->redFloor1);
                    }
                    else
                    {
                        drawWsgSimple(&gameData->sprites.groundTile, drawX, drawY);
                    }
                    
                }
            }
            // Increment X offset
            xOff += gameData->sprites.groundTile.w * 5;
            // Increment marker index
            pIdx = (pIdx + 1) % NUM_CHARACTERS;
        }
        //reset values
        xOff   = ((TFT_WIDTH - gameData->sprites.groundTile.w) >> 1) + gameData->xSelectScrollOffset;
        pIdx   = gameData->selectMarkerIdx;

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
    }


    // Draw characters until you're off screen (sort of)
    while (xOff < TFT_WIDTH + ((gameData->sprites.groundTile.w * 5)>>1))
    {
        for(int8_t i = 0; i < 5; i++)
        {
            if(i == 2)//king is the middle piece
            {
                drawWsgSimple(&gameData->characterAssets[pIdx].kingDown.sprite,
                    xOff + (gameData->sprites.groundTile.w >> 1) * i + gameData->characterAssets[pIdx].kingDown.xOff, 
                    yOff + (gameData->sprites.groundTile.h >> 1) * i + gameData->characterAssets[pIdx].kingDown.yOff);
                drawWsgSimple(&gameData->characterAssets[pIdx].kingUp.sprite,
                    xOff - (gameData->sprites.groundTile.w >> 1) * (4-i)  + gameData->characterAssets[pIdx].kingUp.xOff,
                    yOff + (gameData->sprites.groundTile.h >> 1) * (4+i)  + gameData->characterAssets[pIdx].kingUp.yOff);
            }
            else
            {
                drawWsgSimple(&gameData->characterAssets[pIdx].pawnDown.sprite,
                    xOff + (gameData->sprites.groundTile.w >> 1) * i + gameData->characterAssets[pIdx].pawnDown.xOff,
                    yOff + (gameData->sprites.groundTile.h >> 1) * i + gameData->characterAssets[pIdx].pawnDown.yOff);
                drawWsgSimple(&gameData->characterAssets[pIdx].pawnUp.sprite,
                    xOff - (gameData->sprites.groundTile.w >> 1) * (4-i) + gameData->characterAssets[pIdx].pawnUp.xOff,
                    yOff + (gameData->sprites.groundTile.h >> 1) * (4+i)  + gameData->characterAssets[pIdx].pawnUp.yOff);
            }
        }

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