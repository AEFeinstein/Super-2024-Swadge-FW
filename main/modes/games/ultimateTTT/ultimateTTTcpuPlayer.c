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
        TCPU_LOG("TTT", "Thinking...");
        if (ttt->game.cursorMode == SELECT_SUBGAME)
        {
            TCPU_LOG("TTT", "CPU selecting next subgame...");
            int availableSubgames[9] = {0};
            int availableCount = 0;

            for (int idx = 0; idx < 9; idx++)
            {
                if (TTT_NONE == ttt->game.subgames[idx / 3][idx % 3].winner)
                {
                    TCPU_LOG("TTT", "Sub-game %d, %d is a valid target", idx / 3, idx % 3);
                    availableSubgames[availableCount++] = idx;
                }
            }

            // Pick a random subgame
            int subgameIndex = availableSubgames[esp_random() % availableCount];

            TCPU_LOG("TTT", "Selecting subgame %d, %d", subgameIndex / 3, subgameIndex % 3);

            ttt->game.cpu.destSubgame.x = subgameIndex / 3;
            ttt->game.cpu.destSubgame.y = subgameIndex % 3;
            ttt->game.cpu.state = TCPU_MOVING;

            tttMsgMoveCursor_t payload;
            payload.type = MSG_MOVE_CURSOR;
            payload.cursorMode = SELECT_SUBGAME;
            payload.selectedSubgame.x = ttt->game.selectedSubgame.x;
            payload.selectedSubgame.y = ttt->game.selectedSubgame.y;
            payload.cursor.x = ttt->game.cursor.x;
            payload.cursor.y = ttt->game.cursor.y;
            tttReceiveCursor(ttt, &payload);
        }
        else if (ttt->game.cursorMode == SELECT_CELL || ttt->game.cursorMode == SELECT_CELL_LOCKED)
        {
            TCPU_LOG("TTT", "CPU selecting next cell...");
            int availableCells[9] = {0};
            int availableCount = 0;

            // Pick a random cell!
            for (int idx = 0; idx < 9; idx++)
            {
                tttSubgame_t* subgame = &ttt->game.subgames[ttt->game.selectedSubgame.x][ttt->game.selectedSubgame.y];
                if (TTT_NONE == subgame->game[idx / 3][idx % 3])
                {
                    availableCells[availableCount++] = idx;
                }
            }

            int cellIndex = availableCells[esp_random() % availableCount];
            TCPU_LOG("TTT", "Selecting cell %d, %d", cellIndex / 3, cellIndex % 3);

            ttt->game.cpu.destCell.x = cellIndex / 3;
            ttt->game.cpu.destCell.y = cellIndex % 3;
            ttt->game.cpu.state = TCPU_MOVING;
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
                if (payload.selectedSubgame.x < ttt->game.cpu.destSubgame.x && payload.selectedSubgame.x < 2)
                {
                    payload.selectedSubgame.x++;
                }
                else if (payload.selectedSubgame.y < ttt->game.cpu.destSubgame.y && payload.selectedSubgame.y < 2)
                {
                    payload.selectedSubgame.y++;
                }
                else if (payload.selectedSubgame.x > ttt->game.cpu.destSubgame.x && payload.selectedSubgame.x > 0)
                {
                    payload.selectedSubgame.x--;
                }
                else if (payload.selectedSubgame.y > ttt->game.cpu.destSubgame.y && payload.selectedSubgame.y > 0)
                {
                    payload.selectedSubgame.y--;
                }
                else
                {
                    // We're in the right place already!
                    payload.cursorMode = SELECT_CELL;
                    // TODO don't select an invalid cell
                    payload.cursor.x = 1;
                    payload.cursor.y = 1;
                }
            } while (ttt->game.subgames[payload.selectedSubgame.x][payload.selectedSubgame.y].winner != TTT_NONE);

            payload.cursor.x = payload.selectedSubgame.x;
            payload.cursor.y = payload.selectedSubgame.y;
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
                    placePayload.selectedSubgame = ttt->game.selectedSubgame;
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
