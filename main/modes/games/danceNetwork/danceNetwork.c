#include "danceNetwork.h"
#include "dn_game.h"
#include "dn_characterSelect.h"
#include "dn_howTo.h"
#include "dn_result.h"
#include "dn_typedef.h"
#include "dn_p2p.h"
#include "mainMenu.h"
#include "dn_random.h"

const char danceNetworkName[] = "Alpha Pulse: Dance Network";



//==============================================================================
// Function Prototypes
//==============================================================================

static void dn_EspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi);
static void dn_EspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status);
static void dn_ConCb(p2pInfo* p2p, connectionEvt_t evt);
static void dn_MsgRxCb(p2pInfo* p2p, const uint8_t* payload, uint8_t len);

static void dn_EnterMode(void);
static void dn_ExitMode(void);
static void dn_MainLoop(int64_t elapsedUs);
static void dn_MenuCb(const char* label, bool selected, uint32_t value);
static void dn_BackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);

//==============================================================================
// Variables
//==============================================================================

swadgeMode_t danceNetworkMode = {
    .modeName                 = danceNetworkName, // Assign the name we created here
    .wifiMode                 = ESP_NOW,        // If we want WiFi 
    .overrideUsb              = false,          // Overrides the default USB behavior.
    .usesAccelerometer        = false,          // If we're using motion controls
    .usesThermometer          = false,          // If we're using the internal thermometer
    .overrideSelectBtn        = false,          // The select/Menu button has a default behavior. If you want to override it, you can set this to true but you'll need to re-implement the behavior.
    .fnEnterMode              = dn_EnterMode, // The enter mode function
    .fnExitMode               = dn_ExitMode,  // The exit mode function
    .fnMainLoop               = dn_MainLoop,  // The loop function
    .fnAudioCallback          = NULL,           // If the mode uses the microphone
    .fnBackgroundDrawCallback = dn_BackgroundDrawCallback,           // Draws a section of the display
    .fnEspNowRecvCb           = dn_EspNowRecvCb,           // If using Wifi, add the receive function here
    .fnEspNowSendCb           = dn_EspNowSendCb,           // If using Wifi, add the send function here
    .fnAdvancedUSB            = NULL, // If using advanced USB things.
};

// It's good practice to declare immutable strings as const so they get placed in ROM, not RAM
const char dn_Name[]                  = "Dance Network";
static const char dn_MultiStr[]       = "Multiplayer";
static const char dn_WirelessStr[]    = "Wireless Play";
static const char dn_PassAndPlayStr[] = "Pass and Play";
static const char dn_MultiShortStr[]  = "Connect";
static const char dn_SingleStr[]      = "Single Player";
static const char dn_DiffEasyStr[]    = "Easy";
static const char dn_DiffMediumStr[]  = "Medium";
static const char dn_DiffHardStr[]    = "Hard";
static const char dn_CharacterSelStr[]= "Character Select";
static const char dn_HowToStr[]       = "How To Play";
//static const char dn_ResultStr[]      = "Result";
static const char dn_RecordsStr[]     = "Records";
static const char dn_Exit[]           = "Exit";

// NVS keys
const char dnWinKey[]      = "dn_win";
const char dnLossKey[]     = "dn_loss";
const char dnDrawKey[]     = "dn_draw";
const char dnCharacterKey[]= "dn_character";
const char dnTutorialKey[] = "dn_tutor";
const char dnUnlockKey[]   = "dn_unlock";

static const led_t dn_LedMenuColor = {
    .r = 0x66,
    .g = 0x00,
    .b = 0x66,
};

dn_gameData_t* gameData;

/// @brief A heatshrink decoder to use for all WSG loads rather than allocate a new one for each WSG
/// This helps to prevent memory fragmentation in SPIRAM.
/// Note, this is outside the bb_t struct for easy access to loading fuctions without bb_t references
heatshrink_decoder* dn_hsd;
/// @brief A temporary decode space to use for all WSG loads
uint8_t* dn_decodeSpace;

