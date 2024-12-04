//==============================================================================
// Includes
//==============================================================================

#include <esp_random.h>
#include "ultimateTTT.h"
#include "ultimateTTTgame.h"
#include "ultimateTTThowTo.h"
#include "ultimateTTTmarkerSelect.h"
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
static const char tttName[]           = "Ultimate TTT";
static const char tttMultiStr[]       = "Wireless Connect";
static const char tttPassAndPlayStr[] = "Pass and Play";
static const char tttMultiShortStr[]  = "Connect";
static const char tttSingleStr[]      = "Single Player";
static const char tttDiffEasyStr[]    = "Easy";
static const char tttDiffMediumStr[]  = "Medium";
static const char tttDiffHardStr[]    = "Hard";
static const char tttMarkerSelStr[]   = "Marker Select";
static const char tttHowToStr[]       = "How To Play";
static const char tttResultStr[]      = "Result";
static const char tttRecordsStr[]     = "Records";
static const char tttExit[]           = "Exit";

// NVS keys
const char tttWinKey[]      = "ttt_win";
const char tttLossKey[]     = "ttt_loss";
const char tttDrawKey[]     = "ttt_draw";
const char tttMarkerKey[]   = "ttt_marker";
const char tttTutorialKey[] = "ttt_tutor";
const char tttUnlockKey[]   = "ttt_unlock";

static const led_t utttLedMenuColor = {
    .r = 0xFF,
    .g = 0x99,
    .b = 0xCC,
};

/**
 * Marker names to load WSGs
 */
const char* const markerNames[NUM_UNLOCKABLE_MARKERS] = {
    "x", "o", "sq", "tri", "banana", "dance", "hand", "hat", "hotdog", "lizard", "pixil", "spock", "swadgeman",
};

