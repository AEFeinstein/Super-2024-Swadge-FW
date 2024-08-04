#include "ultimateTTTcpuPlayer.h"

#include <esp_random.h>
#include <esp_log.h>
#include <esp_timer.h>

#include "ultimateTTT.h"
#include "ultimateTTTp2p.h"

//#define TCPU_LOG ESP_LOGI
#define TCPU_LOG ESP_LOGD

// 500ms delay so it's easier to see what's going on
#define DELAY_TIME 500000

typedef enum
{
    PLAYER_0 = 0x0,
    PLAYER_1 = 0x1,
    PLAYER_2 = 0x2,
    PLAYER_3 = 0x3,
    OPPONENT_0 = 0x0,
    OPPONENT_1 = 0x4,
    OPPONENT_2 = 0x8,
    OPPORENT_3 = 0xC,
    NONE_0 = 0x0,
    NONE_1 = 0x10,
    NONE_2 = 0x20,
    NONE_3 = 0x30,
} rowCount_t;

#define TO_PLAYERS(n) ((n) & 0x3)
#define TO_OPPONENTS(n) (((n) & 0xC) >> 2)
#define TO_NONES(n) (((n) & 0x30) >> 4)

// Define some scores for the game based on
// Already-won game
#define WON 20
// Game to win
#define WIN 9
#define BLOCK 8
#define FORK 7
#define BLOCK_FORK 6
#define FORCE_BLOCK 6
#define CENTER 5
#define OPPOSITE_CORNER 4
#define EMPTY_CORNER 3
#define EMPTY_SIDE 2

#define ENCODE_LOC(x, y) (((x) + (y) * 3) << 5)
#define DECODE_LOC_X(r) (((r) >> 5) % 3)
#define DECODE_LOC_Y(r) (((r) >> 5) / 3)
#define DECODE_MOVE(r) ((r) & 0x1F)

static bool selectSubgame_easy(ultimateTTT_t* ttt, int *x, int *y);
static bool selectSubgame_medium(ultimateTTT_t* ttt, int *x, int *y);
static bool selectSubgame_hard(ultimateTTT_t* ttt, int *x, int *y);

static bool selectCell_easy(ultimateTTT_t* ttt, int *x, int *y);
static bool selectCell_medium(ultimateTTT_t* ttt, int *x, int *y);
static bool selectCell_hard(ultimateTTT_t* ttt, int *x, int *y);

static void tttCpuSelectSubgame(ultimateTTT_t* ttt);
static void tttCpuSelectCell(ultimateTTT_t* ttt);

static int hasDiagonal(int x, int y);
static rowCount_t checkRow(const tttPlayer_t game[3][3], int y, tttPlayer_t player);
static rowCount_t checkCol(const tttPlayer_t game[3][3], int x, tttPlayer_t player);
static rowCount_t checkDiag(const tttPlayer_t game[3][3], int n, tttPlayer_t player);

static uint16_t analyzeSubgame(const tttPlayer_t subgame[3][3], tttPlayer_t player);
static void analyzeGame(ultimateTTT_t* ttt);