static void dn_EnterMode(void)
{
    gameData = (dn_gameData_t*)heap_caps_calloc(1, sizeof(dn_gameData_t), MALLOC_CAP_8BIT);
    for (int y = 0; y < BOARD_SIZE; y++)
    {
        for (int x = 0; x < BOARD_SIZE; x++)
        {
            gameData->tiles[y][x].yOffset = ((TFT_HEIGHT >> 2)-15) << DECIMAL_BITS;
        }
    }
    gameData->selection[0] = 2;
    gameData->selection[1] = 2;
    gameData->tiles[gameData->selection[0]][gameData->selection[1]].yOffset = (TFT_HEIGHT >> 2) << DECIMAL_BITS;

    gameData->characterAssets[DN_ALPHA][DN_KING][DN_DOWN].xOff = 10;
    gameData->characterAssets[DN_ALPHA][DN_KING][DN_DOWN].yOff = -40;
    gameData->characterAssets[DN_ALPHA][DN_KING][DN_UP].xOff   = 13;
    gameData->characterAssets[DN_ALPHA][DN_KING][DN_UP].yOff   = -40;
    gameData->characterAssets[DN_ALPHA][DN_PAWN][DN_DOWN].xOff = 15;
    gameData->characterAssets[DN_ALPHA][DN_PAWN][DN_DOWN].yOff = -20;
    gameData->characterAssets[DN_ALPHA][DN_PAWN][DN_UP].xOff   = 8;
    gameData->characterAssets[DN_ALPHA][DN_PAWN][DN_UP].yOff   = -17;

    gameData->characterAssets[DN_CHESS][DN_KING][DN_DOWN].xOff = 15;
    gameData->characterAssets[DN_CHESS][DN_KING][DN_DOWN].yOff = -41;
    gameData->characterAssets[DN_CHESS][DN_KING][DN_UP].xOff   = 15;
    gameData->characterAssets[DN_CHESS][DN_KING][DN_UP].yOff   = -41;
    gameData->characterAssets[DN_CHESS][DN_PAWN][DN_DOWN].xOff = 14;
    gameData->characterAssets[DN_CHESS][DN_PAWN][DN_DOWN].yOff = -30;
    gameData->characterAssets[DN_CHESS][DN_PAWN][DN_UP].xOff   = 14;
    gameData->characterAssets[DN_CHESS][DN_PAWN][DN_UP].yOff   = -30;

    wsgPaletteReset(&gameData->redFloor1);
    wsgPaletteSet(&gameData->redFloor1, c334, c533);

    // Allocate WSG loading helpers
    dn_hsd = heatshrink_decoder_alloc(256, 8, 4);
    // The largest image is bb_menu2.png, decodes to 99124 bytes
    // 99328 is 1024 * 97
    dn_decodeSpace = heap_caps_malloc_tag(99328, MALLOC_CAP_SPIRAM, "decodeSpace");//TODO change the size to the largest sprite

    
    loadWsgInplace(DN_GROUND_TILE_WSG, &gameData->sprites.groundTile, true, dn_decodeSpace, dn_hsd);
    
    // Load some fonts
    loadFont(RODIN_EB_FONT, &gameData->font_rodin, false);
    // loadFont("righteous_150.font", &gameData->font_righteous, false);

    // Initialize a menu renderer
    gameData->menuRenderer = initMenuManiaRenderer(NULL, NULL, NULL);
    // Color the menu my way
    static const paletteColor_t shadowColors[] = {
        c500, c050, c005, c550, c505, c055, c200, c020, c002, c220,
    };

    recolorMenuManiaRenderer(gameData->menuRenderer, //
                             c202, c540, c000,  // titleBgColor, titleTextColor, textOutlineColor
                             c315,              // bgColor
                             c213, c035,        // outerRingColor, innerRingColor
                             c000, c555,        // rowColor, rowTextColor
                             shadowColors, ARRAY_SIZE(shadowColors), dn_LedMenuColor);

    // Initialize the main menu
    gameData->menu = initMenu(dn_Name, dn_MenuCb);
    gameData->menu = startSubMenu(gameData->menu, dn_MultiStr);
    addSingleItemToMenu(gameData->menu, dn_WirelessStr);
    addSingleItemToMenu(gameData->menu, dn_PassAndPlayStr);
    gameData->menu = endSubMenu(gameData->menu);

    gameData->menu = startSubMenu(gameData->menu, dn_SingleStr);
    addSingleItemToMenu(gameData->menu, dn_DiffEasyStr);
    addSingleItemToMenu(gameData->menu, dn_DiffMediumStr);
    addSingleItemToMenu(gameData->menu, dn_DiffHardStr);
    gameData->menu = endSubMenu(gameData->menu);

    addSingleItemToMenu(gameData->menu, dn_CharacterSelStr);
    addSingleItemToMenu(gameData->menu, dn_HowToStr);
    addSingleItemToMenu(gameData->menu, dn_RecordsStr);
    addSingleItemToMenu(gameData->menu, dn_Exit);

    // Initialize a menu with no entries to be used as a background
    gameData->bgMenu = initMenu(dn_CharacterSelStr, NULL);
}