const int16_t markersUnlockedAtWins[NUM_UNLOCKABLE_MARKERS] = {
    0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
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
    // TODO enable speaker if BGM is added
    setDacShutdown(true);

    // Allocate memory for the mode
    ttt = calloc(1, sizeof(ultimateTTT_t));

    // Load markers
    for (int16_t pIdx = 0; pIdx < ARRAY_SIZE(markerNames); pIdx++)
    {
        char assetName[32];

        snprintf(assetName, sizeof(assetName) - 1, "%s_%c%c.wsg", markerNames[pIdx], 'b', 's');

        loadWsg(assetName, &ttt->markerWsg[pIdx].blue.small, true);
        snprintf(assetName, sizeof(assetName) - 1, "%s_%c%c.wsg", markerNames[pIdx], 'b', 'l');
        loadWsg(assetName, &ttt->markerWsg[pIdx].blue.large, true);

        snprintf(assetName, sizeof(assetName) - 1, "%s_%c%c.wsg", markerNames[pIdx], 'r', 's');
        loadWsg(assetName, &ttt->markerWsg[pIdx].red.small, true);
        snprintf(assetName, sizeof(assetName) - 1, "%s_%c%c.wsg", markerNames[pIdx], 'r', 'l');
        loadWsg(assetName, &ttt->markerWsg[pIdx].red.large, true);
    }

    // Load some fonts
    loadFont("rodin_eb.font", &ttt->font_rodin, false);
    loadFont("righteous_150.font", &ttt->font_righteous, false);

    // Initialize a menu renderer
    ttt->menuRenderer = initMenuManiaRenderer(&ttt->font_righteous, NULL, &ttt->font_rodin);
    // Color the menu like Poe
    static const paletteColor_t shadowColors[] = {c500, c511, c522, c533, c544, c555, c544, c533, c522, c511};
    recolorMenuManiaRenderer(ttt->menuRenderer, //
                             c500, c555, c111,  // titleBgColor, titleTextColor, textOutlineColor
                             c333,              // bgColor
                             c534, c544,        // outerRingColor, innerRingColor
                             c212, c555,        // rowColor, rowTextColor
                             shadowColors, ARRAY_SIZE(shadowColors), utttLedMenuColor);

    // Initialize the main menu
    ttt->menu = initMenu(tttName, tttMenuCb);
    addSingleItemToMenu(ttt->menu, tttMultiStr);
    addSingleItemToMenu(ttt->menu, tttPassAndPlayStr);

    ttt->menu = startSubMenu(ttt->menu, tttSingleStr);
    addSingleItemToMenu(ttt->menu, tttDiffEasyStr);
    addSingleItemToMenu(ttt->menu, tttDiffMediumStr);
    addSingleItemToMenu(ttt->menu, tttDiffHardStr);
    ttt->menu = endSubMenu(ttt->menu);

    addSingleItemToMenu(ttt->menu, tttMarkerSelStr);
    addSingleItemToMenu(ttt->menu, tttHowToStr);
    addSingleItemToMenu(ttt->menu, tttRecordsStr);
    addSingleItemToMenu(ttt->menu, tttExit);

    // Initialize a menu with no entries to be used as a background
    ttt->bgMenu = initMenu(tttMarkerSelStr, NULL);

    // Load saved wins and losses counts
    if (true != readNvs32(tttWinKey, &ttt->wins))
    {
        ttt->wins = 0;
        writeNvs32(tttWinKey, ttt->wins);
    }
    if (true != readNvs32(tttLossKey, &ttt->losses))
    {
        ttt->losses = 0;
        writeNvs32(tttLossKey, ttt->losses);
    }
    if (true != readNvs32(tttDrawKey, &ttt->draws))
    {
        ttt->draws = 0;
        writeNvs32(tttDrawKey, ttt->draws);
    }
    if (true != readNvs32(tttUnlockKey, &ttt->numUnlockedMarkers))
    {
        // Start with 2, X and O
        ttt->numUnlockedMarkers = 2;
        writeNvs32(tttUnlockKey, ttt->numUnlockedMarkers);
    }
    if (true != readNvs32(tttMarkerKey, &ttt->activeMarkerIdx))
    {
        // Set this to -1 to force the selection UI
        ttt->activeMarkerIdx = -1;
        writeNvs32(tttMarkerKey, ttt->activeMarkerIdx);
    }
    if (true != readNvs32(tttTutorialKey, &ttt->tutorialRead))
    {
        ttt->tutorialRead = false;
        writeNvs32(tttTutorialKey, ttt->tutorialRead);
    }

    // Initialize p2p
    p2pInitialize(&ttt->game.p2p, 0x25, tttConCb, tttMsgRxCb, -70);

    // Measure the display
    ttt->gameSize    = MIN(TFT_WIDTH, TFT_HEIGHT);
    ttt->cellSize    = ttt->gameSize / 9;
    ttt->subgameSize = ttt->cellSize * 3;
    ttt->gameSize    = ttt->cellSize * 9;

    // Center the game on the screen
    ttt->gameOffset.x = (TFT_WIDTH - ttt->gameSize) / 2;
    ttt->gameOffset.y = (TFT_HEIGHT - ttt->gameSize) / 2;

    // Start on different UIs depending on setup completion
    if (false == ttt->tutorialRead)
    {
        // Start on the how to
        tttShowUi(TUI_HOW_TO);
    }
    else if (-1 == ttt->activeMarkerIdx)
    {
        // Start on marker select
        tttShowUi(TUI_MARKER_SELECT);
    }
    else
    {
        // Start on the main menu
        tttShowUi(TUI_MENU);
    }
}

/**
 * @brief Exit Ulitmate TTT and release all resources
 */