void tttCpuNextMove(ultimateTTT_t* ttt)
{
    TCPU_LOG("TTT", "tttCpuNextMove()");
    if (ttt->game.cpu.state == TCPU_INACTIVE)
    {
        TCPU_LOG("TTT", "Invactive.");
        return;
    }

    int64_t now = esp_timer_get_time();
    if (now < ttt->game.cpu.delayTime)
    {
        TCPU_LOG("TTT", "Sleep...");
        return;
    }

    // Reset the timer for the next one
    ttt->game.cpu.delayTime = now + DELAY_TIME;

    if (ttt->game.cpu.state == TCPU_THINKING)
    {
        TCPU_LOG("TTT", ">>>>>>>>>>>>>>>>>>>>");
        TCPU_LOG("TTT", "Thinking...");
        if (ttt->game.cursorMode == SELECT_SUBGAME)
        {
            tttCpuSelectSubgame(ttt);
        }
        else if (ttt->game.cursorMode == SELECT_CELL || ttt->game.cursorMode == SELECT_CELL_LOCKED)
        {
            tttCpuSelectCell(ttt);
        }
        else
        {
            TCPU_LOG("TTT", "CPU doesn't know what to do!");
            switch (ttt->game.cursorMode)
            {
                case NO_CURSOR:
                TCPU_LOG("TTT", "cursorMode == NO_CURSOR");
                break;

                case SELECT_SUBGAME:
                TCPU_LOG("TTT", "cursorMode == SELECT_SUBGAME");
                break;

                case SELECT_CELL:
                TCPU_LOG("TTT", "cursorMode == SELECT_CELL");
                break;

                case SELECT_CELL_LOCKED:
                TCPU_LOG("TTT", "cursorMode == SELECT_CELL_LOCKED");
                break;
            }
        }
    }
    else if (ttt->game.cpu.state == TCPU_MOVING)
    {
        TCPU_LOG("TTT", "Moving...");

        if (ttt->game.cursorMode == SELECT_SUBGAME)
        {
            TCPU_LOG("TTT", "Moving to Subgame");
            tttMsgMoveCursor_t payload;
            payload.type = MSG_MOVE_CURSOR;
            payload.cursorMode = SELECT_SUBGAME;
            payload.selectedSubgame.x = ttt->game.selectedSubgame.x;
            payload.selectedSubgame.y = ttt->game.selectedSubgame.y;
            payload.cursor.x = ttt->game.cursor.x;
            payload.cursor.y = ttt->game.cursor.y;

            do {
                if (payload.cursor.x < ttt->game.cpu.destSubgame.x && payload.cursor.x < 2)
                {
                    payload.cursor.x++;
                }
                else if (payload.cursor.y < ttt->game.cpu.destSubgame.y && payload.cursor.y < 2)
                {
                    payload.cursor.y++;
                }
                else if (payload.cursor.x > ttt->game.cpu.destSubgame.x && payload.cursor.x > 0)
                {
                    payload.cursor.x--;
                }
                else if (payload.cursor.y > ttt->game.cpu.destSubgame.y && payload.cursor.y > 0)
                {
                    payload.cursor.y--;
                }
                else
                {
                    ttt->game.cpu.state = TCPU_THINKING;
                    // We're in the right place already!
                    payload.cursorMode = SELECT_CELL;
                    // TODO don't select an invalid cell
                    payload.cursor.x = 1;
                    payload.cursor.y = 1;
                    payload.selectedSubgame.x = ttt->game.cpu.destSubgame.x;
                    payload.selectedSubgame.y = ttt->game.cpu.destSubgame.y;
                    tttReceiveCursor(ttt, &payload);

                    if (!tttCursorIsValid(ttt, &payload.cursor))
                    {
                        for (int16_t y = 0; y < 3; y++)
                        {
                            for (int16_t x = 0; x < 3; x++)
                            {
                                if (!tttCursorIsValid(ttt, &payload.cursor))
                                {
                                    payload.cursor.x = (payload.cursor.x + 1) % 3;
                                }
                                else
                                {
                                    break;
                                }
                            }

                            if (!tttCursorIsValid(ttt, &payload.cursor))
                            {
                                payload.cursor.y = (payload.cursor.y + 1) % 3;
                            }
                            else
                            {
                                break;
                            }
                        }
                        tttReceiveCursor(ttt, &payload);
                    }
                    return;
                }
            } while (ttt->game.subgames[payload.selectedSubgame.x][payload.selectedSubgame.y].winner != TTT_NONE);

            tttReceiveCursor(ttt, &payload);
        }
        else if (ttt->game.cursorMode == SELECT_CELL || ttt->game.cursorMode == SELECT_CELL_LOCKED)
        {
            TCPU_LOG("TTT", "Moving to cell");
            tttMsgMoveCursor_t payload;
            payload.type = MSG_MOVE_CURSOR;
            payload.cursor.x = ttt->game.cursor.x;
            payload.cursor.y = ttt->game.cursor.y;
            payload.cursorMode = ttt->game.cursorMode;
            payload.selectedSubgame.x = ttt->game.selectedSubgame.x;
            payload.selectedSubgame.y = ttt->game.selectedSubgame.y;

            do {
                if (payload.cursor.x < ttt->game.cpu.destCell.x && payload.cursor.x < 2)
                {
                    payload.cursor.x++;
                }
                else if (payload.cursor.y < ttt->game.cpu.destCell.y && payload.cursor.y < 2)
                {
                    payload.cursor.y++;
                }
                else if (payload.cursor.x > ttt->game.cpu.destCell.x && payload.cursor.x > 0)
                {
                    payload.cursor.x--;
                }
                else if (payload.cursor.y > ttt->game.cpu.destCell.y && payload.cursor.y > 0)
                {
                    payload.cursor.y--;
                }
                else
                {
                    // The cursor is in the right place, select it!
                    tttMsgPlaceMarker_t placePayload;
                    placePayload.type = MSG_PLACE_MARKER;
                    placePayload.selectedSubgame.x = ttt->game.selectedSubgame.x;
                    placePayload.selectedSubgame.y = ttt->game.selectedSubgame.y;
                    placePayload.selectedCell.x = ttt->game.cpu.destCell.x;
                    placePayload.selectedCell.y = ttt->game.cpu.destCell.y;

                    ttt->game.cpu.state = TCPU_INACTIVE;

                    tttReceivePlacedMarker(ttt, &placePayload);

                    return;
                }
            } while (!tttCursorIsValid(ttt, &payload.cursor));

            tttReceiveCursor(ttt, &payload);
        }
        else
        {
            TCPU_LOG("TTT", "Something else???");
        }
    }
}

