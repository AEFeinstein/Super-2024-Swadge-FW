#include "ultimateTTTgame.h"

void tttPlacePiece(ultimateTTT_t* ttt, const vec_t* subgame, const vec_t* cell, tttPiece_t piece);
static tttPiece_t checkWinner(ultimateTTT_t* ttt);
static tttPiece_t checkSubgameWinner(tttSubgame_t* subgame);

/**
 * @brief TODO
 *
 * @param ttt
 */
void tttBeginGame(ultimateTTT_t* ttt)
{
    // If going first
    if (GOING_FIRST == p2pGetPlayOrder(&ttt->p2p))
    {
        // Set own piece type
        ttt->p1Piece = TTT_RING;

        // Send piece type to other swadge
        tttMsgSelectPiece_t sel = {
            .type  = MSG_SELECT_PIECE,
            .piece = ttt->p1Piece,
        };
        p2pSendMsg(&ttt->p2p, (const uint8_t*)&sel, sizeof(sel), tttMsgTxCbFn);
    }
    // If going second, wait to receive p1's piece before responding
}

typedef void (*cursorFunc_t)(ultimateTTT_t* ttt);

static void incCursorX(ultimateTTT_t* ttt)
{
    ttt->cursor.x = (ttt->cursor.x + 1) % 3;
}

static void decCursorX(ultimateTTT_t* ttt)
{
    if (0 == ttt->cursor.x)
    {
        ttt->cursor.x = 2;
    }
    else
    {
        ttt->cursor.x--;
    }
}

static void incCursorY(ultimateTTT_t* ttt)
{
    ttt->cursor.y = (ttt->cursor.y + 1) % 3;
}

static void decCursorY(ultimateTTT_t* ttt)
{
    if (0 == ttt->cursor.y)
    {
        ttt->cursor.y = 2;
    }
    else
    {
        ttt->cursor.y--;
    }
}

static bool cursorIsValid(ultimateTTT_t* ttt)
{
    switch (ttt->cursorMode)
    {
        case NO_CURSOR:
        default:
        {
            return false;
        }
        case SELECT_SUBGAME:
        {
            return TTT_EMPTY == ttt->subgames[ttt->cursor.x][ttt->cursor.y].winner;
        }
        case SELECT_CELL:
        case SELECT_CELL_LOCKED:
        {
            return TTT_EMPTY
                   == ttt->subgames[ttt->selectedSubgame.x][ttt->selectedSubgame.y].game[ttt->cursor.x][ttt->cursor.y];
        }
    }
}

/**
 * @brief TODO
 *
 * @param ttt
 * @param evt
 */
