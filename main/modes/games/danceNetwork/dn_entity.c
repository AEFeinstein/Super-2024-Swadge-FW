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
            dn_tileData_t* tileData = &boardData->tiles[y][x];
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
            uint16_t newYOffset = tileData->yOffset + tileData->yVel * (self->gameData->elapsedUs >> 14);
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
    dn_boardData_t* boardData = (dn_boardData_t*)self->data;
    // Draw the tiles
    for (int y = 0; y < DN_BOARD_SIZE; y++)
    {
        for (int x = 0; x < DN_BOARD_SIZE; x++)
        {
            int drawX =  ((self->pos.x - self->gameData->camera.pos.x) >> DN_DECIMAL_BITS) + (x - y) * self->gameData->assets[DN_GROUND_TILE_ASSET].originX;
            int drawY = ((self->pos.y - self->gameData->camera.pos.y) >> DN_DECIMAL_BITS) + (x + y) * self->gameData->assets[DN_GROUND_TILE_ASSET].originY - (boardData->tiles[y][x].yOffset >> DN_DECIMAL_BITS);
            drawWsgSimple(&self->gameData->assets[DN_GROUND_TILE_ASSET].frames[0], drawX - self->gameData->assets[DN_GROUND_TILE_ASSET].originX, drawY - self->gameData->assets[DN_GROUND_TILE_ASSET].originY);
            if(boardData->tiles[y][x].unit != NULL)
            {
                // Draw the unit on the tile
                dn_entity_t* unit = boardData->tiles[y][x].unit;
                if((unit->assetIndex == DN_KING_ASSET || unit->assetIndex == DN_PAWN_ASSET) && (unit == boardData->p1Units[0] || unit == boardData->p1Units[1] || unit == boardData->p1Units[2] || unit == boardData->p1Units[3] || unit == boardData->p1Units[4]))
                {
                    drawWsgPaletteSimple(&self->gameData->assets[unit->assetIndex].frames[0],
                            drawX - self->gameData->assets[unit->assetIndex].originX,
                            drawY - self->gameData->assets[unit->assetIndex].originY,
                            &self->gameData->entityManager.palettes[DN_WHITE_CHESS_PALETTE]);
                }
                else
                {
                    drawWsgSimple(&self->gameData->assets[unit->assetIndex].frames[0],
                            drawX - self->gameData->assets[unit->assetIndex].originX,
                            drawY - self->gameData->assets[unit->assetIndex].originY);
                }
            }
        }
    }
    drawCircleFilled(TFT_WIDTH >> 1, TFT_HEIGHT >> 1, 2, c000);
}

void dn_updateCurtain(dn_entity_t* self)
{
    dn_curtainData_t* curtainData = (dn_curtainData_t*)self->data;
    curtainData->separation += (self->gameData->elapsedUs >> 14);
}
void dn_drawCurtain(dn_entity_t* self)
{
    dn_curtainData_t* curtainData = (dn_curtainData_t*)self->data;
    // Draw the curtain asset
    for(int x = 0; x < 4; x++)
    {
        for(int y = 0; y < 12; y++)
        {
            drawWsgSimple(&self->gameData->assets[DN_CURTAIN_ASSET].frames[0], ((curtainData->separation > 0) * -curtainData->separation) + x * self->gameData->assets[DN_CURTAIN_ASSET].frames[0].w, y * self->gameData->assets[DN_CURTAIN_ASSET].frames[0].h);
            drawWsgSimple(&self->gameData->assets[DN_CURTAIN_ASSET].frames[0], (TFT_WIDTH >> 1) + ((curtainData->separation > 0) * curtainData->separation) + x * self->gameData->assets[DN_CURTAIN_ASSET].frames[0].w, y * self->gameData->assets[DN_CURTAIN_ASSET].frames[0].h);
        }
    }

    // Draw the intro text
    if(curtainData->separation > -200 && curtainData->separation < -50)
    {
        char text[32] = "Player 1";
        //get the text width
        int tWidth = textWidth(&self->gameData->font_ibm, text);
        drawText(&self->gameData->font_ibm, c555, text, (TFT_WIDTH >> 2) - (tWidth >> 1), 29);
        drawText(&self->gameData->font_ibm, c101, text, (TFT_WIDTH >> 2) - (tWidth >> 1), 31);
        drawText(&self->gameData->font_ibm, c525, text, (TFT_WIDTH >> 2) - (tWidth >> 1), 30);
        switch(self->gameData->characterSets[0])
        {
            case DN_ALPHA_SET:
                drawWsgSimple(&self->gameData->assets[DN_ALPHA_ORTHO_ASSET].frames[0], (TFT_WIDTH >> 2) - (self->gameData->assets[DN_ALPHA_ORTHO_ASSET].frames[0].w >> 1), 50);
                break;
            case DN_CHESS_SET:
                drawWsgPaletteSimple(&self->gameData->assets[DN_CHESS_ORTHO_ASSET].frames[0],
                        (TFT_WIDTH >> 2) - (self->gameData->assets[DN_CHESS_ORTHO_ASSET].frames[0].w >> 1),
                        50, &self->gameData->entityManager.palettes[DN_WHITE_CHESS_PALETTE]);
                break;
            default:
                break;
        }
    }
    if(curtainData->separation > -150 && curtainData->separation < -50)
    {
        char text[4] = "VS";
        int tWidth = textWidth(&self->gameData->font_righteous, text);

        drawShinyText(&self->gameData->font_righteous, c520, c541, c552, text, (TFT_WIDTH >> 1) - (tWidth >> 1), 60);
    }
    if(curtainData->separation > -100 && curtainData->separation < -50)
    {
        char text[32] = "Player 2";
        int tWidth = textWidth(&self->gameData->font_ibm, text);
        drawText(&self->gameData->font_ibm, c555, text, (TFT_WIDTH >> 1) + (TFT_WIDTH >> 2) - (tWidth >> 1), 29);
        drawText(&self->gameData->font_ibm, c101, text, (TFT_WIDTH >> 1) + (TFT_WIDTH >> 2) - (tWidth >> 1), 31);
        drawText(&self->gameData->font_ibm, c525, text, (TFT_WIDTH >> 1) + (TFT_WIDTH >> 2) - (tWidth >> 1), 30);
        switch(self->gameData->characterSets[1])
        {
            case DN_ALPHA_SET:
                drawWsgSimple(&self->gameData->assets[DN_ALPHA_ORTHO_ASSET].frames[0], (TFT_WIDTH >> 1) + (TFT_WIDTH >> 2) - (self->gameData->assets[DN_ALPHA_ORTHO_ASSET].frames[0].w >> 1), 50);
                break;
            case DN_CHESS_SET:
                drawWsgSimple(&self->gameData->assets[DN_CHESS_ORTHO_ASSET].frames[0], (TFT_WIDTH >> 1) + (TFT_WIDTH >> 2) - (self->gameData->assets[DN_CHESS_ORTHO_ASSET].frames[0].w >> 1), 50);
                break;
            default:
                break;
        }
    }
}