static bool selectSubgame_easy(ultimateTTT_t* ttt, int *x, int *y)
{
    int availableSubgames[9] = {0};
    int availableCount = 0;

    for (int idx = 0; idx < 9; idx++)
    {
        if (TTT_NONE == ttt->game.subgames[idx % 3][idx / 3].winner)
        {
            TCPU_LOG("TTT", "Sub-game %d, %d is a valid target", idx % 3, idx / 3);
            availableSubgames[availableCount++] = idx;
        }
    }

    // Pick a random subgame
    int subgameIndex = availableSubgames[esp_random() % availableCount];

    *x = subgameIndex % 3;
    *y = subgameIndex / 3;
    return true;
}

static bool selectSubgame_medium(ultimateTTT_t* ttt, int *x, int *y)
{
    return selectSubgame_easy(ttt, x, y);
}

static bool selectSubgame_hard(ultimateTTT_t* ttt, int *x, int *y)
{
    return selectSubgame_medium(ttt, x, y);
}

static void tttCpuSelectSubgame(ultimateTTT_t* ttt)
{
    int x, y;
    bool result = false;
    switch (ttt->game.cpu.difficulty)
    {
        case TDIFF_EASY:
        result = selectSubgame_easy(ttt, &x, &y);
        break;

        case TDIFF_MEDIUM:
        result = selectSubgame_medium(ttt, &x, &y);
        break;

        case TDIFF_HARD:
        result = selectSubgame_hard(ttt, &x, &y);
        break;
    }

    if (result)
    {

        TCPU_LOG("TTT", "Selecting subgame %d, %d", x, y);
        ttt->game.cpu.destSubgame.x = x;
        ttt->game.cpu.destSubgame.y = y;
        ttt->game.cpu.state = TCPU_MOVING;

        // Set the cursor up in this mode in order to move the state along
        tttMsgMoveCursor_t payload;
        payload.type = MSG_MOVE_CURSOR;
        payload.cursorMode = SELECT_SUBGAME;
        payload.selectedSubgame.x = ttt->game.selectedSubgame.x;
        payload.selectedSubgame.y = ttt->game.selectedSubgame.y;
        payload.cursor.x = ttt->game.cursor.x;
        payload.cursor.y = ttt->game.cursor.y;
        tttReceiveCursor(ttt, &payload);
    }
}

static bool selectCell_easy(ultimateTTT_t* ttt, int *x, int *y)
{
    int availableCells[9] = {0};
    int availableCount = 0;

    // Pick a random cell!
    tttSubgame_t* subgame = &ttt->game.subgames[ttt->game.selectedSubgame.x][ttt->game.selectedSubgame.y];
    for (int idx = 0; idx < 9; idx++)
    {
        if (TTT_NONE == subgame->game[idx % 3][idx / 3])
        {
            TCPU_LOG("TTT", "Cell %d, %d is a valid target", idx % 3, idx / 3);
            availableCells[availableCount++] = idx;
        }
    }

    int cellIndex = availableCells[esp_random() % availableCount];

    *x = cellIndex % 3;
    *y = cellIndex / 3;
    return true;
}

