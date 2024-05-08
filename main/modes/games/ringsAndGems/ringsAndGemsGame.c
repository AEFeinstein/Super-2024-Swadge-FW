#include "ringsAndGemsGame.h"

void ragPlacePiece(ringsAndGems_t* rag, const vec_t* subgame, const vec_t* cell, ragPiece_t piece);
static ragPiece_t checkWinner(ringsAndGems_t* rag);
static ragPiece_t checkSubgameWinner(ragSubgame_t* subgame);

/**
 * @brief TODO
 *
 * @param rag
 */
void ragBeginGame(ringsAndGems_t* rag)
{
    // If going first
    if (GOING_FIRST == p2pGetPlayOrder(&rag->p2p))
    {
        // Set own piece type
        rag->p1Piece = RAG_RING;

        // Send piece type to other swadge
        ragMsgSelectPiece_t sel = {
            .type  = MSG_SELECT_PIECE,
            .piece = rag->p1Piece,
        };
        p2pSendMsg(&rag->p2p, (const uint8_t*)&sel, sizeof(sel), ragMsgTxCbFn);
    }
    // If going second, wait to receive p1's piece before responding
}

/**
 * @brief TODO
 *
 * @param rag
 * @param evt
 */
void ragHandleGameInput(ringsAndGems_t* rag, buttonEvt_t* evt)
{
    // Do something?
    if (evt->down)
    {
        bool cursorMoved = false;
        switch (evt->button)
        {
            case PB_UP:
            {
                cursorMoved = true;
                if (SELECT_SUBGAME == rag->cursorMode)
                {
                    do
                    {
                        if (0 == rag->cursor.y)
                        {
                            rag->cursor.y = 2;
                        }
                        else
                        {
                            rag->cursor.y--;
                        }
                    } while (RAG_EMPTY != rag->subgames[rag->cursor.x][rag->cursor.y].winner);
                }
                else if ((SELECT_CELL == rag->cursorMode) || (SELECT_CELL_LOCKED == rag->cursorMode))
                {
                    do
                    {
                        if (0 == rag->cursor.y)
                        {
                            rag->cursor.y = 2;
                        }
                        else
                        {
                            rag->cursor.y--;
                        }
                    } while (RAG_EMPTY
                             != rag->subgames[rag->selectedSubgame.x][rag->selectedSubgame.y]
                                    .game[rag->cursor.x][rag->cursor.y]);
                }
                break;
            }
            case PB_DOWN:
            {
                cursorMoved = true;
                if (SELECT_SUBGAME == rag->cursorMode)
                {
                    do
                    {
                        rag->cursor.y = (rag->cursor.y + 1) % 3;
                    } while (RAG_EMPTY != rag->subgames[rag->cursor.x][rag->cursor.y].winner);
                }
                else if ((SELECT_CELL == rag->cursorMode) || (SELECT_CELL_LOCKED == rag->cursorMode))
                {
                    do
                    {
                        rag->cursor.y = (rag->cursor.y + 1) % 3;
                    } while (RAG_EMPTY
                             != rag->subgames[rag->selectedSubgame.x][rag->selectedSubgame.y]
                                    .game[rag->cursor.x][rag->cursor.y]);
                }
                break;
            }
            case PB_LEFT:
            {
                cursorMoved = true;
                if (SELECT_SUBGAME == rag->cursorMode)
                {
                    do
                    {
                        if (0 == rag->cursor.x)
                        {
                            rag->cursor.x = 2;
                        }
                        else
                        {
                            rag->cursor.x--;
                        }
                    } while (RAG_EMPTY != rag->subgames[rag->cursor.x][rag->cursor.y].winner);
                }
                else if ((SELECT_CELL == rag->cursorMode) || (SELECT_CELL_LOCKED == rag->cursorMode))
                {
                    do
                    {
                        if (0 == rag->cursor.x)
                        {
                            rag->cursor.x = 2;
                        }
                        else
                        {
                            rag->cursor.x--;
                        }
                    } while (RAG_EMPTY
                             != rag->subgames[rag->selectedSubgame.x][rag->selectedSubgame.y]
                                    .game[rag->cursor.x][rag->cursor.y]);
                }
                break;
            }
            case PB_RIGHT:
            {
                cursorMoved = true;
                if (SELECT_SUBGAME == rag->cursorMode)
                {
                    do
                    {
                        rag->cursor.x = (rag->cursor.x + 1) % 3;
                    } while (RAG_EMPTY != rag->subgames[rag->cursor.x][rag->cursor.y].winner);
                }
                else if ((SELECT_CELL == rag->cursorMode) || (SELECT_CELL_LOCKED == rag->cursorMode))
                {
                    do
                    {
                        rag->cursor.x = (rag->cursor.x + 1) % 3;
                    } while (RAG_EMPTY
                             != rag->subgames[rag->selectedSubgame.x][rag->selectedSubgame.y]
                                    .game[rag->cursor.x][rag->cursor.y]);
                }
                break;
            }
            case PB_A:
            {
                if (SELECT_SUBGAME == rag->cursorMode)
                {
                    cursorMoved          = true;
                    rag->selectedSubgame = rag->cursor;
                    rag->cursor.x        = 1;
                    rag->cursor.y        = 1;
                    rag->cursorMode      = SELECT_CELL;
                }
                else if ((SELECT_CELL == rag->cursorMode) || (SELECT_CELL_LOCKED == rag->cursorMode))
                {
                    // Send move to the other swadge
                    ragSendPlacedPiece(rag);

                    // Place the piece
                    ragPlacePiece(rag, &rag->selectedSubgame, &rag->cursor,
                                  (GOING_FIRST == p2pGetPlayOrder(&rag->p2p)) ? rag->p1Piece : rag->p2Piece);

                    // Switch to waiting
                    rag->state = RGS_WAITING;
                }
                break;
            }
            case PB_B:
            {
                if (SELECT_CELL == rag->cursorMode)
                {
                    cursorMoved     = true;
                    rag->cursor     = rag->selectedSubgame;
                    rag->cursorMode = SELECT_SUBGAME;
                }
                break;
            }
            default:
            {
                break;
            }
        }

        // Send cursor movement to the other Swadge
        if (cursorMoved)
        {
            ragSendCursor(rag);
        }
    }
}

