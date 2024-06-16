//==============================================================================
// Includes
//==============================================================================

#include <esp_random.h>
#include "ultimateTTT.h"
#include "ultimateTTTgame.h"
#include "ultimateTTThowTo.h"
#include "ultimateTTTpieceSelect.h"
#include "ultimateTTTp2p.h"
#include "ultimateTTTresult.h"
#include "mainMenu.h"

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
static const char tttExit[]        = "Exit";

// NVS keys
const char tttWinKey[]      = "ttt_win";
const char tttLossKey[]     = "ttt_loss";
const char tttDrawKey[]     = "ttt_draw";
const char tttPieceKey[]    = "ttt_piece";
const char tttTutorialKey[] = "ttt_tutor";

/**
 * Piece names to load WSGs
 */
const char* pieceNames[NUM_UNLOCKABLE_PIECES] = {
    "x",
    "o",
    "sq",
    "tri",
};

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
 * @brief Initialize the Ultimate TTT mode
 */
static void tttEnterMode(void)
{
    // Allocate memory for the mode
    ttt = calloc(1, sizeof(ultimateTTT_t));

    // Load markers
    for (int16_t pIdx = 0; pIdx < ARRAY_SIZE(pieceNames); pIdx++)
    {
        char assetName[32];
        snprintf(assetName, sizeof(assetName) - 1, "up**_%s.wsg", pieceNames[pIdx]);

        assetName[2] = 'b'; // blue
        assetName[3] = 's'; // small
        loadWsg(assetName, &ttt->pieceWsg[pIdx].blue.small, true);
        assetName[3] = 'l'; // large
        loadWsg(assetName, &ttt->pieceWsg[pIdx].blue.large, true);

        assetName[2] = 'r'; // red
        assetName[3] = 's'; // small
        loadWsg(assetName, &ttt->pieceWsg[pIdx].red.small, true);
        assetName[3] = 'l'; // large
        loadWsg(assetName, &ttt->pieceWsg[pIdx].red.large, true);
    }

    // Load an arrow
    loadWsg("ut_arrow.wsg", &ttt->selectArrow, true);

    // Load some fonts
    loadFont("rodin_eb.font", &ttt->font_rodin, false);
    loadFont("righteous_150.font", &ttt->font_righteous, false);

    // Initialize a menu renderer
    ttt->menuRenderer = initMenuManiaRenderer(&ttt->font_righteous, NULL, &ttt->font_rodin);

    // Initialize the main menu
    ttt->menu = initMenu(tttName, tttMenuCb);
    addSingleItemToMenu(ttt->menu, tttMultiStr);
    addSingleItemToMenu(ttt->menu, tttSingleStr);
    addSingleItemToMenu(ttt->menu, tttPieceSelStr);
    addSingleItemToMenu(ttt->menu, tttHowToStr);
    addSingleItemToMenu(ttt->menu, tttExit);

    // Initialize a menu with no entries to be used as a background
    ttt->bgMenu = initMenu(tttPieceSelStr, NULL);

    // Load saved wins and losses counts
    if (true != readNvs32(tttWinKey, &ttt->wins))
    {
        ttt->wins = 0;
    }
    if (true != readNvs32(tttLossKey, &ttt->losses))
    {
        ttt->losses = 0;
    }
    if (true != readNvs32(tttDrawKey, &ttt->draws))
    {
        ttt->draws = 0;
    }
    if (true != readNvs32(tttPieceKey, &ttt->activePieceIdx))
    {
        // Set this to -1 to force the selection UI
        ttt->activePieceIdx = -1;
    }
    if (true != readNvs32(tttTutorialKey, &ttt->tutorialRead))
    {
        ttt->tutorialRead = false;
    }

    // Initialize p2p
    p2pInitialize(&ttt->p2p, 0x25, tttConCb, tttMsgRxCb, -70);

    // Start on different UIs depending on setup completion
    if (false == ttt->tutorialRead)
    {
        // Start on the how to
        ttt->ui = TUI_HOW_TO;
    }
    else if (-1 == ttt->activePieceIdx)
    {
        // Start on marker select
        ttt->ui = TUI_PIECE_SELECT;
    }
    else
    {
        // Start on the main menu
        ttt->ui = TUI_MENU;
    }

    // TODO initialize game state separately
    ttt->cursorMode = SELECT_SUBGAME;
}

/**
 * @brief Exit Ulitmate TTT and release all resources
 */