static bool selectCell_medium(ultimateTTT_t* ttt, int *x, int *y)
{
    // Medium plays perfectly within a subgame, but has no overall strategy and picks randomly when selecting a subgame
    tttSubgame_t* subgame = &ttt->game.subgames[ttt->game.selectedSubgame.x][ttt->game.selectedSubgame.y];
    uint16_t result = analyzeSubgame(subgame->game, TTT_P2);
    uint16_t move = DECODE_MOVE(result);
    int moveX = DECODE_LOC_X(result);
    int moveY = DECODE_LOC_Y(result);

    switch (move)
    {
        case WON:
            return false;

        case WIN:
        case BLOCK:
        case FORK:
        case BLOCK_FORK:
        //case FORCE_BLOCK:
        case CENTER:
        case OPPOSITE_CORNER:
        case EMPTY_CORNER:
        case EMPTY_SIDE:
            *x = moveX;
            *y = moveY;
            return true;

        default:
            return false;
    }
}

static bool selectCell_hard(ultimateTTT_t* ttt, int *x, int *y)
{
    return selectCell_medium(ttt, x, y);
}

static void tttCpuSelectCell(ultimateTTT_t* ttt)
{
    TCPU_LOG("TTT", "CPU selecting next cell...");

    int x, y;
    bool result = false;
    switch (ttt->game.cpu.difficulty)
    {
        case TDIFF_EASY:
        result = selectCell_easy(ttt, &x, &y);
        break;

        case TDIFF_MEDIUM:
        result = selectCell_medium(ttt, &x, &y);
        break;

        case TDIFF_HARD:
        result = selectCell_hard(ttt, &x, &y);
        break;
    }

    if (result)
    {
        TCPU_LOG("TTT", "Selecting cell %d, %d", x, y);
        ttt->game.cpu.destCell.x = x;
        ttt->game.cpu.destCell.y = y;
        ttt->game.cpu.state = TCPU_MOVING;
    }
}

/**
 * @brief Return whether or not a given cell is part of the diagonals, and which one it is
 *
 * 0: not part of a diagonal
 * 1: top-left to bottom-right diagonal
 * 2: bottom-left to top-right diagonal
 * 3: both diagonals (center square)
 *
 * @param x
 * @param y
 * @return int
 */
static int hasDiagonal(int x, int y)
{
    if (x == y)
    {
        if (x == 1)
        {
            // Center square is part of both
            return 3;
        }

        return 1;
    }

    if (x == (2 - y))
    {
        return 2;
    }

    return 0;

}

static rowCount_t checkRow(const tttPlayer_t game[3][3], int y, tttPlayer_t player)
{
    int noneCount = 0;
    int playerCount = 0;
    int opponentCount = 0;
    tttPlayer_t otherPlayer = (player == TTT_P1) ? TTT_P2 : TTT_P1;

    for (int x = 0; x < 3; x++)
    {
        if (game[x][y] == TTT_NONE)
        {
            noneCount++;
        }
        else if (game[x][y] == player)
        {
            playerCount++;
        }
        else if (game[x][y] == otherPlayer)
        {
            opponentCount++;
        }
    }

    return (rowCount_t)(playerCount | (opponentCount << 2) | (noneCount << 4));
}

static rowCount_t checkCol(const tttPlayer_t game[3][3], int x, tttPlayer_t player)
{
    int noneCount = 0;
    int playerCount = 0;
    int opponentCount = 0;
    tttPlayer_t otherPlayer = (player == TTT_P1) ? TTT_P2 : TTT_P1;

    for (int y = 0; y < 3; y++)
    {
        if (game[x][y] == TTT_NONE)
        {
            noneCount++;
        }
        else if (game[x][y] == player)
        {
            playerCount++;
        }
        else if (game[x][y] == otherPlayer)
        {
            opponentCount++;
        }
    }

    return (rowCount_t)(playerCount | opponentCount << 2 | noneCount << 4);
}

static rowCount_t checkDiag(const tttPlayer_t game[3][3], int n, tttPlayer_t player)
{
    int noneCount = 0;
    int playerCount = 0;
    int opponentCount = 0;
    tttPlayer_t otherPlayer = (player == TTT_P1) ? TTT_P2 : TTT_P1;

    if (n == 1)
    {
        for (int i = 0; i < 3; i++)
        {
            int x = i;
            int y = (n == 1) ? i : (2 - i);

            if (game[x][y] == TTT_NONE)
            {
                noneCount++;
            }
            else if (game[x][y] == player)
            {
                playerCount++;
            }
            else if (game[x][y] == otherPlayer)
            {
                opponentCount++;
            }
        }
    }

    return (rowCount_t)(playerCount | opponentCount << 2 | noneCount << 4);
}