static void dn_ExitMode(void)
{
    freeWsg(&gameData->sprites.groundTile);
    free(gameData);
    // Free the fonts
    freeFont(&gameData->font_rodin);
}

/**
 * @brief The main loop for Dance Network, responsible for input handling, game logic, and rendering
 *
 * @param elapsedUs The time elapsed since this was last called
 */
static void dn_MainLoop(int64_t elapsedUs)
{
    // Handle inputs
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        switch (gameData->ui)
        {
            case UI_MENU:
            {
                gameData->menu = menuButton(gameData->menu, evt);
                break;
            }
            case UI_CONNECTING:
            {
                dn_HandleConnectingInput(gameData, &evt);
                break;
            }
            case UI_GAME:
            {
                dn_HandleGameInput(gameData, &evt);
                break;
            }
            case UI_CHARACTER_SELECT:
            {
                dn_InputCharacterSelect(gameData, &evt);
                break;
            }
            case UI_HOW_TO:
            {
                dn_InputHowTo(gameData, &evt);
                break;
            }
            case UI_RESULT:
            {
                dn_InputResult(gameData, &evt);
                break;
            }
        }
    }

    // Update the game
    if(gameData->ui == UI_GAME)
    {
        dn_UpdateGame(gameData, elapsedUs);
    }

    // Draw to the TFT
    switch (gameData->ui)
    {
        case UI_MENU:
        {
            // Draw menu
            drawMenuMania(gameData->menu, gameData->menuRenderer, elapsedUs);
            break;
        }
        case UI_CONNECTING:
        {
            dn_DrawConnecting(gameData, elapsedUs);
            break;
        }
        case UI_GAME:
        {
            dn_DrawGame(gameData);
            break;
        }
        case UI_CHARACTER_SELECT:
        {
            dn_DrawCharacterSelect(gameData, elapsedUs);
            break;
        }
        case UI_HOW_TO:
        {
            dn_DrawHowTo(gameData, elapsedUs);
            break;
        }
        case UI_RESULT:
        {
            dn_DrawResult(gameData, elapsedUs);
            break;
        }
    }
}

static void dn_BackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    // Fill the flat background color
    paletteColor_t* frameBuf = getPxTftFramebuffer();
    memset(&frameBuf[(y * TFT_WIDTH) + x], c000, sizeof(paletteColor_t) * w * h);
}

/**
 * @brief Callback for when a Dance Network menu item is selected
 *
 * @param label The string label of the menu item selected
 * @param selected true if this was selected, false if it was moved to
 * @param value The value for settings, unused.
 */
static void dn_MenuCb(const char* label, bool selected, uint32_t value)
{
    if (selected)
    {
        if (dn_WirelessStr == label)
        {
            // Initialize p2p
            p2pInitialize(&gameData->p2p, 0x26, dn_ConCb, dn_MsgRxCb, -70);
            // Start multiplayer
            p2pStartConnection(&gameData->p2p);

            gameData->singleSystem = false;
            gameData->passAndPlay  = false;
            // Show connection UI
            dn_ShowUi(UI_CONNECTING);
            // Start multiplayer
            p2pStartConnection(&gameData->p2p);
        }
        else if (dn_PassAndPlayStr == label)
        {
            gameData->singleSystem = true;
            gameData->passAndPlay  = true;
            //tttBeginGame(ttt);
            dn_ShowUi(UI_GAME);
        }
        else if (dn_DiffEasyStr == label)
        {
            gameData->singleSystem   = true;
            gameData->passAndPlay    = false;
            gameData->cpu.difficulty = TDIFF_EASY;
            //tttBeginGame(ttt);
            dn_ShowUi(UI_GAME);
        }
        else if (dn_DiffMediumStr == label)
        {
            gameData->singleSystem   = true;
            gameData->passAndPlay    = false;
            gameData->cpu.difficulty = TDIFF_MEDIUM;
            //tttBeginGame(ttt);
            dn_ShowUi(UI_GAME);
        }
        else if (dn_DiffHardStr == label)
        {
            gameData->singleSystem   = true;
            gameData->passAndPlay    = false;
            gameData->cpu.difficulty = TDIFF_HARD;
            //tttBeginGame(ttt);
            dn_ShowUi(UI_GAME);
        }
        else if (dn_CharacterSelStr == label)
        {
            // Show character selection UI
            dn_ShowUi(UI_CHARACTER_SELECT);
        }
        else if (dn_HowToStr == label)
        {
            // Show how to play
            dn_ShowUi(UI_HOW_TO);
        }
        else if (dn_RecordsStr == label)
        {
            //ttt->lastResult = TTR_RECORDS;
            dn_ShowUi(UI_RESULT);
        }
        else if (dn_Exit == label)
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
static void dn_EspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi)
{
    // Pass to p2p
    p2pRecvCb(&gameData->p2p, esp_now_info->src_addr, (const uint8_t*)data, len, rssi);
}

/**
 * @brief Callback after an ESP-NOW packet is sent. This passes the status to p2p.
 *
 * @param mac_addr The MAC address which the data was sent to
 * @param status   The status of the transmission
 */
static void dn_EspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status)
{
    // Pass to p2p
    p2pSendCb(&gameData->p2p, mac_addr, status);
}

