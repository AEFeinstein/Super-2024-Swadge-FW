#include "dn_entity.h"
#include "dn_utility.h"
#include "dn_random.h"
#include "shapes.h"
#include <linked_list.h>
#include <limits.h>

//A double array of chars
static const char tutorialText[2][23][2][160] = {
    {
        {"Dance 101", "Welcome to Dance class. Please, take a seat. We have some spots here in the front."},
        {"Commander DancenoNighta", "I am Commander DancenoNighta, and I'll be bringing you up to speed on the dance scene in 2026."},
        {"Win Conditions", "You can win by capturing the opponent's king. The other way to win is by moving your king onto the opponent's control point."},
        {"Control Point", "The control points are simply the tiles where the kings start."},
        {"Dance Arena", "The floor consists of a 5x5 grid built above our home."},
        {"Dance Arena", "Bigma promised to bring us into a post-garbage techno future. And he made the right call. We can just keep building on top of the past!"},
        {"Turn Phases", "On your turn, you will gain two rerolls. More about rerolls later. Just know that the available rerolls for each player are displayed in the led lights."},
        {"Dance Phase", "Tiles only illuminate under your units with valid dances. After selecting a unit, you can press 'b' to go back without committing to the dance."},
        {"Dance Phase", "Your available dances depend on what tracks are written on your album."},
        {"Understanding Tracks", "There's a tiny stick figure in the middle of your album. You can imagine projecting that down onto any of your units then the tracks act relative to that unit."},
        {"Dance Action - Fire", "Fire tracks (red) allow you to do a ranged attack."},
        {"Dance Action - Move", "Moving tracks (blue) allow you to move any of your units to an unoccupied tile."},
        {"Dance Action - Remix", "Remix tracks (purple) allow you to attempt a ranged attack and then move in one turn!"},
        {"Bringing Units Online", "If a unit fires a ranged attack, that unit goes offline and is unable to fire again. It will look grayscale when offline."},
        {"Bringing Units Online", "You need to move a unit to bring it online so it can fire later. Remember, if a unit is fully colored then it's a threat!"},
        {"Skipping a dance action", "You can skip a dance action, gaining another reroll. You might have to resort to skipping if no units have any valid dance tracks."},
        {"Albums", "Each album can hold a variety of tracks. After your dance action, you'll write a new track."},
        {"Albums", "In the writing phase, you'll be able to spend rerolls to influence the kind of track you write. Try to save up and turn the tides in your favor."},
        {"Albums", "A track also randomly appears into the Creative Commons. Artists from around the world are adding tracks into the Creative Commons every single turn."},
        {"Creative Commons", "You can spend 5 rerolls during your dance phase to make the Creative Commons your own moveset."},
        {"Creative Commons", "Think about what your opponent can do with the Creative Commons if they can also afford it."},
        {"Capturing", "When you capture an opponent's unit, you gain 1 reroll and trade albums with the opponent."},
        {"Q&A", "That concludes the introductory course. Thank you for dancing, and long live Bigma!"},
    },
    {
        {"Dance 200", "Welcome back, students!"},
        {"Dance 200", "As you should be well aware by now, the prerequisite for this course is Dance 101."},
        {"Dance 200", "Yes, question in the back with the hand raised..."},
        {"Dance 200", "No, no, no. If you haven't danced in the main game either, you'll need to speak with your advisor to unenroll in Dance 200 before the end of the first week."},
        {"Dance 200", "I'm just as excited as all of you to further hone your skills on the dance floor."},
        {"Dance 200", "But first, a moment of silence for our fallen heroes from the Great Garbage War of '25."},
        {"Bugs", "..."},
        {"Dance Threat Calculus", "Thank you. We're going to go over some light refreshers."},
        {"Dance Action - Fire", "Fire tracks tracks can even target friendly units or unoccupied tiles."},
        {"Dance Action - Fire", "Friendly fire will harvest 3 rerolls. Sometimes sacrifices are necessary. Targeting an unoccupied tile will knock the tile out of the game and bounce the shot."},
        {"Dance Action - Fire", "When bouncing a shot, select a second Fire or Remix track as the target."},
        {"Dance Action - Fire", "You can knock out your own control point to keep it safe from occupation for a while. The Dance Floor can't have more than one hole at a time."},
        {"Dance Action - Fire", "So if another tile is shot out, your control point would come back into play."},
        {"Dance Action - Move", "You could technically move into a hole and succumb to the garbage pit. At least this also results in gaining 3 rerolls."},
        {"Dance Action - Move", "I lost a wing and a leg in the Great Garbage War. But Bigma gave me a second chance."},
        {"Dance Action - Remix", "There are a lot of situational factors to consider with a remix."},
        {"Dance Action - Remix", "That extra momentum of 2 actions in one phase can be quite helpful."},
        {"Dance Action - Remix", "But the intended target of a remix should be occupied or you may incidentally create a hole and move into it, gaining 3 rerolls."},
        {"Bringing Units Online", "Remixes can bring offline units online because they will fail to fire, but move successfully."},
        {"Creative Commons", "If you end a turn with at least 3 rerolls, you'll have enough to take the Creative Commons next turn."},
        {"Capturing", "Think ahead about the track you would give your opponent if you capture, because your albums will swap when you capture."},
        {"Rerolls", "Rerolls aren't just given out in the center of the board. You also get a reroll everytime you get a pawn to the other side of the board."},
        {"Q&A", "That's all for the advanced tips. Your homework will be to achieve every trophy in the swadge."},
    }
};

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

void dn_drawAsset(dn_entity_t* self)
{
    if (!self->paused && (self->type == DN_LOOPING_ANIMATION || self->type == DN_ONESHOT_ANIMATION))
    {
        self->animationTimer++;
        if (self->animationTimer >= self->gameFramesPerAnimationFrame)
        {
            self->animationTimer = 0;
            self->currentAnimationFrame++;
            if (self->currentAnimationFrame >= self->gameData->assets[self->assetIndex].numFrames)
                self->currentAnimationFrame = 0;
        }
    }
    int32_t x = ((self->pos.x - self->gameData->camera.pos.x) >> DN_DECIMAL_BITS)
                - self->gameData->assets[self->assetIndex].originX;
    int32_t y = ((self->pos.y - self->gameData->camera.pos.y) >> DN_DECIMAL_BITS)
                - self->gameData->assets[self->assetIndex].originY;
    if (self->paused)
    {
        drawWsgPalette(&self->gameData->assets[self->assetIndex].frames[self->currentAnimationFrame], x, y,
                       &self->gameData->entityManager.palettes[DN_GRAYSCALE_PALETTE], self->flipped, false, 0);
    }
    else
    {
        drawWsg(&self->gameData->assets[self->assetIndex].frames[self->currentAnimationFrame], x, y, self->flipped,
                false, 0);
    }
}

void dn_drawNothing(dn_entity_t* self)
{
}

