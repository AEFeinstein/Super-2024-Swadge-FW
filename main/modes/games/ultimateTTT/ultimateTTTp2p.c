//==============================================================================
// Includes
//==============================================================================

#include "ultimateTTTp2p.h"

//==============================================================================
// Variables
//==============================================================================

static const char conStartedStr[] = "Connection Started";
static const char conRxStartAck[] = "RX Start Ack Received";
static const char conRxStartMsg[] = "RX Start Msg Received";
static const char conLostStr[]    = "Connection Lost";

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief TODO
 *
 */
void tttHandleConnectingInput(ultimateTTT_t* ttt, buttonEvt_t* evt)
{
    if (evt->down && PB_B == evt->button)
    {
        p2pDeinit(&ttt->p2p);
        ttt->ui = TUI_MENU;
    }
}

/**
 * @brief TODO
 *
 * @param ttt
 */
void tttDrawConnecting(ultimateTTT_t* ttt)
{
    clearPxTft();
    drawText(&ttt->font_rodin, c555, ttt->conStr, 40, 40);
}

/**
 * @brief TODO
 *
 * @param ttt
 * @param evt
 */
void tttHandleCon(ultimateTTT_t* ttt, connectionEvt_t evt)
{
    // TODO handle connection states and disconnection
    switch (evt)
    {
        case CON_STARTED:
        {
            ttt->conStr = conStartedStr;
            break;
        }
        case RX_GAME_START_ACK:
        {
            ttt->conStr = conRxStartAck;
            break;
        }
        case RX_GAME_START_MSG:
        {
            ttt->conStr = conRxStartMsg;
            break;
        }
        case CON_ESTABLISHED:
        {
            tttBeginGame(ttt);
            break;
        }
        case CON_LOST:
        {
            ttt->conStr = conLostStr;
            break;
        }
    }
}

/**
 * @brief TODO
 *
 * @param ttt
 * @param payload
 * @param len
 */
void tttHandleMsgRx(ultimateTTT_t* ttt, const uint8_t* payload, uint8_t len)
{
    // Make sure there is a type to switch on
    if (len < 1)
    {
        return;
    }

    // Handle incoming messages
    switch (payload[0])
    {
        case MSG_SELECT_PIECE:
        {
            if (len == sizeof(tttMsgSelectPiece_t))
            {
                const tttMsgSelectPiece_t* rxSel = (const tttMsgSelectPiece_t*)payload;

                // If this is the second player
                if (GOING_SECOND == p2pGetPlayOrder(&ttt->p2p))
                {
                    // Save p1's piece
                    ttt->p1PieceIdx = rxSel->pieceIdx;

                    // Send p2's piece to p1
                    ttt->p2PieceIdx = ttt->activePieceIdx;

                    // Send sprite selection to other swadge
                    tttMsgSelectPiece_t txSel = {
                        .type     = MSG_SELECT_PIECE,
                        .pieceIdx = ttt->p2PieceIdx,
                    };
                    p2pSendMsg(&ttt->p2p, (const uint8_t*)&txSel, sizeof(txSel), tttMsgTxCbFn);

                    // Wait for p1 to make the first move
                    ttt->state = TGS_WAITING;
                }
                else // Going first
                {
                    // Received p2's piece
                    ttt->p2PieceIdx = rxSel->pieceIdx;

                    // Make the first move
                    ttt->state = TGS_PLACING_PIECE;
                }
            }
            break;
        }
        case MSG_MOVE_CURSOR:
        {
            // Length check
            if (len == sizeof(tttMsgMoveCursor_t))
            {
                tttReceiveCursor(ttt, (const tttMsgMoveCursor_t*)payload);
            }
            break;
        }
        case MSG_PLACE_PIECE:
        {
            // Length check
            if (len == sizeof(tttMsgPlacePiece_t))
            {
                tttReceivePlacedPiece(ttt, (const tttMsgPlacePiece_t*)payload);
            }
            break;
        }
    }
}

/**
 * @brief TODO
 *
 * @param ttt
 * @param status
 * @param data
 * @param len
 */
void tttHandleMsgTx(ultimateTTT_t* ttt, messageStatus_t status, const uint8_t* data, uint8_t len)
{
    // TODO
}