void tttHandleGameInput(ultimateTTT_t* ttt, buttonEvt_t* evt)
{
    // Do something?
    if (evt->down)
    {
        bool cursorMoved                 = false;
        cursorFunc_t cursorFunc          = NULL;
        cursorFunc_t cursorFuncSecondary = NULL;
        switch (evt->button)
        {
            case PB_UP:
            {
                cursorFunc          = decCursorY;
                cursorFuncSecondary = incCursorX;
                break;
            }
            case PB_DOWN:
            {
                cursorFunc          = incCursorY;
                cursorFuncSecondary = incCursorX;
                break;
            }
            case PB_LEFT:
            {
                cursorFunc          = decCursorX;
                cursorFuncSecondary = incCursorY;
                break;
            }
            case PB_RIGHT:
            {
                cursorFunc          = incCursorX;
                cursorFuncSecondary = incCursorY;
                break;
            }
            case PB_A:
            {
                if (SELECT_SUBGAME == ttt->cursorMode)
                {
                    cursorMoved          = true;
                    ttt->selectedSubgame = ttt->cursor;
                    ttt->cursorMode      = SELECT_CELL;

                    // Place the cursor on a valid cell
                    ttt->cursor.x = 1;
                    ttt->cursor.y = 1;
                    for (int16_t y = 0; y < 3; y++)
                    {
                        for (int16_t x = 0; x < 3; x++)
                        {
                            if (!cursorIsValid(ttt))
                            {
                                incCursorX(ttt);
                            }
                            else
                            {
                                break;
                            }
                        }

                        if (!cursorIsValid(ttt))
                        {
                            incCursorY(ttt);
                        }
                        else
                        {
                            break;
                        }
                    }
                }
                else if ((SELECT_CELL == ttt->cursorMode) || (SELECT_CELL_LOCKED == ttt->cursorMode))
                {
                    // Send move to the other swadge
                    tttSendPlacedPiece(ttt);

                    // Place the piece
                    tttPlacePiece(ttt, &ttt->selectedSubgame, &ttt->cursor,
                                  (GOING_FIRST == p2pGetPlayOrder(&ttt->p2p)) ? ttt->p1Piece : ttt->p2Piece);

                    // Switch to waiting
                    ttt->state = TGS_WAITING;
                }
                break;
            }
            case PB_B:
            {
                if (SELECT_CELL == ttt->cursorMode)
                {
                    cursorMoved     = true;
                    ttt->cursor     = ttt->selectedSubgame;
                    ttt->cursorMode = SELECT_SUBGAME;
                }
                break;
            }
            default:
            {
                break;
            }
        }

        // If the cursor should move
        if (NULL != cursorFunc)
        {
            cursorMoved = true;

            // First check along the primary axis
            bool cursorIsSet = false;
            for (int16_t a = 0; a < 2; a++)
            {
                cursorFunc(ttt);
                if (cursorIsValid(ttt))
                {
                    cursorIsSet = true;
                    break;
                }
            }

            // If the primary axis is filled
            if (!cursorIsSet)
            {
                // Move back to where we started
                cursorFunc(ttt);

                // Move along the primary axis
                for (int16_t b = 0; b < 3; b++)
                {
                    cursorFunc(ttt);

                    // Check perpendicular spaces
                    for (int16_t a = 0; a < 3; a++)
                    {
                        cursorFuncSecondary(ttt);

                        // If it's valid
                        if (cursorIsValid(ttt))
                        {
                            // Mark and break
                            cursorIsSet = true;
                            break;
                        }
                    }

                    // If the cursor is valid
                    if (cursorIsSet)
                    {
                        break;
                    }
                }
            }
        }

        // Send cursor movement to the other Swadge
        if (cursorMoved)
        {
            tttSendCursor(ttt);
        }
    }
}

/**
 * @brief TODO
 *
 * @param ttt
 */
void tttSendCursor(ultimateTTT_t* ttt)
{
    // Send cursor type to other swadge
    tttMsgMoveCursor_t move = {
        .type            = MSG_MOVE_CURSOR,
        .cursorMode      = ttt->cursorMode,
        .selectedSubgame = ttt->selectedSubgame,
        .cursor          = ttt->cursor,
    };
    p2pSendMsg(&ttt->p2p, (const uint8_t*)&move, sizeof(move), tttMsgTxCbFn);
}

/**
 * @brief TODO
 *
 * @param ttt
 * @param msg
 */
void tttReceiveCursor(ultimateTTT_t* ttt, const tttMsgMoveCursor_t* msg)
{
    // Move the cursor
    ttt->cursorMode      = msg->cursorMode;
    ttt->selectedSubgame = msg->selectedSubgame;
    ttt->cursor          = msg->cursor;
}

/**
 * @brief TODO
 *
 * @param ttt
 * @param subgame
 * @param cell
 * @param piece
 */
void tttPlacePiece(ultimateTTT_t* ttt, const vec_t* subgame, const vec_t* cell, tttPiece_t piece)
{
    // Place the piece
    ttt->subgames[subgame->x][subgame->y].game[cell->x][cell->y] = piece;

    // Check the board
    checkWinner(ttt);

    // Next move should be in this cell
    ttt->selectedSubgame = *cell;
    ttt->cursorMode      = SELECT_CELL_LOCKED;

    ttt->cursor.x = 1;
    ttt->cursor.y = 1;
    for (int16_t y = 0; y < 3; y++)
    {
        for (int16_t x = 0; x < 3; x++)
        {
            if (!cursorIsValid(ttt))
            {
                incCursorX(ttt);
            }
            else
            {
                break;
            }
        }

        if (!cursorIsValid(ttt))
        {
            incCursorY(ttt);
        }
        else
        {
            break;
        }
    }

    // If that subgame is already won
    if (TTT_EMPTY != ttt->subgames[ttt->selectedSubgame.x][ttt->selectedSubgame.y].winner)
    {
        // Find the next one
        for (int16_t y = 0; y < 3; y++)
        {
            for (int16_t x = 0; x < 3; x++)
            {
                if (TTT_EMPTY == ttt->subgames[x][y].winner)
                {
                    ttt->cursor.x   = x;
                    ttt->cursor.y   = y;
                    ttt->cursorMode = SELECT_SUBGAME;
                    break;
                }
            }
            if (SELECT_SUBGAME == ttt->cursorMode)
            {
                break;
            }
        }
    }
}