/**
 * @brief TODO
 *
 * @param rag
 */
void ragSendCursor(ringsAndGems_t* rag)
{
    // Send cursor type to other swadge
    ragMsgMoveCursor_t move = {
        .type            = MSG_MOVE_CURSOR,
        .cursorMode      = rag->cursorMode,
        .selectedSubgame = rag->selectedSubgame,
        .cursor          = rag->cursor,
    };
    p2pSendMsg(&rag->p2p, (const uint8_t*)&move, sizeof(move), ragMsgTxCbFn);
}

/**
 * @brief TODO
 *
 * @param rag
 * @param msg
 */
void ragReceiveCursor(ringsAndGems_t* rag, const ragMsgMoveCursor_t* msg)
{
    // Move the cursor
    rag->cursorMode      = msg->cursorMode;
    rag->selectedSubgame = msg->selectedSubgame;
    rag->cursor          = msg->cursor;
}

/**
 * @brief TODO
 *
 * @param rag
 * @param subgame
 * @param cell
 * @param piece
 */
void ragPlacePiece(ringsAndGems_t* rag, const vec_t* subgame, const vec_t* cell, ragPiece_t piece)
{
    // Place the piece
    rag->subgames[subgame->x][subgame->y].game[cell->x][cell->y] = piece;

    // Check the board
    checkWinner(rag);

    // Next move should be in this cell
    rag->selectedSubgame = *cell;
    rag->cursor.x        = 1;
    rag->cursor.y        = 1;
    rag->cursorMode      = SELECT_CELL_LOCKED;

    // If that subgame is already won
    if (RAG_EMPTY != rag->subgames[rag->selectedSubgame.x][rag->selectedSubgame.y].winner)
    {
        // Find the next one
        for (int16_t y = 0; y < 3; y++)
        {
            for (int16_t x = 0; x < 3; x++)
            {
                if (RAG_EMPTY == rag->subgames[x][y].winner)
                {
                    rag->cursor.x   = x;
                    rag->cursor.y   = y;
                    rag->cursorMode = SELECT_SUBGAME;
                    break;
                }
            }
            if (SELECT_SUBGAME == rag->cursorMode)
            {
                break;
            }
        }
    }
}

/**
 * @brief TODO
 *
 * @param rag
 */
void ragSendPlacedPiece(ringsAndGems_t* rag)
{
    // Send move to the other swadge
    ragMsgPlacePiece_t place = {
        .type            = MSG_PLACE_PIECE,
        .selectedSubgame = rag->selectedSubgame,
        .selectedCell    = rag->cursor,
    };
    p2pSendMsg(&rag->p2p, (const uint8_t*)&place, sizeof(place), ragMsgTxCbFn);

    rag->state = RGS_WAITING;
}

