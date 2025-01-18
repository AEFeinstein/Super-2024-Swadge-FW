#include "ultimateTTTcpuPlayer.h"

#include <esp_random.h>
#include <esp_log.h>
#include <esp_timer.h>

#include "ultimateTTT.h"
#include "ultimateTTTp2p.h"

// Un-comment to print CPU "thoughts"
// #define DEBUG_TTT_CPU

#ifdef DEBUG_TTT_CPU
    #define TCPU_LOG(...) ESP_LOGI("TTT", __VA_ARGS__)
#else
    #define TCPU_LOG(...)
#endif

// 500ms delay so it's easier to see what's going on
#define DELAY_TIME 500000

typedef enum
{
    PLAYER_0   = 0x0,
    PLAYER_1   = 0x1,
    PLAYER_2   = 0x2,
    PLAYER_3   = 0x3,
    OPPONENT_0 = 0x0,
    OPPONENT_1 = 0x4,
    OPPONENT_2 = 0x8,
    OPPORENT_3 = 0xC,
    NONE_0     = 0x0,
    NONE_1     = 0x10,
    NONE_2     = 0x20,
    NONE_3     = 0x30,
} rowCount_t;

#define TO_PLAYERS(n)   ((n) & 0x3)
#define TO_OPPONENTS(n) (((n) & 0xC) >> 2)
#define TO_NONES(n)     (((n) & 0x30) >> 4)

// Define some scores for the game based on
// Already-won game
#define WON 20
// Game to win
#define WIN             9
#define BLOCK           8
#define FORK            7
#define BLOCK_FORK      6
#define CENTER          5
#define OPPOSITE_CORNER 4
#define EMPTY_CORNER    3
#define EMPTY_SIDE      2

static const char strMoveWon[]            = "Won";
static const char strMoveWin[]            = "Win";
static const char strMoveBlock[]          = "Block";
static const char strMoveFork[]           = "Fork";
static const char strMoveBlockFork[]      = "Block Fork";
static const char strMoveCenter[]         = "Center";
static const char strMoveOppositeCenter[] = "Opposite Corner";
static const char strMoveEmptyCorner[]    = "Empty Corner";
static const char strMoveEmptySide[]      = "Empty Side";

#define ENCODE_LOC(x, y) (((x) + (y) * 3) << 5)
#define DECODE_LOC_X(r)  (((r) >> 5) % 3)
#define DECODE_LOC_Y(r)  (((r) >> 5) / 3)
#define DECODE_MOVE(r)   ((r) & 0x1F)

typedef struct
{
    int subX, subY, cellX, cellY;
} move_t;

typedef struct
{
    bool winsGame;
    bool losesGame;
    move_t bestOpponentMove;
    int movesToEnd;
} moveAnalysis_t;

static bool selectSubgame_easy(ultimateTTT_t* ttt, int* x, int* y);
static bool selectSubgame_medium(ultimateTTT_t* ttt, int* x, int* y);
static bool selectSubgame_hard(ultimateTTT_t* ttt, int* x, int* y);

static bool selectCell_easy(ultimateTTT_t* ttt, int* x, int* y);
static bool selectCell_medium(ultimateTTT_t* ttt, int* x, int* y);
static bool selectCell_hard(ultimateTTT_t* ttt, int* x, int* y);

static void tttCpuSelectSubgame(ultimateTTT_t* ttt);
static void tttCpuSelectCell(ultimateTTT_t* ttt);

static int hasDiagonal(int x, int y);
static rowCount_t checkRow(const tttPlayer_t game[3][3], int y, tttPlayer_t player);
static rowCount_t checkCol(const tttPlayer_t game[3][3], int x, tttPlayer_t player);
static rowCount_t checkDiag(const tttPlayer_t game[3][3], int n, tttPlayer_t player);

static uint16_t movesToWin(const tttPlayer_t subgame[3][3], tttPlayer_t player);
static uint16_t analyzeSubgame(const tttPlayer_t subgame[3][3], tttPlayer_t player, uint16_t filter);
static bool analyzeMove(const tttSubgame_t subgames[3][3], const move_t* move, tttPlayer_t player,
                        moveAnalysis_t* result);