/**
 * @brief TODO
 *
 * @param ttt
 */
void tttSendPlacedPiece(ultimateTTT_t* ttt)
{
    // Send move to the other swadge
    tttMsgPlacePiece_t place = {
        .type            = MSG_PLACE_PIECE,
        .selectedSubgame = ttt->selectedSubgame,
        .selectedCell    = ttt->cursor,
    };
    p2pSendMsg(&ttt->p2p, (const uint8_t*)&place, sizeof(place), tttMsgTxCbFn);

    ttt->state = TGS_WAITING;
}

/**
 * @brief TODO
 *
 * @param ttt
 * @param msg
 */
void tttReceivePlacedPiece(ultimateTTT_t* ttt, const tttMsgPlacePiece_t* msg)
{
    // Place the piece
    tttPlacePiece(ttt, &msg->selectedSubgame, &msg->selectedCell,
                  (GOING_FIRST == p2pGetPlayOrder(&ttt->p2p)) ? ttt->p2Piece : ttt->p1Piece);

    // Transition state to placing a piece
    ttt->state = TGS_PLACING_PIECE;
}

/**
 * @brief TODO
 *
 * @return tttPiece_t
 */
static tttPiece_t checkWinner(ultimateTTT_t* ttt)
{
    // Check all the subgames
    for (uint16_t y = 0; y < 3; y++)
    {
        for (uint16_t x = 0; x < 3; x++)
        {
            checkSubgameWinner(&ttt->subgames[x][y]);
        }
    }

    tttPiece_t winner = TTT_EMPTY;
    for (uint16_t i = 0; i < 3; i++)
    {
        // Check horizontals
        if (ttt->subgames[i][0].winner == ttt->subgames[i][1].winner
            && ttt->subgames[i][1].winner == ttt->subgames[i][2].winner)
        {
            if (TTT_EMPTY != ttt->subgames[i][0].winner)
            {
                winner = ttt->subgames[i][0].winner;
            }
        }

        // Check verticals
        if (ttt->subgames[0][i].winner == ttt->subgames[1][i].winner
            && ttt->subgames[1][i].winner == ttt->subgames[2][i].winner)
        {
            if (TTT_EMPTY != ttt->subgames[0][i].winner)
            {
                winner = ttt->subgames[0][i].winner;
            }
        }
    }

    // Check diagonals
    if (ttt->subgames[0][0].winner == ttt->subgames[1][1].winner
        && ttt->subgames[1][1].winner == ttt->subgames[2][2].winner)
    {
        if (TTT_EMPTY != ttt->subgames[0][0].winner)
        {
            winner = ttt->subgames[0][0].winner;
        }
    }
    else if (ttt->subgames[2][0].winner == ttt->subgames[1][1].winner
             && ttt->subgames[1][1].winner == ttt->subgames[0][2].winner)
    {
        if (TTT_EMPTY != ttt->subgames[2][0].winner)
        {
            winner = ttt->subgames[2][0].winner;
        }
    }
    return winner;
}

/**
 * @brief TODO
 *
 * @param subgame
 * @return tttPiece_t
 */
