//==============================================================================
// Includes
//==============================================================================

#include "ultimateTTTgame.h"

//==============================================================================
// Defines
//==============================================================================

#define P1_COLOR             c500
#define P2_COLOR             c005
#define MAIN_GRID_COLOR      c010
#define SUB_GRID_COLOR       c020
#define WAITING_PLAYER_COLOR c222

#define CURSOR_STROKE 4

//==============================================================================
// Typedefs
//==============================================================================

typedef void (*cursorFunc_t)(ultimateTTT_t* ttt);

//==============================================================================
// Function Declarations
//==============================================================================

static void tttPlaceMarker(ultimateTTT_t* ttt, const vec_t* subgame, const vec_t* cell, tttPlayer_t marker);
static tttPlayer_t checkWinner(ultimateTTT_t* ttt);
static tttPlayer_t checkSubgameWinner(tttSubgame_t* subgame);
static wsg_t* getMarkerWsg(ultimateTTT_t* ttt, tttPlayer_t p, bool isBig);
static playOrder_t tttGetPlayOrder(ultimateTTT_t* ttt);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Start a multiplayer game. This is called after a connection is established
 *
 * @param ttt The entire game state
 */
void tttBeginGame(ultimateTTT_t* ttt)
{
    // Set the state as not playing yet
    ttt->game.state = TGS_NOT_PLAYING;

    // Reset the board
    for (int16_t y = 0; y < 3; y++)
    {
        for (int16_t x = 0; x < 3; x++)
        {
            ttt->game.subgames[x][y].winner = TTT_NONE;
            for (int16_t sy = 0; sy < 3; sy++)
            {
                for (int16_t sx = 0; sx < 3; sx++)
                {
                    ttt->game.subgames[x][y].game[sx][sy] = TTT_NONE;
                }
            }
        }
    }

    // Reset the cursor
    ttt->game.cursor.x          = 0;
    ttt->game.cursor.y          = 0;
    ttt->game.selectedSubgame.x = 0;
    ttt->game.selectedSubgame.y = 0;
    ttt->game.cursorMode        = SELECT_SUBGAME;

    // Default indices
    ttt->game.p1MarkerIdx = 0;
    ttt->game.p2MarkerIdx = 0;

    // Clean up after showing instructions
    ttt->showingInstructions = false;

    // Show the game UI
    tttShowUi(TUI_GAME);

    // Set the cursor mode

    // If going first
    if (GOING_FIRST == tttGetPlayOrder(ttt))
    {
        // Set own marker type
        ttt->game.p1MarkerIdx = ttt->activeMarkerIdx;
        // Send it to the second player
        tttSendMarker(ttt, ttt->game.p1MarkerIdx);
    }
    // If going second, wait to receive p1's marker before responding
}

/**
 * @brief Helper function to increment the cursor along the X axis
 *
 * @param ttt The entire game state
 */
static void incCursorX(ultimateTTT_t* ttt)
{
    ttt->game.cursor.x = (ttt->game.cursor.x + 1) % 3;
}

/**
 * @brief Helper function to decrement the cursor along the X axis
 *
 * @param ttt The entire game state
 */
static void decCursorX(ultimateTTT_t* ttt)
{
    if (0 == ttt->game.cursor.x)
    {
        ttt->game.cursor.x = 2;
    }
    else
    {
        ttt->game.cursor.x--;
    }
}

/**
 * @brief Helper function to increment the cursor along the Y axis
 *
 * @param ttt The entire game state
 */
static void incCursorY(ultimateTTT_t* ttt)
{
    ttt->game.cursor.y = (ttt->game.cursor.y + 1) % 3;
}

/**
 * @brief Helper function to decrement the cursor along the Y axis
 *
 * @param ttt The entire game state
 */
static void decCursorY(ultimateTTT_t* ttt)
{
    if (0 == ttt->game.cursor.y)
    {
        ttt->game.cursor.y = 2;
    }
    else
    {
        ttt->game.cursor.y--;
    }
}