static void tttExitMode(void)
{
    // Deinitialize p2p
    p2pDeinit(&ttt->p2p);

    // Free marker assets
    for (int16_t pIdx = 0; pIdx < ARRAY_SIZE(pieceNames); pIdx++)
    {
        freeWsg(&ttt->pieceWsg[pIdx].blue.small);
        freeWsg(&ttt->pieceWsg[pIdx].blue.large);
        freeWsg(&ttt->pieceWsg[pIdx].red.small);
        freeWsg(&ttt->pieceWsg[pIdx].red.large);
    }
    freeWsg(&ttt->selectArrow);

    // Free the menu renderer
    deinitMenuManiaRenderer(ttt->menuRenderer);

    // Free the menus
    deinitMenu(ttt->menu);
    deinitMenu(ttt->bgMenu);

    // Free the fonts
    freeFont(&ttt->font_rodin);
    freeFont(&ttt->font_righteous);

    // Free everything
    free(ttt);
}

/**
 * @brief The main loop for Ultimate TTT, responsible for input handling, game logic, and rendering
 *
 * @param elapsedUs The time elapsed since this was last called
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
            case TUI_RESULT:
            {
                tttInputResult(ttt, &evt);
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
            tttDrawConnecting(ttt, elapsedUs);
            break;
        }
        case TUI_GAME:
        {
            tttDrawGame(ttt);
            break;
        }
        case TUI_PIECE_SELECT:
        {
            tttDrawPieceSelect(ttt, elapsedUs);
            break;
        }
        case TUI_HOW_TO:
        {
            tttDrawHowTo(ttt, elapsedUs);
            break;
        }
        case TUI_RESULT:
        {
            tttDrawResult(ttt, elapsedUs);
            break;
        }
    }
}

/**
 * @brief Callback for when an Ultimate TTT menu item is selected
 *
 * @param label The string label of the menu item selected
 * @param selected true if this was selected, false if it was moved to
 * @param value The value for settings, unused.
 */
static void tttMenuCb(const char* label, bool selected, uint32_t value)
{
    if (selected)
    {
        if (tttMultiStr == label)
        {
            // Show connection UI
            ttt->bgMenu->title = tttMultiStr;
            ttt->ui            = TUI_CONNECTING;
            // Start multiplayer
            p2pStartConnection(&ttt->p2p);
        }
        else if (tttSingleStr == label)
        {
            // TODO implement single player
            printf("Implement Single Player\n");
        }
        else if (tttPieceSelStr == label)
        {
            // Show piece selection UI
            ttt->bgMenu->title  = tttPieceSelStr;
            ttt->ui             = TUI_PIECE_SELECT;
            ttt->selectPieceIdx = ttt->activePieceIdx;
        }
        else if (tttHowToStr == label)
        {
            // Show how to play
            ttt->bgMenu->title = tttHowToStr;
            ttt->pageIdx       = 0;
            ttt->ui            = TUI_HOW_TO;
        }
        else if (tttExit == label)
        {
            // Exit to the main menu
            switchToSwadgeMode(&mainMenuMode);
        }
    }
}

/**
 * @brief Callback for when an ESP-NOW packet is received. This passes the packet to p2p.
 *
 * @param esp_now_info Information about the transmission, including The MAC addresses
 * @param data The received packet
 * @param len The length of the received packet
 * @param rssi The signal strength of the received packet
 */
static void tttEspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi)
{
    // Pass to p2p
    p2pRecvCb(&ttt->p2p, esp_now_info->src_addr, (const uint8_t*)data, len, rssi);
}

/**
 * @brief Callback after an ESP-NOW packet is sent. This passes the status to p2p.
 *
 * @param mac_addr The MAC address which the data was sent to
 * @param status   The status of the transmission
 */
static void tttEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status)
{
    // Pass to p2p
    p2pSendCb(&ttt->p2p, mac_addr, status);
}

/**
 * @brief Callback for when p2p is establishing a connection
 *
 * @param p2p The p2pInfo
 * @param evt The connection event
 */
static void tttConCb(p2pInfo* p2p, connectionEvt_t evt)
{
    tttHandleCon(ttt, evt);
}

/**
 * @brief Callback for when a P2P message is received.
 *
 * @param p2p The p2pInfo
 * @param payload The data that was received
 * @param len The length of the data that was received
 */
static void tttMsgRxCb(p2pInfo* p2p, const uint8_t* payload, uint8_t len)
{
    tttHandleMsgRx(ttt, payload, len);
}

/**
 * @brief Callback after a P2P message is transmitted
 *
 * @param p2p The p2pInfo
 * @param status The status of the transmission
 * @param data The data that was transmitted
 * @param len The length of the data that was transmitted
 */
void tttMsgTxCbFn(p2pInfo* p2p, messageStatus_t status, const uint8_t* data, uint8_t len)
{
    tttHandleMsgTx(ttt, status, data, len);
}
