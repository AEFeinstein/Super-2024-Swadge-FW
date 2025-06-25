#include "dn_entity.h"

void dn_setData(dn_entity_t* self, void* data, dn_dataType_t dataType)
{
    if (self->data != NULL)
    {
        heap_caps_free(self->data);
        self->data = NULL;
    }
    self->data     = data;
    self->dataType = dataType;
}

void dn_updateBoard(dn_entity_t* self)
{
    dn_boardData_t* boardData = (dn_boardData_t*)self->data;

    //perform hooke's law on neighboring tiles
    for (int y = 0; y < DN_BOARD_SIZE; y++)
    {
        for (int x = 0; x < DN_BOARD_SIZE; x++)
        {
            // Get the current tile
            dn_tileData_t* tileData = (dn_tileData_t*)boardData->tiles[y][x];
            int8_t dampen = 3;
            if(x == boardData->impactPos.x && y == boardData->impactPos.y)
            {
                //the selected tile approaches a particular offset
                tileData->yVel += (((int16_t)(((TFT_HEIGHT >> 2) << DN_DECIMAL_BITS) - tileData->yOffset)) / 3);
            }
            else
            {
                //all unselected tiles approach neighboring tiles
                if (y > boardData->impactPos.y)
                {
                    tileData->yVel += (((int16_t)(boardData->tiles[y - 1][x].yOffset - tileData->yOffset)) / 1);
                    dampen += y - boardData->impactPos.y;
                }
                if (y < boardData->impactPos.y)
                {
                    tileData->yVel += (((int16_t)(boardData->tiles[y + 1][x].yOffset - tileData->yOffset)) / 1);
                    dampen += boardData->impactPos.y - y;
                }
                if (x > boardData->impactPos.x)
                {
                    tileData->yVel += (((int16_t)(boardData->tiles[y][x - 1].yOffset - tileData->yOffset)) / 1);
                    dampen += x - boardData->impactPos.x;
                }
                if (x < boardData->impactPos.x)
                {
                    tileData->yVel += (((int16_t)(boardData->tiles[y][x + 1].yOffset - tileData->yOffset)) / 1);
                    dampen += boardData->impactPos.x - x;
                }
            }

            tileData->yVel /= dampen;

            // Update position with smaller time step
            uint16_t newYOffset = tileData->yOffset + tileData->yVel * (elapsedUs >> 14);
            // If the the yOffset would wrap around
            if(((tileData->yOffset & 0x8000) && !(newYOffset & 0x8000) && tileData->yVel > 0) ||
                (!(tileData->yOffset & 0x8000) && (newYOffset & 0x8000) && tileData->yVel < 0))
            {
                //print a message
                ESP_LOGI("Dance Network", "Tile %d,%d yOffset hit the limit", x, y);
                // Set yVel to 0
                tileData->yVel = 0;
            }
            else{
                tileData->yOffset  = newYOffset;
            }
        }
    }
}

void dn_drawBoard(dn_entity_t* self)
{
    // Draw the tiles
    for (int y = 0; y < DN_BOARD_SIZE; y++)
    {
        for (int x = 0; x < DN_BOARD_SIZE; x++)
        {
            int drawX = (TFT_WIDTH >> 1) + (x - y - 1) * (gameData->sprites.groundTile.w >> 1);
            int drawY = 155 + (x + y) * 13 - (gameData->tiles[y][x].yOffset >> DECIMAL_BITS);
            drawWsgSimple(&gameData->sprites.groundTile, drawX, drawY);
        }
    }
}