static void tttExitMode(void)
{
    // Deinitialize p2p
    p2pDeinit(&ttt->game.p2p);

    // Free marker assets
    for (int16_t pIdx = 0; pIdx < ARRAY_SIZE(markerNames); pIdx++)
    {
        freeWsg(&ttt->markerWsg[pIdx].blue.small);
        freeWsg(&ttt->markerWsg[pIdx].blue.large);
        freeWsg(&ttt->markerWsg[pIdx].red.small);
        freeWsg(&ttt->markerWsg[pIdx].red.large);
    }

    // Clear out this list
    while (0 != ttt->instructionHistory.length)
    {
        free(pop(&ttt->instructionHistory));
    }

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
            case TUI_MARKER_SELECT:
            {
                tttInputMarkerSelect(ttt, &evt);
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
        case TUI_MARKER_SELECT:
        {
            tttDrawMarkerSelect(ttt, elapsedUs);
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
            ttt->game.singleSystem = false;
            ttt->game.passAndPlay  = false;
            // Show connection UI
            tttShowUi(TUI_CONNECTING);
            // Start multiplayer
            p2pStartConnection(&ttt->game.p2p);
        }
        else if (tttPassAndPlayStr == label)
        {
            ttt->game.singleSystem = true;
            ttt->game.passAndPlay  = true;
            tttBeginGame(ttt);
            tttShowUi(TUI_GAME);
        }
        else if (tttDiffEasyStr == label)
        {
            ttt->game.singleSystem   = true;
            ttt->game.passAndPlay    = false;
            ttt->game.cpu.difficulty = TDIFF_EASY;
            tttBeginGame(ttt);
            tttShowUi(TUI_GAME);
        }
        else if (tttDiffMediumStr == label)
        {
            ttt->game.singleSystem   = true;
            ttt->game.passAndPlay    = false;
            ttt->game.cpu.difficulty = TDIFF_MEDIUM;
            tttBeginGame(ttt);
            tttShowUi(TUI_GAME);
        }
        else if (tttDiffHardStr == label)
        {
            ttt->game.singleSystem   = true;
            ttt->game.passAndPlay    = false;
            ttt->game.cpu.difficulty = TDIFF_HARD;
            tttBeginGame(ttt);
            tttShowUi(TUI_GAME);
        }
        else if (tttMarkerSelStr == label)
        {
            // Show marker selection UI
            tttShowUi(TUI_MARKER_SELECT);
        }
        else if (tttHowToStr == label)
        {
            // Show how to play
            tttShowUi(TUI_HOW_TO);
        }
        else if (tttRecordsStr == label)
        {
            ttt->lastResult = TTR_RECORDS;
            tttShowUi(TUI_RESULT);
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
    p2pRecvCb(&ttt->game.p2p, esp_now_info->src_addr, (const uint8_t*)data, len, rssi);
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
    p2pSendCb(&ttt->game.p2p, mac_addr, status);
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

/**
 * @brief Switch to showing a different UI
 *
 * @param ui The UI to show
 */
void tttShowUi(tttUi_t ui)
{
    // Set the UI
    ttt->ui = ui;

    // Assume menu LEDs should be on
    setManiaLedsOn(ttt->menuRenderer, true);
    ttt->menuRenderer->baseLedColor = utttLedMenuColor;
    setManiaDrawRings(ttt->menuRenderer, true);

    // Initialize the new UI
    switch (ttt->ui)
    {
        case TUI_MENU:
        {
            break;
        }
        case TUI_CONNECTING:
        {
            ttt->bgMenu->title = tttMultiShortStr;
            break;
        }
        case TUI_GAME:
        {
            // Initialization done in tttBeginGame()
            break;
        }
        case TUI_MARKER_SELECT:
        {
            ttt->bgMenu->title       = tttMarkerSelStr;
            ttt->selectMarkerIdx     = ttt->activeMarkerIdx;
            ttt->xSelectScrollTimer  = 0;
            ttt->xSelectScrollOffset = 0;
            break;
        }
        case TUI_HOW_TO:
        {
            // Turn LEDs off for reading
            setManiaLedsOn(ttt->menuRenderer, false);
            setManiaDrawRings(ttt->menuRenderer, false);
            ttt->bgMenu->title   = tttHowToStr;
            ttt->pageIdx         = 0;
            ttt->arrowBlinkTimer = 0;
            break;
        }
        case TUI_RESULT:
        {
            if (TTR_RECORDS == ttt->lastResult)
            {
                ttt->bgMenu->title = tttRecordsStr;
            }
            else
            {
                ttt->bgMenu->title = tttResultStr;
            }
            break;
        }
    }
}