static uint16_t analyzeSubgame(const tttPlayer_t subgame[3][3], tttPlayer_t player)
{
/*
1. Win: If you have two in a row, play the third to get three in a row.
2. Block: If the opponent has two in a row, play the third to block them.
3. Fork: Create an opportunity where you can win in two ways.
4. Block Opponent's Fork:
   - Option 1:
       Create two in a row to force the opponent into defending, as long as it doesn't
       result in them creating a fork or winning. For example, if "X" has a corner, "O"
       has the center, and "X" has the opposite corner as well, "O" must not play a corner
       in order to win. (Playing a corner in this scenario creates a fork for "X" to win.)
   - Option 2:
       If there is a configuration where the opponent can fork, block that fork.
5. Center: Play the center.
6. Opposite Corner: If the opponent is in the corner, play the opposite corner.
7. Empty Corner: Play an empty corner.
8. Empty Side: Play an empty side.
*/

    tttPlayer_t otherPlayer = (player == TTT_P1) ? TTT_P2 : TTT_P1;

    // 0. Check for already won
    if (player == tttCheckWinner(subgame))
    {
        return WON;
    }

    // 1. Win
    // - Check for any 3 <player> in a row
    // 2. Block
    // - Check for any 2 <player> in a row, plus a NONE
    // - Horizontals
    // - Verticals
    // - Diagonals
    {
        rowCount_t counts;

        // Horizontals
        for (int y = 0; y < 3; y++)
        {
            counts = checkRow(subgame, y, player);

            if (TO_PLAYERS(counts) == 2 && TO_NONES(counts) == 1)
            {
                return WIN | ENCODE_LOC(((subgame[0][y] == TTT_NONE) ? 0 : ((subgame[1][y] == TTT_NONE) ? 1 : 2)), y);
            }

            if (TO_OPPONENTS(counts) == 2 && TO_NONES(counts) == 1)
            {
                return BLOCK | ENCODE_LOC(((subgame[0][y] == TTT_NONE) ? 0 : ((subgame[1][y] == TTT_NONE) ? 1 : 2)), y);
            }
        }

        // Verticals
        for (int x = 0; x < 3; x++)
        {
            counts = checkCol(subgame, x, player);

            if (TO_PLAYERS(counts) == 2 && TO_NONES(counts) == 1)
            {
                return WIN | ENCODE_LOC(x, ((subgame[x][0] == TTT_NONE) ? 0 : ((subgame[x][1] == TTT_NONE) ? 1 : 2)));
            }

            if (TO_OPPONENTS(counts) == 2 && TO_NONES(counts) == 1)
            {
                return BLOCK | ENCODE_LOC(x, ((subgame[x][0] == TTT_NONE) ? 0 : ((subgame[x][1] == TTT_NONE) ? 1 : 2)));
            }
        }

        // Diagonals

        // Top-left to bottom-right diagonal
        for (int n = 0; n < 2; n++)
        {
            counts = checkDiag(subgame, n + 1, player);
            int loc = (subgame[0][n ? 2 : 0] == TTT_NONE) ? 0 : ((subgame[1][1] == TTT_NONE) ? 1 : 2);

            if (TO_PLAYERS(counts) == 2 && TO_NONES(counts) == 1)
            {
                return WIN | ENCODE_LOC(loc, loc);
            }

            if (TO_OPPONENTS(counts) == 2 && TO_NONES(counts) == 1)
            {
                return BLOCK | ENCODE_LOC(loc, loc);
            }
        }
    }

    // 3. Fork: Create an opportunity where you can win in two ways.
    {
        // This one's a little more ambiguous...
        // First we must learn, what is fork?
        // A spot where a fork can be created is any empty spot, which
        // - is part of two intersecting rows/diagonals
        // - if filled, causes both rows to have 2 <player> and 1 <NONE>

        // First, count up the values in each row/column/diagonal separately
        rowCount_t rows[3];
        rowCount_t cols[3];
        rowCount_t diags[2];

        for (int n = 0; n < 3; n++)
        {
            rows[n] = checkRow(subgame, n, player);
            cols[n] = checkCol(subgame, n, player);
            if (n > 0)
            {
                diags[n-1] = checkDiag(subgame, n, player);
            }
        }

        // Next, take those totals and evaluate each spot on the board for fork creation
        for (int x = 0; x < 3; x++)
        {
            for (int y = 0; y < 3; y++)
            {
                // Spot is empty!
                if (TTT_NONE == subgame[x][y])
                {
                    // So, okay, what we need to do here is figure out, for the current square:
                    // - If it is empty
                    // - If two of its rows/columns/diagonals meet these criteria:
                    //    - Two NONEs, and one PLAYER

                    int matchCount = 0;
                    if (TO_PLAYERS(rows[y]) == 1 && TO_NONES(rows[y]) == 2)
                    {
                        matchCount++;
                    }

                    if (TO_PLAYERS(cols[x]) == 1 && TO_NONES(cols[x]) == 2)
                    {
                        matchCount++;
                    }

                    if ((hasDiagonal(x, y) & 1) && TO_PLAYERS(diags[0]) == 1 && TO_NONES(diags[0]) == 2)
                    {
                        matchCount++;
                    }

                    if ((hasDiagonal(x, y) & 2) && TO_PLAYERS(diags[1]) == 1 && TO_NONES(diags[1]) == 2)
                    {
                        matchCount++;
                    }

                    if (matchCount > 1)
                    {
                        return FORK | ENCODE_LOC(x, y);
                    }
                }
            }
        }

        // Now, do the same pass again but check if there's anywhere the opponent can block instead
        for (int x = 0; x < 3; x++)
        {
            for (int y = 0; y < 3; y++)
            {
                // Spot is empty!
                if (TTT_NONE == subgame[x][y])
                {
                    // So, okay, what we need to do here is figure out, for the current square:
                    // - If it is empty
                    // - If two of its rows/columns/diagonals meet these criteria:
                    //    - Two NONEs, and one OPPONENT

                    int matchCount = 0;
                    if (TO_OPPONENTS(rows[y]) == 1 && TO_NONES(rows[y]) == 2)
                    {
                        matchCount++;
                    }

                    if (TO_OPPONENTS(cols[x]) == 1 && TO_NONES(cols[x]) == 2)
                    {
                        matchCount++;
                    }

                    if ((hasDiagonal(x, y) & 1) && TO_OPPONENTS(diags[0]) == 1 && TO_NONES(diags[0]) == 2)
                    {
                        matchCount++;
                    }

                    if ((hasDiagonal(x, y) & 2) && TO_OPPONENTS(diags[1]) == 1 && TO_NONES(diags[1]) == 2)
                    {
                        matchCount++;
                    }

                    if (matchCount > 1)
                    {
                        return BLOCK_FORK | ENCODE_LOC(x, y);
                    }
                }
            }
        }
    }

    // 5. Center: Play the center.
    if (TTT_NONE == subgame[1][1])
    {
        return CENTER | ENCODE_LOC(1, 1);
    }

    // 6. Opposite Corner: If the opponent is in the corner, play the opposite corner.
    for (int n = 0; n < 4; n++)
    {
        int x = (n % 2) * 2;
        int y = (n / 2) * 2;
        if (otherPlayer == subgame[x][y] && TTT_NONE == subgame[2-x][2-y])
        {
            return OPPOSITE_CORNER | ENCODE_LOC(2-x, 2-y);
        }
    }

    // 7. Empty Corner: Play an empty corner.
    for (int n = 0; n < 4; n++)
    {
        int x = (n % 2) * 2;
        int y = (n / 2) * 2;
        if (TTT_NONE == subgame[x][y])
        {
            return EMPTY_CORNER | ENCODE_LOC(x, y);
        }
    }

    // 8. Empty Side: Play an empty side.
    for (int n = 0; n < 4; n++)
    {
        // I'm sure I could math it but that's hard
        const int xs[] = {0, 1, 2, 1};
        const int ys[] = {1, 0, 1, 2};
        if (TTT_NONE == subgame[xs[n]][ys[n]])
        {
            return EMPTY_SIDE | ENCODE_LOC(xs[n], ys[n]);
        }
    }

    TCPU_LOG("TTT", "We should have picked a move already? Not sure this is possible");
    return 0;
}

static void analyzeGame(ultimateTTT_t* ttt)
{
    uint16_t opponentScore[3][3];
    uint16_t myScore[3][3];


}