/**
 * @brief TODO
 *
 * @param rag
 * @param msg
 */
void ragReceivePlacedPiece(ringsAndGems_t* rag, const ragMsgPlacePiece_t* msg)
{
    // Place the piece
    ragPlacePiece(rag, &msg->selectedSubgame, &msg->selectedCell,
                  (GOING_FIRST == p2pGetPlayOrder(&rag->p2p)) ? rag->p2Piece : rag->p1Piece);

    // Transition state to placing a piece
    rag->state = RGS_PLACING_PIECE;
}

/**
 * @brief TODO
 *
 * @return ragPiece_t
 */
static ragPiece_t checkWinner(ringsAndGems_t* rag)
{
    // Check all the subgames
    for (uint16_t y = 0; y < 3; y++)
    {
        for (uint16_t x = 0; x < 3; x++)
        {
            checkSubgameWinner(&rag->subgames[x][y]);
        }
    }

    ragPiece_t winner = RAG_EMPTY;
    for (uint16_t i = 0; i < 3; i++)
    {
        // Check horizontals
        if (rag->subgames[i][0].winner == rag->subgames[i][1].winner
            && rag->subgames[i][1].winner == rag->subgames[i][2].winner)
        {
            if (RAG_EMPTY != rag->subgames[i][0].winner)
            {
                winner = rag->subgames[i][0].winner;
            }
        }

        // Check verticals
        if (rag->subgames[0][i].winner == rag->subgames[1][i].winner
            && rag->subgames[1][i].winner == rag->subgames[2][i].winner)
        {
            if (RAG_EMPTY != rag->subgames[0][i].winner)
            {
                winner = rag->subgames[0][i].winner;
            }
        }
    }

    // Check diagonals
    if (rag->subgames[0][0].winner == rag->subgames[1][1].winner
        && rag->subgames[1][1].winner == rag->subgames[2][2].winner)
    {
        if (RAG_EMPTY != rag->subgames[0][0].winner)
        {
            winner = rag->subgames[0][0].winner;
        }
    }
    else if (rag->subgames[2][0].winner == rag->subgames[1][1].winner
             && rag->subgames[1][1].winner == rag->subgames[0][2].winner)
    {
        if (RAG_EMPTY != rag->subgames[2][0].winner)
        {
            winner = rag->subgames[2][0].winner;
        }
    }
    return winner;
}

/**
 * @brief TODO
 *
 * @param subgame
 * @return ragPiece_t
 */
static ragPiece_t checkSubgameWinner(ragSubgame_t* subgame)
{
    if (RAG_EMPTY == subgame->winner)
    {
        for (uint16_t i = 0; i < 3; i++)
        {
            // Check horizontals
            if (subgame->game[i][0] == subgame->game[i][1] && subgame->game[i][1] == subgame->game[i][2])
            {
                if (RAG_EMPTY != subgame->game[i][0])
                {
                    subgame->winner = subgame->game[i][0];
                    return subgame->game[i][0];
                }
            }

            // Check verticals
            if (subgame->game[0][i] == subgame->game[1][i] && subgame->game[1][i] == subgame->game[2][i])
            {
                if (RAG_EMPTY != subgame->game[0][i])
                {
                    subgame->winner = subgame->game[0][i];
                    return subgame->game[0][i];
                }
            }
        }

        // Check diagonals
        if (subgame->game[0][0] == subgame->game[1][1] && subgame->game[1][1] == subgame->game[2][2])
        {
            if (RAG_EMPTY != subgame->game[0][0])
            {
                subgame->winner = subgame->game[0][0];
                return subgame->game[0][0];
            }
        }
        else if (subgame->game[2][0] == subgame->game[1][1] && subgame->game[1][1] == subgame->game[0][2])
        {
            if (RAG_EMPTY != subgame->game[2][0])
            {
                subgame->winner = subgame->game[2][0];
                return subgame->game[2][0];
            }
        }
    }
    else
    {
        return subgame->winner;
    }
    return RAG_EMPTY;
}

///////////////////////////////////////////////////////////////////////////////

/**
 * @brief TODO
 *
 */
