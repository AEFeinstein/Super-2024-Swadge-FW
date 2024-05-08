//==============================================================================
// Includes
//==============================================================================

#include "ringsAndGems.h"
#include "ringsAndGemsGame.h"

//==============================================================================
// Function Prototypes
//==============================================================================

static void ragEnterMode(void);
static void ragExitMode(void);
static void ragMainLoop(int64_t elapsedUs);

static void ragEspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi);
static void ragEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status);
static void ragConCb(p2pInfo* p2p, connectionEvt_t evt);
static void ragMsgRxCb(p2pInfo* p2p, const uint8_t* payload, uint8_t len);

//==============================================================================
// Variables
//==============================================================================

// It's good practice to declare immutable strings as const so they get placed in ROM, not RAM
static const char ragName[] = "Rings and Gems";

swadgeMode_t ragMode = {
    .modeName                 = ragName,
    .wifiMode                 = ESP_NOW,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = ragEnterMode,
    .fnExitMode               = ragExitMode,
    .fnMainLoop               = ragMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = NULL,
    .fnEspNowRecvCb           = ragEspNowRecvCb,
    .fnEspNowSendCb           = ragEspNowSendCb,
    .fnAdvancedUSB            = NULL,
    .fnDacCb                  = NULL,
};

ringsAndGems_t* rag;

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief TODO
 *
 */
static void ragEnterMode(void)
{
    // Allocate memory for the mode
    rag = calloc(1, sizeof(ringsAndGems_t));

    rag->cursorMode = SELECT_SUBGAME;

    loadWsg("x_small.wsg", &rag->piece_x_small, true);
    loadWsg("x_large.wsg", &rag->piece_x_big, true);
    loadWsg("o_small.wsg", &rag->piece_o_small, true);
    loadWsg("o_large.wsg", &rag->piece_o_big, true);

    // Initialize and start p2p
    p2pInitialize(&rag->p2p, 0x25, ragConCb, ragMsgRxCb, -70);
    p2pStartConnection(&rag->p2p);
}

/**
 * @brief TODO
 *
 */
static void ragExitMode(void)
{
    // Free memory
    freeWsg(&rag->piece_x_small);
    freeWsg(&rag->piece_x_big);
    freeWsg(&rag->piece_o_small);
    freeWsg(&rag->piece_o_big);

    // Deinitialize p2p
    p2pDeinit(&rag->p2p);

    // Free memory
    free(rag);
}

/**
 * @brief TODO
 *
 * @param elapsedUs
 */
static void ragMainLoop(int64_t elapsedUs)
{
    // Handle inputs
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        switch (rag->state)
        {
            case RGS_MENU:
            {
                // TODO menu button inputs
                break;
            }
            case RGS_PLACING_PIECE:
            {
                // Move the cursor
                ragHandleGameInput(rag, &evt);
                break;
            }
            default:
            case RGS_WAITING:
            {
                // Do nothing
                break;
            }
        }
    }

    // Draw to the TFT
    switch (rag->state)
    {
        default:
        case RGS_MENU:
        {
            // TODO draw menu
            clearPxTft();
            break;
        }
        case RGS_PLACING_PIECE:
        case RGS_WAITING:
        {
            ragDrawGame(rag);
            break;
        }
    }
}

/**
 * @brief TODO
 *
 * @param esp_now_info
 * @param data
 * @param len
 * @param rssi
 */
static void ragEspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi)
{
    // Pass to p2p
    p2pRecvCb(&rag->p2p, esp_now_info->src_addr, (const uint8_t*)data, len, rssi);
}

/**
 * @brief TODO
 *
 * @param mac_addr
 * @param status
 */
static void ragEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status)
{
    // Pass to p2p
    p2pSendCb(&rag->p2p, mac_addr, status);
}

/**
 * @brief TODO
 *
 * @param p2p
 * @param evt
 */
static void ragConCb(p2pInfo* p2p, connectionEvt_t evt)
{
    // TODO handle connection states and disconnection
    switch (evt)
    {
        case CON_STARTED:
        {
            break;
        }
        case RX_GAME_START_ACK:
        {
            break;
        }
        case RX_GAME_START_MSG:
        {
            break;
        }
        case CON_ESTABLISHED:
        {
            ragBeginGame(rag);
            break;
        }
        case CON_LOST:
        {
            break;
        }
    }
}

/**
 * @brief TODO
 *
 * @param p2p
 * @param payload
 * @param len
 */
static void ragMsgRxCb(p2pInfo* p2p, const uint8_t* payload, uint8_t len)
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
            if (len == sizeof(ragMsgSelectPiece_t))
            {
                const ragMsgSelectPiece_t* rxSel = (const ragMsgSelectPiece_t*)payload;

                // If this is the second player
                if (GOING_SECOND == p2pGetPlayOrder(&rag->p2p))
                {
                    // Save p1's piece
                    rag->p1Piece = rxSel->piece;

                    // Send p2's piece to p1
                    rag->p2Piece = RAG_GEM;

                    // Send sprite selection to other swadge
                    ragMsgSelectPiece_t txSel = {
                        .type  = MSG_SELECT_PIECE,
                        .piece = rag->p2Piece,
                    };
                    p2pSendMsg(&rag->p2p, (const uint8_t*)&txSel, sizeof(txSel), ragMsgTxCbFn);

                    // Wait for p1 to make the first move
                    rag->state = RGS_WAITING;
                }
                else // Going first
                {
                    // Received p2's piece
                    rag->p2Piece = rxSel->piece;

                    // Make the first move
                    rag->state = RGS_PLACING_PIECE;
                }
            }
            break;
        }
        case MSG_MOVE_CURSOR:
        {
            // Length check
            if (len == sizeof(ragMsgMoveCursor_t))
            {
                ragReceiveCursor(rag, (const ragMsgMoveCursor_t*)payload);
            }
            break;
        }
        case MSG_PLACE_PIECE:
        {
            // Length check
            if (len == sizeof(ragMsgPlacePiece_t))
            {
                ragReceivePlacedPiece(rag, (const ragMsgPlacePiece_t*)payload);
            }
            break;
        }
    }
}

/**
 * @brief TODO
 *
 * @param p2p
 * @param status
 * @param data
 * @param len
 */
void ragMsgTxCbFn(p2pInfo* p2p, messageStatus_t status, const uint8_t* data, uint8_t len)
{
    // TODO
}