#include "dn_p2p.h"

//==============================================================================
// Variables
//==============================================================================

static const char* conStartedStrs[] = {
    "Connection Started.",
    "Hold two Swadges",
    "close together.",
};
static const char* conRxStartAck[] = {
    "RX Start Ack Received",
};
static const char* conRxStartMsg[] = {
    "RX Start Msg Received",
};

//==============================================================================
// Functions
//==============================================================================

void dn_DrawConnecting(dn_gameData_t* gameData, int64_t elapsedUs)
{
    // Draw the background
    drawMenuMania(gameData->bgMenu, gameData->menuRenderer, elapsedUs);

    // Spacing between lines
    int16_t ySpacing = 8;

    // Center text vertically under the title
    int16_t tHeight = (gameData->numConStrs * gameData->font_ibm.height) + ((gameData->numConStrs - 1) * ySpacing);
    int16_t yOff    = MANIA_TITLE_HEIGHT + (MANIA_BODY_HEIGHT - tHeight) / 2;

    // Draw the connection strings, centered
    for (int16_t tIdx = 0; tIdx < gameData->numConStrs; tIdx++)
    {
        int16_t tWidth = textWidth(&gameData->font_ibm, gameData->conStrs[tIdx]);
        drawText(&gameData->font_ibm, c000, gameData->conStrs[tIdx], (TFT_WIDTH - tWidth) / 2, yOff);
        yOff += (gameData->font_ibm.height + ySpacing);
    }
}

/**
 * @brief Handle input when showing the connection UI
 *
 * @param data The entire game state
 * @param evt The button event
 */
void dn_HandleConnectingInput(dn_gameData_t* data, buttonEvt_t* evt)
{
    if (evt->down && PB_B == evt->button)
    {
        // Cancel the connection and return to the main menu
        p2pDeinit(&data->p2p);
        dn_ShowUi(UI_MENU);
    }
}

/**
 * @brief Handle connection events and update the UI
 *
 * @param ttt The entire game state
 * @param evt The connection event
 */
void dn_HandleCon(dn_gameData_t* gameData, connectionEvt_t evt)
{
    // Pick a new string for each connection state
    switch (evt)
    {
        case CON_STARTED:
        {
            gameData->conStrs    = conStartedStrs;
            gameData->numConStrs = ARRAY_SIZE(conStartedStrs);
            break;
        }
        case RX_GAME_START_ACK:
        {
            gameData->conStrs    = conRxStartAck;
            gameData->numConStrs = ARRAY_SIZE(conRxStartAck);
            break;
        }
        case RX_GAME_START_MSG:
        {
            gameData->conStrs    = conRxStartMsg;
            gameData->numConStrs = ARRAY_SIZE(conRxStartMsg);
            break;
        }
        case CON_ESTABLISHED:
        {
            //tttBeginGame(ttt);
            dn_ShowUi(UI_GAME);
            break;
        }
        case CON_LOST:
        {
            // Disconnected, show that UI
            gameData->lastResult = DN_RESULT_DISCONNECT;
            dn_ShowUi(UI_RESULT);
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
void dn_HandleMsgRx(dn_gameData_t* gameData, const uint8_t* payload, uint8_t len)
{
    // Make sure there is a type to switch on
    if (len < 1)
    {
        return;
    }

    // Handle incoming messages
    // switch (payload[0])
    // {
    //     case MSG_SELECT_MARKER:
    //     {
    //         // Validate length
    //         if (len == sizeof(tttMsgSelectMarker_t))
    //         {
    //             tttReceiveMarker(ttt, (const tttMsgSelectMarker_t*)payload);
    //         }
    //         break;
    //     }
    //     case MSG_MOVE_CURSOR:
    //     {
    //         // Length check
    //         if (len == sizeof(tttMsgMoveCursor_t))
    //         {
    //             tttReceiveCursor(ttt, (const tttMsgMoveCursor_t*)payload);
    //         }
    //         break;
    //     }
    //     case MSG_PLACE_MARKER:
    //     {
    //         // Length check
    //         if (len == sizeof(tttMsgPlaceMarker_t))
    //         {
    //             tttReceivePlacedMarker(ttt, (const tttMsgPlaceMarker_t*)payload);
    //         }
    //         break;
    //     }
    // }
}

/**
 * @brief Callback after a P2P message is sent
 *
 * @param ttt The entire game state
 * @param status The status of the transmission
 * @param data The data that was transmitted
 * @param len The length of the data that was transmitted
 */
void dn_HandleMsgTx(dn_gameData_t* gameData, messageStatus_t status, const uint8_t* data, uint8_t len)
{
    // Check transmission status
    switch (status)
    {
        case MSG_ACKED:
        {
            // Cool, move along
            break;
        }
        default:
        case MSG_FAILED:
        {
            // Message failure means disconnection
            gameData->lastResult = DN_RESULT_DISCONNECT;
            dn_ShowUi(UI_RESULT);
            break;
        }
    }
}