void dn_updateBoard(dn_entity_t* self)
{
    dn_boardData_t* boardData = (dn_boardData_t*)self->data;

    // perform hooke's law on neighboring tiles
    for (int y = 0; y < DN_BOARD_SIZE; y++)
    {
        for (int x = 0; x < DN_BOARD_SIZE; x++)
        {
            // Get the current tile
            dn_tileData_t* tileData = &boardData->tiles[y][x];
            int8_t dampen           = 3;
            if (x == boardData->impactPos.x && y == boardData->impactPos.y)
            {
                // the selected tile approaches a particular offset
                tileData->yVel += (((int16_t)(((TFT_HEIGHT >> 2) << DN_DECIMAL_BITS) - tileData->yOffset)) / 3);
            }
            else
            {
                // all unselected tiles approach neighboring tiles
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
            if (((tileData->yOffset & 0x8000) && !(newYOffset & 0x8000) && tileData->yVel > 0)
                || (!(tileData->yOffset & 0x8000) && (newYOffset & 0x8000) && tileData->yVel < 0))
            {
                // print a message
                ESP_LOGI("Dance Network", "Tile %d,%d yOffset hit the limit", x, y);
                // Set yVel to 0
                tileData->yVel = 0;
            }
            else
            {
                tileData->yOffset = newYOffset;
            }

            // other stuff
            if (tileData->timeout)
            {
                tileData->timeoutOffset++;
                if (tileData->timeoutOffset > 35)
                {
                    tileData->timeoutOffset = 35;
                    if (tileData->unit)
                    {
                        // A unit dies
                        if (tileData->unit == ((dn_boardData_t*)self->gameData->entityManager.board->data)->p1Units[0]
                            || tileData->unit
                                   == ((dn_boardData_t*)self->gameData->entityManager.board->data)
                                          ->p2Units[0]) // king is captured
                        {//a king has plunged
                            if(dn_findLastEntityOfType(self, DN_PROMPT_DATA))
                            {
                                break;
                            }

                            ///////////////////////////////
                            // Make the prompt Game Over //
                            ///////////////////////////////
                            dn_entity_t* promptGameOver = dn_createPrompt(&self->gameData->entityManager,
                                                                          (vec_t){0xffff, 0xffff}, self->gameData);
                            dn_promptData_t* promptData = (dn_promptData_t*)promptGameOver->data;

                            strcpy(promptData->text,
                                   self->gameData->playerNames
                                       [tileData->unit
                                        == ((dn_boardData_t*)self->gameData->entityManager.board->data)->p1Units[0]]);
                            strcat(promptData->text, " wins!");
                            promptData->isPurple = true;
                            promptData->options  = heap_caps_calloc(1, sizeof(list_t), MALLOC_CAP_8BIT);
                            memset(promptData->options, 0, sizeof(list_t));

                            dn_promptOption_t* option1 = heap_caps_malloc(sizeof(dn_promptOption_t), MALLOC_CAP_8BIT);
                            memset(option1, 0, sizeof(dn_promptOption_t));
                            strcpy(option1->text, "OK");
                            option1->callback          = NULL;
                            option1->downPressDetected = false;
                            push(promptData->options, (void*)option1);
                            promptData->numOptions = 1;
                        }
                        else // a pawn has plunged
                        {
                            ////////////////////////////
                            // Make the prompt Sudoku //
                            ////////////////////////////
                            dn_entity_t* promptSudoku   = dn_createPrompt(&self->gameData->entityManager,
                                                                          (vec_t){0xffff, 0xffff}, self->gameData);
                            dn_promptData_t* promptData = (dn_promptData_t*)promptSudoku->data;

                            promptData->usesTwoLinesOfText = true;
                            strcpy(promptData->text, "Your unit has taken the plunge.");
                            strcpy(promptData->text2, "3 rerolls rise from the pit.");
                            promptData->options = heap_caps_calloc(1, sizeof(list_t), MALLOC_CAP_8BIT);
                            memset(promptData->options, 0, sizeof(list_t));

                            dn_promptOption_t* option1 = heap_caps_malloc(sizeof(dn_promptOption_t), MALLOC_CAP_8BIT);
                            memset(option1, 0, sizeof(dn_promptOption_t));
                            strcpy(option1->text, "OK");
                            option1->callback          = dn_afterPlunge;
                            option1->downPressDetected = false;
                            push(promptData->options, (void*)option1);
                            promptData->numOptions = 1;
                        }

                        tileData->unit->destroyFlag = true;
                        for (uint8_t i = 0; i < 5; i++)
                        {
                            if (boardData->p1Units[i] == tileData->unit)
                            {
                                boardData->p1Units[i] = NULL;
                                break;
                            }
                            if (boardData->p2Units[i] == tileData->unit)
                            {
                                boardData->p2Units[i] = NULL;
                                break;
                            }
                        }
                        tileData->unit = NULL;
                    }
                }
            }
            else if (tileData->timeoutOffset > 0)
            {
                tileData->timeoutOffset--;
            }
        }
    }
}

bool dn_belongsToP1(dn_entity_t* unit)
{
    dn_boardData_t* bData = (dn_boardData_t*)unit->gameData->entityManager.board->data;
    return (unit == bData->p1Units[0] || unit == bData->p1Units[1] || unit == bData->p1Units[2]
            || unit == bData->p1Units[3] || unit == bData->p1Units[4]);
}

void dn_drawBoard(dn_entity_t* self)
{
    dn_boardData_t* boardData = (dn_boardData_t*)self->data;
    // Draw the tiles
    for (int y = 0; y < DN_BOARD_SIZE; y++)
    {
        for (int x = 0; x < DN_BOARD_SIZE; x++)
        {
            int drawX = ((self->pos.x - self->gameData->camera.pos.x) >> DN_DECIMAL_BITS)
                        + (x - y) * self->gameData->assets[DN_GROUND_TILE_ASSET].originX - 1;
            int drawY = ((self->pos.y - self->gameData->camera.pos.y) >> DN_DECIMAL_BITS)
                        + (x + y) * self->gameData->assets[DN_GROUND_TILE_ASSET].originY
                        - (boardData->tiles[y][x].yOffset >> DN_DECIMAL_BITS) + boardData->tiles[y][x].timeoutOffset;
            int miniDrawX = -85 + ((self->pos.x - self->gameData->camera.pos.x) >> DN_DECIMAL_BITS)
                            + (x - y) * self->gameData->assets[DN_MINI_TILE_ASSET].originX - 1;
            int miniDrawY = -233 + ((self->pos.y - self->gameData->camera.pos.y) >> DN_DECIMAL_BITS)
                            + (x + y) * self->gameData->assets[DN_MINI_TILE_ASSET].originY
                            - (boardData->tiles[y][x].yOffset >> DN_DECIMAL_BITS)
                            + boardData->tiles[y][x].timeoutOffset;

            if (boardData->tiles[y][x].selectionType == DN_UNIT_SELECTION)
            {
                drawWsgPaletteSimple(&self->gameData->assets[DN_GROUND_TILE_ASSET].frames[0],
                                     drawX - self->gameData->assets[DN_GROUND_TILE_ASSET].originX,
                                     drawY - self->gameData->assets[DN_GROUND_TILE_ASSET].originY,
                                     &self->gameData->entityManager
                                          .palettes[DN_RED_FLOOR_PALETTE
                                                    + (((y * ((self->gameData->generalTimer >> 10) % 10) + x + 2)
                                                        + (self->gameData->generalTimer >> 6))
                                                       % 6)]);
            }
            else if (boardData->tiles[y][x].selectionType == DN_MOVE_SELECTION)
            {
                drawWsgPaletteSimple(&self->gameData->assets[DN_GROUND_TILE_ASSET].frames[0],
                                     drawX - self->gameData->assets[DN_GROUND_TILE_ASSET].originX,
                                     drawY - self->gameData->assets[DN_GROUND_TILE_ASSET].originY,
                                     &self->gameData->entityManager
                                          .palettes[DN_MOVE1_FLOOR_PALETTE + 2
                                                    - abs((((y * ((self->gameData->generalTimer >> 9) % 10) + x + 2)
                                                            + (self->gameData->generalTimer >> 6))
                                                           % 4)
                                                          - 2)]);
            }
            else if (boardData->tiles[y][x].selectionType == DN_ATTACK_SELECTION)
            {
                drawWsgPaletteSimple(&self->gameData->assets[DN_GROUND_TILE_ASSET].frames[0],
                                     drawX - self->gameData->assets[DN_GROUND_TILE_ASSET].originX,
                                     drawY - self->gameData->assets[DN_GROUND_TILE_ASSET].originY,
                                     &self->gameData->entityManager
                                          .palettes[DN_ATTACK1_FLOOR_PALETTE + 2
                                                    - abs((((y * ((self->gameData->generalTimer >> 9) % 10) + x + 2)
                                                            + (self->gameData->generalTimer >> 6))
                                                           % 4)
                                                          - 2)]);
            }
            else if (boardData->tiles[y][x].selectionType == DN_REMIX_SELECTION)
            {
                drawWsgPaletteSimple(&self->gameData->assets[DN_GROUND_TILE_ASSET].frames[0],
                                     drawX - self->gameData->assets[DN_GROUND_TILE_ASSET].originX,
                                     drawY - self->gameData->assets[DN_GROUND_TILE_ASSET].originY,
                                     &self->gameData->entityManager
                                          .palettes[DN_REMIX1_FLOOR_PALETTE + 2
                                                    - abs((((y * ((self->gameData->generalTimer >> 9) % 10) + x + 2)
                                                            + (self->gameData->generalTimer >> 6))
                                                           % 4)
                                                          - 2)]);
            }
            else
            {
                drawWsgSimple(&self->gameData->assets[DN_GROUND_TILE_ASSET].frames[0],
                              drawX - self->gameData->assets[DN_GROUND_TILE_ASSET].originX,
                              drawY - self->gameData->assets[DN_GROUND_TILE_ASSET].originY);
            }
            bool drawOutline = false;
            paletteColor_t color = c050;
            if (boardData->tiles[y][x].rewards)
            {
                char reward[6];
                snprintf(reward, sizeof(reward), "+%d", boardData->tiles[y][x].rewards);
                drawShinyText(&self->gameData->font_ibm, c153, c254, c555, reward, drawX - 8, drawY - 5);
                drawOutline = true;
            }
            if(x == 2 && (y == 0 || y == 4))
            {
                drawOutline = true;
                color = y == 4 ? c055 : c550;
            }
            if(drawOutline)
            {
                drawLineFast(drawX, drawY - self->gameData->assets[DN_GROUND_TILE_ASSET].originY + 1,
                             drawX + self->gameData->assets[DN_GROUND_TILE_ASSET].originX - 1, drawY, color);
                drawLineFast(drawX + self->gameData->assets[DN_GROUND_TILE_ASSET].originX - 1, drawY, drawX,
                             drawY + self->gameData->assets[DN_GROUND_TILE_ASSET].originY - 1, color);
                drawLineFast(drawX, drawY - self->gameData->assets[DN_GROUND_TILE_ASSET].originY + 1,
                             drawX - self->gameData->assets[DN_GROUND_TILE_ASSET].originX + 1, drawY, color);
                drawLineFast(drawX - self->gameData->assets[DN_GROUND_TILE_ASSET].originX + 2, drawY, drawX,
                             drawY + self->gameData->assets[DN_GROUND_TILE_ASSET].originY - 2, color);
            }
            if (boardData->tiles[y][x].selector != NULL)
            {
                // Draw the back part of the selector
                dn_drawTileSelectorBackHalf(boardData->tiles[y][x].selector, drawX,
                                            drawY - boardData->tiles[y][x].timeoutOffset);
            }

            // Draw the mini board
            drawWsgSimple(&self->gameData->assets[DN_MINI_TILE_ASSET].frames[0],
                          miniDrawX - self->gameData->assets[DN_MINI_TILE_ASSET].originX,
                          miniDrawY - self->gameData->assets[DN_MINI_TILE_ASSET].originY);
            if (boardData->tiles[y][x].rewards)
            {
                // char reward[6];
                // snprintf(reward, sizeof(reward), "+%d", boardData->tiles[y][x].rewards);
                // drawShinyText(&self->gameData->font_ibm, c031, c254, c555, reward, miniDrawX, miniDrawY);
                drawLineFast(miniDrawX, miniDrawY - self->gameData->assets[DN_MINI_TILE_ASSET].originY + 1,
                             miniDrawX + self->gameData->assets[DN_MINI_TILE_ASSET].originX - 1, miniDrawY, c050);
                drawLineFast(miniDrawX + self->gameData->assets[DN_MINI_TILE_ASSET].originX - 1, miniDrawY, miniDrawX,
                             miniDrawY + self->gameData->assets[DN_MINI_TILE_ASSET].originY - 1, c050);
                drawLineFast(miniDrawX, miniDrawY - self->gameData->assets[DN_MINI_TILE_ASSET].originY + 1,
                             miniDrawX - self->gameData->assets[DN_MINI_TILE_ASSET].originX + 1, miniDrawY, c050);
                drawLineFast(miniDrawX - self->gameData->assets[DN_MINI_TILE_ASSET].originX + 1, miniDrawY, miniDrawX,
                             miniDrawY + self->gameData->assets[DN_MINI_TILE_ASSET].originY - 1, c050);
            }
            if (boardData->tiles[y][x].unit != NULL)
            {
                // Draw the unit on the tile
                dn_entity_t* unit            = boardData->tiles[y][x].unit;
                bool drawn                   = false;
                dn_assetIdx_t miniAssetIndex = dn_isKing(unit) ? DN_KING_SMALL_ASSET : DN_PAWN_SMALL_ASSET;
                if (dn_belongsToP1(unit))
                {
                    if (unit->assetIndex == DN_KING_ASSET || unit->assetIndex == DN_PAWN_ASSET)
                    {
                        // draw unit
                        drawWsgPaletteSimple(
                            &self->gameData->assets[unit->assetIndex].frames[0],
                            drawX - self->gameData->assets[unit->assetIndex].originX,
                            drawY - self->gameData->assets[unit->assetIndex].originY,
                            &self->gameData->entityManager
                                 .palettes[unit->paused ? DN_GRAYSCALE_PALETTE : DN_WHITE_CHESS_PALETTE]);
                        drawn = true;
                    }
                    // draw mini chess unit
                    drawWsgPaletteSimple(&self->gameData->assets[miniAssetIndex].frames[0],
                                         miniDrawX - self->gameData->assets[miniAssetIndex].originX,
                                         miniDrawY - self->gameData->assets[miniAssetIndex].originY,
                                         &self->gameData->entityManager
                                              .palettes[unit->paused ? DN_GRAYSCALE_PALETTE : DN_WHITE_CHESS_PALETTE]);
                    if(unit->paused)
                    {
                        drawWsgPaletteSimple(&self->gameData->assets[miniAssetIndex].frames[1],
                                         miniDrawX - self->gameData->assets[miniAssetIndex].originX,
                                         miniDrawY - self->gameData->assets[miniAssetIndex].originY,
                                         &self->gameData->entityManager.palettes[DN_WHITE_CHESS_PALETTE]);
                    }
                }
                else
                {
                    // draw mini chess unit
                    if (unit->paused)
                    {
                        drawWsgPaletteSimple(&self->gameData->assets[miniAssetIndex].frames[0],
                                             miniDrawX - self->gameData->assets[miniAssetIndex].originX,
                                             miniDrawY - self->gameData->assets[miniAssetIndex].originY,
                                             &self->gameData->entityManager.palettes[DN_GRAYSCALE_PALETTE]);
                    }
                    else
                    {
                        drawWsgSimple(&self->gameData->assets[miniAssetIndex].frames[0],
                                      miniDrawX - self->gameData->assets[miniAssetIndex].originX,
                                      miniDrawY - self->gameData->assets[miniAssetIndex].originY);
                    }
                    if(unit->paused)
                    {
                        drawWsgSimple(&self->gameData->assets[miniAssetIndex].frames[1],    
                                  miniDrawX - self->gameData->assets[miniAssetIndex].originX,
                                  miniDrawY - self->gameData->assets[miniAssetIndex].originY);
                    }
                }
                if (!drawn)
                {
                    // draw unit
                    if (unit->paused)
                    {
                        drawWsgPaletteSimple(&self->gameData->assets[unit->assetIndex].frames[0],
                                             drawX - self->gameData->assets[unit->assetIndex].originX,
                                             drawY - self->gameData->assets[unit->assetIndex].originY,
                                             &self->gameData->entityManager.palettes[DN_GRAYSCALE_PALETTE]);
                    }
                    else
                    {
                        drawWsgSimple(&self->gameData->assets[unit->assetIndex].frames[0],
                                      drawX - self->gameData->assets[unit->assetIndex].originX,
                                      drawY - self->gameData->assets[unit->assetIndex].originY);
                    }
                }
            }
            if (boardData->tiles[y][x].selector != NULL)
            {
                // Draw the front part of the selector
                dn_drawTileSelectorFrontHalf(boardData->tiles[y][x].selector, drawX,
                                             drawY - boardData->tiles[y][x].timeoutOffset);
            }
        }
    }
    // Uncomment to visualize center of screen.
    // drawCircleFilled(TFT_WIDTH >> 1, TFT_HEIGHT >> 1, 2, c000);
}

bool dn_availableMoves(dn_entity_t* unit, list_t* tracks)
{
    if (unit == NULL)
    {
        return false;
    }
    dn_entity_t* album = (dn_entity_t*)((dn_albumsData_t*)unit->gameData->entityManager.albums->data)->p1Album;

    bool isP1 = dn_belongsToP1(unit);

    if (!isP1)
    {
        album = (dn_entity_t*)((dn_albumsData_t*)unit->gameData->entityManager.albums->data)->p2Album;
    }

    dn_albumData_t* albumData = (dn_albumData_t*)album->data;

    for (paletteColor_t check = c255; check <= c322; check += 1)
    {
        if (albumData->screenOnPalette.newColors[check] != c555
            || albumData->screenAttackPalette.newColors[check] != cTransparent) // c555 is no action
        {
            // This is a track
            dn_boardPos_t track     = dn_colorToTrackCoords(check);
            dn_boardPos_t unitPos   = dn_getUnitBoardPos(unit);
            dn_action_t* unitAction = heap_caps_malloc(sizeof(dn_action_t), MALLOC_CAP_8BIT);
            memset(unitAction, 0, sizeof(dn_action_t));
            (*unitAction).pos = (dn_boardPos_t){.x = unitPos.x + (1 - 2 * !isP1) * track.x,
                                                .y = unitPos.y + (1 - 2 * isP1) * track.y};
            if (unitAction->pos.x >= 0 && unitAction->pos.x <= 4 && unitAction->pos.y >= 0 && unitAction->pos.y <= 4)
            {
                // It is in bounds
                dn_entity_t* unitAtTrack = ((dn_boardData_t*)unit->gameData->entityManager.board->data)
                                               ->tiles[unitAction->pos.y][unitAction->pos.x]
                                               .unit;
                unitAction->action = dn_trackTypeAtCoords(album, track);
                switch (unitAction->action)
                {
                    case DN_REMIX_TRACK:
                    {
                        if(unitAtTrack == NULL || !unit->paused)
                        {
                            push(tracks, (void*)unitAction);
                        }
                        break;
                    }
                    case DN_RED_TRACK: // ranged attack and remixed track
                    {
                        // You can shoot ANY tile. (muahahaha)
                        if (!unit->paused)
                        {
                            push(tracks, (void*)unitAction);
                        }
                        break;
                    }
                    case DN_BLUE_TRACK: // movement
                    {
                        // You can move to any unoccupied tile. Even knocked out ones. HAHAHAAHAAAA
                        if (unitAtTrack == NULL)
                        {
                            push(tracks, (void*)unitAction);
                        }
                        break;
                    }
                    default:
                    {
                        free(unitAction);
                        break;
                    }
                }
            }
            else
            {
                free(unitAction);
            }
        }
    }
    return tracks->first != NULL;
}

dn_track_t dn_trackTypeAtColor(dn_entity_t* album, paletteColor_t trackCoords)
{
    dn_albumData_t* aData = (dn_albumData_t*)album->data;
    if (aData->screenOnPalette.newColors[trackCoords] == c105)
    {
        if (aData->screenAttackPalette.newColors[trackCoords] == c510)
        {
            return DN_REMIX_TRACK;
        }
        return DN_BLUE_TRACK;
    }
    else if (aData->screenAttackPalette.newColors[trackCoords] == c510)
    {
        return DN_RED_TRACK;
    }
    return DN_NONE_TRACK;
}

dn_track_t dn_trackTypeAtCoords(dn_entity_t* album, dn_boardPos_t trackCoords)
{
    return dn_trackTypeAtColor(album, dn_trackCoordsToColor(trackCoords).lit);
}

bool dn_calculateMoveableUnits(dn_entity_t* board)
{
    dn_boardData_t* boardData = (dn_boardData_t*)board->data;
    dn_entity_t** playerUnits = board->gameData->phase < DN_P2_DANCE_PHASE ? boardData->p1Units : boardData->p2Units;

    bool playerHasMoves = false;

    for (int i = 0; i < 5; i++)
    {
        if (playerUnits[i] == NULL)
        {
            // That unit has died
            continue;
        }
        list_t* myList = heap_caps_calloc(1, sizeof(list_t), MALLOC_CAP_8BIT);
        memset(myList, 0, sizeof(list_t));
        if (dn_availableMoves(playerUnits[i], myList))
        {
            playerHasMoves                               = true;
            dn_boardPos_t pos                            = dn_getUnitBoardPos(playerUnits[i]);
            boardData->tiles[pos.y][pos.x].selectionType = DN_UNIT_SELECTION;
        }
        clear(myList);
        free(myList);
    }

    return playerHasMoves;
}

bool dn_isKing(dn_entity_t* unit)
{
    dn_boardData_t* boardData = (dn_boardData_t*)unit->gameData->entityManager.board->data;
    return (unit == boardData->p1Units[0] || unit == boardData->p2Units[0]);
}

void dn_updateCurtain(dn_entity_t* self)
{
    dn_curtainData_t* curtainData = (dn_curtainData_t*)self->data;
    curtainData->separation += (self->gameData->elapsedUs >> 13);

    dn_entity_t* board = (dn_entity_t*)self->gameData->entityManager.board;
    if (curtainData->separation > 100 && !board->updateFunction)
    {
        dn_boardData_t* boardData                                                = (dn_boardData_t*)board->data;
        boardData->tiles[boardData->impactPos.y][boardData->impactPos.x].yOffset = (TFT_HEIGHT >> 2) << DN_DECIMAL_BITS;
        board->updateFunction                                                    = dn_updateBoard;
    }
    if (curtainData->separation > (TFT_WIDTH >> 1))
    {
        self->destroyFlag = true;
    }
}
void dn_drawCurtain(dn_entity_t* self)
{
    dn_curtainData_t* curtainData = (dn_curtainData_t*)self->data;
    // Draw the curtain asset
    drawWsgSimple(&self->gameData->assets[DN_CURTAIN_ASSET].frames[0],
                  ((curtainData->separation > 0) * -curtainData->separation), 0);
    drawWsg(&self->gameData->assets[DN_CURTAIN_ASSET].frames[0],
            (TFT_WIDTH >> 1) + ((curtainData->separation > 0) * curtainData->separation), 0, true, false, 0);
    // get the text width
    uint16_t tWidth = textWidth(&self->gameData->font_ibm, self->gameData->playerNames[0]);
    int16_t x       = (TFT_WIDTH >> 2) - (tWidth >> 1);
    int16_t y       = 210;
    // Draw the intro text
    if (curtainData->separation > -700 && curtainData->separation < -50)
    {
        drawCircleFilled((TFT_WIDTH >> 2) + 8, y - 38, 80, c055);
        drawCircleFilled(TFT_WIDTH >> 2, y - 30, 80, c000);
        drawShinyText(&self->gameData->font_ibm, c245, c355, c555, self->gameData->playerNames[0], x, y);
        switch (self->gameData->characterSets[0])
        {
            case DN_ALPHA_SET:
                drawWsgSimple(&self->gameData->assets[DN_ALPHA_ORTHO_ASSET].frames[0],
                              (TFT_WIDTH >> 2) - (self->gameData->assets[DN_ALPHA_ORTHO_ASSET].frames[0].w >> 1), 95);
                break;
            case DN_CHESS_SET:
                drawWsgPaletteSimple(&self->gameData->assets[DN_CHESS_ORTHO_ASSET].frames[0],
                                     (TFT_WIDTH >> 2) - (self->gameData->assets[DN_CHESS_ORTHO_ASSET].frames[0].w >> 1),
                                     95, &self->gameData->entityManager.palettes[DN_WHITE_CHESS_PALETTE]);
                break;
            default:
                break;
        }
    }
    if (curtainData->separation > -600 && curtainData->separation < -50)
    {
        tWidth = textWidth(&self->gameData->font_righteous, "VS");
        drawText(&self->gameData->font_righteous, c535, "VS", (TFT_WIDTH >> 1) - (tWidth >> 1), 90);
        drawText(&self->gameData->outline_righteous, c314, "VS", (TFT_WIDTH >> 1) - (tWidth >> 1), 90);
    }
    if (curtainData->separation > -500 && curtainData->separation < -50)
    {
        tWidth = textWidth(&self->gameData->font_ibm, self->gameData->playerNames[1]);
        x      = (TFT_WIDTH >> 1) + (TFT_WIDTH >> 2) - (tWidth >> 1);
        y      = 30;

        drawCircleFilled(((TFT_WIDTH >> 1) + (TFT_WIDTH >> 2)) - 8, y - 22, 80, c550);
        drawCircleFilled((TFT_WIDTH >> 1) + (TFT_WIDTH >> 2), y - 30, 80, c000);
        drawShinyText(&self->gameData->font_ibm, c442, c553, c555, self->gameData->playerNames[1], x, y);
        // drawText(&self->gameData->font_ibm, c555, text, (TFT_WIDTH >> 1) + (TFT_WIDTH >> 2) - (tWidth >> 1), 30);
        // drawText(&self->gameData->font_ibm, c555, text, (TFT_WIDTH >> 1) + (TFT_WIDTH >> 2) - (tWidth >> 1), 29);
        // drawText(&self->gameData->font_ibm, c101, text, (TFT_WIDTH >> 1) + (TFT_WIDTH >> 2) - (tWidth >> 1), 31);
        // drawText(&self->gameData->font_ibm, c525, text, (TFT_WIDTH >> 1) + (TFT_WIDTH >> 2) - (tWidth >> 1), 30);
        switch (self->gameData->characterSets[1])
        {
            case DN_ALPHA_SET:
                drawWsgSimple(&self->gameData->assets[DN_ALPHA_ORTHO_ASSET].frames[0],
                              (TFT_WIDTH >> 1) + (TFT_WIDTH >> 2)
                                  - (self->gameData->assets[DN_ALPHA_ORTHO_ASSET].frames[0].w >> 1),
                              50);
                break;
            case DN_CHESS_SET:
                drawWsgSimple(&self->gameData->assets[DN_CHESS_ORTHO_ASSET].frames[0],
                              (TFT_WIDTH >> 1) + (TFT_WIDTH >> 2)
                                  - (self->gameData->assets[DN_CHESS_ORTHO_ASSET].frames[0].w >> 1),
                              50);
                break;
            default:
                break;
        }
    }
}

void dn_drawAlbums(dn_entity_t* self)
{
    char text[10]   = "Player 1";
    uint16_t tWidth = textWidth(&self->gameData->font_ibm, text);
    drawShinyText(&self->gameData->font_ibm, c245, c355, c555, text,
                  ((self->pos.x - self->gameData->camera.pos.x) >> DN_DECIMAL_BITS) - (tWidth >> 1) - 80,
                  ((self->pos.y - self->gameData->camera.pos.y) >> DN_DECIMAL_BITS));

    if (self->pos.y < 63550)
    {
        strcpy(text, "Creative");
        tWidth = textWidth(&self->gameData->font_ibm, text);
        drawShinyText(&self->gameData->font_ibm, c425, c535, c555, text,
                      ((self->pos.x - self->gameData->camera.pos.x) >> DN_DECIMAL_BITS) - (tWidth >> 1),
                      ((self->pos.y - self->gameData->camera.pos.y) >> DN_DECIMAL_BITS) - 1);
        strcpy(text, "Commons");
        tWidth = textWidth(&self->gameData->font_ibm, text);
        drawShinyText(&self->gameData->font_ibm, c425, c535, c555, text,
                      ((self->pos.x - self->gameData->camera.pos.x) >> DN_DECIMAL_BITS) - (tWidth >> 1),
                      ((self->pos.y - self->gameData->camera.pos.y) >> DN_DECIMAL_BITS) + 10);
    }

    strcpy(text, "Player 2");
    tWidth = textWidth(&self->gameData->font_ibm, text);
    drawShinyText(&self->gameData->font_ibm, c442, c553, c555, text,
                  ((self->pos.x - self->gameData->camera.pos.x) >> DN_DECIMAL_BITS) - (tWidth >> 1) + 80,
                  ((self->pos.y - self->gameData->camera.pos.y) >> DN_DECIMAL_BITS));
}

dn_boardPos_t dn_colorToTrackCoords(paletteColor_t color)
{
    switch (color)
    {
        case c255:
        case c155:
        {
            return (dn_boardPos_t){-1, 2};
        }
        case c300:
        case c200:
        {
            return (dn_boardPos_t){0, 2};
        }
        case c301:
        case c201:
        {
            return (dn_boardPos_t){1, 2};
        }
        case c302:
        case c202:
        {
            return (dn_boardPos_t){-2, 1};
        }
        case c303:
        {
            return (dn_boardPos_t){-1, 1};
        }
        case c304:
        {
            return (dn_boardPos_t){0, 1};
        }
        case c305:
        {
            return (dn_boardPos_t){1, 1};
        }
        case c310:
        {
            return (dn_boardPos_t){2, 1};
        }
        case c311:
        case c111:
        {
            return (dn_boardPos_t){-2, 0};
        }
        case c312:
        {
            return (dn_boardPos_t){-1, 0};
        }
        case c313:
        {
            return (dn_boardPos_t){1, 0};
        }
        case c314:
        {
            return (dn_boardPos_t){2, 0};
        }
        case c315:
        {
            return (dn_boardPos_t){-1, -1};
        }
        case c320:
        {
            return (dn_boardPos_t){0, -1};
        }
        case c321:
        {
            return (dn_boardPos_t){1, -1};
        }
        case c322:
        {
            return (dn_boardPos_t){0, -2};
        }
        default:
        {
            return (dn_boardPos_t){0, 0};
        }
    }
}

dn_twoColors_t dn_trackCoordsToColor(dn_boardPos_t trackCoords)
{
    switch (trackCoords.y)
    {
        case 2: // forward 2
        {
            switch (trackCoords.x)
            {
                case -1: // left 1
                {
                    return (dn_twoColors_t){(paletteColor_t){c255}, (paletteColor_t){c155}};
                }
                case 0: // left 0
                {
                    return (dn_twoColors_t){(paletteColor_t){c300}, (paletteColor_t){c200}};
                }
                case 1: // right 1
                {
                    return (dn_twoColors_t){(paletteColor_t){c301}, (paletteColor_t){c201}};
                }
                default:
                {
                    break;
                }
            }
            break;
        }
        case 1: // forward 1
        {
            switch (trackCoords.x)
            {
                case -2: // left 2
                {
                    return (dn_twoColors_t){(paletteColor_t){c302}, (paletteColor_t){c202}};
                }
                case -1: // left 1
                {
                    return (dn_twoColors_t){(paletteColor_t){c303}, (paletteColor_t){c303}};
                }
                case 0: // left 0
                {
                    return (dn_twoColors_t){(paletteColor_t){c304}, (paletteColor_t){c304}};
                }
                case 1: // right 1
                {
                    return (dn_twoColors_t){(paletteColor_t){c305}, (paletteColor_t){c305}};
                }
                case 2: // right 2
                {
                    return (dn_twoColors_t){(paletteColor_t){c310}, (paletteColor_t){c310}};
                }
                default:
                {
                    break;
                }
            }
            break;
        }
        case 0: // forward 0
        {
            switch (trackCoords.x)
            {
                case -2: // left 2
                {
                    return (dn_twoColors_t){(paletteColor_t){c311}, (paletteColor_t){c111}};
                }
                case -1: // left 1
                {
                    return (dn_twoColors_t){(paletteColor_t){c312}, (paletteColor_t){c312}};
                }
                case 1: // right 1
                {
                    return (dn_twoColors_t){(paletteColor_t){c313}, (paletteColor_t){c313}};
                }
                case 2: // right 2
                {
                    return (dn_twoColors_t){(paletteColor_t){c314}, (paletteColor_t){c314}};
                }
                default:
                {
                    break;
                }
            }
            break;
        }
        case -1: // backward 1
        {
            switch (trackCoords.x)
            {
                case -1: // left 1
                {
                    return (dn_twoColors_t){(paletteColor_t){c315}, (paletteColor_t){c315}};
                }
                case 0: // left 0
                {
                    return (dn_twoColors_t){(paletteColor_t){c320}, (paletteColor_t){c320}};
                }
                case 1: // right 1
                {
                    return (dn_twoColors_t){(paletteColor_t){c321}, (paletteColor_t){c321}};
                }
                default:
                {
                    break;
                }
            }
            break;
        }
        case -2: // backward 1
        {
            switch (trackCoords.x)
            {
                case 0: // left 0
                {
                    return (dn_twoColors_t){(paletteColor_t){c322}, (paletteColor_t){c322}};
                }
                default:
                {
                    break;
                }
            }
            break;
        }
        default:
        {
            break;
        }
    }
    // out of range
    return (dn_twoColors_t){(paletteColor_t){c000}, (paletteColor_t){c000}};
}

void dn_addTrackToAlbum(dn_entity_t* album, dn_boardPos_t trackCoords, dn_track_t track)
{
    dn_albumData_t* aData = (dn_albumData_t*)album->data;
    dn_twoColors_t colors = dn_trackCoordsToColor(trackCoords);
    wsgPaletteSet(&aData->screenOnPalette, colors.unlit, c344);
    wsgPaletteSet(&aData->screenOnPalette, colors.lit, c555);
    wsgPaletteSet(&aData->screenAttackPalette, colors.lit, cTransparent);
    if (track == DN_REMIX_TRACK || track == DN_BLUE_TRACK)
    {
        wsgPaletteSet(&aData->screenOnPalette, colors.unlit, c103);
        wsgPaletteSet(&aData->screenOnPalette, colors.lit, c105);
    }
    if (track == DN_REMIX_TRACK || track == DN_RED_TRACK)
    {
        wsgPaletteSet(&aData->screenAttackPalette, colors.lit, c510);
    }
}

void dn_updateAlbum(dn_entity_t* self)
{
    dn_albumData_t* aData = (dn_albumData_t*)self->data;
    if (!aData->screenIsOn)
    {
        aData->timer -= self->gameData->elapsedUs;
        if (aData->timer <= 0)
        {
            while (true && self != ((dn_albumsData_t*)self->gameData->entityManager.albums->data)->creativeCommonsAlbum)
            {
                dn_boardPos_t pos = (dn_boardPos_t){dn_randomInt(-2, 2), 1};
                if (dn_trackTypeAtCoords(self, pos) == DN_NONE_TRACK)
                {
                    dn_addTrackToAlbum(self, pos, DN_RED_TRACK);
                    break;
                }
            }
            if (self == ((dn_albumsData_t*)self->gameData->entityManager.albums->data)->p2Album)
            {
                // third album has finished the bootup animation
                self->gameData->phase = DN_P1_DANCE_PHASE;
                dn_startTurn(self);
            }
            aData->cornerLightOn = true;
            aData->screenIsOn    = true;
            aData->timer         = 0;
            self->updateFunction = NULL;
        }
    }
}

void dn_drawAlbum(dn_entity_t* self)
{
    dn_albumData_t* aData = (dn_albumData_t*)self->data;
    int32_t x             = ((self->pos.x - self->gameData->camera.pos.x) >> DN_DECIMAL_BITS)
                - self->gameData->assets[DN_ALBUM_ASSET].originX;
    int32_t y = ((self->pos.y - self->gameData->camera.pos.y) >> DN_DECIMAL_BITS)
                - self->gameData->assets[DN_ALBUM_ASSET].originY;
    drawWsgPalette(&self->gameData->assets[DN_ALBUM_ASSET].frames[0], x, y,
                   aData->screenIsOn ? &aData->screenOnPalette : &aData->screenOffPalette, false, false, aData->rot);
    drawWsgPalette(&self->gameData->assets[DN_ALBUM_EXPLOSION_ASSET].frames[self->currentAnimationFrame], x, y,
                   &aData->screenAttackPalette, false, false, aData->rot);
    if ((aData->cornerLightOn && !aData->cornerLightBlinking)
        || (aData->cornerLightOn && aData->cornerLightBlinking && (self->gameData->generalTimer & 0b111111) > 15))
    {
        if (aData->rot == 180)
        {
            x += 5;
            y += 54;
        }
        else
        {
            x += 53;
            y += 4;
        }
        drawWsgSimple(&self->gameData->assets[DN_STATUS_LIGHT_ASSET].frames[0], x, y);
    }
    self->animationTimer++;
    if (self->animationTimer >= self->gameFramesPerAnimationFrame)
    {
        self->animationTimer = 0;
        self->currentAnimationFrame++;
        if (self->currentAnimationFrame >= self->gameData->assets[DN_ALBUM_EXPLOSION_ASSET].numFrames)
        {
            self->currentAnimationFrame = 0;
        }
    }
}

void dn_updateCharacterSelect(dn_entity_t* self)
{
    dn_characterSelectData_t* cData = (dn_characterSelectData_t*)self->data;
    if (self->gameData->btnDownState & PB_A)
    {
        // select marker
        self->gameData->characterSets[0] = cData->selectCharacterIdx;
        // save to NVS
        writeNvs32(dnCharacterKey, self->gameData->characterSets[0]);

        dn_setCharacterSetPalette(&self->gameData->entityManager, self->gameData->characterSets[0]);
    }
    if (self->gameData->btnDownState & PB_B)
    {
        dn_exitSubMode(self);
        return;
    }
    else if (self->gameData->btnState & PB_LEFT)
    {
        if (cData->xSelectScrollOffset == 0)
        {
            // scroll left
            if (cData->selectCharacterIdx == 0)
            {
                cData->selectCharacterIdx = DN_NUM_CHARACTERS - 1;
            }
            else
            {
                cData->selectCharacterIdx--;
            }
            cData->xSelectScrollOffset -= self->gameData->assets[DN_GROUND_TILE_ASSET].frames[0].w * 5;
        }
    }
    else if (self->gameData->btnState & PB_RIGHT)
    {
        if (cData->xSelectScrollOffset == 0)
        {
            // scroll right
            cData->selectCharacterIdx = (cData->selectCharacterIdx + 1) % DN_NUM_CHARACTERS;
            // increment the offset to scroll
            cData->xSelectScrollOffset += self->gameData->assets[DN_GROUND_TILE_ASSET].frames[0].w * 5;
        }
    }

    // Scroll the offset if it's not centered yet
    cData->xSelectScrollTimer += self->gameData->elapsedUs;
    while (cData->xSelectScrollTimer >= 3000)
    {
        cData->xSelectScrollTimer -= 3000;
        if (cData->xSelectScrollOffset > 0)
        {
            cData->xSelectScrollOffset--;
        }
        else if (cData->xSelectScrollOffset < 0)
        {
            cData->xSelectScrollOffset++;
        }
    }
}
void dn_drawCharacterSelect(dn_entity_t* self)
{
    dn_characterSelectData_t* cData = (dn_characterSelectData_t*)self->data;

    // Draw the background, a blank menu
    drawMenuMega(self->gameData->bgMenu, self->gameData->menuRenderer, self->gameData->elapsedUs);

    // Set up variables for drawing
    int16_t yOff = MANIA_TITLE_HEIGHT + 20;
    int16_t xOff
        = ((TFT_WIDTH - self->gameData->assets[DN_GROUND_TILE_ASSET].frames[0].w) >> 1) + cData->xSelectScrollOffset;
    int8_t pIdx = cData->selectCharacterIdx;

    // 'Rewind' characters until they're off screen
    while (xOff > 0)
    {
        xOff -= self->gameData->assets[DN_GROUND_TILE_ASSET].frames[0].w * 5;
        pIdx--;
    }

    // Don't use a negative index!
    if (pIdx < 0)
    {
        pIdx = DN_NUM_CHARACTERS - 1;
    }
    // pIdx = dn_wrap(pIdx, DN_NUM_CHARACTERS - 1);

    // Draw floor tiles
    for (int16_t y = 0; y < 9; y++)
    {
        // Draw tiles until you're off screen
        while (xOff < TFT_WIDTH + ((self->gameData->assets[DN_GROUND_TILE_ASSET].frames[0].w * 5) >> 1))
        {
            for (int16_t x = -2; x < 3; x++)
            {
                int16_t drawX = xOff + x * self->gameData->assets[DN_GROUND_TILE_ASSET].frames[0].w
                                + ((self->gameData->assets[DN_GROUND_TILE_ASSET].frames[0].w >> 1) * (y % 2));
                int16_t drawY = yOff + y * (self->gameData->assets[DN_GROUND_TILE_ASSET].frames[0].h >> 1);
                if (drawX >= -self->gameData->assets[DN_GROUND_TILE_ASSET].frames[0].w && drawX <= TFT_WIDTH)
                {
                    // If this is the active maker, draw swapped pallete
                    if (pIdx == self->gameData->characterSets[0] && cData->selectDiamondShape[y * 5 + x + 2])
                    {
                        drawWsgPaletteSimple(
                            &self->gameData->assets[DN_GROUND_TILE_ASSET].frames[0], drawX, drawY,
                            &self->gameData->entityManager
                                 .palettes[DN_RED_FLOOR_PALETTE
                                           + (((y * ((self->gameData->generalTimer >> 10) % 10) + x + 2)
                                               + (self->gameData->generalTimer >> 6))
                                              % 6)]);
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
        // reset values
        xOff = ((TFT_WIDTH - self->gameData->assets[DN_GROUND_TILE_ASSET].frames[0].w) >> 1)
               + cData->xSelectScrollOffset;
        pIdx = cData->selectCharacterIdx;

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

    xOff += self->gameData->assets[DN_GROUND_TILE_ASSET].originX;
    yOff += self->gameData->assets[DN_GROUND_TILE_ASSET].originY;

    // Draw characters until you're off screen (sort of)
    while (xOff < TFT_WIDTH + ((self->gameData->assets[DN_GROUND_TILE_ASSET].frames[0].w * 5) >> 1))
    {
        dn_assetIdx_t kingDown = 0;
        dn_assetIdx_t kingUp   = 0;
        dn_assetIdx_t pawnDown = 0;
        dn_assetIdx_t pawnUp   = 0;

        switch (pIdx)
        {
            case DN_ALPHA_SET:
            {
                kingDown = DN_ALPHA_DOWN_ASSET;
                kingUp   = DN_ALPHA_UP_ASSET;
                pawnDown = DN_BUCKET_HAT_DOWN_ASSET;
                pawnUp   = DN_BUCKET_HAT_UP_ASSET;
                break;
            }
            case DN_CHESS_SET:
            {
                kingDown = DN_KING_ASSET;
                kingUp   = DN_KING_ASSET;
                pawnDown = DN_PAWN_ASSET;
                pawnUp   = DN_PAWN_ASSET;
                break;
            }
            default:
            {
                break;
            }
        }
        for (int8_t i = 0; i < 5; i++)
        {
            if (i == 2) // king is the middle piece
            {
                drawWsgSimple(&self->gameData->assets[kingDown].frames[0],
                              xOff + (self->gameData->assets[DN_GROUND_TILE_ASSET].frames[0].w >> 1) * i
                                  - self->gameData->assets[kingDown].originX,
                              yOff + (self->gameData->assets[DN_GROUND_TILE_ASSET].frames[0].h >> 1) * i
                                  - self->gameData->assets[kingDown].originY);
                if (kingUp == DN_KING_ASSET)
                {
                    drawWsgPaletteSimple(
                        &self->gameData->assets[kingUp].frames[0],
                        xOff - (self->gameData->assets[DN_GROUND_TILE_ASSET].frames[0].w >> 1) * (4 - i)
                            - self->gameData->assets[kingUp].originX,
                        yOff + (self->gameData->assets[DN_GROUND_TILE_ASSET].frames[0].h >> 1) * (4 + i)
                            - self->gameData->assets[kingUp].originY,
                        &self->gameData->entityManager.palettes[DN_WHITE_CHESS_PALETTE]);
                }
                else
                {
                    drawWsgSimple(&self->gameData->assets[kingUp].frames[0],
                                  xOff - (self->gameData->assets[DN_GROUND_TILE_ASSET].frames[0].w >> 1) * (4 - i)
                                      - self->gameData->assets[kingUp].originX,
                                  yOff + (self->gameData->assets[DN_GROUND_TILE_ASSET].frames[0].h >> 1) * (4 + i)
                                      - self->gameData->assets[kingUp].originY);
                }
            }
            else
            {
                drawWsgSimple(&self->gameData->assets[pawnDown].frames[0],
                              xOff + (self->gameData->assets[DN_GROUND_TILE_ASSET].frames[0].w >> 1) * i
                                  - self->gameData->assets[pawnDown].originX,
                              yOff + (self->gameData->assets[DN_GROUND_TILE_ASSET].frames[0].h >> 1) * i
                                  - self->gameData->assets[pawnDown].originY);
                if (pawnUp == DN_PAWN_ASSET)
                {
                    drawWsgPaletteSimple(
                        &self->gameData->assets[pawnUp].frames[0],
                        xOff - (self->gameData->assets[DN_GROUND_TILE_ASSET].frames[0].w >> 1) * (4 - i)
                            - self->gameData->assets[pawnUp].originX,
                        yOff + (self->gameData->assets[DN_GROUND_TILE_ASSET].frames[0].h >> 1) * (4 + i)
                            - self->gameData->assets[pawnUp].originY,
                        &self->gameData->entityManager.palettes[DN_WHITE_CHESS_PALETTE]);
                }
                else
                {
                    drawWsgSimple(&self->gameData->assets[pawnUp].frames[0],
                                  xOff - (self->gameData->assets[DN_GROUND_TILE_ASSET].frames[0].w >> 1) * (4 - i)
                                      - self->gameData->assets[pawnUp].originX,
                                  yOff + (self->gameData->assets[DN_GROUND_TILE_ASSET].frames[0].h >> 1) * (4 + i)
                                      - self->gameData->assets[pawnUp].originY);
                }
            }
        }

        // Increment X offset
        xOff += self->gameData->assets[DN_GROUND_TILE_ASSET].frames[0].w * 5;
        // Increment marker index
        pIdx = (pIdx + 1) % DN_NUM_CHARACTERS;
    }

    // Draw arrows to indicate this can be scrolled
    // Blink the arrows

    if ((self->gameData->generalTimer % 256) > 128)
    {
        // Draw arrows to indicate this can be scrolled
        drawText(&self->gameData->font_righteous, c000, "<", 3, 41);
        drawText(&self->gameData->font_righteous, c550, "<", 3, 38);

        drawText(&self->gameData->font_righteous, c000, ">", TFT_WIDTH - 20, 41);
        drawText(&self->gameData->font_righteous, c550, ">", TFT_WIDTH - 20, 38);
    }
}

void dn_updateTileSelector(dn_entity_t* self)
{
    dn_tileSelectorData_t* tData = (dn_tileSelectorData_t*)self->data;
    dn_boardData_t* bData        = (dn_boardData_t*)self->gameData->entityManager.board->data;
    // move with WASD
    bData->tiles[tData->pos.y][tData->pos.x].selector = NULL;
    if (self->gameData->btnDownState & PB_LEFT)
    {
        tData->pos.x--;
    }
    if (self->gameData->btnDownState & PB_UP)
    {
        tData->pos.y--;
    }
    if (self->gameData->btnDownState & PB_RIGHT)
    {
        tData->pos.x++;
    }
    if (self->gameData->btnDownState & PB_DOWN)
    {
        tData->pos.y++;
    }

    tData->pos.x = CLAMP(tData->pos.x, 0, 5);
    tData->pos.y = CLAMP(tData->pos.y, 0, 5);

    if (tData->pos.y == 5)
    {
        dn_findLastEntityOfType(self, DN_SWAPBUTTON_DATA)->paused = false;
        self->destroyFlag                           = true;
        return;
    }
    if (tData->pos.x == 5)
    {
        dn_findLastEntityOfType(self, DN_SKIPBUTTON_DATA)->paused = false;
        self->destroyFlag                           = true;
        return;
    }

    bData->tiles[tData->pos.y][tData->pos.x].selector = self;

    if (tData->b_callback && self->gameData->btnDownState & PB_B)
    {
        tData->b_callback(self);
    }
    else if (self->gameData->btnDownState & PB_A)
    {
        tData->a_callback(self);
    }

    // lines move up at varying rates
    for (int line = 0; line < NUM_SELECTOR_LINES; line++)
    {
        tData->lineYs[line] += line % 7;
        if (dn_randomInt(0, 600) < tData->lineYs[line])
        {
            tData->lineYs[line] = 0;
        }
    }
}

void dn_drawTileSelectorBackHalf(dn_entity_t* self, int16_t x, int16_t y)
{
    dn_tileSelectorData_t* tData = (dn_tileSelectorData_t*)self->data;
    for (int line = 0; line < NUM_SELECTOR_LINES; line++)
    {
        drawLineFast(x - 23, y - (tData->lineYs[line] >> 3), x, y - 11 - (tData->lineYs[line] >> 3),
                     tData->colors[line % 3]);
        drawLineFast(x, y - 11 - (tData->lineYs[line] >> 3), x + 23, y - (tData->lineYs[line] >> 3),
                     tData->colors[line % 3]);
    }
}

void dn_drawTileSelectorFrontHalf(dn_entity_t* self, int16_t x, int16_t y)
{
    dn_tileSelectorData_t* tData = (dn_tileSelectorData_t*)self->data;
    for (int line = 0; line < NUM_SELECTOR_LINES; line++)
    {
        drawLineFast(x - 23, y - (tData->lineYs[line] >> 3), x, y + 11 - (tData->lineYs[line] >> 3),
                     tData->colors[line % 3]);
        drawLineFast(x, y + 11 - (tData->lineYs[line] >> 3), x + 23, y - (tData->lineYs[line] >> 3),
                     tData->colors[line % 3]);
    }
}

void dn_trySelectUnit(dn_entity_t* self)
{
    dn_tileSelectorData_t* tData = (dn_tileSelectorData_t*)self->data;
    dn_boardData_t* bData        = (dn_boardData_t*)self->gameData->entityManager.board->data;
    if (bData->tiles[tData->pos.y][tData->pos.x].unit != NULL && bData->tiles[tData->pos.y][tData->pos.x].selectionType)
    {
        tData->selectedUnit = bData->tiles[tData->pos.y][tData->pos.x].unit;

        dn_clearSelectableTiles(self);

        // recalculate selectable tiles
        list_t* myList = heap_caps_calloc(1, sizeof(list_t), MALLOC_CAP_8BIT);
        memset(myList, 0, sizeof(list_t));
        if (dn_availableMoves(tData->selectedUnit, myList))
        {
            node_t* cur = myList->first;
            while (cur != NULL)
            {
                dn_action_t* action                                      = ((dn_action_t*)cur->val);
                bData->tiles[action->pos.y][action->pos.x].selectionType = action->action;
                cur                                                      = cur->next;
            }
        }
        clear(myList);
        free(myList);

        // would make it do the selection idle here later
        tData->a_callback   = dn_trySelectTrack;
        tData->b_callback   = dn_cancelSelectTrack;
    }
}

void dn_cancelSelectUnit(dn_entity_t* self)
{
    self->gameData->phase--;
    dn_boardData_t* bData = (dn_boardData_t*)self->gameData->entityManager.board->data;
    for (int y = 0; y < DN_BOARD_SIZE; y++)
    {
        for (int x = 0; x < DN_BOARD_SIZE; x++)
        {
            if (bData->tiles[y][x].selector)
            {
                bData->tiles[y][x].selector->destroyFlag = true;
                bData->tiles[y][x].selector              = NULL;
            }
        }
    }
    dn_setupDancePhase(self);
}

void dn_trySelectTrack(dn_entity_t* self)
{
    dn_tileSelectorData_t* tData = (dn_tileSelectorData_t*)self->data;
    dn_boardData_t* bData        = (dn_boardData_t*)self->gameData->entityManager.board->data;
    if (bData->tiles[tData->pos.y][tData->pos.x].selectionType)
    {
        dn_clearSelectableTiles(self);

        // determine the vector FROM the unit TO the track
        dn_boardPos_t from = dn_getUnitBoardPos(tData->selectedUnit);
        // tData->pos is "to"
        dn_boardPos_t relativeTrack = (dn_boardPos_t){tData->pos.x - from.x, tData->pos.y - from.y};

        dn_entity_t* album = ((dn_albumsData_t*)self->gameData->entityManager.albums->data)->p1Album;
        // Make it relative to the player's facing direction
        if (self->gameData->phase < DN_P2_DANCE_PHASE)
        {
            relativeTrack.y *= -1;
        }
        else
        {
            relativeTrack.x *= -1;
            album = ((dn_albumsData_t*)self->gameData->entityManager.albums->data)->p2Album;
        }
        dn_track_t type                                   = dn_trackTypeAtCoords(album, relativeTrack);
        if (type == DN_REMIX_TRACK && tData->selectedUnit->paused)
        {
            // just treat it like movement, because the unit can't shoot.
            type = DN_BLUE_TRACK;
        }
        switch (type)
        {
            case DN_BLUE_TRACK:
            {
                ((dn_unitData_t*)tData->selectedUnit->data)->moveTo = tData->pos;
                tData->selectedUnit->updateFunction                 = dn_moveUnit;
                bData->tiles[tData->pos.y][tData->pos.x].selector = NULL;
                self->destroyFlag                                 = true;
                break;
            }
            case DN_RED_TRACK:
            case DN_REMIX_TRACK:
            {
                if(!bData->tiles[tData->pos.y][tData->pos.x].unit && !bData->tiles[tData->pos.y][tData->pos.x].timeout)
                {
                    tData->bounce = tData->pos;
                    //////////////////////////////////
                    // make the prompt bullet bounce//
                    //////////////////////////////////
                    dn_entity_t* promptBulletBounce
                        = dn_createPrompt(&self->gameData->entityManager, (vec_t){0xffff, 0xffff}, self->gameData);
                    dn_promptData_t* promptData = (dn_promptData_t*)promptBulletBounce->data;

                    promptData->usesTwoLinesOfText = true;
                    strcpy(promptData->text, "Targeted an empty tile.");
                    strcpy(promptData->text2, "Choose bounce destination.");
                    promptData->options = heap_caps_calloc(1, sizeof(list_t), MALLOC_CAP_8BIT);
                    memset(promptData->options, 0, sizeof(list_t));

                    dn_promptOption_t* option1 = heap_caps_malloc(sizeof(dn_promptOption_t), MALLOC_CAP_8BIT);
                    memset(option1, 0, sizeof(dn_promptOption_t));
                    strcpy(option1->text, "OK");
                    option1->callback          = dn_setupBounceOptions;
                    option1->downPressDetected = false;
                    push(promptData->options, (void*)option1);
                    promptData->numOptions = 1;
                    self->gameData->resolvingRemix                = type == DN_REMIX_TRACK;
                }
                else
                {
                    bData->tiles[tData->pos.y][tData->pos.x].selector = NULL;
                    self->destroyFlag                                 = true;
                    bData->tiles[from.y][from.x].unit->paused = true;
                    /////////////////////
                    // Make the bullet //
                    /////////////////////

                    vec_t start = (vec_t){
                        self->gameData->entityManager.board->pos.x
                            + (from.x - from.y) * (self->gameData->assets[DN_GROUND_TILE_ASSET].originX << DN_DECIMAL_BITS)
                            - (1 << DN_DECIMAL_BITS),
                        self->gameData->entityManager.board->pos.y
                            + (from.x + from.y) * (self->gameData->assets[DN_GROUND_TILE_ASSET].originY << DN_DECIMAL_BITS)
                            - (bData->tiles[from.y][from.x].yOffset)};

                    dn_entity_t* bullet = dn_createEntitySpecial(&self->gameData->entityManager, 0, DN_NO_ANIMATION, true,
                                                                DN_NO_ASSET, 0, start, self->gameData);
                    bullet->updateFunction = dn_updateBullet;
                    bullet->drawFunction   = dn_drawBullet;
                    bullet->data           = heap_caps_calloc(1, sizeof(dn_bulletData_t), MALLOC_CAP_SPIRAM);
                    memset(bullet->data, 0, sizeof(dn_bulletData_t));
                    bullet->dataType                             = DN_BULLET_DATA;
                    ((dn_bulletData_t*)bullet->data)->firstTarget = tData->pos;
                    ((dn_bulletData_t*)bullet->data)->secondTarget = (dn_boardPos_t){-1,-1};
                    ((dn_bulletData_t*)bullet->data)->start      = bullet->pos;
                    ((dn_bulletData_t*)bullet->data)->end
                        = (vec_t){self->gameData->entityManager.board->pos.x
                                    + (tData->pos.x - tData->pos.y)
                                            * (self->gameData->assets[DN_GROUND_TILE_ASSET].originX << DN_DECIMAL_BITS)
                                    - (1 << DN_DECIMAL_BITS),
                                self->gameData->entityManager.board->pos.y
                                    + (tData->pos.x + tData->pos.y)
                                            * (self->gameData->assets[DN_GROUND_TILE_ASSET].originY << DN_DECIMAL_BITS)
                                    - (bData->tiles[tData->pos.y][tData->pos.x].yOffset)};
                    if (type == DN_REMIX_TRACK)
                    {
                        ((dn_bulletData_t*)bullet->data)->ownerToMove = from;
                        self->gameData->resolvingRemix                = true;
                    }
                }
                break;
            }
            default:
            {
                break;
            }
        }
    }
}
void dn_cancelSelectTrack(dn_entity_t* self)
{
    dn_clearSelectableTiles(self);
    dn_tileSelectorData_t* tData = (dn_tileSelectorData_t*)self->data;
    tData->selectedUnit          = NULL;
    dn_calculateMoveableUnits(self->gameData->entityManager.board);

    tData->a_callback = dn_trySelectUnit;
    tData->b_callback = NULL;
}

void dn_drawPlayerTurn(dn_entity_t* self)
{
    // Temporary solution to showing rerolls until LED Matrix works
    drawWsgSimpleScaled(&self->gameData->assets[DN_NUMBER_ASSET].frames[self->gameData->rerolls[0]], 5,
                        30 + 100 * (self->gameData->camera.pos.y < 60060), 3, 3);
    drawWsgSimpleScaled(&self->gameData->assets[DN_NUMBER_ASSET].frames[self->gameData->rerolls[1]], 257,
                        30 + 100 * (self->gameData->camera.pos.y < 60060), 3, 3);

    paletteColor_t col = self->gameData->phase < DN_P2_DANCE_PHASE ? c055 : c550;
    drawCircleQuadrants(41, 41, 41, false, false, true, false, col);
    drawCircleQuadrants(TFT_WIDTH - 42, 41, 41, false, false, false, true, col);
    drawCircleQuadrants(41, TFT_HEIGHT - 42, 41, false, true, false, false, col);
    drawCircleQuadrants(TFT_WIDTH - 42, TFT_HEIGHT - 42, 41, true, false, false, false, col);
    drawRect(0, 0, TFT_WIDTH - 0, TFT_HEIGHT - 0, col);

    drawCircleQuadrants(41, 41, 40, false, false, true, false, col);
    drawCircleQuadrants(TFT_WIDTH - 42, 41, 40, false, false, false, true, col);
    drawCircleQuadrants(41, TFT_HEIGHT - 42, 40, false, true, false, false, col);
    drawCircleQuadrants(TFT_WIDTH - 42, TFT_HEIGHT - 42, 40, true, false, false, false, col);
    drawRect(1, 1, TFT_WIDTH - 1, TFT_HEIGHT - 1, col);
}

void dn_updatePrompt(dn_entity_t* self)
{
    dn_promptData_t* pData = (dn_promptData_t*)self->data;
    node_t* cur            = pData->options->first;
    while (cur != NULL)
    {
        dn_promptOption_t* option = (dn_promptOption_t*)cur->val;
        option->selectionAmount -= self->gameData->elapsedUs >> 6;
        if (option->selectionAmount < 0)
        {
            option->selectionAmount = 0;
        }
        cur = cur->next;
    }
    if (pData->animatingIntroSlide)
    {
        pData->yOffset -= self->gameData->elapsedUs >> 11;
        if (pData->yOffset < 70)
        {
            pData->yOffset             = 70;
            pData->animatingIntroSlide = false;
        }
    }
    else
    {
        if (self->gameData->btnDownState & PB_A)
        {
            cur = pData->options->first;
            for (int i = 1; i <= pData->selectionIdx; i++)
            {
                cur = cur->next;
            }
            dn_promptOption_t* option = (dn_promptOption_t*)cur->val;
            option->downPressDetected = true;
        }
        if (self->gameData->btnState & PB_UP)
        {
            pData->playerHasSlidThis = true;
            pData->yOffset -= self->gameData->elapsedUs >> 13;
        }
        if (self->gameData->btnState & PB_DOWN)
        {
            pData->playerHasSlidThis = true;
            pData->yOffset += self->gameData->elapsedUs >> 13;
        }
        pData->yOffset = CLAMP(pData->yOffset, -44, 220);
        if (self->gameData->btnDownState & PB_LEFT && pData->selectionIdx > 0)
        {
            cur = pData->options->first;
            for (int i = 1; i <= pData->selectionIdx; i++)
            {
                cur = cur->next;
            }
            dn_promptOption_t* option = (dn_promptOption_t*)cur->val;
            option->downPressDetected = false;
            pData->selectionIdx--;
        }
        if (self->gameData->btnDownState & PB_RIGHT && pData->selectionIdx < pData->numOptions - 1)
        {
            cur = pData->options->first;
            for (int i = 1; i <= pData->selectionIdx; i++)
            {
                cur = cur->next;
            }
            dn_promptOption_t* option = (dn_promptOption_t*)cur->val;
            option->downPressDetected = false;
            pData->selectionIdx++;
        }
        if (self->gameData->btnState & PB_A)
        {
            cur = pData->options->first;
            for (int i = 1; i <= pData->selectionIdx; i++)
            {
                cur = cur->next;
            }
            dn_promptOption_t* option = (dn_promptOption_t*)cur->val;
            if (option->downPressDetected)
            {
                option->selectionAmount += self->gameData->elapsedUs >> 3;
                if (option->selectionAmount >= 30000)
                {
                    option->selectionAmount = 30000;
                    option->callback(self);

                    clear(pData->options);
                    free(pData->options);

                    self->destroyFlag = true;
                }
            }
        }
    }
}
void dn_drawPrompt(dn_entity_t* self)
{
    dn_promptData_t* pData = (dn_promptData_t*)self->data;
    // black banner
    drawRectFilled(0, pData->yOffset, TFT_WIDTH, pData->yOffset + 60, c000);
    int16_t xOff = TFT_WIDTH >> 1;
    int16_t yOff = pData->yOffset + 11;
    // prompt text
    paletteColor_t outer  = c425;
    paletteColor_t middle = c535;
    paletteColor_t inner  = c555;
    if (!pData->isPurple)
    {
        outer  = self->gameData->phase < DN_P2_DANCE_PHASE ? c245 : c442;
        middle = self->gameData->phase < DN_P2_DANCE_PHASE ? c355 : c553;
    }
    uint16_t tWidth = textWidth(&self->gameData->font_ibm, pData->text);
    if (pData->usesTwoLinesOfText)
    {
        yOff -= 6;
    }
    drawShinyText(&self->gameData->font_ibm, outer, middle, inner, pData->text, xOff - (tWidth >> 1), yOff);
    if (pData->usesTwoLinesOfText)
    {
        yOff += 12;
        tWidth = textWidth(&self->gameData->font_ibm, pData->text2);
        drawShinyText(&self->gameData->font_ibm, outer, middle, inner, pData->text2, xOff - (tWidth >> 1), yOff);
    }

    node_t* option = pData->options->first;
    int xPos       = (TFT_WIDTH / 2) / pData->options->length;
    int separation = xPos * 2;
    for (int i = 0; i < pData->options->length; i++)
    {
        dn_promptOption_t* optionVal = (dn_promptOption_t*)option->val;
        xPos += separation * i;
        // fill effect
        drawRectFilled(xPos - 25, pData->yOffset + 34,
                       dn_lerp(xPos - 25, xPos + 25, dn_logRemap(optionVal->selectionAmount)), pData->yOffset + 56,
                       c022);
        // outline
        drawRect(xPos - 25, pData->yOffset + 34, xPos + 25, pData->yOffset + 56,
                 pData->selectionIdx == i ? middle : c222);
        xOff = xPos - 25;
        yOff = pData->yOffset + 39;
        // option text
        drawTextWordWrapCentered(&self->gameData->font_ibm, pData->selectionIdx == i ? c555 : c222, optionVal->text,
                                 &xOff, &yOff, xPos + 25, pData->yOffset + 56);
        option = option->next;
    }

    // flashy arrows up and down
    if (pData->yOffset == 70 && !pData->playerHasSlidThis && (self->gameData->generalTimer % 256) > 128)
    {
        drawWsgPaletteSimple(
            &self->gameData->assets[DN_MMM_UP_ASSET].frames[0],
            (TFT_WIDTH >> 1) - self->gameData->assets[DN_MMM_UP_ASSET].originX,
            pData->yOffset - 4 - self->gameData->assets[DN_MMM_UP_ASSET].originY,
            &self->gameData->entityManager
                 .palettes[self->gameData->phase >= DN_P2_DANCE_PHASE ? DN_P2_ARROW_PALETTE : DN_PURPLE_FLOOR_PALETTE]);
        drawWsgPalette(
            &self->gameData->assets[DN_MMM_UP_ASSET].frames[0],
            (TFT_WIDTH >> 1) - self->gameData->assets[DN_MMM_UP_ASSET].originX,
            pData->yOffset + 64 - self->gameData->assets[DN_MMM_UP_ASSET].originY,
            &self->gameData->entityManager
                 .palettes[self->gameData->phase >= DN_P2_DANCE_PHASE ? DN_P2_ARROW_PALETTE : DN_PURPLE_FLOOR_PALETTE],
            false, true, 0);
    }
}

void dn_startTurn(dn_entity_t* self)
{
    // Create an array for all LEDs
    led_t leds[CONFIG_NUM_LEDS];
    for (uint8_t i = 0; i < CONFIG_NUM_LEDS; i++)
    {
        leds[i].r = self->gameData->phase == DN_P1_DANCE_PHASE ? 0 : 255;
        leds[i].g = 255;
        leds[i].b = self->gameData->phase == DN_P1_DANCE_PHASE ? 255 : 0;
    }
    // Set the LED output
    setLeds(leds, CONFIG_NUM_LEDS);

    dn_setBlinkingLights(self);

    ////////////////////////////////
    // Make the turn start prompt //
    ////////////////////////////////
    dn_entity_t* promptToStart = dn_createEntitySpecial(&self->gameData->entityManager, 0, DN_NO_ANIMATION, true,
                                                        DN_NO_ASSET, 0, (vec_t){0xffff, 0xffff}, self->gameData);
    promptToStart->data        = heap_caps_calloc(1, sizeof(dn_promptData_t), MALLOC_CAP_SPIRAM);
    memset(promptToStart->data, 0, sizeof(dn_promptData_t));

    dn_promptData_t* promptData     = (dn_promptData_t*)promptToStart->data;
    promptData->animatingIntroSlide = true;
    promptData->yOffset             = 320; // way off screen to allow more time to look at albums.

    if (self->gameData->rerolls[self->gameData->phase >= DN_P2_DANCE_PHASE] != 9)
    {
        snprintf(promptData->text, sizeof(promptData->text), "%s's Turn! Gain 2 rerolls.",
                 self->gameData->shortPlayerNames[self->gameData->phase >= DN_P2_DANCE_PHASE]);
    }
    else
    {
        snprintf(promptData->text, sizeof(promptData->text), "%s's Turn! 9 rerolls reached.",
                 self->gameData->shortPlayerNames[self->gameData->phase >= DN_P2_DANCE_PHASE]);
    }

    promptData->options = heap_caps_calloc(1, sizeof(list_t), MALLOC_CAP_8BIT);
    memset(promptData->options, 0, sizeof(list_t));

    dn_promptOption_t* option1 = heap_caps_malloc(sizeof(dn_promptOption_t), MALLOC_CAP_8BIT);
    memset(option1, 0, sizeof(dn_promptOption_t));
    strcpy(option1->text, "OK");
    option1->callback          = dn_gainRerollAndSetupDancePhase;
    option1->downPressDetected = false;
    push(promptData->options, (void*)option1);
    promptData->numOptions        = 1;
    promptToStart->dataType       = DN_PROMPT_DATA;
    promptToStart->updateFunction = dn_updatePrompt;
    promptToStart->drawFunction   = dn_drawPrompt;
}

void dn_gainReroll(dn_entity_t* self)
{
    self->gameData->rerolls[self->gameData->phase >= DN_P2_DANCE_PHASE]++;
    self->gameData->rerolls[self->gameData->phase >= DN_P2_DANCE_PHASE]
        = CLAMP(self->gameData->rerolls[self->gameData->phase >= DN_P2_DANCE_PHASE], 0, 9);
}

void dn_gainRerollAndSetupDancePhase(dn_entity_t* self)
{
    dn_gainReroll(self);
    dn_gainReroll(self);

    dn_setupDancePhase(self);
}

void dn_setupDancePhase(dn_entity_t* self) // used to be dn_startMovePhase
{
    dn_clearSelectableTiles(self);
    dn_calculateMoveableUnits(self->gameData->entityManager.board);

    ///////////////////////////
    // Make the tile selector//
    ///////////////////////////
    dn_entity_t* tileSelector = dn_createEntitySpecial(&self->gameData->entityManager, 0, DN_NO_ANIMATION, true,
                                                       DN_NO_ASSET, 0, self->gameData->camera.pos, self->gameData);
    tileSelector->data        = heap_caps_calloc(1, sizeof(dn_tileSelectorData_t), MALLOC_CAP_SPIRAM);
    memset(tileSelector->data, 0, sizeof(dn_tileSelectorData_t));
    ((dn_tileSelectorData_t*)tileSelector->data)->a_callback = dn_trySelectUnit;
    ((dn_tileSelectorData_t*)tileSelector->data)->b_callback = NULL;
    tileSelector->dataType                                   = DN_TILE_SELECTOR_DATA;
    tileSelector->drawFunction                               = dn_drawNothing;
    dn_tileSelectorData_t* tData                             = (dn_tileSelectorData_t*)tileSelector->data;
    for (int i = 0; i < NUM_SELECTOR_LINES; i++)
    {
        tData->lineYs[i] = (255 * i) / NUM_SELECTOR_LINES;
    }
    tData->pos = (dn_boardPos_t){2, 2};
    // fancy line colors for player one
    tData->colors[0] = c125;
    tData->colors[1] = c345;
    tData->colors[2] = c555;
    if (self->gameData->phase >= DN_P2_DANCE_PHASE)
    {
        tData->colors[0] = c442;
        tData->colors[1] = c543;
        tData->colors[2] = c555;
    }
    tileSelector->updateFunction = dn_updateTileSelector;
    // Don't set the draw function, because it needs to happen in two parts behind and in front of units.

    ((dn_boardData_t*)self->gameData->entityManager.board->data)->tiles[2][2].selector = tileSelector;
}

void dn_acceptRerollAndSkip(dn_entity_t* self)
{
    dn_gainReroll(self);
    dn_clearSelectableTiles(self);
    dn_incrementPhase(self); // It is the upgrade phase

    dn_startUpgradeMenu(self, 0);
}

void dn_acceptRerollAndSwapHelper(dn_entity_t* self, bool progressPhase)
{
    dn_gainReroll(self);
    ///////////////////
    // Make the swap //
    ///////////////////
    dn_entity_t* swap = dn_createEntitySpecial(
        &self->gameData->entityManager, 0, DN_NO_ANIMATION, true, DN_NO_ASSET, 0,
        addVec2d(self->gameData->camera.pos, (vec_t){(107 << DN_DECIMAL_BITS), -(68 << DN_DECIMAL_BITS)}),
        self->gameData);
    swap->data = heap_caps_calloc(1, sizeof(dn_swapAlbumsData_t), MALLOC_CAP_SPIRAM);
    memset(swap->data, 0, sizeof(dn_swapAlbumsData_t));

    dn_swapAlbumsData_t* swapData = (dn_swapAlbumsData_t*)swap->data;
    swapData->progressPhase       = progressPhase;
    dn_albumsData_t* aData        = (dn_albumsData_t*)self->gameData->entityManager.albums->data;
    ((dn_albumData_t*)aData->p1Album->data)->cornerLightOn              = false;
    ((dn_albumData_t*)aData->creativeCommonsAlbum->data)->cornerLightOn = false;
    ((dn_albumData_t*)aData->p2Album->data)->cornerLightOn              = false;
    if (self->gameData->phase < DN_P2_DANCE_PHASE)
    {
        swapData->firstAlbum    = aData->p1Album;
        swapData->firstAlbumIdx = 0;

        swapData->secondAlbum    = aData->p2Album;
        swapData->secondAlbumIdx = 2;
    }
    else
    {
        swapData->firstAlbum    = aData->p2Album;
        swapData->firstAlbumIdx = 2;

        swapData->secondAlbum    = aData->p1Album;
        swapData->secondAlbumIdx = 0;
    }
    swap->dataType       = DN_SWAP_DATA;
    swap->updateFunction = dn_updateSwapAlbums;
    swap->drawFunction   = NULL;
}

void dn_acceptRerollAndSwap(dn_entity_t* self)
{
    dn_acceptRerollAndSwapHelper(self, false);
}

void dn_acceptRerollAndSwapAndProgress(dn_entity_t* self)
{
    dn_acceptRerollAndSwapHelper(self, true);
}

void dn_acceptThreeRerolls(dn_entity_t* self)
{
    dn_gainReroll(self);
    dn_gainReroll(self);
    dn_gainReroll(self);
    if (!self->gameData->resolvingRemix)
    {
        dn_incrementPhase(self);
        dn_startUpgradeMenu(self, 2 << 20);
    }
    self->gameData->resolvingRemix = false;
}

void dn_clearSelectableTiles(dn_entity_t* self)
{
    dn_boardData_t* bData = (dn_boardData_t*)self->gameData->entityManager.board->data;
    // set selectable tiles off
    for (int i = 0; i < 5; i++)
    {
        for (int j = 0; j < 5; j++)
        {
            bData->tiles[i][j].selectionType = DN_NO_SELECTION;
        }
    }
}

void dn_startUpgradeMenu(dn_entity_t* self, int32_t countOff)
{
    self->gameData->resolvingRemix = false;

    //////////////////////////
    // Make the upgrade menu//
    //////////////////////////
    dn_entity_t* upgradeMenu = dn_createEntitySpecial(
        &self->gameData->entityManager, 0, DN_NO_ANIMATION, true, DN_NO_ASSET, 0,
        addVec2d(self->gameData->camera.pos, (vec_t){(107 << DN_DECIMAL_BITS), -(140 << DN_DECIMAL_BITS)}),
        self->gameData);
    upgradeMenu->data = heap_caps_calloc(1, sizeof(dn_upgradeMenuData_t), MALLOC_CAP_SPIRAM);
    memset(upgradeMenu->data, 0, sizeof(dn_upgradeMenuData_t));
    ((dn_upgradeMenuData_t*)upgradeMenu->data)->timer = countOff;
    ((dn_upgradeMenuData_t*)upgradeMenu->data)->numTracksToAdd = -1;
    upgradeMenu->dataType                             = DN_UPGRADE_MENU_DATA;
    upgradeMenu->updateFunction                       = dn_updateUpgradeMenu;
    upgradeMenu->drawFunction                         = dn_drawUpgradeMenu;

    dn_initializeSecondUpgradeOption(upgradeMenu);
    dn_initializeThirdUpgradeOption(upgradeMenu);
    dn_initializeFirstUpgradeOption(upgradeMenu);
    dn_initializeUpgradeConfirmOption(upgradeMenu);
}

void dn_acceptSwapCC(dn_entity_t* self)
{
    self->paused = true;
    if (self->gameData->rerolls[self->gameData->phase >= DN_P2_DANCE_PHASE] < 5)
    {
        //////////////////////////////////////
        // make the prompt not enough rerolls//
        //////////////////////////////////////
        dn_entity_t* promptNotEnough
            = dn_createPrompt(&self->gameData->entityManager, (vec_t){0xffff, 0xffff}, self->gameData);
        dn_promptData_t* promptData = (dn_promptData_t*)promptNotEnough->data;

        strcpy(promptData->text, "Not enough rerolls.");
        promptData->options = heap_caps_calloc(1, sizeof(list_t), MALLOC_CAP_8BIT);
        memset(promptData->options, 0, sizeof(list_t));

        dn_promptOption_t* option1 = heap_caps_malloc(sizeof(dn_promptOption_t), MALLOC_CAP_8BIT);
        memset(option1, 0, sizeof(dn_promptOption_t));
        strcpy(option1->text, "OK");
        option1->callback          = dn_unpauseSwapButton;
        option1->downPressDetected = false;
        push(promptData->options, (void*)option1);
        promptData->numOptions = 1;

        return;
    }

    self->gameData->rerolls[self->gameData->phase >= DN_P2_DANCE_PHASE] -= 5;
    ///////////////////
    // Make the swap //
    ///////////////////
    dn_entity_t* swap = dn_createEntitySpecial(
        &self->gameData->entityManager, 0, DN_NO_ANIMATION, true, DN_NO_ASSET, 0,
        addVec2d(self->gameData->camera.pos, (vec_t){(107 << DN_DECIMAL_BITS), -(68 << DN_DECIMAL_BITS)}),
        self->gameData);
    swap->data = heap_caps_calloc(1, sizeof(dn_swapAlbumsData_t), MALLOC_CAP_SPIRAM);
    memset(swap->data, 0, sizeof(dn_swapAlbumsData_t));
    dn_swapAlbumsData_t* swapData = (dn_swapAlbumsData_t*)swap->data;
    dn_albumsData_t* aData        = (dn_albumsData_t*)self->gameData->entityManager.albums->data;
    ((dn_albumData_t*)aData->p1Album->data)->cornerLightOn              = false;
    ((dn_albumData_t*)aData->creativeCommonsAlbum->data)->cornerLightOn = false;
    ((dn_albumData_t*)aData->p2Album->data)->cornerLightOn              = false;
    if (self->gameData->phase < DN_P2_DANCE_PHASE)
    {
        swapData->firstAlbum    = aData->p1Album;
        swapData->firstAlbumIdx = 0;
    }
    else
    {
        swapData->firstAlbum    = aData->p2Album;
        swapData->firstAlbumIdx = 2;
    }
    swapData->secondAlbum    = aData->creativeCommonsAlbum;
    swapData->secondAlbumIdx = 1;
    swap->dataType           = DN_SWAP_DATA;
    swap->updateFunction     = dn_updateSwapAlbums;
    swap->drawFunction       = NULL;
}

void dn_incrementPhase(dn_entity_t* self)
{
    self->gameData->phase++;
    if (self->gameData->phase > DN_P2_UPGRADE_PHASE)
    {
        self->gameData->phase = DN_P1_DANCE_PHASE;
    }

    // update the unit selection colors
    if (self->gameData->phase == DN_P1_DANCE_PHASE)
    {
        dn_setCharacterSetPalette(&self->gameData->entityManager, self->gameData->characterSets[0]);
    }
    else if (self->gameData->phase == DN_P2_DANCE_PHASE)
    {
        dn_setCharacterSetPalette(&self->gameData->entityManager, self->gameData->characterSets[1]);
    }

    dn_setBlinkingLights(self);

    // debug stuff
    switch (self->gameData->phase)
    {
        case DN_P1_DANCE_PHASE:
        {
            printf("DN_P1_DANCE_PHASE\n");
            break;
        }
        case DN_P1_UPGRADE_PHASE:
        {
            printf("DN_P1_UPGRADE_PHASE\n");
            break;
        }
        case DN_P2_DANCE_PHASE:
        {
            printf("DN_P2_DANCE_PHASE\n");
            break;
        }
        case DN_P2_UPGRADE_PHASE:
        {
            printf("DN_P2_UPGRADE_PHASE\n");
            break;
        }
    }
}

void dn_drawPit(dn_entity_t* self)
{
    int32_t x = ((self->pos.x - self->gameData->camera.pos.x) >> DN_DECIMAL_BITS)
                - self->gameData->assets[self->assetIndex].originX;
    int32_t y = ((self->pos.y - self->gameData->camera.pos.y) >> DN_DECIMAL_BITS);
    drawWsgSimple(&self->gameData->assets[self->assetIndex].frames[self->currentAnimationFrame], x, y);

    x = ((self->pos.x - self->gameData->camera.pos.x) >> DN_DECIMAL_BITS) - 1;
    drawWsgPalette(&self->gameData->assets[self->assetIndex].frames[self->currentAnimationFrame], x, y,
                   &self->gameData->entityManager.palettes[DN_PIT_WALL_PALETTE], true, false, 0);
    drawRectFilled(x - 138, y - 16, x + 139, y, c323);
    drawRectFilled(x - 138, y, x - 125, y + 152, c323);
    drawRectFilled(x + 126, y, x + 139, y + 152, c323);
}

void dn_drawPitForeground(dn_entity_t* self)
{
    drawTriangleOutlined(((self->pos.x - self->gameData->camera.pos.x) >> DN_DECIMAL_BITS) - 126,
                         ((self->pos.y - self->gameData->camera.pos.y) >> DN_DECIMAL_BITS) + 51,
                         ((self->pos.x - self->gameData->camera.pos.x) >> DN_DECIMAL_BITS) - 126,
                         ((self->pos.y - self->gameData->camera.pos.y) >> DN_DECIMAL_BITS) + 116,
                         ((self->pos.x - self->gameData->camera.pos.x) >> DN_DECIMAL_BITS) - 1,
                         ((self->pos.y - self->gameData->camera.pos.y) >> DN_DECIMAL_BITS) + 116, c323, c323);

    drawTriangleOutlined(((self->pos.x - self->gameData->camera.pos.x) >> DN_DECIMAL_BITS) + 124,
                         ((self->pos.y - self->gameData->camera.pos.y) >> DN_DECIMAL_BITS) + 51,
                         ((self->pos.x - self->gameData->camera.pos.x) >> DN_DECIMAL_BITS) + 124,
                         ((self->pos.y - self->gameData->camera.pos.y) >> DN_DECIMAL_BITS) + 116,
                         ((self->pos.x - self->gameData->camera.pos.x) >> DN_DECIMAL_BITS),
                         ((self->pos.y - self->gameData->camera.pos.y) >> DN_DECIMAL_BITS) + 116, c323, c323);

    drawLineFast(((self->pos.x - self->gameData->camera.pos.x) >> DN_DECIMAL_BITS) - 126,
                 ((self->pos.y - self->gameData->camera.pos.y) >> DN_DECIMAL_BITS) + 117,
                 ((self->pos.x - self->gameData->camera.pos.x) >> DN_DECIMAL_BITS) + 124,
                 ((self->pos.y - self->gameData->camera.pos.y) >> DN_DECIMAL_BITS) + 117, c323);
}

// Helper function to get the board position of a unit
// Returns {-1, -1} if not found or invalid input
dn_boardPos_t dn_getUnitBoardPos(dn_entity_t* unit)
{
    dn_boardPos_t foundPos    = {-1, -1};
    dn_boardData_t* boardData = (dn_boardData_t*)unit->gameData->entityManager.board->data;
    for (int y = 0; y < DN_BOARD_SIZE; y++)
    {
        for (int x = 0; x < DN_BOARD_SIZE; x++)
        {
            if (boardData->tiles[y][x].unit == unit)
            {
                foundPos.x = x;
                foundPos.y = y;
                return foundPos;
            }
        }
    }
    return foundPos;
}

void dn_updateUpgradeMenu(dn_entity_t* self)
{
    dn_upgradeMenuData_t* umData = (dn_upgradeMenuData_t*)self->data;

    if (umData->timer > 0)
    {
        umData->timer -= self->gameData->elapsedUs;
        if (umData->timer > 0)
        {
            return;
        }
    }

    for (uint8_t option = 0; option < 4; option++)
    {
        umData->options[option].selectionAmount -= self->gameData->elapsedUs >> 6;
        if (umData->options[option].selectionAmount < 0)
        {
            umData->options[option].selectionAmount = 0;
        }
    }

    if (self->gameData->btnDownState & PB_A)
    {
        umData->options[umData->selectionIdx].downPressDetected = true;
    }
    if (self->gameData->btnDownState & PB_UP)
    {
        umData->options[umData->selectionIdx].downPressDetected = false;
        umData->selectionIdx--;
    }
    if (self->gameData->btnDownState & PB_DOWN)
    {
        umData->options[umData->selectionIdx].downPressDetected = false;
        umData->selectionIdx++;
    }
    umData->selectionIdx = CLAMP(umData->selectionIdx, 0, 3);

    if (self->gameData->btnState & PB_A)
    {
        if (umData->options[umData->selectionIdx].downPressDetected
            && (self->gameData->rerolls[self->gameData->phase >= DN_P2_DANCE_PHASE] || umData->selectionIdx == 3))
        {
            umData->options[umData->selectionIdx].selectionAmount
                += self->gameData->elapsedUs >> (3 + (umData->selectionIdx == 3) * 2); // confirm takes longer to press
            if (umData->options[umData->selectionIdx].selectionAmount >= 30000)
            {
                umData->options[umData->selectionIdx].selectionAmount = 30000;
                if (umData->options[umData->selectionIdx].callback)
                {
                    umData->options[umData->selectionIdx].callback(self);
                    umData->options[umData->selectionIdx].selectionAmount   = 0;
                    umData->options[umData->selectionIdx].downPressDetected = false;
                }
            }
        }
    }

    if (self->gameData->camera.pos.y > (self->pos.y - (26 << DN_DECIMAL_BITS)))
    {
        self->gameData->camera.pos.y -= self->gameData->elapsedUs >> 8;
        self->gameData->entityManager.albums->pos.y -= self->gameData->elapsedUs / 1900;
        dn_albumsData_t* aData = (dn_albumsData_t*)self->gameData->entityManager.albums->data;
        aData->p1Album->pos.y -= self->gameData->elapsedUs / 1900;
        aData->creativeCommonsAlbum->pos.y -= self->gameData->elapsedUs / 1900;
        aData->p2Album->pos.y -= self->gameData->elapsedUs / 1900;
    }
    else if (self->gameData->entityManager.albums->pos.y != 63427)
    {
        self->gameData->camera.pos.y                = self->pos.y - (26 << DN_DECIMAL_BITS);
        self->gameData->entityManager.albums->pos.y = 63427;
        dn_albumsData_t* aData                      = (dn_albumsData_t*)self->gameData->entityManager.albums->data;
        aData->p1Album->pos.y                       = 62912;
        aData->creativeCommonsAlbum->pos.y          = 62912;
        aData->p2Album->pos.y                       = 62912;
    }

    if(umData->numTracksToAdd > -1)
    {
        umData->flashyBoxSize -= self->gameData->elapsedUs >> 13;
        if (umData->flashyBoxSize < 0)
        {
            switch(umData->numTracksToAdd)
            {
                case 7:
                case 1:
                {
                    dn_entity_t* album = NULL;
                    switch (umData->album[0])
                    {
                        case 0:
                        {
                            album = (dn_entity_t*)((dn_albumsData_t*)self->gameData->entityManager.albums->data)->p1Album;
                            break;
                        }
                        case 1:
                        {
                            album = (dn_entity_t*)((dn_albumsData_t*)self->gameData->entityManager.albums->data)->p2Album;
                            break;
                        }
                        case 2:
                        {
                            album = (dn_entity_t*)((dn_albumsData_t*)self->gameData->entityManager.albums->data)->creativeCommonsAlbum;
                            break;
                        }
                    }
                    dn_addTrackToAlbum(album, umData->track[0], umData->trackColor);
                    break;
                }
                case 5:// add a random track to creative commons in the upgrade menu
                {
                    dn_initializeFirstUpgradeOption(self);
                    break;
                }
                case 4:// add a random track to creative commons in the upgrade menu
                {
                    dn_initializeSecondUpgradeOption(self);
                    break;
                }
                case 3:// add a random track to creative commons in the upgrade menu
                {
                    umData->album[0] = 2;
                    break;
                }
                case 0:
                {
                    self->updateFunction = dn_updateAfterUpgradeMenu;
                    break;
                }
                default:
                {
                    break;
                }
            }
            umData->flashyBoxSize = 127;
            umData->numTracksToAdd--;
        }
    }
}

void dn_updateAfterUpgradeMenu(dn_entity_t* self)
{
    self->gameData->camera.pos.y += self->gameData->elapsedUs >> 9;
    dn_albumsData_t* aData = (dn_albumsData_t*)self->gameData->entityManager.albums->data;
    aData->p1Album->pos.y += self->gameData->elapsedUs / 1900;
    aData->creativeCommonsAlbum->pos.y += self->gameData->elapsedUs / 1900;
    aData->p2Album->pos.y += self->gameData->elapsedUs / 1900;

    // function moves the camera back down after upgrade was chosen.
    if (self->gameData->camera.pos.y < 62703)
    {
        self->gameData->camera.pos.y += self->gameData->elapsedUs >> 8;
        self->gameData->entityManager.albums->pos.y += self->gameData->elapsedUs / 1900;
        aData->p1Album->pos.y += self->gameData->elapsedUs / 1900;
        aData->creativeCommonsAlbum->pos.y += self->gameData->elapsedUs / 1900;
        aData->p2Album->pos.y += self->gameData->elapsedUs / 1900;
    }
    else
    {
        dn_incrementPhase(self);
        dn_startTurn(self);
        self->gameData->camera.pos.y                = 62703;
        self->gameData->entityManager.albums->pos.y = 63823;
        aData->p1Album->pos.y                       = 0xFFFF - (139 << DN_DECIMAL_BITS);
        aData->creativeCommonsAlbum->pos.y          = 0xFFFF - (139 << DN_DECIMAL_BITS);
        aData->p2Album->pos.y                       = 0xFFFF - (139 << DN_DECIMAL_BITS);
        self->destroyFlag                           = true;
    }
}

void dn_drawUpgradeMenu(dn_entity_t* self)
{
    dn_upgradeMenuData_t* umData = (dn_upgradeMenuData_t*)self->data;

    int32_t x = (self->pos.x - self->gameData->camera.pos.x) >> DN_DECIMAL_BITS;
    int32_t y = (self->pos.y - self->gameData->camera.pos.y) >> DN_DECIMAL_BITS;

    // corner brackets
    // top left
    drawLineFast(x, y, x + 5, y, c555);
    drawLineFast(x, y, x, y + 5, c555);
    // top right
    drawLineFast(x + 160, y, x + 165, y, c555);
    drawLineFast(x + 165, y, x + 165, y + 5, c555);
    // bottom left
    drawLineFast(x, y + 97, x + 5, y + 97, c555);
    drawLineFast(x, y + 92, x, y + 97, c555);
    // bottom right
    drawLineFast(x + 160, y + 97, x + 165, y + 97, c555);
    drawLineFast(x + 165, y + 92, x + 165, y + 97, c555);
    // vertical left line
    drawLineFast(x, y + 9, x, y + 88, c555);
    // vertical right line
    drawLineFast(x + 165, y + 9, x + 165, y + 88, c555);

    uint16_t tWidth = 0;

    for (uint8_t option = 0; option < 3; option++)
    {
        // option 1
        // THIS IS THE LEFT BOX
        drawRect(x + 2, y + 3 + 31 * option, x + 144, y + 32 + 31 * option,
                 umData->selectionIdx == option ? c555 : c434);
        if (umData->selectionIdx == option)
        {
            drawRectFilled(x + 2, y + 3 + 31 * option, x + 144, y + 32 + 31 * option, c323);
        }
        drawRect(x + 143, y + 3 + 31 * option, x + 164, y + 32 + 31 * option,
                 umData->selectionIdx == option ? c555 : c434);
        drawRectFilled(x + 144,
                       y + 31 + 31 * option - dn_lerp(0, 27, dn_logRemap(umData->options[option].selectionAmount)),
                       x + 163, y + 31 + 31 * option, c521);
        if (umData->selectionIdx == option)
        {
            drawWsgPaletteSimple(&self->gameData->assets[DN_REROLL_ASSET].frames[0], x + 144, y + 4 + 31 * option,
                                 &self->gameData->entityManager.palettes[DN_REROLL_PALETTE]);
        }
        else
        {
            drawWsgSimple(&self->gameData->assets[DN_REROLL_ASSET].frames[0], x + 144, y + 4 + 31 * option);
        }
        char text[31] = "";
        switch (option)
        {
            case 0:
            {
                switch (umData->trackColor)
                {
                    case DN_RED_TRACK:
                    {
                        snprintf(text, sizeof(text), "Write a fire track,");
                        break;
                    }
                    case DN_BLUE_TRACK:
                    {
                        snprintf(text, sizeof(text), "Write a moving track,");
                        break;
                    }
                    case DN_REMIX_TRACK:
                    {
                        snprintf(text, sizeof(text), "Write a remix,");
                        break;
                    }
                    default:
                    {
                        break;
                    }
                }
                break;
            }
            case 1:
            {
                if (umData->track[0].x != 0 && umData->track[0].y != 0)
                {
                    snprintf(text, sizeof(text), "%s %d, %s %d,", umData->track[0].y < 0 ? "back" : "forward",
                             (uint8_t)ABS(umData->track[0].y), umData->track[0].x < 0 ? "left" : "right",
                             (uint8_t)ABS(umData->track[0].x));
                }
                else if (umData->track[0].x == 0 && umData->track[0].y != 0)
                {
                    snprintf(text, sizeof(text), "%s %d,", umData->track[0].y < 0 ? "back" : "forward",
                             (uint8_t)ABS(umData->track[0].y));
                }
                else if (umData->track[0].x != 0 && umData->track[0].y == 0)
                {
                    snprintf(text, sizeof(text), "%s %d,", umData->track[0].x < 0 ? "left" : "right",
                             (uint8_t)ABS(umData->track[0].x));
                }
                break;
            }
            case 2:
            {
                switch (umData->album[0])
                {
                    case 0:
                    case 1:
                    {
                        snprintf(text, sizeof(text), "on %s's album.",
                                 self->gameData->shortPlayerNames[umData->album[0]]);
                        break;
                    }
                    case 2:
                    {
                        snprintf(text, sizeof(text), "on the Creative Commons album.");
                        break;
                    }
                }
                break;
            }
        }
        int16_t xValue = x + 6;
        int16_t yValue = y + 7 + 31 * option;
        uint8_t length = *(&text + 1) - text - 1;
        drawTextWordWrapCentered(&self->gameData->font_ibm, umData->selectionIdx == option ? c555 : c545, text, &xValue,
                                 &yValue, x + 141 + (length < 19 ? 8 : 0), y + 30 + 31 * option);

        //Draw glitch art
        if((umData->numTracksToAdd == 3 && option >= 2) ||
             (umData->numTracksToAdd == 4 && option >= 1) ||
             umData->numTracksToAdd == 5)
        {
            for(uint8_t glitchX = 0; glitchX < 35; glitchX++)
            {
                for(uint8_t glitchY = 0; glitchY < 3; glitchY++)
                {
                    if(dn_randomInt(0,2))
                    {
                        // Use a deterministic pseudo-random value based on glitchX, glitchY, and flashyBoxSize
                        uint8_t arbitraryByteFromMemory = (uint8_t)(
                            ((option * 30 + glitchX + glitchY + umData->flashyBoxSize / 10) ^ 0xA5) % 256
                        );
                        if(arbitraryByteFromMemory > 100)
                        {
                            drawWsgSimple(&self->gameData->assets[DN_GLITCH_ASSET].frames[arbitraryByteFromMemory % 6], x + 4 + glitchX * 4, y + 6 + 31 * option + glitchY * 8);
                        }
                    }
                }
            }
        }
    }

    if (self->gameData->camera.pos.y == (self->pos.y - (26 << DN_DECIMAL_BITS)) && self->updateFunction == dn_updateUpgradeMenu)
    {
        if(umData->numTracksToAdd == -1)
        {
            //confirm button
            drawRectFilled(x + 40, y + 98, x + dn_lerp(40, 125, dn_logRemap(umData->options[3].selectionAmount)), y + 116,
                        c521);
            drawRect(x + 40, y + 98, x + 125, y + 116, umData->selectionIdx == 3 ? c555 : c434);
            tWidth = textWidth(&self->gameData->font_ibm, "CONFIRM");
            drawText(&self->gameData->font_ibm, umData->selectionIdx == 3 ? c555 : c545, "CONFIRM", x + 82 - (tWidth >> 1),
                    y + 102);
        }

        //here's the flashy rainbow square
        uint16_t indicatorX = 56;
        uint16_t indicatorY = 175;

        if (umData->album[0] == 1)
        {
            indicatorX += 161;
            indicatorY -= 5;
        }
        else if (umData->album[0] == 2)
        {
            indicatorX += 80;
        }

        indicatorX += umData->track[0].x * 8 * (umData->album[0] == 1 ? -1 : 1);
        indicatorY -= umData->track[0].y * 8 * (umData->album[0] == 1 ? -1 : 1);

        if(umData->numTracksToAdd == -1 || umData->numTracksToAdd == 7 || umData->numTracksToAdd == 1)
        {
            bool drawAnimated = umData->numTracksToAdd == 7 || umData->numTracksToAdd == 1;
            drawRect(indicatorX - umData->flashyBoxSize * drawAnimated, indicatorY - umData->flashyBoxSize* drawAnimated, indicatorX + 9 + umData->flashyBoxSize* drawAnimated, indicatorY + 9 + umData->flashyBoxSize* drawAnimated, (paletteColor_t)dn_randomInt(0, 216));
            drawRect(indicatorX + 1 - umData->flashyBoxSize* drawAnimated, indicatorY + 1 - umData->flashyBoxSize* drawAnimated, indicatorX + 8 + umData->flashyBoxSize* drawAnimated, indicatorY + 8 + umData->flashyBoxSize* drawAnimated, (paletteColor_t)dn_randomInt(0, 216));

            
            //draw warning text if track already exists
            dn_entity_t* album = NULL;
            switch(umData->album[0])
            {
                case 0:
                {
                    album = (dn_entity_t*)((dn_albumsData_t*)self->gameData->entityManager.albums->data)->p1Album;
                    break;
                }
                case 1:
                {
                    album = (dn_entity_t*)((dn_albumsData_t*)self->gameData->entityManager.albums->data)->p2Album;
                    break;
                }
                case 2:
                {
                    album = (dn_entity_t*)((dn_albumsData_t*)self->gameData->entityManager.albums->data)->creativeCommonsAlbum;
                    break;
                }
            }
            if(dn_trackTypeAtCoords(album, umData->track[0]))
            {
                tWidth = textWidth(&self->gameData->font_ibm, "WARNING! This would overwrite!");
                drawText(&self->gameData->font_ibm, c511, "WARNING! This would overwrite!", (TFT_WIDTH>>1) - (tWidth >> 1), 10);
            }
        }
    }
}

void dn_initializeSecondUpgradeOption(dn_entity_t* self)
{
    dn_upgradeMenuData_t* umData = (dn_upgradeMenuData_t*)self->data;
    for (int track = 107; track <= 122; track++)
    {
        umData->track[track - 107] = dn_colorToTrackCoords((paletteColor_t)track);
    }
    umData->track[16] = (dn_boardPos_t){0, 0}; // null separator

    // shuffle
    for (int8_t i = sizeof(umData->track) / sizeof(umData->track[0]) - 2; i > 0; i--)
    {
        int8_t j           = (int8_t)(dn_randomInt(0, INT_MAX) % (i + 1));
        dn_boardPos_t temp = umData->track[i];
        umData->track[i]   = umData->track[j];
        umData->track[j]   = temp;
    }

    umData->options[1].callback = dn_rerollSecondUpgradeOption;
}
void dn_initializeThirdUpgradeOption(dn_entity_t* self)
{
    dn_upgradeMenuData_t* umData = (dn_upgradeMenuData_t*)self->data;
    for (int album = 0; album <= 2; album++)
    {
        umData->album[album] = album;
    }
    umData->album[3] = 3; // 3 separator
    umData->album[0] = self->gameData->phase >= DN_P2_DANCE_PHASE;
    umData->album[1] = self->gameData->phase < DN_P2_DANCE_PHASE;
    umData->album[2] = 2;
    if (dn_randomInt(0, 1))
    {
        umData->album[1] = 2;
        umData->album[2] = self->gameData->phase < DN_P2_DANCE_PHASE;
    }

    umData->options[2].callback = dn_rerollThirdUpgradeOption;
}
void dn_initializeFirstUpgradeOption(dn_entity_t* self)
{
    dn_rerollFirstUpgradeOptionFree(self);
    ((dn_upgradeMenuData_t*)self->data)->options[0].callback = dn_rerollFirstUpgradeOption;
}

void dn_initializeUpgradeConfirmOption(dn_entity_t* self)
{
    dn_upgradeMenuData_t* umData = (dn_upgradeMenuData_t*)self->data;
    umData->options[3].callback  = dn_confirmUpgrade;
}

void dn_rerollSecondUpgradeOption(dn_entity_t* self)
{
    self->gameData->rerolls[self->gameData->phase >= DN_P2_DANCE_PHASE]--;
    dn_upgradeMenuData_t* umData = (dn_upgradeMenuData_t*)self->data;
    uint8_t separatorIdx         = 0;
    for (int i = 0; i < sizeof(umData->track) / sizeof(umData->track[0]); i++)
    {
        if (umData->track[i].x == 0 && umData->track[i].y == 0)
        {
            separatorIdx = i;
            break;
        }
    }

    umData->track[separatorIdx]     = umData->track[0];
    umData->track[0]                = umData->track[separatorIdx - 1];
    umData->track[separatorIdx - 1] = (dn_boardPos_t){0, 0};

    if (umData->track[0].x == 0 && umData->track[0].y == 0)
    {
        dn_initializeSecondUpgradeOption(self);
    }
}
void dn_rerollThirdUpgradeOption(dn_entity_t* self)
{
    self->gameData->rerolls[self->gameData->phase >= DN_P2_DANCE_PHASE]--;
    dn_upgradeMenuData_t* umData = (dn_upgradeMenuData_t*)self->data;
    uint8_t separatorIdx         = 0;
    for (int i = 0; i < sizeof(umData->album) / sizeof(umData->album[0]); i++)
    {
        if (umData->album[i] == 3)
        {
            separatorIdx = i;
            break;
        }
    }

    umData->album[separatorIdx]     = umData->album[0];
    umData->album[0]                = umData->album[separatorIdx - 1];
    umData->album[separatorIdx - 1] = 3;

    if (umData->album[0] == 3)
    {
        dn_initializeThirdUpgradeOption(self);
    }
}

void dn_rerollFirstUpgradeOptionFree(dn_entity_t* self)
{
    dn_upgradeMenuData_t* umData = (dn_upgradeMenuData_t*)self->data;
    dn_track_t previous          = umData->trackColor;
    while (umData->trackColor == previous)
    {
        uint8_t roll = dn_randomInt(0, 100);
        if (roll < 50)
        {
            umData->trackColor = DN_BLUE_TRACK;
        }
        else if (roll < 80)
        {
            umData->trackColor = DN_RED_TRACK;
        }
        else
        {
            umData->trackColor = DN_REMIX_TRACK;
        }
    }
}

void dn_rerollFirstUpgradeOption(dn_entity_t* self)
{
    self->gameData->rerolls[self->gameData->phase >= DN_P2_DANCE_PHASE]--;
    dn_rerollFirstUpgradeOptionFree(self);
}

void dn_confirmUpgrade(dn_entity_t* self)
{
    dn_upgradeMenuData_t* umData       = (dn_upgradeMenuData_t*)self->data;
    umData->options[3].selectionAmount = 0;
    umData->numTracksToAdd = 7; //It really adds 2 tracks. Different things happen happen at 7,6,5,4,3,2,1, and 0
    umData->flashyBoxSize  = 127;
}

void dn_updateSwapAlbums(dn_entity_t* self)
{
    dn_albumsData_t* aData     = (dn_albumsData_t*)self->gameData->entityManager.albums->data;
    dn_swapAlbumsData_t* sData = (dn_swapAlbumsData_t*)self->data;
    if (self->gameData->camera.pos.y > self->pos.y)
    {
        self->gameData->camera.pos.y -= self->gameData->elapsedUs >> 9;
        aData->p1Album->pos.y -= self->gameData->elapsedUs / 1900;
        aData->creativeCommonsAlbum->pos.y -= self->gameData->elapsedUs / 1900;
        aData->p2Album->pos.y -= self->gameData->elapsedUs / 1900;
    }
    else if (self->gameData->camera.pos.y != self->pos.y)
    {
        self->gameData->camera.pos.y       = self->pos.y;
        aData->p1Album->pos.y              = 63021;
        aData->creativeCommonsAlbum->pos.y = 63021;
        aData->p2Album->pos.y              = 63021;

        sData->center = (vec_t){(sData->firstAlbum->pos.x + sData->secondAlbum->pos.x) / 2,
                                (sData->firstAlbum->pos.y + sData->secondAlbum->pos.y) / 2};
        sData->offset = subVec2d(sData->firstAlbum->pos, sData->center);
    }
    else
    {
        sData->lerpAmount += self->gameData->elapsedUs >> 6;
        if (sData->lerpAmount >= 30000)
        {
            sData->lerpAmount       = 30000;
            vec_t offset            = rotateVec2d(sData->offset, 180);
            sData->firstAlbum->pos  = addVec2d(sData->center, offset);
            offset                  = mulVec2d(offset, -1);
            sData->secondAlbum->pos = addVec2d(sData->center, offset);
            if (sData->firstAlbum == aData->p2Album)
            {
                ((dn_albumData_t*)sData->firstAlbum->data)->rot  = 0;
                ((dn_albumData_t*)sData->secondAlbum->data)->rot = 180;
            }
            else if (sData->secondAlbum == aData->p2Album)
            {
                ((dn_albumData_t*)sData->firstAlbum->data)->rot  = 180;
                ((dn_albumData_t*)sData->secondAlbum->data)->rot = 0;
            }
            switch (sData->firstAlbumIdx)
            {
                case 0:
                {
                    aData->p1Album = sData->secondAlbum;
                    break;
                }
                case 1:
                {
                    aData->creativeCommonsAlbum = sData->secondAlbum;
                    break;
                }
                case 2:
                {
                    aData->p2Album = sData->secondAlbum;
                    break;
                }
            }
            switch (sData->secondAlbumIdx)
            {
                case 0:
                {
                    aData->p1Album = sData->firstAlbum;
                    break;
                }
                case 1:
                {
                    aData->creativeCommonsAlbum = sData->firstAlbum;
                    break;
                }
                case 2:
                {
                    aData->p2Album = sData->firstAlbum;
                    break;
                }
            }
            self->updateFunction = dn_updateAfterSwap;
        }
        else
        {
            int16_t angle           = dn_lerp(0, 180, sData->lerpAmount);
            vec_t offset            = rotateVec2d(sData->offset, angle);
            sData->firstAlbum->pos  = addVec2d(sData->center, offset);
            offset                  = mulVec2d(offset, -1);
            sData->secondAlbum->pos = addVec2d(sData->center, offset);
            if (sData->firstAlbum == aData->p2Album)
            {
                ((dn_albumData_t*)sData->firstAlbum->data)->rot  = 180 + angle;
                ((dn_albumData_t*)sData->secondAlbum->data)->rot = angle;
            }
            else if (sData->secondAlbum == aData->p2Album)
            {
                ((dn_albumData_t*)sData->firstAlbum->data)->rot  = angle;
                ((dn_albumData_t*)sData->secondAlbum->data)->rot = 180 + angle;
            }
        }
    }
}

void dn_updateAfterSwap(dn_entity_t* self)
{
    self->gameData->camera.pos.y += self->gameData->elapsedUs >> 9;
    dn_albumsData_t* aData = (dn_albumsData_t*)self->gameData->entityManager.albums->data;
    aData->p1Album->pos.y += self->gameData->elapsedUs / 1900;
    aData->creativeCommonsAlbum->pos.y += self->gameData->elapsedUs / 1900;
    aData->p2Album->pos.y += self->gameData->elapsedUs / 1900;

    if (self->gameData->camera.pos.y > 62703)
    {
        self->gameData->camera.pos.y       = 62703;
        aData->p1Album->pos.y              = 0xFFFF - (139 << DN_DECIMAL_BITS);
        aData->creativeCommonsAlbum->pos.y = 0xFFFF - (139 << DN_DECIMAL_BITS);
        aData->p2Album->pos.y              = 0xFFFF - (139 << DN_DECIMAL_BITS);
        dn_swapAlbumsData_t* sData         = (dn_swapAlbumsData_t*)self->data;

        if (sData->progressPhase)
        {
            dn_incrementPhase(self);
            if (!self->gameData->resolvingRemix)
            {
                dn_startUpgradeMenu(self, 2 << 20);
            }
            self->gameData->resolvingRemix = false;
        }
        else
        {
            dn_unpauseSwapButton(self);
        }
        self->destroyFlag = true;
    }
}

void dn_setBlinkingLights(dn_entity_t* self)
{
    dn_albumsData_t* aData = (dn_albumsData_t*)self->gameData->entityManager.albums->data;

    ((dn_albumData_t*)aData->p1Album->data)->cornerLightOn              = true;
    ((dn_albumData_t*)aData->creativeCommonsAlbum->data)->cornerLightOn = true;
    ((dn_albumData_t*)aData->p2Album->data)->cornerLightOn              = true;

    ((dn_albumData_t*)aData->p1Album->data)->cornerLightBlinking
        = self->gameData->phase <= DN_P1_DANCE_PHASE || self->gameData->phase > DN_P2_DANCE_PHASE;
    ((dn_albumData_t*)aData->creativeCommonsAlbum->data)->cornerLightBlinking = false;
    ((dn_albumData_t*)aData->p2Album->data)->cornerLightBlinking
        = self->gameData->phase <= DN_P2_DANCE_PHASE && self->gameData->phase > DN_P1_DANCE_PHASE;
}

void dn_updateBullet(dn_entity_t* self)
{
    // increment the lerpAmount
    dn_bulletData_t* buData = (dn_bulletData_t*)self->data;
    buData->lerpAmount += self->gameData->elapsedUs >> 7;
    if (buData->lerpAmount > 30000)
    {
        buData->lerpAmount    = 30000;
        dn_boardData_t* bData = (dn_boardData_t*)self->gameData->entityManager.board->data;

        bData->impactPos          = buData->firstTarget;
        dn_tileData_t* targetTile = &bData->tiles[bData->impactPos.y][bData->impactPos.x];
        if (!targetTile->timeout)
        {
            targetTile->yVel = -700;
        }

        if (targetTile->unit)
        {
            if (targetTile->unit == ((dn_boardData_t*)self->gameData->entityManager.board->data)->p1Units[0]
                || targetTile->unit
                       == ((dn_boardData_t*)self->gameData->entityManager.board->data)->p2Units[0]) // king is captured
            {
                ///////////////////////////////
                // Make the prompt Game Over //
                ///////////////////////////////
                dn_entity_t* promptGameOver
                    = dn_createPrompt(&self->gameData->entityManager, (vec_t){0xffff, 0xffff}, self->gameData);
                dn_promptData_t* promptData = (dn_promptData_t*)promptGameOver->data;

                promptData->usesTwoLinesOfText = false;
                char text[40];
                strcpy(text,
                       self->gameData
                           ->playerNames[targetTile->unit
                                         == ((dn_boardData_t*)self->gameData->entityManager.board->data)->p1Units[0]]);
                strcat(text, " wins!");
                strcpy(promptData->text, text);
                promptData->isPurple = true;
                promptData->options  = heap_caps_calloc(1, sizeof(list_t), MALLOC_CAP_8BIT);
                memset(promptData->options, 0, sizeof(list_t));

                dn_promptOption_t* option1 = heap_caps_malloc(sizeof(dn_promptOption_t), MALLOC_CAP_8BIT);
                memset(option1, 0, sizeof(dn_promptOption_t));
                strcpy(option1->text, "OK");
                option1->callback          = NULL;
                option1->downPressDetected = false;
                push(promptData->options, (void*)option1);
                promptData->numOptions = 1;
            }
            else if ((dn_belongsToP1(targetTile->unit) && self->gameData->phase >= DN_P2_DANCE_PHASE)
                     || (!dn_belongsToP1(targetTile->unit) && self->gameData->phase < DN_P2_DANCE_PHASE))
            {
                ///////////////////////////////////
                // Make the prompt enemy captured//
                ///////////////////////////////////
                dn_entity_t* promptCaptured
                    = dn_createPrompt(&self->gameData->entityManager, (vec_t){0xffff, 0xffff}, self->gameData);
                dn_promptData_t* promptData = (dn_promptData_t*)promptCaptured->data;

                promptData->usesTwoLinesOfText = true;
                strcpy(promptData->text, "Enemy unit captured.");
                strcpy(promptData->text2, "Gain 1 reroll and swap albums.");
                promptData->options = heap_caps_calloc(1, sizeof(list_t), MALLOC_CAP_8BIT);
                memset(promptData->options, 0, sizeof(list_t));

                dn_promptOption_t* option1 = heap_caps_malloc(sizeof(dn_promptOption_t), MALLOC_CAP_8BIT);
                memset(option1, 0, sizeof(dn_promptOption_t));
                strcpy(option1->text, "OK");
                option1->callback          = dn_acceptRerollAndSwapAndProgress;
                option1->downPressDetected = false;
                push(promptData->options, (void*)option1);
                promptData->numOptions = 1;
            }
            else // friendly fire!
            {
                //////////////////////////////////////
                // Make the prompt for friendly fire//
                //////////////////////////////////////
                dn_entity_t* promptCaptured
                    = dn_createPrompt(&self->gameData->entityManager, (vec_t){0xffff, 0xffff}, self->gameData);
                dn_promptData_t* promptData = (dn_promptData_t*)promptCaptured->data;

                promptData->usesTwoLinesOfText = true;
                strcpy(promptData->text, "Friendly fire!!!");
                strcpy(promptData->text2, "Receive 3 rerolls from the afterlife.");
                promptData->options = heap_caps_calloc(1, sizeof(list_t), MALLOC_CAP_8BIT);
                memset(promptData->options, 0, sizeof(list_t));

                dn_promptOption_t* option1 = heap_caps_malloc(sizeof(dn_promptOption_t), MALLOC_CAP_8BIT);
                memset(option1, 0, sizeof(dn_promptOption_t));
                strcpy(option1->text, "OK");
                option1->callback          = dn_acceptThreeRerolls;
                option1->downPressDetected = false;
                push(promptData->options, (void*)option1);
                promptData->numOptions = 1;
            }
            // A unit dies
            targetTile->unit->destroyFlag = true;
            for (uint8_t i = 0; i < 5; i++)
            {
                if (bData->p1Units[i] == targetTile->unit)
                {
                    bData->p1Units[i] = NULL;
                    break;
                }
                if (bData->p2Units[i] == targetTile->unit)
                {
                    bData->p2Units[i] = NULL;
                    break;
                }
            }
            targetTile->unit = NULL;
        }
        else
        {
            // Patch any other holes currently
            for (int y = 0; y < DN_BOARD_SIZE; y++)
            {
                for (int x = 0; x < DN_BOARD_SIZE; x++)
                {
                    bData->tiles[y][x].timeout = false;
                }
            }

            // knock this tile out
            targetTile->timeout = true;

            
            if (buData->secondTarget.x != -1)
            {
                buData->firstTarget = buData->secondTarget;
                buData->lerpAmount = 0;
                buData->start = buData->end;
                buData->end
                        = (vec_t){self->gameData->entityManager.board->pos.x
                            + (buData->secondTarget.x - buData->secondTarget.y)
                                * (self->gameData->assets[DN_GROUND_TILE_ASSET].originX << DN_DECIMAL_BITS)
                            - (1 << DN_DECIMAL_BITS),
                        self->gameData->entityManager.board->pos.y
                            + (buData->secondTarget.x + buData->secondTarget.y)
                                * (self->gameData->assets[DN_GROUND_TILE_ASSET].originY << DN_DECIMAL_BITS)
                            - (bData->tiles[buData->secondTarget.y][buData->secondTarget.x].yOffset)};
                buData->secondTarget = (dn_boardPos_t){-1,-1};
                return;
            }
            else if (!self->gameData->resolvingRemix)
            {
                dn_incrementPhase(self); // now the upgrade phase
                dn_startUpgradeMenu(self, 2 << 20);
            }
            else // remix falls in a hole, because no unit was also at the target.
            {
                dn_entity_t* owner                    = bData->tiles[buData->ownerToMove.y][buData->ownerToMove.x].unit;
                ((dn_unitData_t*)owner->data)->moveTo = buData->firstTarget;
                owner->updateFunction                 = dn_moveUnit;
            }
        }
        if (self->gameData->resolvingRemix && buData->secondTarget.x == -1)
        {
            dn_entity_t* owner                    = bData->tiles[buData->ownerToMove.y][buData->ownerToMove.x].unit;
            ((dn_unitData_t*)owner->data)->moveTo = buData->firstTarget;
            owner->updateFunction                 = dn_moveUnit;
        }
        self->destroyFlag = true;
    }
    self->pos.x = dn_lerp(buData->start.x, buData->end.x, buData->lerpAmount);
    self->pos.y = dn_lerp(buData->start.y, buData->end.y, buData->lerpAmount);

    // sine wave that peaks at 75 pixels to fake a parabola.
    buData->yOffset = (int8_t)(sin(buData->lerpAmount * 0.00010471975511965977461542144610931676280657) * 75);
}

void dn_drawBullet(dn_entity_t* self)
{
    dn_bulletData_t* buData = (dn_bulletData_t*)self->data;
    int32_t x               = ((self->pos.x - self->gameData->camera.pos.x) >> DN_DECIMAL_BITS);
    int32_t y               = ((self->pos.y - self->gameData->camera.pos.y) >> DN_DECIMAL_BITS) - buData->yOffset;

    drawCircleFilled(x, y, 8, c555);
    // The bullet outline flashes with random colors from the current player's tile colors.
    drawCircleOutline(
        x, y, dn_randomInt(5, 11), dn_randomInt(1, 4),
        self->gameData->entityManager.palettes[DN_RED_FLOOR_PALETTE + dn_randomInt(0, 5)].newColors[c223]);
}

void dn_moveUnit(dn_entity_t* self)
{
    self->paused          = false;
    dn_boardData_t* bData = (dn_boardData_t*)self->gameData->entityManager.board->data;
    dn_unitData_t* uData  = (dn_unitData_t*)self->data;

    for (int y = 0; y < DN_BOARD_SIZE; y++)
    {
        for (int x = 0; x < DN_BOARD_SIZE; x++)
        {
            if (bData->tiles[y][x].unit == self)
            {
                bData->tiles[y][x].unit = NULL;
                if (y != 0 && y != 4
                    && ((self->gameData->phase < DN_P2_DANCE_PHASE && uData->moveTo.y == 0)
                        || (self->gameData->phase >= DN_P2_DANCE_PHASE && uData->moveTo.y == 4)))
                {
                    // units get a reroll for moving into the farthest rank.
                    dn_gainReroll(self);
                }
                break;
            }
        }
    }
    bData->tiles[uData->moveTo.y][uData->moveTo.x].unit = self;
    bData->impactPos                                    = uData->moveTo;

    self->gameData->rerolls[self->gameData->phase >= DN_P2_DANCE_PHASE]
        += bData->tiles[bData->impactPos.y][bData->impactPos.x].rewards;
    self->gameData->rerolls[self->gameData->phase >= DN_P2_DANCE_PHASE]
        = CLAMP(self->gameData->rerolls[self->gameData->phase >= DN_P2_DANCE_PHASE], 0, 9);
    bData->tiles[bData->impactPos.y][bData->impactPos.x].rewards = 0;

    bData->tiles[bData->impactPos.y][bData->impactPos.x].yVel = -700;

    if ((self == ((dn_boardData_t*)self->gameData->entityManager.board->data)->p1Units[0] && bData->impactPos.y == 0
         && bData->impactPos.x == 2)
        || (self == ((dn_boardData_t*)self->gameData->entityManager.board->data)->p2Units[0] && bData->impactPos.y == 4
            && bData->impactPos.x == 2)) // king moved to the opponent's throne
    {
        ///////////////////////////////
        // Make the prompt Game Over //
        ///////////////////////////////
        dn_entity_t* promptGameOver
            = dn_createPrompt(&self->gameData->entityManager, (vec_t){0xffff, 0xffff}, self->gameData);
        dn_promptData_t* promptData = (dn_promptData_t*)promptGameOver->data;

        promptData->usesTwoLinesOfText = true;
        char text[40];
        strcpy(
            text,
            self->gameData
                ->playerNames[bData->tiles[bData->impactPos.y][bData->impactPos.x].timeout
                                  ? self != ((dn_boardData_t*)self->gameData->entityManager.board->data)->p1Units[0]
                                  : self == ((dn_boardData_t*)self->gameData->entityManager.board->data)->p2Units[0]]);
        strcat(text, " wins!");
        promptData->isPurple = true;
        strcpy(promptData->text, text);
        promptData->options = heap_caps_calloc(1, sizeof(list_t), MALLOC_CAP_8BIT);
        memset(promptData->options, 0, sizeof(list_t));

        dn_promptOption_t* option1 = heap_caps_malloc(sizeof(dn_promptOption_t), MALLOC_CAP_8BIT);
        memset(option1, 0, sizeof(dn_promptOption_t));
        strcpy(option1->text, "OK");
        option1->callback          = dn_exitSubMode;
        option1->downPressDetected = false;
        push(promptData->options, (void*)option1);
        promptData->numOptions = 1;
    }
    else if (!bData->tiles[bData->impactPos.y][bData->impactPos.x].timeout & !self->gameData->resolvingRemix)
    {
        dn_incrementPhase(self); // now the upgrade phase
        dn_startUpgradeMenu(self, 2 << 20);
    }
    self->gameData->resolvingRemix = false;
    self->updateFunction           = NULL;
}

void dn_afterPlunge(dn_entity_t* self)
{
    dn_gainReroll(self);
    dn_gainReroll(self);
    dn_gainReroll(self);
    dn_incrementPhase(self); // now the upgrade phase
    dn_startUpgradeMenu(self, 0);
}

void dn_sharedButtonLogic(dn_entity_t* self)
{
    if (self->gameData->btnDownState & PB_UP)
    {
        self->paused = true;
        dn_setupDancePhase(self);
        self->gameData->btnDownState = 0;
    }
}

void dn_updateSwapButton(dn_entity_t* self)
{
    if (self->paused)
    {
        return;
    }

    dn_sharedButtonLogic(self);

    if (self->gameData->btnDownState & PB_RIGHT)
    {
        self->paused = true;
        dn_unpauseSkipButton(self);
    }
    else if (self->gameData->btnDownState & PB_A)
    {
        self->paused = true;
        dn_acceptSwapCC(self);
    }
}

void dn_drawSwapButton(dn_entity_t* self)
{
    dn_drawAsset(self);
    if (self->paused)
    {
        return;
    }
    int16_t x = ((self->pos.x - self->gameData->camera.pos.x) >> DN_DECIMAL_BITS) + 11;
    int16_t y = ((self->pos.y - self->gameData->camera.pos.y) >> DN_DECIMAL_BITS) + 23;
    drawShinyText(&self->gameData->font_ibm, self->gameData->phase < DN_P2_DANCE_PHASE ? c245 : c442,
                  self->gameData->phase < DN_P2_DANCE_PHASE ? c355 : c553, c555, "-5", x, y);

    drawWsgPaletteSimple(&self->gameData->assets[DN_REROLL_ASSET].frames[0], x + 17, y - 14,
                         &self->gameData->entityManager.palettes[DN_DICE_NO_ARROW_PALETTE]);

    x -= 27;
    y -= 26;

    drawShinyText(&self->gameData->font_ibm, self->gameData->phase < DN_P2_DANCE_PHASE ? c245 : c442,
                  self->gameData->phase < DN_P2_DANCE_PHASE ? c355 : c553, c555, "SWAP", x, y);
}

void dn_unpauseSwapButton(dn_entity_t* self)
{
    dn_findLastEntityOfType(self, DN_SWAPBUTTON_DATA)->paused = false;
}

void dn_updateSkipButton(dn_entity_t* self)
{
    if (self->paused)
    {
        return;
    }

    dn_sharedButtonLogic(self);

    if (self->gameData->btnDownState & PB_LEFT)
    {
        self->paused = true;
        dn_unpauseSwapButton(self);
    }
    else if (self->gameData->btnDownState & PB_A)
    {
        self->paused                 = true;
        self->gameData->btnDownState = 0;
        dn_acceptRerollAndSkip(self);
    }
}

void dn_drawSkipButton(dn_entity_t* self)
{
    dn_drawAsset(self);
    if (self->paused)
    {
        return;
    }
    int16_t x = ((self->pos.x - self->gameData->camera.pos.x) >> DN_DECIMAL_BITS) + 3;
    int16_t y = ((self->pos.y - self->gameData->camera.pos.y) >> DN_DECIMAL_BITS) - 6;
    drawShinyText(&self->gameData->font_ibm, self->gameData->phase < DN_P2_DANCE_PHASE ? c245 : c442,
                  self->gameData->phase < DN_P2_DANCE_PHASE ? c355 : c553, c555, "SKIP", x, y);
    y += 27;
    x -= 7;
    drawShinyText(&self->gameData->font_ibm, self->gameData->phase < DN_P2_DANCE_PHASE ? c245 : c442,
                  self->gameData->phase < DN_P2_DANCE_PHASE ? c355 : c553, c555, "+1", x, y);

    drawWsgPaletteSimple(&self->gameData->assets[DN_REROLL_ASSET].frames[0], x + 16, y - 11,
                         &self->gameData->entityManager.palettes[DN_DICE_NO_ARROW_PALETTE]);
}

void dn_unpauseSkipButton(dn_entity_t* self)
{
    dn_findLastEntityOfType(self, DN_SKIPBUTTON_DATA)->paused = false;
}

void dn_updateTutorial(dn_entity_t* self)
{
    dn_tutorialData_t* tData = (dn_tutorialData_t*)self->data;
    if(self->gameData->btnDownState & PB_RIGHT && tData->page < sizeof(tutorialText[tData->advancedTips]) / sizeof(tutorialText[tData->advancedTips][0]))
    {
        tData->page++;
    }
    else if(self->gameData->btnDownState & PB_LEFT && tData->page)
    {
        tData->page--;
    }
    if(tData->page == sizeof(tutorialText[tData->advancedTips]) / sizeof(tutorialText[tData->advancedTips][0]))
    {
        dn_exitSubMode(self);
    }
}

void dn_drawTutorial(dn_entity_t* self)
{
    dn_tutorialData_t* tData = (dn_tutorialData_t*)self->data;

    drawWsgSimple(&self->gameData->assets[DN_DANCENONYDA_ASSET].frames[0], 141, 20);
    drawWsgSimple(&self->gameData->assets[DN_TEXTBOX_ASSET].frames[0], 7, 125);
    drawWsgSimple(&self->gameData->assets[DN_TFT_ASSET].frames[0], 3, 3);

    int16_t x = 7;
    int16_t y = 45;
    drawTextWordWrapCentered(&self->gameData->font_ibm, c445, tutorialText[tData->advancedTips][tData->page][0], &x, &y, 136, 114);

    x = 7;
    y = 100;
    char buffer[8];
    snprintf(buffer, sizeof(buffer), "%d/%d", tData->page + 1, (uint8_t)(sizeof(tutorialText[tData->advancedTips]) / sizeof(tutorialText[tData->advancedTips][0])));
    drawTextWordWrapCentered(&self->gameData->font_ibm, c555, buffer, &x, &y, 136, 114);

    x = 24;
    y = 140;
    drawTextWordWrap(&self->gameData->font_ibm, c555, tutorialText[tData->advancedTips][tData->page][1], &x, &y, x+225, y+80);

    if((self->gameData->generalTimer % 256) > 128)
    {
        if(tData->page)
        {
            drawWsg(&self->gameData->assets[DN_MMM_SUBMENU_ASSET].frames[0], 10, 205, true, false, 0);
        }
        if(tData->page < sizeof(tutorialText[tData->advancedTips]) / sizeof(tutorialText[tData->advancedTips][0]))
        {
            drawWsgSimple(&self->gameData->assets[DN_MMM_SUBMENU_ASSET].frames[0], 242, 205);
        }
    }

}

dn_entity_t* dn_findLastEntityOfType(dn_entity_t* self, dn_dataType_t type)
{
    node_t* cur = self->gameData->entityManager.entities->last;
    while (((dn_entity_t*)cur->val)->dataType != type)
    {
        cur = cur->prev;
        if(!cur)
        {
            return NULL;
        }
    }
    return (dn_entity_t*)cur->val;
}

void dn_setupBounceOptions(dn_entity_t* self)
{
    dn_tileSelectorData_t* tData = (dn_tileSelectorData_t*)dn_findLastEntityOfType(self, DN_TILE_SELECTOR_DATA)->data;
    dn_boardData_t* bData        = (dn_boardData_t*)self->gameData->entityManager.board->data;

    dn_clearSelectableTiles(self);

    // recalculate selectable tiles
    list_t* myList = heap_caps_calloc(1, sizeof(list_t), MALLOC_CAP_8BIT);
    memset(myList, 0, sizeof(list_t));
    if (dn_availableMoves(tData->selectedUnit, myList))
    {
        node_t* cur = myList->first;
        while (cur != NULL)
        {
            dn_action_t* action                                      = ((dn_action_t*)cur->val);
            if(action->action == DN_RED_TRACK || action->action == DN_REMIX_TRACK)
            {
                bData->tiles[action->pos.y][action->pos.x].selectionType = self->gameData->resolvingRemix ? DN_REMIX_TRACK : DN_RED_TRACK;
            }
            cur                                                      = cur->next;
        }
    }
    clear(myList);
    free(myList);

    tData->a_callback   = dn_trySelectBounceDest;
    tData->b_callback   = dn_cancelSelectBounceDest;
}

void dn_trySelectBounceDest(dn_entity_t* self)
{
    dn_tileSelectorData_t* tData = (dn_tileSelectorData_t*)self->data;
    dn_boardData_t* bData        = (dn_boardData_t*)self->gameData->entityManager.board->data;
    if (bData->tiles[tData->pos.y][tData->pos.x].selectionType)
    {
        dn_clearSelectableTiles(self);

        // determine the vector FROM the unit TO the track
        dn_boardPos_t from = dn_getUnitBoardPos(tData->selectedUnit);
        // tData->pos is "to"
        dn_boardPos_t relativeTrack = (dn_boardPos_t){tData->pos.x - from.x, tData->pos.y - from.y};

        dn_entity_t* album = ((dn_albumsData_t*)self->gameData->entityManager.albums->data)->p1Album;
        // Make it relative to the player's facing direction
        if (self->gameData->phase < DN_P2_DANCE_PHASE)
        {
            relativeTrack.y *= -1;
        }
        else
        {
            relativeTrack.x *= -1;
            album = ((dn_albumsData_t*)self->gameData->entityManager.albums->data)->p2Album;
        }
        bData->tiles[tData->pos.y][tData->pos.x].selector = NULL;
        self->destroyFlag                                 = true;
        dn_track_t type                                   = dn_trackTypeAtCoords(album, relativeTrack);
        if (type == DN_REMIX_TRACK || type == DN_RED_TRACK)
        {
            bData->tiles[from.y][from.x].unit->paused = true;
            /////////////////////
            // Make the bullet //
            /////////////////////
            vec_t start = (vec_t){
                self->gameData->entityManager.board->pos.x
                    + (from.x - from.y) * (self->gameData->assets[DN_GROUND_TILE_ASSET].originX << DN_DECIMAL_BITS)
                    - (1 << DN_DECIMAL_BITS),
                self->gameData->entityManager.board->pos.y
                    + (from.x + from.y) * (self->gameData->assets[DN_GROUND_TILE_ASSET].originY << DN_DECIMAL_BITS)
                    - (bData->tiles[from.y][from.x].yOffset)};

            dn_entity_t* bullet = dn_createEntitySpecial(&self->gameData->entityManager, 0, DN_NO_ANIMATION, true,
                                                            DN_NO_ASSET, 0, start, self->gameData);
            bullet->updateFunction = dn_updateBullet;
            bullet->drawFunction   = dn_drawBullet;
            bullet->data           = heap_caps_calloc(1, sizeof(dn_bulletData_t), MALLOC_CAP_SPIRAM);
            memset(bullet->data, 0, sizeof(dn_bulletData_t));
            bullet->dataType                             = DN_BULLET_DATA;
            ((dn_bulletData_t*)bullet->data)->ownerToMove = from;
            ((dn_bulletData_t*)bullet->data)->firstTarget = tData->bounce;
            ((dn_bulletData_t*)bullet->data)->secondTarget = tData->pos;
            ((dn_bulletData_t*)bullet->data)->start      = bullet->pos;
            ((dn_bulletData_t*)bullet->data)->end
                = (vec_t){self->gameData->entityManager.board->pos.x
                                + (tData->bounce.x - tData->bounce.y)
                                    * (self->gameData->assets[DN_GROUND_TILE_ASSET].originX << DN_DECIMAL_BITS)
                                - (1 << DN_DECIMAL_BITS),
                            self->gameData->entityManager.board->pos.y
                                + (tData->bounce.x + tData->bounce.y)
                                    * (self->gameData->assets[DN_GROUND_TILE_ASSET].originY << DN_DECIMAL_BITS)
                                - (bData->tiles[tData->bounce.y][tData->bounce.x].yOffset)};
        }
    }
}

void dn_cancelSelectBounceDest(dn_entity_t* self)
{

}

void dn_exitSubMode(dn_entity_t* self)
{
    dn_freeAllAssets(self->gameData);
    dn_destroyAllEntities(&self->gameData->entityManager);
    dn_ShowUi(UI_MENU);
}
