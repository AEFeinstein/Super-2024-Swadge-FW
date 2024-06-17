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
 * @brief Handle input when showing the connection UI
 *
 * @param ttt The entire game state
 * @param evt The button event
 */
void tttHandleConnectingInput(ultimateTTT_t* ttt, buttonEvt_t* evt)
{
    if (evt->down && PB_B == evt->button)
    {
        // Cancel the connection and return to the main menu
        p2pDeinit(&ttt->p2p);
        tttShowUi(TUI_MENU);
    }
}

/**
 * @brief Draw the marker selection UI
 *
 * @param ttt The entire game state
 * @param elapsedUs The time elapsed since this was last called
 */
void tttDrawConnecting(ultimateTTT_t* ttt, int64_t elapsedUs)
{
    // Draw the background
    drawMenuMania(ttt->bgMenu, ttt->menuRenderer, elapsedUs);

    // Draw the connection string, centered
    int16_t tWidth = textWidth(&ttt->font_rodin, ttt->conStr);
    drawText(&ttt->font_rodin, c000, ttt->conStr, (TFT_WIDTH - tWidth) / 2, TFT_HEIGHT / 2);
}

/**
 * @brief Handle connection events and update the UI
 *
 * @param ttt The entire game state
 * @param evt The connection event
 */
void tttHandleCon(ultimateTTT_t* ttt, connectionEvt_t evt)
{
    // Pick a new string for each connection state
    // TODO replace with percentage?
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
 * @brief Handle a received P2P message
 *
 * @param ttt The entire game state
 * @param payload The message received
 * @param len THe length of the message received
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
            // Validate length
            if (len == sizeof(tttMsgSelectPiece_t))
            {
                tttReceiveMarker(ttt, (const tttMsgSelectPiece_t*)payload);
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
 * @brief Callback after a P2P message is sent
 *
 * @param ttt The entire game state
 * @param status The status of the transmission
 * @param data The data that was transmitted
 * @param len The length of the data that was transmitted
 */
void tttHandleMsgTx(ultimateTTT_t* ttt, messageStatus_t status, const uint8_t* data, uint8_t len)
{
    // TODO do something with the transmission status?
}
