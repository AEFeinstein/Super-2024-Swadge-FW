#include "danceNetwork.h"
#include "dn_game.h"

void dn_HandleGameInput(dn_gameData_t* gameData, buttonEvt_t* evt)
{
    if(evt->down)
    {
        if(evt->button == PB_UP && gameData->selection[1] > 0)
        {
            gameData->selection[1]--;
            gameData->tiles[gameData->selection[1]][gameData->selection[0]].yOffset = (TFT_HEIGHT >> 2) << DECIMAL_BITS;
            gameData->tiles[gameData->selection[1]][gameData->selection[0]].yVel = -700;
            gameData->alphaFaceDir = 2; //face up
        }
        else if(evt->button == PB_DOWN && gameData->selection[1] < BOARD_SIZE - 1)
        {
            gameData->selection[1]++;
            gameData->tiles[gameData->selection[1]][gameData->selection[0]].yOffset = (TFT_HEIGHT >> 2) << DECIMAL_BITS;
            gameData->tiles[gameData->selection[1]][gameData->selection[0]].yVel = -700;
            gameData->alphaFaceDir = 0; //face down
        }
        else if(evt->button == PB_LEFT && gameData->selection[0] > 0)
        {
            gameData->selection[0]--;
            gameData->tiles[gameData->selection[1]][gameData->selection[0]].yOffset = (TFT_HEIGHT >> 2) << DECIMAL_BITS;
            gameData->tiles[gameData->selection[1]][gameData->selection[0]].yVel = -700;
            gameData->alphaFaceDir = 1; //face left
        }
        else if(evt->button == PB_RIGHT && gameData->selection[0] < BOARD_SIZE - 1)
        {
            gameData->selection[0]++;
            gameData->tiles[gameData->selection[1]][gameData->selection[0]].yOffset = (TFT_HEIGHT >> 2) << DECIMAL_BITS;
            gameData->tiles[gameData->selection[1]][gameData->selection[0]].yVel = -700;
            gameData->alphaFaceDir = 3; //face right
        }
    }
}

void dn_UpdateGame(dn_gameData_t* gameData, uint32_t elapsedUs)
{
    //perform hooke's law on neighboring tiles
    for (int y = 0; y < BOARD_SIZE; y++)
    {
        for (int x = 0; x < BOARD_SIZE; x++)
        {
            // Get the current tile
            dn_tileData_t* tile = &gameData->tiles[y][x];
            int8_t dampen = 3;
            if(x == gameData->selection[0] && y == gameData->selection[1])
            {
                //the selected tile approaches a particular offset
                tile->yVel += (((int16_t)(((TFT_HEIGHT >> 2) << DECIMAL_BITS) - tile->yOffset)) / 3);
            }
            else
            {
                //all unselected tiles approach neighboring tiles
                if (y > gameData->selection[1])
                {
                    tile->yVel += (((int16_t)(gameData->tiles[y - 1][x].yOffset - tile->yOffset)) / 1);
                    dampen += y - gameData->selection[1];
                }
                if (y < gameData->selection[1])
                {
                    tile->yVel += (((int16_t)(gameData->tiles[y + 1][x].yOffset - tile->yOffset)) / 1);
                    dampen += gameData->selection[1] - y;
                }
                if (x > gameData->selection[0])
                {
                    tile->yVel += (((int16_t)(gameData->tiles[y][x - 1].yOffset - tile->yOffset)) / 1);
                    dampen += x - gameData->selection[0];
                }
                if (x < gameData->selection[0])
                {
                    tile->yVel += (((int16_t)(gameData->tiles[y][x + 1].yOffset - tile->yOffset)) / 1);
                    dampen += gameData->selection[0] - x;
                }
            }

            tile->yVel /= dampen;

            // Update position with smaller time step
            uint16_t newYOffset = tile->yOffset + tile->yVel * (elapsedUs >> 14);
            // If the the yOffset would wrap around
            if(((tile->yOffset & 0x8000) && !(newYOffset & 0x8000) && tile->yVel > 0) ||
                (!(tile->yOffset & 0x8000) && (newYOffset & 0x8000) && tile->yVel < 0))
            {
                //print a message
                ESP_LOGI("Dance Network", "Tile %d,%d yOffset hit the limit", x, y);
                // Set yVel to 0
                tile->yVel = 0;
            }
            else{
                tile->yOffset  = newYOffset;
            }
        }
    }
}

void dn_DrawGame(dn_gameData_t* gameData)
{
    dn_DrawTiles(gameData);
    int drawX = (TFT_WIDTH >> 1) + (gameData->selection[0] - gameData->selection[1] - 1) * (gameData->sprites.groundTile.w >> 1);
    int drawY = 155 + (gameData->selection[0] + gameData->selection[1]) * 13 - (gameData->tiles[gameData->selection[1]][gameData->selection[0]].yOffset >> DECIMAL_BITS);
    drawX += 10;
    drawY -= 41;
    switch (gameData->alphaFaceDir)
    {
        case(0)://face down
            drawWsgSimple(&gameData->characterAssets[DN_ALPHA].kingDown.sprite, drawX, drawY);
            /* code */
            break;
        case(1)://face left
            drawWsg(&gameData->characterAssets[DN_ALPHA].kingUp.sprite, drawX, drawY, true, false, 0);
            break;
        case(2)://face up
            drawWsgSimple(&gameData->characterAssets[DN_ALPHA].kingUp.sprite, drawX, drawY);
            /* code */
            break;
        case(3)://face right
            drawWsg(&gameData->characterAssets[DN_ALPHA].kingDown.sprite, drawX, drawY, true, false, 0);
            break;
        default:
            break;
    }
}

void dn_DrawTiles(dn_gameData_t* gameData)
{
    // Draw the tiles
    for (int y = 0; y < BOARD_SIZE; y++)
    {
        for (int x = 0; x < BOARD_SIZE; x++)
        {
            int drawX = (TFT_WIDTH >> 1) + (x - y - 1) * (gameData->sprites.groundTile.w >> 1);
            int drawY = 155 + (x + y) * 13 - (gameData->tiles[y][x].yOffset >> DECIMAL_BITS);
            drawWsgSimple(&gameData->sprites.groundTile, drawX, drawY);
        }
    }
}