const char* getMoveName(uint16_t move);
void printGame(const tttPlayer_t subgame[3][3]);

void tttCpuNextMove(ultimateTTT_t* ttt)
{
    if (ttt->game.cpu.state == TCPU_INACTIVE)
    {
        TCPU_LOG("Invactive.");
        return;
    }

    int64_t now = esp_timer_get_time();
    if (now < ttt->game.cpu.delayTime)
    {
        return;
    }

    // Reset the timer for the next one
    ttt->game.cpu.delayTime = now + DELAY_TIME;

    if (ttt->game.cpu.state == TCPU_THINKING)
    {
        TCPU_LOG(">>>>>>>>>>>>>>>>>>>>");
        TCPU_LOG("Thinking...");
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
            TCPU_LOG("CPU doesn't know what to do!");
            switch (ttt->game.cursorMode)
            {
                case NO_CURSOR:
                    TCPU_LOG("cursorMode == NO_CURSOR");
                    break;

                case SELECT_SUBGAME:
                    TCPU_LOG("cursorMode == SELECT_SUBGAME");
                    break;

                case SELECT_CELL:
                    TCPU_LOG("cursorMode == SELECT_CELL");
                    break;

                case SELECT_CELL_LOCKED:
                    TCPU_LOG("cursorMode == SELECT_CELL_LOCKED");
                    break;
            }
        }
    }
    else if (ttt->game.cpu.state == TCPU_MOVING)
    {
        TCPU_LOG("Moving...");

        if (ttt->game.cursorMode == SELECT_SUBGAME)
        {
            TCPU_LOG("Moving to Subgame");
            tttMsgMoveCursor_t payload;
            payload.type              = MSG_MOVE_CURSOR;
            payload.cursorMode        = SELECT_SUBGAME;
            payload.selectedSubgame.x = ttt->game.selectedSubgame.x;
            payload.selectedSubgame.y = ttt->game.selectedSubgame.y;
            payload.cursor.x          = ttt->game.cursor.x;
            payload.cursor.y          = ttt->game.cursor.y;

            do
            {
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
                    payload.cursor.x          = 1;
                    payload.cursor.y          = 1;
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
            TCPU_LOG("Moving to cell");
            tttMsgMoveCursor_t payload;
            payload.type              = MSG_MOVE_CURSOR;
            payload.cursor.x          = ttt->game.cursor.x;
            payload.cursor.y          = ttt->game.cursor.y;
            payload.cursorMode        = ttt->game.cursorMode;
            payload.selectedSubgame.x = ttt->game.selectedSubgame.x;
            payload.selectedSubgame.y = ttt->game.selectedSubgame.y;

            do
            {
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
                    if (!tttCursorIsValid(ttt, &ttt->game.cursor))
                    {
                        TCPU_LOG("??????????????? cursor is not valid????????\n");
                    }
                    // The cursor is in the right place, select it!
                    tttMsgPlaceMarker_t placePayload;
                    placePayload.type              = MSG_PLACE_MARKER;
                    placePayload.selectedSubgame.x = ttt->game.selectedSubgame.x;
                    placePayload.selectedSubgame.y = ttt->game.selectedSubgame.y;
                    placePayload.selectedCell.x    = ttt->game.cpu.destCell.x;
                    placePayload.selectedCell.y    = ttt->game.cpu.destCell.y;

                    ttt->game.cpu.state = TCPU_INACTIVE;

                    tttReceivePlacedMarker(ttt, &placePayload);

                    return;
                }
            } while (!tttCursorIsValid(ttt, &payload.cursor));

            tttReceiveCursor(ttt, &payload);
        }
        else
        {
            TCPU_LOG("Something else???");
        }
    }
}

