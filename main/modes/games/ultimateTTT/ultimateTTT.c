//==============================================================================
// Includes
//==============================================================================

#include "ultimateTTT.h"
#include "ultimateTTTgame.h"
#include "ultimateTTThowTo.h"
#include "ultimateTTTpieceSelect.h"
#include "ultimateTTTp2p.h"

//==============================================================================
// Function Prototypes
//==============================================================================

static void tttEnterMode(void);
static void tttExitMode(void);
static void tttMainLoop(int64_t elapsedUs);
static void tttMenuCb(const char* label, bool selected, uint32_t value);

static void tttEspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi);
static void tttEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status);
static void tttConCb(p2pInfo* p2p, connectionEvt_t evt);
static void tttMsgRxCb(p2pInfo* p2p, const uint8_t* payload, uint8_t len);

//==============================================================================
// Variables
//==============================================================================

// It's good practice to declare immutable strings as const so they get placed in ROM, not RAM
static const char tttName[]        = "Ultimate TTT";
static const char tttMultiStr[]    = "Wireless Connect";
static const char tttSingleStr[]   = "Single Player";
static const char tttPieceSelStr[] = "Piece Select";
static const char tttHowToStr[]    = "How To Play";

swadgeMode_t tttMode = {
    .modeName                 = tttName,
    .wifiMode                 = ESP_NOW,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = tttEnterMode,
    .fnExitMode               = tttExitMode,
    .fnMainLoop               = tttMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = NULL,
    .fnEspNowRecvCb           = tttEspNowRecvCb,
    .fnEspNowSendCb           = tttEspNowSendCb,
    .fnAdvancedUSB            = NULL,
    .fnDacCb                  = NULL,
};

ultimateTTT_t* ttt;

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief TODO
 *
 */
static void tttEnterMode(void)
{
    // Allocate memory for the mode
    ttt = calloc(1, sizeof(ultimateTTT_t));

    ttt->cursorMode = SELECT_SUBGAME;

    loadWsg("x_small.wsg", &ttt->piece_x_small, true);
    loadWsg("x_large.wsg", &ttt->piece_x_big, true);
    loadWsg("o_small.wsg", &ttt->piece_o_small, true);
    loadWsg("o_large.wsg", &ttt->piece_o_big, true);

    loadFont("rodin_eb.font", &ttt->font_rodin, false);
    loadFont("righteous_150.font", &ttt->font_righteous, false);

    ttt->menu         = initMenu(tttName, tttMenuCb);
    ttt->menuRenderer = initMenuManiaRenderer(&ttt->font_righteous, &ttt->font_rodin);

    addSingleItemToMenu(ttt->menu, tttMultiStr);
    addSingleItemToMenu(ttt->menu, tttSingleStr);
    addSingleItemToMenu(ttt->menu, tttPieceSelStr);
    addSingleItemToMenu(ttt->menu, tttHowToStr);

    ttt->ui = TUI_MENU;

    // Initialize p2p
    p2pInitialize(&ttt->p2p, 0x25, tttConCb, tttMsgRxCb, -70);
}

/**
 * @brief TODO
 *
 */
static void tttExitMode(void)
{
    // Deinitialize p2p
    p2pDeinit(&ttt->p2p);

    // Free memory
    freeWsg(&ttt->piece_x_small);
    freeWsg(&ttt->piece_x_big);
    freeWsg(&ttt->piece_o_small);
    freeWsg(&ttt->piece_o_big);

    // Free the menu
    deinitMenuManiaRenderer(ttt->menuRenderer);
    deinitMenu(ttt->menu);

    // Free the font
    freeFont(&ttt->font_rodin);
    freeFont(&ttt->font_righteous);

    // Free memory
    free(ttt);
}

/**
 * @brief TODO
 *
 * @param elapsedUs
 */
static void tttMainLoop(int64_t elapsedUs)
{
    // Handle inputs
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        switch (ttt->ui)
        {
            case TUI_MENU:
            {
                ttt->menu = menuButton(ttt->menu, evt);
                break;
            }
            case TUI_CONNECTING:
            {
                tttHandleConnectingInput(ttt, &evt);
                break;
            }
            case TUI_GAME:
            {
                tttHandleGameInput(ttt, &evt);
                break;
            }
            case TUI_PIECE_SELECT:
            {
                tttInputPieceSelect(ttt, &evt);
                break;
            }
            case TUI_HOW_TO:
            {
                tttInputHowTo(ttt, &evt);
                break;
            }
        }
    }

    // Draw to the TFT
    switch (ttt->ui)
    {
        case TUI_MENU:
        {
            // Draw menu
            drawMenuMania(ttt->menu, ttt->menuRenderer, elapsedUs);
            break;
        }
        case TUI_CONNECTING:
        {
            tttDrawConnecting(ttt);
            break;
        }
        case TUI_GAME:
        {
            tttDrawGame(ttt);
            break;
        }
        case TUI_PIECE_SELECT:
        {
            tttDrawPieceSelect(ttt);
            break;
        }
        case TUI_HOW_TO:
        {
            tttDrawHowTo(ttt);
            break;
        }
    }
}

/**
 * @brief TODO
 *
 * @param label
 * @param selected
 * @param value
 */
static void tttMenuCb(const char* label, bool selected, uint32_t value)
{
    if (selected)
    {
        if (tttMultiStr == label)
        {
            // TODO multiplayer
            p2pStartConnection(&ttt->p2p);
            ttt->ui = TUI_CONNECTING;
        }
        else if (tttSingleStr == label)
        {
            // TODO single player
            printf("Implement Single Player\n");
        }
        else if (tttPieceSelStr == label)
        {
            // Show piece selection UI
            ttt->ui = TUI_PIECE_SELECT;
        }
        else if (tttHowToStr == label)
        {
            // Show how to play
            ttt->ui = TUI_HOW_TO;
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
static void tttEspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi)
{
    // Pass to p2p
    p2pRecvCb(&ttt->p2p, esp_now_info->src_addr, (const uint8_t*)data, len, rssi);
}

/**
 * @brief TODO
 *
 * @param mac_addr
 * @param status
 */
static void tttEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status)
{
    // Pass to p2p
    p2pSendCb(&ttt->p2p, mac_addr, status);
}

/**
 * @brief TODO
 *
 * @param p2p
 * @param evt
 */
static void tttConCb(p2pInfo* p2p, connectionEvt_t evt)
{
    tttHandleCon(ttt, evt);
}

/**
 * @brief TODO
 *
 * @param p2p
 * @param payload
 * @param len
 */
static void tttMsgRxCb(p2pInfo* p2p, const uint8_t* payload, uint8_t len)
{
    tttHandleMsgRx(ttt, payload, len);
}

/**
 * @brief TODO
 *
 * @param p2p
 * @param status
 * @param data
 * @param len
 */
void tttMsgTxCbFn(p2pInfo* p2p, messageStatus_t status, const uint8_t* data, uint8_t len)
{
    // TODO
    tttHandleMsgTx(ttt, status, data, len);
}