/**
 * @brief Callback for when p2p is establishing a connection
 *
 * @param p2p The p2pInfo
 * @param evt The connection event
 */
static void dn_ConCb(p2pInfo* p2p, connectionEvt_t evt)
{
    dn_HandleCon(gameData, evt);
}

/**
 * @brief Callback for when a P2P message is received.
 *
 * @param p2p The p2pInfo
 * @param payload The data that was received
 * @param len The length of the data that was received
 */
static void dn_MsgRxCb(p2pInfo* p2p, const uint8_t* payload, uint8_t len)
{
    dn_HandleMsgRx(gameData, payload, len);
}

/**
 * @brief Callback after a P2P message is transmitted
 *
 * @param p2p The p2pInfo
 * @param status The status of the transmission
 * @param data The data that was transmitted
 * @param len The length of the data that was transmitted
 */
void dn_MsgTxCbFn(p2pInfo* p2p, messageStatus_t status, const uint8_t* data, uint8_t len)
{
    dn_HandleMsgTx(gameData, status, data, len);
}


/**
 * @brief Switch to showing a different UI
 *
 * @param ui The UI to show
 */
void dn_ShowUi(dn_Ui_t ui)
{
    // Set the UI
    gameData->ui = ui;

    // Assume menu LEDs should be on
    setManiaLedsOn(gameData->menuRenderer, true);
    gameData->menuRenderer->baseLedColor = dn_LedMenuColor;
    setManiaDrawRings(gameData->menuRenderer, true);

    // Initialize the new UI
    switch (gameData->ui)
    {
        case UI_MENU:
        {
            break;
        }
        case UI_CONNECTING:
        {
            gameData->bgMenu->title = dn_MultiShortStr;
            break;
        }
        case UI_GAME:
        {
            // Initialization done in tttBeginGame()
            dn_InitializeGame();
            break;
        }
        case UI_CHARACTER_SELECT:
        {
            dn_InitializeCharacterSelect();
            gameData->bgMenu->title       = dn_CharacterSelStr;
            // gameData->selectMarkerIdx     = ttt->activeMarkerIdx;
            // gameData->xSelectScrollTimer  = 0;
            // gameData->xSelectScrollOffset = 0;
            break;
        }
        case UI_HOW_TO:
        {
            // Turn LEDs off for reading
            setManiaLedsOn(gameData->menuRenderer, false);
            setManiaDrawRings(gameData->menuRenderer, false);
            gameData->bgMenu->title   = dn_HowToStr;
            // gameData->pageIdx         = 0;
            // gameData->arrowBlinkTimer = 0;
            break;
        }
        case UI_RESULT:
        {
            // Game over, deinitialize p2p just in case
            p2pDeinit(&gameData->p2p);

            // if (TTR_RECORDS == gameData->lastResult)
            // {
            //     gameData->bgMenu->title = tttRecordsStr;
            // }
            // else
            // {
            //     ttt->bgMenu->title = tttResultStr;
            // }
            break;
        }
    }
}