static bool selectSubgame_easy(ultimateTTT_t* ttt, int* x, int* y)
{
    int availableSubgames[9] = {0};
    int availableCount       = 0;

    for (int idx = 0; idx < 9; idx++)
    {
        if (TTT_NONE == ttt->game.subgames[idx % 3][idx / 3].winner)
        {
            TCPU_LOG("Sub-game %d, %d is a valid target", idx % 3, idx / 3);
            availableSubgames[availableCount++] = idx;
        }
    }

    // Pick a random subgame
    int subgameIndex = availableSubgames[esp_random() % availableCount];

    *x = subgameIndex % 3;
    *y = subgameIndex / 3;
    return true;
}

static bool selectSubgame_medium(ultimateTTT_t* ttt, int* x, int* y)
{
    // Still just random
    return selectSubgame_easy(ttt, x, y);
}

static bool selectSubgame_hard(ultimateTTT_t* ttt, int* x, int* y)
{
    // - Construct a 'subgame' to match the main board (which we can modify if needed to simulate stuff) and also to
    // avoid rewriting the algorithm
    // - Its board will have all the completed subgames marked with the winner, and the rest as NONE (TODO: how to
    // handle a tie??? Just use the TIE player? Yeah probably)
    // - We calculate the next possible move on that board, and then take a look at the board we'd need to win to get
    // our marker there
    // But also check every valid move for a game-winning move, and try that first

    // Who are we?
    tttPlayer_t cpuPlayer = (ttt->game.singlePlayerPlayOrder == GOING_FIRST) ? TTT_P2 : TTT_P1;

    // To avoid a second pass
    int32_t maxScore = INT32_MIN;
    move_t maxMove   = {0};

    // The whole game turned into a subgame
    tttPlayer_t subgame[3][3];
    for (int gx = 0; gx < 3; gx++)
    {
        for (int gy = 0; gy < 3; gy++)
        {
            subgame[gx][gy] = ttt->game.subgames[gx][gy].winner;

            if (subgame[gx][gy] == TTT_NONE)
            {
                for (int cx = 0; cx < 3; cx++)
                {
                    for (int cy = 0; cy < 3; cy++)
                    {
                        int score = INT32_MIN;

                        move_t move = {
                            .subX  = gx,
                            .subY  = gy,
                            .cellX = cx,
                            .cellY = cy,
                        };
                        moveAnalysis_t analysis = {0};

                        if (analyzeMove(ttt->game.subgames, &move, cpuPlayer, &analysis))
                        {
                            if (analysis.winsGame)
                            {
                                score = INT32_MAX;
                            }
                            else if (analysis.losesGame)
                            {
                                score = INT32_MIN + 1;
                            }
                            else
                            {
                                score = 1;
                            }
                        }
                        else
                        {
                            // Impossible move, give it minimum score possible
                            score = INT32_MIN;
                        }

                        if (score > maxScore)
                        {
                            maxScore      = score;
                            maxMove.subX  = gx;
                            maxMove.subY  = gy;
                            maxMove.cellX = cx;
                            maxMove.cellY = cy;
                        }
                    }
                }
            }
        }
    }

    uint16_t mainResult = analyzeSubgame(subgame, cpuPlayer, 0);
    uint16_t mainMove   = DECODE_MOVE(mainResult);
    int mainX           = DECODE_LOC_X(mainResult);
    int mainY           = DECODE_LOC_Y(mainResult);

    // Override the best subgame if there's a winning move
    if (maxScore == INT32_MAX)
    {
        mainX = maxMove.subX;
        mainY = maxMove.subY;
    }

    if (WON != mainMove && mainMove)
    {
        // X/Y must have been set

        *x = mainX;
        *y = mainY;

        return true;
    }

    return false;
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
        TCPU_LOG("Selecting subgame %d, %d", x, y);
        ttt->game.cpu.destSubgame.x = x;
        ttt->game.cpu.destSubgame.y = y;
        ttt->game.cpu.state         = TCPU_MOVING;

        // Set the cursor up in this mode in order to move the state along
        tttMsgMoveCursor_t payload;
        payload.type              = MSG_MOVE_CURSOR;
        payload.cursorMode        = SELECT_SUBGAME;
        payload.selectedSubgame.x = ttt->game.selectedSubgame.x;
        payload.selectedSubgame.y = ttt->game.selectedSubgame.y;
        payload.cursor.x          = ttt->game.cursor.x;
        payload.cursor.y          = ttt->game.cursor.y;
        tttReceiveCursor(ttt, &payload);
    }
}