void ragDrawGame(ringsAndGems_t* rag)
{
    // Clear before drawing
    clearPxTft();

    // Calculate the game size based on the largest possible cell size
    int16_t gameSize    = MIN(TFT_WIDTH, TFT_HEIGHT);
    int16_t cellSize    = gameSize / 9;
    int16_t subgameSize = cellSize * 3;
    gameSize            = cellSize * 9;

    // Center the game on the screen
    int16_t gameOffsetX = (TFT_WIDTH - gameSize) / 2;
    int16_t gameOffsetY = (TFT_HEIGHT - gameSize) / 2;

    // Draw the main gridlines
    ragDrawGrid(gameOffsetX, gameOffsetY, gameOffsetX + gameSize - 1, gameOffsetY + gameSize - 1, 0, c010);

    // For each subgame
    for (int subY = 0; subY < 3; subY++)
    {
        for (int subX = 0; subX < 3; subX++)
        {
            // Get this subgame's rectangle
            int16_t sX0 = gameOffsetX + (subX * subgameSize);
            int16_t sY0 = gameOffsetY + (subY * subgameSize);
            int16_t sX1 = sX0 + subgameSize - 1;
            int16_t sY1 = sY0 + subgameSize - 1;

            // Draw the subgame grid lines
            ragDrawGrid(sX0, sY0, sX1, sY1, 4, c020);

            // If selected, draw the cursor on this subgame
            if (SELECT_SUBGAME == rag->cursorMode && //
                rag->cursor.x == subX && rag->cursor.y == subY)
            {
                paletteColor_t color = (GOING_FIRST == p2pGetPlayOrder(&rag->p2p)) ? c500 : c005;

                if (rag->state == RGS_WAITING)
                {
                    color = c111;
                }
                drawRect(sX0, sY0, sX1, sY1, color);
            }

            // Check if the subgame has a winner
            switch (rag->subgames[subX][subY].winner)
            {
                case RAG_RING:
                {
                    // Draw big winner sprite
                    drawWsgSimple(&rag->piece_x_big, sX0, sY0);
                    break;
                }
                case RAG_GEM:
                {
                    // Draw big winner sprite
                    drawWsgSimple(&rag->piece_o_big, sX0, sY0);
                    break;
                }
                default:
                case RAG_EMPTY:
                {
                    // Draw the subgame. For each cell
                    for (int cellY = 0; cellY < 3; cellY++)
                    {
                        for (int cellX = 0; cellX < 3; cellX++)
                        {
                            // Get the location for this cell
                            int16_t cX0 = sX0 + (cellX * cellSize);
                            int16_t cY0 = sY0 + (cellY * cellSize);

                            // Draw sprites
                            switch (rag->subgames[subX][subY].game[cellX][cellY])
                            {
                                default:
                                case RAG_EMPTY:
                                {
                                    break;
                                }
                                case RAG_RING:
                                {
                                    drawWsgSimple(&rag->piece_x_small, cX0, cY0);
                                    break;
                                }
                                case RAG_GEM:
                                {
                                    drawWsgSimple(&rag->piece_o_small, cX0, cY0);
                                    break;
                                }
                            }

                            // If selected, draw the cursor on this cell
                            if ((SELECT_CELL == rag->cursorMode || SELECT_CELL_LOCKED == rag->cursorMode) && //
                                rag->selectedSubgame.x == subX && rag->selectedSubgame.y == subY &&          //
                                rag->cursor.x == cellX && rag->cursor.y == cellY)
                            {
                                // Get the other rectangle coordinates
                                int16_t cX1 = cX0 + cellSize - 1;
                                int16_t cY1 = cY0 + cellSize - 1;
                                // Draw the cursor
                                paletteColor_t color = (GOING_FIRST == p2pGetPlayOrder(&rag->p2p)) ? c500 : c005;
                                if (rag->state == RGS_WAITING)
                                {
                                    color = c111;
                                }
                                drawRect(cX0, cY0, cX1, cY1, color);
                            }
                        }
                    }
                    break;
                }
            }
        }
    }
}

/**
 * @brief TODO
 *
 * @param x0
 * @param y0
 * @param x1
 * @param y1
 * @param m
 * @param color
 */
void ragDrawGrid(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t m, paletteColor_t color)
{
    int16_t cellWidth  = (x1 - x0) / 3;
    int16_t cellHeight = (y1 - y0) / 3;

    // Horizontal lines
    drawLineFast(x0 + m, y0 + cellHeight, //
                 x1 - 1 - m, y0 + cellHeight, color);
    drawLineFast(x0 + m, y0 + (2 * cellHeight) + 1, //
                 x1 - 1 - m, y0 + (2 * cellHeight) + 1, color);

    // Vertical lines
    drawLineFast(x0 + cellWidth, y0 + m, //
                 x0 + cellWidth, y1 - 1 - m, color);
    drawLineFast(x0 + (2 * cellWidth) + 1, y0 + m, //
                 x0 + (2 * cellWidth) + 1, y1 - 1 - m, color);
}