void dn_InitializeGame()
{
    //will change later to specifically load sprites for this match.
    dn_InitializeCharacterSelect();

    for(int8_t player = 0; player < 2; player++)
    {
        for(int8_t unit = 0; unit < 5; unit++)
        {
            gameData->UnitPositions[player][unit].x = unit;
            gameData->UnitPositions[player][unit].y = !player * 4;
        }
    }

    //player vs CPU
    if(gameData->singleSystem && !gameData->passAndPlay)
    {
        //The player may randomly be p1 or p2.
        gameData->isPlayer1 = dn_randomInt(0,1);
        if(!gameData->isPlayer1)
        {
            //copy player 1's character over to player 2 position.
            gameData->characterIndices[1] = gameData->characterIndices[0];
        }
        //[gameData->isPlayer1] actually gets the opponent.
        //give the CPU a random character.
        gameData->characterIndices[gameData->isPlayer1] = dn_randomInt(0,1);
    }
}

/**
 * @brief Load sprites needed in this UI
 *
 * @param
 */
void dn_InitializeCharacterSelect()
{
    if(!gameData->characterAssets[DN_ALPHA][DN_KING][DN_DOWN].sprite.w && !gameData->characterAssets[DN_ALPHA][DN_KING][DN_DOWN].sprite.h)
    {
        loadWsgInplace(DN_ALPHA_DOWN_WSG,     &gameData->characterAssets[DN_ALPHA][DN_KING][DN_DOWN].sprite, true, dn_decodeSpace, dn_hsd);
    }
    if(!gameData->characterAssets[DN_ALPHA][DN_KING][DN_UP].sprite.w && !gameData->characterAssets[DN_ALPHA][DN_KING][DN_UP].sprite.h)
    {
        loadWsgInplace(DN_ALPHA_UP_WSG,        &gameData->characterAssets[DN_ALPHA][DN_KING][DN_UP].sprite,   true, dn_decodeSpace, dn_hsd);
    }
    if(!gameData->characterAssets[DN_ALPHA][DN_PAWN][DN_DOWN].sprite.w && !gameData->characterAssets[DN_ALPHA][DN_PAWN][DN_DOWN].sprite.h)
    {
        loadWsgInplace(DN_BUCKET_HAT_DOWN_WSG, &gameData->characterAssets[DN_ALPHA][DN_PAWN][DN_DOWN].sprite, true, dn_decodeSpace, dn_hsd);
    }
    if(!gameData->characterAssets[DN_ALPHA][DN_PAWN][DN_UP].sprite.w && !gameData->characterAssets[DN_ALPHA][DN_PAWN][DN_UP].sprite.h)
    {
        loadWsgInplace(DN_BUCKET_HAT_UP_WSG,   &gameData->characterAssets[DN_ALPHA][DN_PAWN][DN_UP].sprite,   true, dn_decodeSpace, dn_hsd);
    }


    if(!gameData->characterAssets[DN_CHESS][DN_KING][DN_DOWN].sprite.w && !gameData->characterAssets[DN_CHESS][DN_KING][DN_DOWN].sprite.h)
    {
        loadWsgInplace(DN_BLACK_KING_WSG, &gameData->characterAssets[DN_CHESS][DN_KING][DN_DOWN].sprite, true, dn_decodeSpace, dn_hsd);
    }
    if(!gameData->characterAssets[DN_CHESS][DN_KING][DN_UP].sprite.w && !gameData->characterAssets[DN_CHESS][DN_KING][DN_UP].sprite.h)
    {
        loadWsgInplace(DN_WHITE_KING_WSG, &gameData->characterAssets[DN_CHESS][DN_KING][DN_UP].sprite,   true, dn_decodeSpace, dn_hsd);
    }
    if(!gameData->characterAssets[DN_CHESS][DN_PAWN][DN_DOWN].sprite.w && !gameData->characterAssets[DN_CHESS][DN_PAWN][DN_DOWN].sprite.h)
    {
        loadWsgInplace(DN_BLACK_PAWN_WSG, &gameData->characterAssets[DN_CHESS][DN_PAWN][DN_DOWN].sprite, true, dn_decodeSpace, dn_hsd);
    }
    if(!gameData->characterAssets[DN_CHESS][DN_PAWN][DN_UP].sprite.w && !gameData->characterAssets[DN_CHESS][DN_PAWN][DN_UP].sprite.h)
    {
        loadWsgInplace(DN_WHITE_PAWN_WSG, &gameData->characterAssets[DN_CHESS][DN_PAWN][DN_UP].sprite,   true, dn_decodeSpace, dn_hsd);
    }
}