#include "dn_entity.h"
#include "dn_utility.h"

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
    //Uncomment to visualize center of screen.
    //drawCircleFilled(TFT_WIDTH >> 1, TFT_HEIGHT >> 1, 2, c000);
}

void dn_updateCurtain(dn_entity_t* self)
{
    dn_curtainData_t* curtainData = (dn_curtainData_t*)self->data;
    curtainData->separation += (self->gameData->elapsedUs >> 13);
    
    dn_entity_t* board = (dn_entity_t*)self->gameData->entityManager.board;
    if(curtainData->separation > 100 && !board->updateFunction)
    {
        dn_boardData_t* boardData = (dn_boardData_t*)board->data;
        boardData->tiles[boardData->impactPos.y][boardData->impactPos.x].yOffset = (TFT_HEIGHT >> 2) << DN_DECIMAL_BITS;
        board->updateFunction = dn_updateBoard;
    }
    if(curtainData->separation > (TFT_WIDTH >> 1))
    {
        self->destroyFlag = true;
    }
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
    char text[9] = "Player 1";
    //get the text width
    uint16_t tWidth = textWidth(&self->gameData->font_ibm, text);
    int16_t x = (TFT_WIDTH >> 2) - (tWidth >> 1);
    int16_t y = 29;
    // Draw the intro text
    if(curtainData->separation > -700 && curtainData->separation < -50)
    {
        drawText(&self->gameData->font_ibm, c001, text, x, y);
        y++;
        x++;
        drawText(&self->gameData->font_ibm, c001, text, x, y);
        y++;
        x--;
        drawText(&self->gameData->font_ibm, c001, text, x, y);
        y--;
        x--;
        drawText(&self->gameData->font_ibm, c001, text, x, y);
        x++;
        drawShinyText(&self->gameData->font_ibm, c245, c355, c555, text, x, y);
        // drawText(&self->gameData->font_ibm, c555, text, (TFT_WIDTH >> 2) - (tWidth >> 1), 30);
        // drawText(&self->gameData->font_ibm, c555, text, (TFT_WIDTH >> 2) - (tWidth >> 1), 29);
        // drawText(&self->gameData->font_ibm, c101, text, (TFT_WIDTH >> 2) - (tWidth >> 1), 31);
        // drawText(&self->gameData->font_ibm, c525, text, (TFT_WIDTH >> 2) - (tWidth >> 1), 30);
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
    if(curtainData->separation > -600 && curtainData->separation < -50)
    {
        strcpy(text, "VS");
        tWidth = textWidth(&self->gameData->font_righteous, text);
        x = (TFT_WIDTH >> 1) - (tWidth >> 1);
        y = 59;
        drawText(&self->gameData->font_righteous, c550, text, x, y);
        y++;
        x++;
        drawText(&self->gameData->font_righteous, c550, text, x, y);
        y++;
        x--;
        drawText(&self->gameData->font_righteous, c550, text, x, y);
        y--;
        x--;
        drawText(&self->gameData->font_righteous, c550, text, x, y);
        x++;
        drawShinyText(&self->gameData->font_righteous, c430, c540, c552, text, (TFT_WIDTH >> 1) - (tWidth >> 1), 60);
    }
    if(curtainData->separation > -500 && curtainData->separation < -50)
    {
        strcpy(text, "Player 2");
        tWidth = textWidth(&self->gameData->font_ibm, text);
        x = (TFT_WIDTH >> 1) + (TFT_WIDTH >> 2) - (tWidth >> 1);
        y = 29;
        
        drawText(&self->gameData->font_ibm, c001, text, x, y);
        y++;
        x++;
        drawText(&self->gameData->font_ibm, c001, text, x, y);
        y++;
        x--;
        drawText(&self->gameData->font_ibm, c001, text, x, y);
        y--;
        x--;
        drawText(&self->gameData->font_ibm, c001, text, x, y);
        x++;
        drawShinyText(&self->gameData->font_ibm, c245, c355, c555, text, x, y);
        // drawText(&self->gameData->font_ibm, c555, text, (TFT_WIDTH >> 1) + (TFT_WIDTH >> 2) - (tWidth >> 1), 30);
        // drawText(&self->gameData->font_ibm, c555, text, (TFT_WIDTH >> 1) + (TFT_WIDTH >> 2) - (tWidth >> 1), 29);
        // drawText(&self->gameData->font_ibm, c101, text, (TFT_WIDTH >> 1) + (TFT_WIDTH >> 2) - (tWidth >> 1), 31);
        // drawText(&self->gameData->font_ibm, c525, text, (TFT_WIDTH >> 1) + (TFT_WIDTH >> 2) - (tWidth >> 1), 30);
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

void dn_drawAlbums(dn_entity_t* self)
{
    char text[10] = "Player 1";
    uint16_t tWidth = textWidth(&self->gameData->font_ibm, text);
    drawShinyText(&self->gameData->font_ibm, c245, c355, c555, text,
        ((self->pos.x - self->gameData->camera.pos.x) >> DN_DECIMAL_BITS)- (tWidth >> 1) - 80,
        ((self->pos.y - self->gameData->camera.pos.y) >> DN_DECIMAL_BITS));

    // strcpy(text, "Creative");
    // tWidth = textWidth(&self->gameData->font_ibm, text);
    // drawShinyText(&self->gameData->font_ibm, c425, c535, c555, text,
    //     ((self->pos.x - self->gameData->camera.pos.x) >> DN_DECIMAL_BITS) - (tWidth >> 1),
    //     ((self->pos.y - self->gameData->camera.pos.y) >> DN_DECIMAL_BITS) - 6);
    // strcpy(text, "Commons");
    // tWidth = textWidth(&self->gameData->font_ibm, text);
    // drawShinyText(&self->gameData->font_ibm, c425, c535, c555, text,
    //     ((self->pos.x - self->gameData->camera.pos.x) >> DN_DECIMAL_BITS) - (tWidth >> 1),
    //     ((self->pos.y - self->gameData->camera.pos.y) >> DN_DECIMAL_BITS) + 5);

    strcpy(text, "Player 2");
    tWidth = textWidth(&self->gameData->font_ibm, text);
    drawShinyText(&self->gameData->font_ibm, c245, c355, c555, text,
        ((self->pos.x - self->gameData->camera.pos.x) >> DN_DECIMAL_BITS) - (tWidth >> 1) + 80,
        ((self->pos.y - self->gameData->camera.pos.y) >> DN_DECIMAL_BITS));
}

void dn_drawAlbum(dn_entity_t* self)
{
    dn_albumData_t* aData = (dn_albumData_t*)self->data;
    drawWsgPalette(&self->gameData->assets[DN_ALBUM_ASSET].frames[0],
        ((self->pos.x - self->gameData->camera.pos.x) >> DN_DECIMAL_BITS) - self->gameData->assets[DN_ALBUM_ASSET].originX,
        ((self->pos.y - self->gameData->camera.pos.y) >> DN_DECIMAL_BITS) - self->gameData->assets[DN_ALBUM_ASSET].originY,
        &aData->tracksPalette,
        false, false, aData->rot);
}

void dn_updateCharacterSelect(dn_entity_t* self)
{
    dn_characterSelectData_t* cData = (dn_characterSelectData_t*)self->data;
    if(self->gameData->btnDownState & PB_A)
    {
        self->gameData->characterSets[0] = cData->selectCharacterIdx;
    }
    if(self->gameData->btnDownState & PB_B)
    {
        //free assets
        dn_freeAsset(&self->gameData->assets[DN_ALPHA_DOWN_ASSET]);
        dn_freeAsset(&self->gameData->assets[DN_ALPHA_UP_ASSET]);
        dn_freeAsset(&self->gameData->assets[DN_BUCKET_HAT_DOWN_ASSET]);
        dn_freeAsset(&self->gameData->assets[DN_BUCKET_HAT_UP_ASSET]);
        dn_freeAsset(&self->gameData->assets[DN_KING_ASSET]);
        dn_freeAsset(&self->gameData->assets[DN_PAWN_ASSET]);
        dn_freeAsset(&self->gameData->assets[DN_GROUND_TILE_ASSET]);
        self->destroyFlag = true;
        dn_ShowUi(UI_MENU);
        return;
    }
    else if(self->gameData->btnDownState & PB_LEFT)
    {
        cData->selectCharacterIdx = dn_wrap(cData->selectCharacterIdx - 1, DN_NUM_CHARACTERS - 1);
        cData->xSelectScrollOffset -= self->gameData->assets[DN_GROUND_TILE_ASSET].frames[0].w * 5;
    }
    else if(self->gameData->btnDownState & PB_RIGHT)
    {
        cData->selectCharacterIdx = dn_wrap(cData->selectCharacterIdx + 1, DN_NUM_CHARACTERS - 1);
        cData->xSelectScrollOffset += self->gameData->assets[DN_GROUND_TILE_ASSET].frames[0].w * 5;
    }

    // Scroll the offset if it's not centered yet
    cData->xSelectScrollTimer += self->gameData->elapsedUs;
    while(cData->xSelectScrollTimer >= 3000)
    {
        cData->xSelectScrollTimer -= 3000;
        if(cData->xSelectScrollOffset > 0)
        {
            cData->xSelectScrollOffset--;
        }
        else if(cData->xSelectScrollOffset < 0)
        {
            cData->xSelectScrollOffset++;
        }
    }
}
void dn_drawCharacterSelect(dn_entity_t* self)
{
    dn_characterSelectData_t* cData = (dn_characterSelectData_t*)self->data;

    // Draw the background, a blank menu
    drawMenuMania(self->gameData->bgMenu, self->gameData->menuRenderer, self->gameData->elapsedUs);

    // Set up variables for drawing
    int16_t yOff   = MANIA_TITLE_HEIGHT + 20;
    int16_t xOff   = ((TFT_WIDTH - self->gameData->assets[DN_GROUND_TILE_ASSET].frames[0].w) >> 1) + cData->xSelectScrollOffset;
    int8_t pIdx   = cData->selectCharacterIdx;

    // 'Rewind' characters until they're off screen
    while (xOff > 0)
    {
        xOff -= self->gameData->assets[DN_GROUND_TILE_ASSET].frames[0].w * 5;
        pIdx--;
    }

    // Don't use a negative index!
    while (pIdx < 0)
    {
        pIdx += DN_NUM_CHARACTERS;
    }

    //Draw floor tiles
    for(int16_t y = 0; y < 9; y++)
    {
        // Draw tiles until you're off screen
        while (xOff < TFT_WIDTH + ((self->gameData->assets[DN_GROUND_TILE_ASSET].frames[0].w * 5)>>1))
        {
            for(int16_t x = -2; x < 3; x++)
            {
                int16_t drawX = xOff + x * self->gameData->assets[DN_GROUND_TILE_ASSET].frames[0].w + ((self->gameData->assets[DN_GROUND_TILE_ASSET].frames[0].w >> 1) * (y % 2));
                int16_t drawY = yOff + y * (self->gameData->assets[DN_GROUND_TILE_ASSET].frames[0].h >> 1);
                if(drawX >= -self->gameData->assets[DN_GROUND_TILE_ASSET].frames[0].w &&
                    drawX <= TFT_WIDTH)
                {
                    // If this is the active maker, draw swapped pallete
                    if (pIdx == self->gameData->characterSets[0] && cData->selectDiamondShape[y * 5 + x+2])
                    {
                        drawWsgPaletteSimple(&self->gameData->assets[DN_GROUND_TILE_ASSET].frames[0], drawX, drawY, &self->gameData->entityManager.palettes[DN_RED_FLOOR_PALETTE]);
                    }
                    else
                    {
                        drawWsgSimple(&self->gameData->assets[DN_GROUND_TILE_ASSET].frames[0], drawX, drawY);
                    }
                    
                }
            }
            // Increment X offset
            xOff += self->gameData->assets[DN_GROUND_TILE_ASSET].frames[0].w * 5;
            // Increment marker index
            pIdx = (pIdx + 1) % DN_NUM_CHARACTERS;
        }
        //reset values
        xOff   = ((TFT_WIDTH - self->gameData->assets[DN_GROUND_TILE_ASSET].frames[0].w) >> 1) + cData->xSelectScrollOffset;
        pIdx   = cData->selectCharacterIdx;

        // 'Rewind' characters until they're off screen
        while (xOff > 0)
        {
            xOff -= self->gameData->assets[DN_GROUND_TILE_ASSET].frames[0].w * 5;
            pIdx--;
        }
        // Don't use a negative index!
        while (pIdx < 0)
        {
            pIdx += DN_NUM_CHARACTERS;
        }
    }


    // Draw characters until you're off screen (sort of)
    while (xOff < TFT_WIDTH + ((self->gameData->assets[DN_GROUND_TILE_ASSET].frames[0].w * 5)>>1))
    {
        for(int8_t i = 0; i < 5; i++)
        {
            if(i == 2)//king is the middle piece
            {
                drawWsgSimple(&self->gameData->assets[DN_KING_ASSET].frames[0],
                    xOff + (self->gameData->assets[DN_GROUND_TILE_ASSET].frames[0].w >> 1) * i + self->gameData->assets[DN_KING_ASSET].originX, 
                    yOff + (self->gameData->assets[DN_GROUND_TILE_ASSET].frames[0].h >> 1) * i + self->gameData->assets[DN_KING_ASSET].originY);
                drawWsgSimple(&self->gameData->assets[DN_KING_ASSET].frames[0],
                    xOff - (self->gameData->assets[DN_GROUND_TILE_ASSET].frames[0].w >> 1) * (4-i)  + self->gameData->assets[DN_KING_ASSET].originX,
                    yOff + (self->gameData->assets[DN_GROUND_TILE_ASSET].frames[0].h >> 1) * (4+i)  + self->gameData->assets[DN_KING_ASSET].originY);
            }
            else
            {
                drawWsgSimple(&self->gameData->assets[DN_PAWN_ASSET].frames[0],
                    xOff + (self->gameData->assets[DN_GROUND_TILE_ASSET].frames[0].w >> 1) * i + self->gameData->assets[DN_KING_ASSET].originX, 
                    yOff + (self->gameData->assets[DN_GROUND_TILE_ASSET].frames[0].h >> 1) * i + self->gameData->assets[DN_KING_ASSET].originY);
                drawWsgSimple(&self->gameData->assets[DN_PAWN_ASSET].frames[0],
                    xOff - (self->gameData->assets[DN_GROUND_TILE_ASSET].frames[0].w >> 1) * (4-i)  + self->gameData->assets[DN_KING_ASSET].originX,
                    yOff + (self->gameData->assets[DN_GROUND_TILE_ASSET].frames[0].h >> 1) * (4+i)  + self->gameData->assets[DN_KING_ASSET].originY);
            }
        }

        // Increment X offset
        xOff += self->gameData->assets[DN_GROUND_TILE_ASSET].frames[0].w * 5;
        // Increment marker index
        pIdx = (pIdx + 1) % DN_NUM_CHARACTERS;
    }

    // Draw arrows to indicate this can be scrolled
    // Blink the arrows
    self->gameData->generalTimer += self->gameData->elapsedUs >> 12;

    if (self->gameData->generalTimer > 127)
    {
        // Draw arrows to indicate this can be scrolled
        drawText(&self->gameData->font_ibm, c000, "<", 3, 53);
        drawText(&self->gameData->font_ibm, c000, ">", TFT_WIDTH - 3 - textWidth(&self->gameData->font_ibm, ">"), 53);
    }
}