static bool selectCell_easy(ultimateTTT_t* ttt, int* x, int* y)
{
    int availableCells[9] = {0};
    int availableCount    = 0;

    // Pick a random cell!
    tttSubgame_t* subgame = &ttt->game.subgames[ttt->game.selectedSubgame.x][ttt->game.selectedSubgame.y];
    for (int idx = 0; idx < 9; idx++)
    {
        if (TTT_NONE == subgame->game[idx % 3][idx / 3])
        {
            TCPU_LOG("Cell %d, %d is a valid target", idx % 3, idx / 3);
            availableCells[availableCount++] = idx;
        }
    }

    int cellIndex = availableCells[esp_random() % availableCount];

    *x = cellIndex % 3;
    *y = cellIndex / 3;
    return true;
}

static bool selectCell_medium(ultimateTTT_t* ttt, int* x, int* y)
{
    // Medium plays perfectly within a subgame, but has no overall strategy and picks randomly when selecting a subgame
    tttPlayer_t cpuPlayer = (ttt->game.singlePlayerPlayOrder == GOING_FIRST) ? TTT_P2 : TTT_P1;
    tttSubgame_t* subgame = &ttt->game.subgames[ttt->game.selectedSubgame.x][ttt->game.selectedSubgame.y];
    uint16_t result       = analyzeSubgame(subgame->game, cpuPlayer, 0);
    uint16_t move         = DECODE_MOVE(result);
    int moveX             = DECODE_LOC_X(result);
    int moveY             = DECODE_LOC_Y(result);

    switch (move)
    {
        case WON:
            return false;

        case WIN:
        case BLOCK:
        case FORK:
        case BLOCK_FORK:
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

static bool selectCell_hard(ultimateTTT_t* ttt, int* x, int* y)
{
    // For selecting the next move within a subgame, we would want to take into account the current subgame, but...
    // depending on the score of the next action, we have some freedom. Basically we don't need to worry _too_ much
    // about playing the cell perfectly and instead we can base the decision mostly on where the opponent will go.
    // Obviously if we want to win this subgame (maybe it isn't necessary!) you first would want to take into account
    // any wins/blocks, but anything past that isn't going to matter, especially since your opponent doesn't necessarily
    // get another turn in the same square immediately.
    // So, once any wins/blocks in this cell are considered, you would then rank each of the most favorable moves within
    // this cell against which one has the most favorable score for the opponent
    // Somehow those two priorities will need to be traded off -- probably should make that configurable.

    /////////////
    // Ok, so what we have here is pretty good. It actually strategizes!
    // But it can be better. What it needs to do instead of the weird scoring that doesn't take into account easy
    // winning... Is to score the cell we pick, we then go to the corresponding subgame, and try every cell for the
    // opponent Then _that_ is what we score the result of, considering the worst possible one (highest score for
    // opponent) of all that subgame's playable cells. And then we use new scoring too. So, winning the subgame should
    // be like -100 (unless we don't care?) and winning the whole game should be like -1000 On the other hand, getting
    // to pick our next subgame should be like a +50? or maybe it just inherits the score of the best possible result of
    // all subgames Should the 'next move' scoring be considered? Ok....

    // The score of the opponent's ideal next move in each subgame
    uint16_t opponentScore[3][3];
    uint16_t maxScore = 0;
#define SCORE_MAX 0xFFFF

    tttPlayer_t cpuPlayer   = (ttt->game.singlePlayerPlayOrder == GOING_FIRST) ? TTT_P2 : TTT_P1;
    tttPlayer_t otherPlayer = (cpuPlayer == TTT_P1) ? TTT_P2 : TTT_P1;
    tttSubgame_t* subgame   = &ttt->game.subgames[ttt->game.selectedSubgame.x][ttt->game.selectedSubgame.y];

    for (int ix = 0; ix < 3; ix++)
    {
        for (int iy = 0; iy < 3; iy++)
        {
            tttSubgame_t* oppSubgame = &ttt->game.subgames[ix][iy];

            if (subgame->game[ix][iy] != TTT_NONE)
            {
                // Invalid position, don't bother scoring it
                continue;
            }

            // Check if placing this cell wins us the entire game
            tttPlayer_t tempSubgame[3][3];
            memcpy(tempSubgame, oppSubgame->game, sizeof(tempSubgame));

            // Place the marker we would have added it this turn
            tempSubgame[ix][iy] = cpuPlayer;

            tttPlayer_t winner = tttCheckWinner(tempSubgame);
            if (winner == cpuPlayer)
            {
                // Hey, making this move wins us the subgame!
                // Check if that subgame wins us the whole game too!
                tttPlayer_t tempMetagame[3][3];
                for (int jx = 0; jx < 3; jx++)
                {
                    for (int jy = 0; jy < 3; jy++)
                    {
                        tempMetagame[jx][jy] = ttt->game.subgames[jx][jy].winner;
                    }
                }
                tempMetagame[ix][iy] = cpuPlayer;
                if (tttCheckWinner(tempMetagame) == cpuPlayer)
                {
                    // This is a game-winning move, so immediately decide to move there
                    *x = ix;
                    *y = iy;
                    return true;
                }
            }

            // The corresponding subgame to this square has a winner, so the opponent gets to go anywhere
            if (oppSubgame->winner != TTT_NONE)
            {
                opponentScore[ix][iy] = SCORE_MAX;
                continue;
            }

            if (subgame == oppSubgame)
            {
                // This cell is the current cell, which we analyzed earlier, so use that result
                // Check if we caused the subgame to end (i.e. in a win or draw)
                if (winner != TTT_NONE)
                {
                    // Opponent gets to pick any board they want, they get the max score
                    opponentScore[ix][iy] = SCORE_MAX;
                }
                else
                {
                    // Opponent is locked to this board, so use their score for the subgame
                    opponentScore[ix][iy] = DECODE_MOVE(analyzeSubgame(tempSubgame, otherPlayer, 0));
                    opponentScore[ix][iy] += 50 - 10 * movesToWin(tempSubgame, otherPlayer);
                }
            }
            else
            {
                // Score the current state of that board as the opponent
                opponentScore[ix][iy] = DECODE_MOVE(analyzeSubgame(oppSubgame->game, otherPlayer, 0));
                opponentScore[ix][iy] += 50 - 10 * movesToWin(tempSubgame, otherPlayer);
            }

            // Update the max score, used as the value when the opponent can pick any subgame
            if (opponentScore[ix][iy] != SCORE_MAX && opponentScore[ix][iy] > maxScore)
            {
                maxScore = opponentScore[ix][iy];
            }
        }
    }

#ifdef DEBUG_TTT_CPU
    int minOppX          = 0;
    int minOppY          = 0;
    uint16_t minOppScore = SCORE_MAX;
    for (int ix = 0; ix < 3; ix++)
    {
        for (int iy = 0; iy < 3; iy++)
        {
            if (subgame->game[ix][iy] == TTT_NONE && opponentScore[ix][iy] < minOppScore)
            {
                minOppScore = opponentScore[ix][iy];
                minOppX     = ix;
                minOppY     = iy;
            }
        }
    }
#endif

    // Just some debugging
    TCPU_LOG("--------------------\n");
    TCPU_LOG(" In subgame (%" PRIu32 ", %" PRIu32 "):\n", ttt->game.selectedSubgame.x, ttt->game.selectedSubgame.y);
    TCPU_LOG("[-------] - Invalid move\n");
    TCPU_LOG("[xx /.. ] - Our score for the current subgame if this cell is selected\n");
    TCPU_LOG("[.. /xx ] - Opponent's score for subgame corresponding to this cell\n");
    TCPU_LOG("[xx*/.. ] - Ideal subgame move\n");
    TCPU_LOG("[.. /xx^] - Least favorable for opponent\n");

    int16_t combinedScores[3][3];
    int16_t maxCombScore = INT16_MIN;
    int maxCombX         = 0;
    int maxCombY         = 0;

    for (int iy = 0; iy < 3; iy++)
    {
        for (int ix = 0; ix < 3; ix++)
        {
            if (subgame->game[ix][iy] != TTT_NONE)
            {
                combinedScores[ix][iy] = INT16_MIN;

                // Not valid
                TCPU_LOG("[----]   ");
                continue;
            }

            tttPlayer_t simulation[3][3];
            memcpy(simulation, subgame->game, sizeof(simulation));
            simulation[ix][iy] = cpuPlayer;

            uint16_t thisCellAnalysis = analyzeSubgame(simulation, cpuPlayer, 0);
            uint16_t thisCellScore    = DECODE_MOVE(thisCellAnalysis);
            thisCellScore += 50 - 10 * movesToWin(simulation, cpuPlayer);

            uint16_t score = opponentScore[ix][iy];
            if (opponentScore[ix][iy] == SCORE_MAX)
            {
                score = maxScore;
            }

            combinedScores[ix][iy] = ((int16_t)thisCellScore - (int16_t)score);

            if (combinedScores[ix][iy] > maxCombScore)
            {
                maxCombScore = combinedScores[ix][iy];
                maxCombX     = ix;
                maxCombY     = iy;
            }

#ifdef DEBUG_TTT_CPU
            uint16_t result = analyzeSubgame(subgame->game, cpuPlayer, 0);
            int moveX       = DECODE_LOC_X(result);
            int moveY       = DECODE_LOC_Y(result);

            char oppFlag = ' ';
            if (ix == minOppX && iy == minOppY)
            {
                oppFlag = '^';
            }

            char playerFlag = ' ';
            if (ix == moveX && iy == moveY)
            {
                playerFlag = '*';
            }
            // TCPU_LOG("[%02" PRIu16 "%c/%02" PRIu16 "%c]   ", thisCellScore, playerFlag, score, oppFlag);
            TCPU_LOG("[%02" PRId16 "%c%c]   ", ((int16_t)thisCellScore - (int16_t)score), playerFlag, oppFlag);
#endif
        }
    }

    *x = maxCombX;
    *y = maxCombY;
    return true;
}

static void tttCpuSelectCell(ultimateTTT_t* ttt)
{
    TCPU_LOG("CPU selecting next cell...");

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
        TCPU_LOG("Selecting cell %d, %d", x, y);
        ttt->game.cpu.destCell.x = x;
        ttt->game.cpu.destCell.y = y;
        ttt->game.cpu.state      = TCPU_MOVING;
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
    int noneCount           = 0;
    int playerCount         = 0;
    int opponentCount       = 0;
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
    int noneCount           = 0;
    int playerCount         = 0;
    int opponentCount       = 0;
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
    int noneCount           = 0;
    int playerCount         = 0;
    int opponentCount       = 0;
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

// So naively scoring based on just the moves to make isn't enough
// - Move hierarchy doesn't sufficiently capture the value of a move
// - You have to think about not just how that move advances, but also how many moves away from winning we are
static uint16_t movesToWin(const tttPlayer_t subgame[3][3], tttPlayer_t player)
{
    /* uint16_t movesToWin = 0;
    tttPlayer_t result  = */
    tttCheckWinner(subgame);
    tttPlayer_t simulation[3][3];

    memcpy(simulation, subgame, sizeof(simulation));

#ifdef DEBUG_TTT_CPU
    TCPU_LOG(">>> Moves to win game:\n");
    printGame(subgame);
    TCPU_LOG("---\n");
#endif

    uint16_t minMovesToWin = 5;

    for (int y = 0; y < 3; y++)
    {
        rowCount_t rowCount  = checkRow(subgame, y, player);
        uint16_t movesNeeded = 3 - TO_PLAYERS(rowCount);
        if (0 == TO_OPPONENTS(rowCount) && movesNeeded < minMovesToWin)
        {
            minMovesToWin = movesNeeded;
        }
    }

    for (int x = 0; x < 3; x++)
    {
        rowCount_t rowCount  = checkCol(subgame, x, player);
        uint16_t movesNeeded = 3 - TO_PLAYERS(rowCount);
        if (0 == TO_OPPONENTS(rowCount) && movesNeeded < minMovesToWin)
        {
            minMovesToWin = movesNeeded;
        }
    }

    for (int n = 0; n < 2; n++)
    {
        rowCount_t rowCount  = checkDiag(subgame, n, player);
        uint16_t movesNeeded = 3 - TO_PLAYERS(rowCount);
        if (0 == TO_OPPONENTS(rowCount) && movesNeeded < minMovesToWin)
        {
            minMovesToWin = movesNeeded;
        }
    }

    return minMovesToWin;
}

static uint16_t analyzeSubgame(const tttPlayer_t subgame[3][3], tttPlayer_t player, uint16_t filter)
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

#define CHECK_FILTER(v) (!filter || (filter > (v)))
    tttPlayer_t otherPlayer = (player == TTT_P1) ? TTT_P2 : TTT_P1;

    // 0. Check for already won
    if (CHECK_FILTER(WON) && player == tttCheckWinner(subgame))
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

            if (CHECK_FILTER(WIN) && TO_PLAYERS(counts) == 2 && TO_NONES(counts) == 1)
            {
                return WIN | ENCODE_LOC(((subgame[0][y] == TTT_NONE) ? 0 : ((subgame[1][y] == TTT_NONE) ? 1 : 2)), y);
            }

            if (CHECK_FILTER(BLOCK) && TO_OPPONENTS(counts) == 2 && TO_NONES(counts) == 1)
            {
                return BLOCK | ENCODE_LOC(((subgame[0][y] == TTT_NONE) ? 0 : ((subgame[1][y] == TTT_NONE) ? 1 : 2)), y);
            }
        }

        // Verticals
        for (int x = 0; x < 3; x++)
        {
            counts = checkCol(subgame, x, player);

            if (CHECK_FILTER(WIN) && TO_PLAYERS(counts) == 2 && TO_NONES(counts) == 1)
            {
                return WIN | ENCODE_LOC(x, ((subgame[x][0] == TTT_NONE) ? 0 : ((subgame[x][1] == TTT_NONE) ? 1 : 2)));
            }

            if (CHECK_FILTER(BLOCK) && TO_OPPONENTS(counts) == 2 && TO_NONES(counts) == 1)
            {
                return BLOCK | ENCODE_LOC(x, ((subgame[x][0] == TTT_NONE) ? 0 : ((subgame[x][1] == TTT_NONE) ? 1 : 2)));
            }
        }

        // Diagonals

        // Top-left to bottom-right diagonal
        for (int n = 0; n < 2; n++)
        {
            counts  = checkDiag(subgame, n + 1, player);
            int loc = (subgame[0][n ? 2 : 0] == TTT_NONE) ? 0 : ((subgame[1][1] == TTT_NONE) ? 1 : 2);

            if (CHECK_FILTER(WIN) && TO_PLAYERS(counts) == 2 && TO_NONES(counts) == 1)
            {
                return WIN | ENCODE_LOC(loc, loc);
            }

            if (CHECK_FILTER(BLOCK) && TO_OPPONENTS(counts) == 2 && TO_NONES(counts) == 1)
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
                diags[n - 1] = checkDiag(subgame, n, player);
            }
        }

        if (CHECK_FILTER(FORK))
        {
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
        }

        if (CHECK_FILTER(BLOCK_FORK))
        {
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
    }

    // 5. Center: Play the center.
    if (CHECK_FILTER(CENTER) && TTT_NONE == subgame[1][1])
    {
        return CENTER | ENCODE_LOC(1, 1);
    }

    // 6. Opposite Corner: If the opponent is in the corner, play the opposite corner.
    if (CHECK_FILTER(OPPOSITE_CORNER))
    {
        for (int n = 0; n < 4; n++)
        {
            int x = (n % 2) * 2;
            int y = (n / 2) * 2;
            if (otherPlayer == subgame[x][y] && TTT_NONE == subgame[2 - x][2 - y])
            {
                return OPPOSITE_CORNER | ENCODE_LOC(2 - x, 2 - y);
            }
        }
    }

    if (CHECK_FILTER(EMPTY_CORNER))
    {
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
    }

    if (CHECK_FILTER(EMPTY_SIDE))
    {
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
    }

    TCPU_LOG("We should have picked a move already? Not sure this is possible");
    return 0;
}