/**
 * @brief Check if the cursor is on a valid subgame (i.e not won) or valid cell (empty)
 *
 * @param ttt The entire game state
 * @return true if the cursor is on a valid subgame or cell, false otherwise
 */
static bool cursorIsValid(ultimateTTT_t* ttt)
{
    switch (ttt->game.cursorMode)
    {
        case NO_CURSOR:
        default:
        {
            // This is never valid
            return false;
        }
        case SELECT_SUBGAME:
        {
            // Subgames are valid if there is no winner
            return TTT_NONE == ttt->game.subgames[ttt->game.cursor.x][ttt->game.cursor.y].winner;
        }
        case SELECT_CELL:
        case SELECT_CELL_LOCKED:
        {
            // Cells are valid if there is no marker
            return TTT_NONE
                   == ttt->game.subgames[ttt->game.selectedSubgame.x][ttt->game.selectedSubgame.y]
                          .game[ttt->game.cursor.x][ttt->game.cursor.y];
        }
    }
}

/**
 * @brief Handle button input when showing the game UI
 *
 * @param ttt The entire game state
 * @param evt The button event
 */
void tttHandleGameInput(ultimateTTT_t* ttt, buttonEvt_t* evt)
{
    // Return if not placing a marker
    if (TGS_PLACING_MARKER != ttt->game.state)
    {
        return;
    }

    // If the button was pressed
    if (evt->down)
    {
        // Declare function pointers for cursor movement
        bool cursorMoved                 = false;
        cursorFunc_t cursorFunc          = NULL;
        cursorFunc_t cursorFuncSecondary = NULL;

        // Assign function pointers based on the button press
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
                // If a subgame is being selected
                if (SELECT_SUBGAME == ttt->game.cursorMode)
                {
                    // Set the cursor in the subgame
                    cursorMoved               = true;
                    ttt->game.selectedSubgame = ttt->game.cursor;
                    ttt->game.cursorMode      = SELECT_CELL;

                    // Place the cursor on a valid cell
                    ttt->game.cursor.x = 1;
                    ttt->game.cursor.y = 1;
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
                // If a cell is being selected
                else if ((SELECT_CELL == ttt->game.cursorMode) || (SELECT_CELL_LOCKED == ttt->game.cursorMode))
                {
                    // Send move to the other swadge
                    tttSendPlacedMarker(ttt);

                    // Place the marker
                    tttPlaceMarker(ttt, &ttt->game.selectedSubgame, &ttt->game.cursor,
                                   (GOING_FIRST == tttGetPlayOrder(ttt)) ? TTT_P1 : TTT_P2);

                    // Switch to waiting (i.e. the other player's turn)
                    ttt->game.state = TGS_WAITING;
                }
                break;
            }
            case PB_B:
            {
                // If a cell is being selected, and not locked into the subgame
                if (SELECT_CELL == ttt->game.cursorMode)
                {
                    // Go back to selecting a subgame
                    cursorMoved          = true;
                    ttt->game.cursor     = ttt->game.selectedSubgame;
                    ttt->game.cursorMode = SELECT_SUBGAME;
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
 * @brief Send the marker (cosmetic) to the other swadge for setup
 *
 * @param ttt The entire game state
 * @param markerIdx The marker index
 */
void tttSendMarker(ultimateTTT_t* ttt, int32_t markerIdx)
{
    if (ttt->game.p2p.cnc.isConnected)
    {
        tttMsgSelectMarker_t txSel = {
            .type      = MSG_SELECT_MARKER,
            .markerIdx = markerIdx,
        };
        p2pSendMsg(&ttt->game.p2p, (const uint8_t*)&txSel, sizeof(txSel), tttMsgTxCbFn);
    }
}

/**
 * @brief Receive the marker (cosmetic) from the other swadge and advance the game state
 *
 * @param ttt The entire game state
 * @param rxSel The message received with the other marker
 */
void tttReceiveMarker(ultimateTTT_t* ttt, const tttMsgSelectMarker_t* rxSel)
{
    // If this is the second player
    if (GOING_SECOND == tttGetPlayOrder(ttt))
    {
        // Save p1's marker
        ttt->game.p1MarkerIdx = rxSel->markerIdx;

        // Set p2's marker
        ttt->game.p2MarkerIdx = ttt->activeMarkerIdx;

        // Send sprite selection to other swadge
        tttSendMarker(ttt, ttt->game.p2MarkerIdx);

        // Wait for p1 to make the first move
        ttt->game.state = TGS_WAITING;
    }
    else // Going first
    {
        // Received p2's marker
        ttt->game.p2MarkerIdx = rxSel->markerIdx;

        // Make the first move
        ttt->game.state = TGS_PLACING_MARKER;
    }
}

/**
 * @brief Send the cursor position to the other Swadge. This should be done whenever the cursor moves.
 *
 * @param ttt The entire game state
 */
void tttSendCursor(ultimateTTT_t* ttt)
{
    if (ttt->game.p2p.cnc.isConnected)
    {
        // Send cursor type to other swadge
        tttMsgMoveCursor_t move = {
            .type            = MSG_MOVE_CURSOR,
            .cursorMode      = ttt->game.cursorMode,
            .selectedSubgame = ttt->game.selectedSubgame,
            .cursor          = ttt->game.cursor,
        };
        p2pSendMsg(&ttt->game.p2p, (const uint8_t*)&move, sizeof(move), tttMsgTxCbFn);
    }
}

/**
 * @brief Receive the cursor position from the other Swadge.
 *
 * @param ttt The entire game state
 * @param msg The message with the cursor position
 */
void tttReceiveCursor(ultimateTTT_t* ttt, const tttMsgMoveCursor_t* msg)
{
    // Move the cursor
    ttt->game.cursorMode      = msg->cursorMode;
    ttt->game.selectedSubgame = msg->selectedSubgame;
    ttt->game.cursor          = msg->cursor;
}

/**
 * @brief Send the last placed marker to the other Swadge
 *
 * @param ttt The entire game state
 */
void tttSendPlacedMarker(ultimateTTT_t* ttt)
{
    if (ttt->game.p2p.cnc.isConnected)
    {
        // Send move to the other swadge
        tttMsgPlaceMarker_t place = {
            .type            = MSG_PLACE_MARKER,
            .selectedSubgame = ttt->game.selectedSubgame,
            .selectedCell    = ttt->game.cursor,
        };
        p2pSendMsg(&ttt->game.p2p, (const uint8_t*)&place, sizeof(place), tttMsgTxCbFn);

        ttt->game.state = TGS_WAITING;
    }
}

/**
 * @brief Receive a placed marker from the other Swadge and place it
 *
 * @param ttt The entire game state
 * @param msg The message indicating where the marker was placed
 */
void tttReceivePlacedMarker(ultimateTTT_t* ttt, const tttMsgPlaceMarker_t* msg)
{
    // Place the marker
    tttPlaceMarker(ttt, &msg->selectedSubgame, &msg->selectedCell,
                   (GOING_FIRST == tttGetPlayOrder(ttt)) ? TTT_P2 : TTT_P1);

    // Transition state to placing a marker
    ttt->game.state = TGS_PLACING_MARKER;
}

/**
 * @brief Place a marker on the game board and check for any winners
 *
 * @param ttt The entire game state
 * @param subgame The index of the subgame a marker is placed in
 * @param cell The index of the cell the marker is placed in
 * @param marker The player who is placing the marker
 */
static void tttPlaceMarker(ultimateTTT_t* ttt, const vec_t* subgame, const vec_t* cell, tttPlayer_t marker)
{
    // Place the marker
    ttt->game.subgames[subgame->x][subgame->y].game[cell->x][cell->y] = marker;

    // Check the board
    bool won  = false;
    bool lost = false;
    bool drew = false;
    switch (checkWinner(ttt))
    {
        case TTT_DRAW:
        {
            drew = true;
            break;
        }
        case TTT_P1:
        {
            // Player 1 won, figure out who that is
            if (GOING_FIRST == tttGetPlayOrder(ttt))
            {
                won = true;
            }
            else
            {
                lost = true;
            }
            break;
        }
        case TTT_P2:
        {
            // Player 2 won, figure out who that is
            if (GOING_SECOND == tttGetPlayOrder(ttt))
            {
                won = true;
            }
            else
            {
                lost = true;
            }
            break;
        }
        case TTT_NONE:
        {
            // Next move should be in the subgame indicated by the cell
            ttt->game.selectedSubgame = *cell;
            ttt->game.cursorMode      = SELECT_CELL_LOCKED;

            // If that subgame is already won
            if (TTT_NONE != ttt->game.subgames[ttt->game.selectedSubgame.x][ttt->game.selectedSubgame.y].winner)
            {
                // Find the next valid subgame
                for (int16_t y = 0; y < 3; y++)
                {
                    for (int16_t x = 0; x < 3; x++)
                    {
                        if (TTT_NONE == ttt->game.subgames[x][y].winner)
                        {
                            ttt->game.cursor.x   = x;
                            ttt->game.cursor.y   = y;
                            ttt->game.cursorMode = SELECT_SUBGAME;
                            break;
                        }
                    }
                    if (SELECT_SUBGAME == ttt->game.cursorMode)
                    {
                        break;
                    }
                }
            }
            else
            {
                // Find a valid cell in the next subgame
                ttt->game.cursor.x = 1;
                ttt->game.cursor.y = 1;
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
            break;
        }
    }

    // If the game ended
    if (won || lost || drew)
    {
        // Record the outcome
        if (won)
        {
            // Increment wins
            ttt->wins++;
            writeNvs32(tttWinKey, ttt->wins);
            ttt->lastResult = TTR_WIN;

            // Check for unlocked markers
            for (int16_t mIdx = 0; mIdx < NUM_UNLOCKABLE_MARKERS; mIdx++)
            {
                // If the player got the required number of wins
                if (markersUnlockedAtWins[mIdx] == ttt->wins)
                {
                    // Unlock the next marker
                    ttt->numUnlockedMarkers++;
                    // Save to NVS
                    writeNvs32(tttUnlockKey, ttt->numUnlockedMarkers);
                    break;
                }
            }
        }
        else if (lost)
        {
            ttt->losses++;
            writeNvs32(tttLossKey, ttt->losses);
            ttt->lastResult = TTR_LOSE;
        }
        else if (drew)
        {
            ttt->draws++;
            writeNvs32(tttDrawKey, ttt->draws);
            ttt->lastResult = TTR_DRAW;
        }

        // Stop p2p
        p2pDeinit(&ttt->game.p2p);

        // Show the result
        tttShowUi(TUI_RESULT);
    }
}

/**
 * @brief Check all subgames and the main game for wins or draws
 *
 * @param ttt The entire game state
 * @return Who won the game, if there was a winner
 */
static tttPlayer_t checkWinner(ultimateTTT_t* ttt)
{
    // Check all the subgames
    for (uint16_t y = 0; y < 3; y++)
    {
        for (uint16_t x = 0; x < 3; x++)
        {
            checkSubgameWinner(&ttt->game.subgames[x][y]);
        }
    }

    if (ttt->showingInstructions)
    {
        return TTT_NONE;
    }

    // Check the main game
    tttPlayer_t winner = TTT_NONE;
    for (uint16_t i = 0; i < 3; i++)
    {
        // Check horizontals
        if (ttt->game.subgames[i][0].winner == ttt->game.subgames[i][1].winner
            && ttt->game.subgames[i][1].winner == ttt->game.subgames[i][2].winner)
        {
            if (TTT_NONE != ttt->game.subgames[i][0].winner)
            {
                winner = ttt->game.subgames[i][0].winner;
                break;
            }
        }

        // Check verticals
        if (ttt->game.subgames[0][i].winner == ttt->game.subgames[1][i].winner
            && ttt->game.subgames[1][i].winner == ttt->game.subgames[2][i].winner)
        {
            if (TTT_NONE != ttt->game.subgames[0][i].winner)
            {
                winner = ttt->game.subgames[0][i].winner;
                break;
            }
        }
    }

    // Check diagonals
    if (ttt->game.subgames[0][0].winner == ttt->game.subgames[1][1].winner
        && ttt->game.subgames[1][1].winner == ttt->game.subgames[2][2].winner)
    {
        if (TTT_NONE != ttt->game.subgames[0][0].winner)
        {
            winner = ttt->game.subgames[0][0].winner;
        }
    }
    else if (ttt->game.subgames[2][0].winner == ttt->game.subgames[1][1].winner
             && ttt->game.subgames[1][1].winner == ttt->game.subgames[0][2].winner)
    {
        if (TTT_NONE != ttt->game.subgames[2][0].winner)
        {
            winner = ttt->game.subgames[2][0].winner;
        }
    }

    // Check for a draw
    if (TTT_NONE == winner)
    {
        // Assume it's a draw
        bool isDraw = true;
        // Check for an empty subgame
        for (uint16_t y = 0; y < 3; y++)
        {
            for (uint16_t x = 0; x < 3; x++)
            {
                if (TTT_NONE == ttt->game.subgames[x][y].winner)
                {
                    // Empty space means not a draw
                    isDraw = false;
                    break;
                }
            }
        }

        if (isDraw)
        {
            return TTT_DRAW;
        }
    }

    return winner;
}

/**
 * @brief Check a subgame if it was a win, loss, or draw
 *
 * @param subgame The subgame to check
 * @return The winner of the subgame, if there was one
 */
static tttPlayer_t checkSubgameWinner(tttSubgame_t* subgame)
{
    // If it wasn't already won
    if (TTT_NONE == subgame->winner)
    {
        for (uint16_t i = 0; i < 3; i++)
        {
            // Check horizontals
            if (subgame->game[i][0] == subgame->game[i][1] && subgame->game[i][1] == subgame->game[i][2])
            {
                if (TTT_NONE != subgame->game[i][0])
                {
                    subgame->winner = subgame->game[i][0];
                    return subgame->game[i][0];
                }
            }

            // Check verticals
            if (subgame->game[0][i] == subgame->game[1][i] && subgame->game[1][i] == subgame->game[2][i])
            {
                if (TTT_NONE != subgame->game[0][i])
                {
                    subgame->winner = subgame->game[0][i];
                    return subgame->game[0][i];
                }
            }
        }

        // Check diagonals
        if (subgame->game[0][0] == subgame->game[1][1] && subgame->game[1][1] == subgame->game[2][2])
        {
            if (TTT_NONE != subgame->game[0][0])
            {
                subgame->winner = subgame->game[0][0];
                return subgame->game[0][0];
            }
        }
        else if (subgame->game[2][0] == subgame->game[1][1] && subgame->game[1][1] == subgame->game[0][2])
        {
            if (TTT_NONE != subgame->game[2][0])
            {
                subgame->winner = subgame->game[2][0];
                return subgame->game[2][0];
            }
        }

        // Check for a draw
        if (TTT_NONE == subgame->winner)
        {
            // Assume it's a draw
            bool isDraw = true;
            // Check for an empty space
            for (uint16_t y = 0; y < 3; y++)
            {
                for (uint16_t x = 0; x < 3; x++)
                {
                    if (TTT_NONE == subgame->game[x][y])
                    {
                        // Empty space means not a draw
                        isDraw = false;
                        break;
                    }
                }
            }

            if (isDraw)
            {
                subgame->winner = TTT_DRAW;
                return subgame->winner;
            }
        }
    }
    else
    {
        return subgame->winner;
    }
    return TTT_NONE;
}

/**
 * @brief Gets the play order
 *
 * Unlike p2pGetPlayOrder(), this doesn't check connection status first
 *
 * @param ttt The entire game state
 * @return ::GOING_FIRST or ::GOING_SECOND
 */
static playOrder_t tttGetPlayOrder(ultimateTTT_t* ttt)
{
    return ttt->game.p2p.cnc.playOrder;
}

/**
 * @brief Draw the Ultimate TTT game UI
 *
 * @param ttt The entire game state
 */
void tttDrawGame(ultimateTTT_t* ttt)
{
    bool isP1 = (GOING_FIRST == tttGetPlayOrder(ttt));

    // Light LEDs for p1/p2
    led_t leds[CONFIG_NUM_LEDS] = {0};
    for (int32_t lIdx = 0; lIdx < CONFIG_NUM_LEDS; lIdx++)
    {
        if (isP1)
        {
            leds[lIdx].r = 0x80;
        }
        else
        {
            leds[lIdx].b = 0x80;
        }
    }
    setLeds(leds, CONFIG_NUM_LEDS);

    // Clear before drawing
    clearPxTft();

    // Draw some borders to indicate who you are
    fillDisplayArea(0, 0, ttt->gameOffset.x - 4, TFT_HEIGHT, isP1 ? P1_COLOR : P2_COLOR);
    fillDisplayArea(ttt->gameOffset.x + ttt->gameSize + 4, 0, TFT_WIDTH, TFT_HEIGHT, isP1 ? P1_COLOR : P2_COLOR);

    // Draw the main grid lines
    tttDrawGrid(ttt->gameOffset.x, ttt->gameOffset.y, ttt->gameOffset.x + ttt->gameSize - 1,
                ttt->gameOffset.y + ttt->gameSize - 1, 0, MAIN_GRID_COLOR);

    // For each subgame
    for (int subY = 0; subY < 3; subY++)
    {
        for (int subX = 0; subX < 3; subX++)
        {
            // Get this subgame's rectangle
            int16_t sX0 = ttt->gameOffset.x + (subX * ttt->subgameSize);
            int16_t sY0 = ttt->gameOffset.y + (subY * ttt->subgameSize);
            int16_t sX1 = sX0 + ttt->subgameSize - 1;
            int16_t sY1 = sY0 + ttt->subgameSize - 1;

            // Draw the subgame grid lines
            tttDrawGrid(sX0, sY0, sX1, sY1, 4, SUB_GRID_COLOR);

            // If selected, draw the cursor on this subgame
            if (SELECT_SUBGAME == ttt->game.cursorMode && //
                ttt->game.cursor.x == subX && ttt->game.cursor.y == subY)
            {
                paletteColor_t color;
                if (ttt->game.state == TGS_WAITING)
                {
                    color = WAITING_PLAYER_COLOR;
                }
                else
                {
                    color = isP1 ? P1_COLOR : P2_COLOR;
                }

                // Draw a rectangle with a 4px stroke
                for (int16_t i = 0; i < CURSOR_STROKE; i++)
                {
                    drawRect(sX0 + i, sY0 + i, sX1 - i, sY1 - i, color);
                }
            }

            // Check if the subgame has a winner
            switch (ttt->game.subgames[subX][subY].winner)
            {
                case TTT_P1:
                case TTT_P2:
                {
                    // Draw a big marker for a winner
                    drawWsgSimple(getMarkerWsg(ttt, ttt->game.subgames[subX][subY].winner, true), sX0, sY0);
                    break;
                }
                default:
                case TTT_DRAW:
                case TTT_NONE:
                {
                    // Draw the subgame. For each cell
                    for (int cellY = 0; cellY < 3; cellY++)
                    {
                        for (int cellX = 0; cellX < 3; cellX++)
                        {
                            // Get the location for this cell
                            int16_t cX0 = sX0 + (cellX * ttt->cellSize);
                            int16_t cY0 = sY0 + (cellY * ttt->cellSize);

                            // Draw sprites
                            switch (ttt->game.subgames[subX][subY].game[cellX][cellY])
                            {
                                default:
                                case TTT_DRAW:
                                case TTT_NONE:
                                {
                                    break;
                                }
                                case TTT_P1:
                                case TTT_P2:
                                {
                                    // Draw a small marker
                                    drawWsgSimple(
                                        getMarkerWsg(ttt, ttt->game.subgames[subX][subY].game[cellX][cellY], false),
                                        cX0, cY0);
                                    break;
                                }
                            }

                            // If selected, draw the cursor on this cell
                            if ((SELECT_CELL == ttt->game.cursorMode || SELECT_CELL_LOCKED == ttt->game.cursorMode)
                                &&                                                                            //
                                ttt->game.selectedSubgame.x == subX && ttt->game.selectedSubgame.y == subY && //
                                ttt->game.cursor.x == cellX && ttt->game.cursor.y == cellY)
                            {
                                // Get the other rectangle coordinates
                                int16_t cX1 = cX0 + ttt->cellSize - 1;
                                int16_t cY1 = cY0 + ttt->cellSize - 1;
                                // Draw the cursor
                                paletteColor_t color;
                                if (ttt->game.state == TGS_WAITING)
                                {
                                    color = WAITING_PLAYER_COLOR;
                                }
                                else
                                {
                                    color = isP1 ? P1_COLOR : P2_COLOR;
                                }

                                // Draw a rectangle with a 4px stroke
                                for (uint16_t i = 0; i < CURSOR_STROKE; i++)
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
 * @brief Get a player's marker's WSG
 *
 * @param ttt The entire game state
 * @param p The player to get a marker for
 * @param isBig true for the big version, false for the small version
 * @return A pointer to the WSG to draw
 */
static wsg_t* getMarkerWsg(ultimateTTT_t* ttt, tttPlayer_t p, bool isBig)
{
    bool isP1                      = (TTT_P1 == p);
    tttMarkerColorAssets_t* colors = &ttt->markerWsg[(isP1 ? ttt->game.p1MarkerIdx : ttt->game.p2MarkerIdx)];
    tttMarkerSizeAssets_t* sizes   = (isP1 ? &colors->red : &colors->blue);
    return (isBig ? &sizes->large : &sizes->small);
}

/**
 * @brief Helper function to draw a grid
 *
 * @param x0 The starting X coordinate for the grid
 * @param y0 The starting Y coordinate for the grid
 * @param x1 The finishing X coordinate for the grid
 * @param y1 The finishing Y coordinate for the grid
 * @param margin The margin to pad around the grid
 * @param color The color of the grid lines to draw
 */
void tttDrawGrid(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t margin, paletteColor_t color)
{
    int16_t cellWidth  = (x1 - x0) / 3;
    int16_t cellHeight = (y1 - y0) / 3;

    // Horizontal lines
    drawLineFast(x0 + margin, y0 + cellHeight, //
                 x1 - 1 - margin, y0 + cellHeight, color);
    drawLineFast(x0 + margin, y0 + (2 * cellHeight) + 1, //
                 x1 - 1 - margin, y0 + (2 * cellHeight) + 1, color);

    // Vertical lines
    drawLineFast(x0 + cellWidth, y0 + margin, //
                 x0 + cellWidth, y1 - 1 - margin, color);
    drawLineFast(x0 + (2 * cellWidth) + 1, y0 + margin, //
                 x0 + (2 * cellWidth) + 1, y1 - 1 - margin, color);
}