static tttPiece_t checkSubgameWinner(tttSubgame_t* subgame)
{
    if (TTT_EMPTY == subgame->winner)
    {
        for (uint16_t i = 0; i < 3; i++)
        {
            // Check horizontals
            if (subgame->game[i][0] == subgame->game[i][1] && subgame->game[i][1] == subgame->game[i][2])
            {
                if (TTT_EMPTY != subgame->game[i][0])
                {
                    subgame->winner = subgame->game[i][0];
                    return subgame->game[i][0];
                }
            }

            // Check verticals
            if (subgame->game[0][i] == subgame->game[1][i] && subgame->game[1][i] == subgame->game[2][i])
            {
                if (TTT_EMPTY != subgame->game[0][i])
                {
                    subgame->winner = subgame->game[0][i];
                    return subgame->game[0][i];
                }
            }
        }

        // Check diagonals
        if (subgame->game[0][0] == subgame->game[1][1] && subgame->game[1][1] == subgame->game[2][2])
        {
            if (TTT_EMPTY != subgame->game[0][0])
            {
                subgame->winner = subgame->game[0][0];
                return subgame->game[0][0];
            }
        }
        else if (subgame->game[2][0] == subgame->game[1][1] && subgame->game[1][1] == subgame->game[0][2])
        {
            if (TTT_EMPTY != subgame->game[2][0])
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
    return TTT_EMPTY;
}

///////////////////////////////////////////////////////////////////////////////

/**
 * @brief TODO
 *
 */
void tttDrawGame(ultimateTTT_t* ttt)
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
    tttDrawGrid(gameOffsetX, gameOffsetY, gameOffsetX + gameSize - 1, gameOffsetY + gameSize - 1, 0, c010);

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
            tttDrawGrid(sX0, sY0, sX1, sY1, 4, c020);

            // If selected, draw the cursor on this subgame
            if (SELECT_SUBGAME == ttt->cursorMode && //
                ttt->cursor.x == subX && ttt->cursor.y == subY)
            {
                paletteColor_t color = (GOING_FIRST == p2pGetPlayOrder(&ttt->p2p)) ? c500 : c005;

                if (ttt->state == TGS_WAITING)
                {
                    color = c222;
                }
                for (int16_t i = 0; i < 4; i++)
                {
                    drawRect(sX0 + i, sY0 + i, sX1 - i, sY1 - i, color);
                }
            }

            // Check if the subgame has a winner
            switch (ttt->subgames[subX][subY].winner)
            {
                case TTT_RING:
                {
                    // Draw big winner sprite
                    drawWsgSimple(&ttt->piece_x_big, sX0, sY0);
                    break;
                }
                case TTT_GEM:
                {
                    // Draw big winner sprite
                    drawWsgSimple(&ttt->piece_o_big, sX0, sY0);
                    break;
                }
                default:
                case TTT_EMPTY:
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
                            switch (ttt->subgames[subX][subY].game[cellX][cellY])
                            {
                                default:
                                case TTT_EMPTY:
                                {
                                    break;
                                }
                                case TTT_RING:
                                {
                                    drawWsgSimple(&ttt->piece_x_small, cX0, cY0);
                                    break;
                                }
                                case TTT_GEM:
                                {
                                    drawWsgSimple(&ttt->piece_o_small, cX0, cY0);
                                    break;
                                }
                            }

                            // If selected, draw the cursor on this cell
                            if ((SELECT_CELL == ttt->cursorMode || SELECT_CELL_LOCKED == ttt->cursorMode) && //
                                ttt->selectedSubgame.x == subX && ttt->selectedSubgame.y == subY &&          //
                                ttt->cursor.x == cellX && ttt->cursor.y == cellY)
                            {
                                // Get the other rectangle coordinates
                                int16_t cX1 = cX0 + cellSize - 1;
                                int16_t cY1 = cY0 + cellSize - 1;
                                // Draw the cursor
                                paletteColor_t color = (GOING_FIRST == p2pGetPlayOrder(&ttt->p2p)) ? c500 : c005;
                                if (ttt->state == TGS_WAITING)
                                {
                                    color = c222;
                                }

                                for (uint16_t i = 0; i < 4; i++)
                                {
                                    drawRect(cX0 + i, cY0 + i, cX1 - i, cY1 - i, color);
                                }
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
void tttDrawGrid(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t m, paletteColor_t color)
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