static bool analyzeMove(const tttSubgame_t subgames[3][3], const move_t* move, tttPlayer_t player,
                        moveAnalysis_t* result)
{
    tttSubgame_t board[3][3];
    if (subgames[move->subX][move->subY].winner != TTT_NONE
        || subgames[move->subX][move->subY].game[move->cellX][move->cellY] != TTT_NONE)
    {
        // Impossible move!
        return false;
    }

    memcpy(board, subgames, sizeof(board));

    const tttPlayer_t opponent = (player == TTT_P1) ? TTT_P2 : TTT_P1;
    tttPlayer_t turn           = player;
    move_t lastMove            = {0};
    memcpy(&lastMove, move, sizeof(move_t));
    int moveNum = 0;

    board[lastMove.subX][lastMove.subY].game[lastMove.cellX][lastMove.cellY] = player;

    // Check if that wins the cell and update it accordingly
    tttPlayer_t subgameWinner = tttCheckWinner(board[lastMove.subX][lastMove.subY].game);
    if (subgameWinner != TTT_NONE)
    {
        board[lastMove.subX][lastMove.subY].winner = subgameWinner;
    }

    // Get the overall board to check the game's status
    tttPlayer_t bigBoard[3][3];
    for (int gx = 0; gx < 3; gx++)
    {
        for (int gy = 0; gy < 3; gy++)
        {
            bigBoard[gx][gy] = board[gx][gy].winner;
        }
    }

    tttPlayer_t winner = tttCheckWinner(bigBoard);
    if (winner == player)
    {
        result->winsGame   = true;
        result->losesGame  = false;
        result->movesToEnd = moveNum;
        memset(&result->bestOpponentMove, 0, sizeof(move_t));
        return true;
    }
    else if (winner == opponent)
    {
        result->winsGame   = false;
        result->losesGame  = true;
        result->movesToEnd = moveNum;
        memcpy(&result->bestOpponentMove, &lastMove, sizeof(move_t));
        return true;
    }
    else
    {
        // TTT_NONE
        result->winsGame  = false;
        result->losesGame = false;
        return true;
    }

    return false;
}

