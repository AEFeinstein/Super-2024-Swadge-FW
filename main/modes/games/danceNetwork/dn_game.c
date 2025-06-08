#include "danceNetwork.h"

void dn_HandleGameInput(dn_gameData_t* gameData, buttonEvt_t* evt)
{
    while(checkButtonQueueWrapper(&evt))
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
}

void dn_DrawGame(dn_gameData_t* gameData, uint32_t elapsedUs)
{
    
}