const char* getMoveName(uint16_t move)
{
    switch (move)
    {
        case WON:
            return strMoveWon;
        // Game to win
        case WIN:
            return strMoveWin;
        case BLOCK:
            return strMoveBlock;
        case FORK:
            return strMoveFork;
        case BLOCK_FORK:
            return strMoveBlockFork;
        case CENTER:
            return strMoveCenter;
        case OPPOSITE_CORNER:
            return strMoveOppositeCenter;
        case EMPTY_CORNER:
            return strMoveEmptyCorner;
        case EMPTY_SIDE:
            return strMoveEmptySide;
        default:
            return "?";
    }
}

void printGame(const tttPlayer_t subgame[3][3])
{
    char buf[22];
    char* out = buf;

    for (int y = 0; y < 3; y++)
    {
        for (int x = 0; x < 3; x++)
        {
            switch (subgame[x][y])
            {
                case TTT_NONE:
                    *out++ = '_';
                    break;

                case TTT_P1:
                    *out++ = 'X';
                    break;

                case TTT_P2:
                    *out++ = 'O';
                    break;

                case TTT_DRAW:
                    *out++ = '?';
                    break;
            }

            *out++ = ' ';
        }

        *out++ = '\n';
    }
    *out = '\0';
    TCPU_LOG("%s", buf);
}