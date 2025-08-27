#include "danceNetwork.h"
#include "dn_howTo.h"
#include "dn_result.h"
#include "dn_typedef.h"
#include "dn_p2p.h"
#include "mainMenu.h"
#include "dn_random.h"
#include "dn_entity.h"
#include "dn_entityManager.h"
#include "dn_utility.h"
#include "hdw-ch32v003.h"

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

static void dn_initializeGame(void);
static void dn_initializeCharacterSelect(void);
static void dn_freeAssets(void);

//==============================================================================
// Variables
//==============================================================================

swadgeMode_t danceNetworkMode = {
    .modeName          = danceNetworkName, // Assign the name we created here
    .wifiMode          = ESP_NOW,          // If we want WiFi
    .overrideUsb       = false,            // Overrides the default USB behavior.
    .usesAccelerometer = false,            // If we're using motion controls
    .usesThermometer   = false,            // If we're using the internal thermometer
    .overrideSelectBtn = false, // The select/Menu button has a default behavior. If you want to override it, you can
                                // set this to true but you'll need to re-implement the behavior.
    .fnEnterMode              = dn_EnterMode,              // The enter mode function
    .fnExitMode               = dn_ExitMode,               // The exit mode function
    .fnMainLoop               = dn_MainLoop,               // The loop function
    .fnAudioCallback          = NULL,                      // If the mode uses the microphone
    .fnBackgroundDrawCallback = dn_BackgroundDrawCallback, // Draws a section of the display
    .fnEspNowRecvCb           = dn_EspNowRecvCb,           // If using Wifi, add the receive function here
    .fnEspNowSendCb           = dn_EspNowSendCb,           // If using Wifi, add the send function here
    .fnAdvancedUSB            = NULL,                      // If using advanced USB things.
};

// It's good practice to declare immutable strings as const so they get placed in ROM, not RAM
const char dn_Name[]                   = "Dance Network";
static const char dn_MultiStr[]        = "Multiplayer";
static const char dn_WirelessStr[]     = "Wireless Play";
static const char dn_PassAndPlayStr[]  = "Pass and Play";
static const char dn_MultiShortStr[]   = "Connect";
static const char dn_SingleStr[]       = "Single Player";
static const char dn_DiffEasyStr[]     = "Easy";
static const char dn_DiffMediumStr[]   = "Medium";
static const char dn_DiffHardStr[]     = "Hard";
static const char dn_CharacterSelStr[] = "Select Pieces";
static const char dn_HowToStr[]        = "How To Play";
// static const char dn_ResultStr[]      = "Result";
static const char dn_RecordsStr[] = "Records";
static const char dn_Exit[]       = "Exit";

static const paletteColor_t starterAlbums[16][4] = {
    {c300, c320, cTransparent, cTransparent},
    {c302, c310, c315, c321},
    {c311, c303, c321, cTransparent},
    {c315, c305, c314, cTransparent},
    {c311, c304, c314, cTransparent},
    {c303, c312, c313, c305},
    {c303, c312, c313, c321},
    {c315, c312, c313, c305},
    {c303, c305, c315, c321},
    {c303, c305, c320, cTransparent},
    {c312, c304, c320, cTransparent},
    {c313, c304, c320, cTransparent},
    {c304, c315, c321, cTransparent},
    {c312, c304, c313, cTransparent},
    {c303, c313, c315, cTransparent},
    {c303, c312, c315, cTransparent},
};

/// @brief A heatshrink decoder to use for all WSG loads rather than allocate a new one for each WSG
/// This helps to prevent memory fragmentation in SPIRAM.
/// Note, this is outside the dn_t struct for easy access to loading fuctions without dn_t references
heatshrink_decoder* dn_hsd;
/// @brief A temporary decode space to use for all WSG loads
uint8_t* dn_decodeSpace;

// This is in order such that index is the assetIdx.
static const cnfsFileIdx_t dn_assetToWsgLookup[]
    = {DN_ALPHA_DOWN_WSG, DN_ALPHA_ORTHO_WSG,  DN_ALPHA_UP_WSG,        DN_KING_WSG,          DN_KING_SMALL_0_WSG,
       DN_PAWN_WSG,       DN_PAWN_SMALL_0_WSG, DN_BUCKET_HAT_DOWN_WSG, DN_BUCKET_HAT_UP_WSG, DN_GROUND_TILE_WSG};

// NVS keys
const char dnCharacterKey[] = "dn_character";

dn_gameData_t* gameData;

static void dn_EnterMode(void)
{
    gameData = (dn_gameData_t*)heap_caps_calloc(1, sizeof(dn_gameData_t), MALLOC_CAP_8BIT);
    memset(gameData, 0, sizeof(dn_gameData_t));

    int32_t outVal;
    readNvs32(dnCharacterKey, &outVal);
    gameData->characterSets[0] = outVal;

    strcpy(gameData->playerNames[0], "Player 1");
    strcpy(gameData->playerNames[1], "Player 2");

    strcpy(gameData->shortPlayerNames[0], "Player 1");
    strcpy(gameData->shortPlayerNames[1], "Player 2");

    // set the camera to the center of positive ints
    gameData->camera.pos
        = (vec_t){0xFFFF - (TFT_WIDTH << (DN_DECIMAL_BITS - 1)), 0xFFFF - (TFT_HEIGHT << (DN_DECIMAL_BITS - 1))};
    gameData->camera.pos.y -= (57 << DN_DECIMAL_BITS); // Move the camera a bit.
    dn_initializeEntityManager(&gameData->entityManager, gameData);

    gameData->assets[DN_ALPHA_DOWN_ASSET].originX   = 9;
    gameData->assets[DN_ALPHA_DOWN_ASSET].originY   = 54;
    gameData->assets[DN_ALPHA_DOWN_ASSET].numFrames = 1;

    gameData->assets[DN_ALPHA_ORTHO_ASSET].originX   = 10;
    gameData->assets[DN_ALPHA_ORTHO_ASSET].originY   = 10;
    gameData->assets[DN_ALPHA_ORTHO_ASSET].numFrames = 1;

    gameData->assets[DN_ALPHA_UP_ASSET].originX   = 14;
    gameData->assets[DN_ALPHA_UP_ASSET].originY   = 53;
    gameData->assets[DN_ALPHA_UP_ASSET].numFrames = 1;

    gameData->assets[DN_KING_ASSET].originX   = 10;
    gameData->assets[DN_KING_ASSET].originY   = 54;
    gameData->assets[DN_KING_ASSET].numFrames = 1;

    gameData->assets[DN_KING_SMALL_ASSET].originX   = 4;
    gameData->assets[DN_KING_SMALL_ASSET].originY   = 19;
    gameData->assets[DN_KING_SMALL_ASSET].numFrames = 1;

    gameData->assets[DN_PAWN_ASSET].originX   = 10;
    gameData->assets[DN_PAWN_ASSET].originY   = 44;
    gameData->assets[DN_PAWN_ASSET].numFrames = 1;

    gameData->assets[DN_PAWN_SMALL_ASSET].originX   = 4;
    gameData->assets[DN_PAWN_SMALL_ASSET].originY   = 14;
    gameData->assets[DN_PAWN_SMALL_ASSET].numFrames = 1;

    gameData->assets[DN_BUCKET_HAT_DOWN_ASSET].originX   = 10;
    gameData->assets[DN_BUCKET_HAT_DOWN_ASSET].originY   = 33;
    gameData->assets[DN_BUCKET_HAT_DOWN_ASSET].numFrames = 1;

    gameData->assets[DN_BUCKET_HAT_UP_ASSET].originX   = 14;
    gameData->assets[DN_BUCKET_HAT_UP_ASSET].originY   = 30;
    gameData->assets[DN_BUCKET_HAT_UP_ASSET].numFrames = 1;

    gameData->assets[DN_GROUND_TILE_ASSET].originX   = 25;
    gameData->assets[DN_GROUND_TILE_ASSET].originY   = 13;
    gameData->assets[DN_GROUND_TILE_ASSET].numFrames = 1;

    gameData->assets[DN_CURTAIN_ASSET].numFrames = 1;

    gameData->assets[DN_ALBUM_ASSET].originX = 31;
    gameData->assets[DN_ALBUM_ASSET].originY = 34;

    gameData->assets[DN_SPEAKER_ASSET].originX = 12;
    gameData->assets[DN_SPEAKER_ASSET].originY = 44;

    gameData->assets[DN_SPEAKER_STAND_ASSET].originX = 6;
    gameData->assets[DN_SPEAKER_STAND_ASSET].originY = 8;

    gameData->assets[DN_PIT_ASSET].originX = 126;
    gameData->assets[DN_PIT_ASSET].originY = 0;

    gameData->assets[DN_MINI_TILE_ASSET].originX = 10;
    gameData->assets[DN_MINI_TILE_ASSET].originY = 5;

    gameData->assets[DN_MMM_UP_ASSET].originX = 17 / 2;
    gameData->assets[DN_MMM_UP_ASSET].originY = 12 / 2;

    // Allocate WSG loading helpers
    dn_hsd = heatshrink_decoder_alloc(256, 8, 4);
    // The largest image is bb_menu2.png, decodes to 99124 bytes
    // 99328 is 1024 * 97
    dn_decodeSpace
        = heap_caps_malloc_tag(99328, MALLOC_CAP_SPIRAM, "decodeSpace"); // TODO change the size to the largest sprite

    // Load some fonts
    loadFont(IBM_VGA_8_FONT, &gameData->font_ibm, true);
    loadFont(RIGHTEOUS_150_FONT, &gameData->font_righteous, true);
    makeOutlineFont(&gameData->font_righteous, &gameData->outline_righteous, true);

    // Initialize a menu renderer
    gameData->menuRenderer = initMenuMegaRenderer(NULL, NULL, NULL);
    // Color the menu my way
    // static const paletteColor_t shadowColors[] = {
    //     c500, c050, c005, c550, c505, c055, c200, c020, c002, c220,
    // };

    // recolorMenuManiaRenderer(gameData->menuRenderer, //
    //                          c202, c540, c000,       // titleBgColor, titleTextColor, textOutlineColor
    //                          c315,                   // bgColor
    //                          c213, c035,             // outerRingColor, innerRingColor
    //                          c000, c555,             // rowColor, rowTextColor
    //                          shadowColors, ARRAY_SIZE(shadowColors), dn_LedMenuColor);

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
    dn_freeAssets();
    // Free the fonts
    freeFont(&gameData->font_ibm);
    freeFont(&gameData->font_righteous);
    freeFont(&gameData->outline_righteous);
    free(gameData);
}

/**
 * @brief The main loop for Dance Network
 *
 * @param elapsedUs The time elapsed since this was last called
 */
static void dn_MainLoop(int64_t elapsedUs)
{
    // Handle inputs
    gameData->btnDownState = 0;
    buttonEvt_t evt        = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        gameData->btnState = evt.state;
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
                if (evt.down)
                {
                    // store the down presses for this frame
                    gameData->btnDownState += evt.button;
                }
                break;
            }
            default:
            {
                break;
            }
        }
    }

    if (gameData->ui == UI_GAME)
    {
        gameData->elapsedUs = elapsedUs;
        gameData->generalTimer += gameData->elapsedUs >> 12;
        // update the whole engine via entity management
        dn_updateEntities(&gameData->entityManager);
    }

    // Draw to the TFT
    switch (gameData->ui)
    {
        case UI_MENU:
        {
            drawMenuMega(gameData->menu, gameData->menuRenderer, elapsedUs);
            break;
        }
        case UI_CONNECTING:
        {
            dn_DrawConnecting(gameData, elapsedUs);
            break;
        }
        case UI_GAME:
        {
            dn_drawEntities(&gameData->entityManager);
            break;
        }
        default:
        {
            break;
        }
    }
}

static void dn_BackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    // Fill the flat background color
    paletteColor_t* frameBuf = getPxTftFramebuffer();
    memset(&frameBuf[(y * TFT_WIDTH) + x], c212, sizeof(paletteColor_t) * w * h);
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
            dn_initializeGame();
            dn_ShowUi(UI_GAME);
        }
        else if (dn_DiffEasyStr == label)
        {
            gameData->singleSystem   = true;
            gameData->passAndPlay    = false;
            gameData->cpu.difficulty = TDIFF_EASY;
            dn_initializeGame();
            dn_ShowUi(UI_GAME);
        }
        else if (dn_DiffMediumStr == label)
        {
            gameData->singleSystem   = true;
            gameData->passAndPlay    = false;
            gameData->cpu.difficulty = TDIFF_MEDIUM;
            dn_initializeGame();
            dn_ShowUi(UI_GAME);
        }
        else if (dn_DiffHardStr == label)
        {
            gameData->singleSystem   = true;
            gameData->passAndPlay    = false;
            gameData->cpu.difficulty = TDIFF_HARD;
            dn_initializeGame();
            dn_ShowUi(UI_GAME);
        }
        else if (dn_CharacterSelStr == label)
        {
            dn_initializeCharacterSelect();
            gameData->bgMenu->title = dn_CharacterSelStr;
            dn_ShowUi(UI_GAME);
        }
        else if (dn_HowToStr == label)
        {
            // Show how to play
            dn_initializeTutorial();
            dn_ShowUi(UI_GAME);
        }
        else if (dn_RecordsStr == label)
        {
            // ttt->lastResult = TTR_RECORDS;
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
    setMegaLedsOn(gameData->menuRenderer, true);

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


static void dn_initializeTutorial(void)
{
    setMegaLedsOn(gameData->menuRenderer, false);
}

static void dn_initializeGame(void)
{
    setMegaLedsOn(gameData->menuRenderer, false);
    // if player vs CPU
    if (gameData->singleSystem && !gameData->passAndPlay)
    {
        // The player may randomly be p1 or p2.
        gameData->isPlayer1 = dn_randomInt(0, 1);
        if (!gameData->isPlayer1)
        {
            // copy player 1's character over to player 2 position.
            gameData->characterSets[1] = gameData->characterSets[0];
        }
        //[gameData->isPlayer1] actually gets the opponent.
        // give the CPU a random character.
        gameData->characterSets[gameData->isPlayer1] = (dn_characterSet_t)dn_randomInt(0, 1);
    }

    ////////////////////
    // load the assets//
    ////////////////////
    dn_loadAsset(DN_CURTAIN_WSG, 1, &gameData->assets[DN_CURTAIN_ASSET]);

    for (int player = 0; player < 2; player++)
    {
        for (int rank = 0; rank < 2; rank++)
        {
            dn_assetIdx_t curAssetIdx = dn_getAssetIdx(gameData->characterSets[player], rank, player);
            dn_loadAsset(dn_assetToWsgLookup[curAssetIdx], gameData->assets[curAssetIdx].numFrames,
                         &gameData->assets[curAssetIdx]);
        }
    }

    dn_loadAsset(DN_GROUND_TILE_WSG, 1, &gameData->assets[DN_GROUND_TILE_ASSET]);

    dn_loadAsset(DN_ALBUM_WSG, 1, &gameData->assets[DN_GROUND_TILE_ASSET]);

    dn_loadAsset(DN_SPEAKER_0_WSG, 6, &gameData->assets[DN_SPEAKER_ASSET]);

    dn_loadAsset(DN_SPEAKER_STAND_WSG, 1, &gameData->assets[DN_SPEAKER_STAND_ASSET]);

    dn_loadAsset(DN_PIT_WSG, 1, &gameData->assets[DN_PIT_ASSET]);

    dn_loadAsset(DN_MINI_TILE_WSG, 1, &gameData->assets[DN_MINI_TILE_ASSET]);

    dn_loadAsset(DN_KING_SMALL_0_WSG, 2, &gameData->assets[DN_KING_SMALL_ASSET]);

    dn_loadAsset(DN_PAWN_SMALL_0_WSG, 2, &gameData->assets[DN_PAWN_SMALL_ASSET]);

    dn_loadAsset(DN_REROLL_WSG, 1, &gameData->assets[DN_REROLL_ASSET]);

    dn_loadAsset(DN_NUMBER_0_WSG, 10, &gameData->assets[DN_NUMBER_ASSET]);

    dn_loadAsset(DN_ALBUM_EXPLOSION_0_WSG, 7, &gameData->assets[DN_ALBUM_EXPLOSION_ASSET]);

    dn_loadAsset(MMM_UP_WSG, 1, &gameData->assets[DN_MMM_UP_ASSET]);

    dn_loadAsset(DN_SWAP_0_WSG, 16, &gameData->assets[DN_SWAP_ASSET]);

    dn_loadAsset(DN_SKIP_0_WSG, 9, &gameData->assets[DN_SKIP_ASSET]);

    dn_loadAsset(DN_GLITCH_0_WSG, 6, &gameData->assets[DN_GLITCH_ASSET]);

    // LED MATRIX TEST
    // ch32v003WriteFlash(&gameData->assets[DN_NUMBER_ASSET].frames[0],
    // sizeof(gameData->assets[DN_NUMBER_ASSET].frames[0]));
    // ch32v003WriteMemory(&gameData->assets[DN_NUMBER_ASSET].frames[0],
    // sizeof(gameData->assets[DN_NUMBER_ASSET].frames[0]), 0); ch32v003Resume();

    //////////////////
    // Make the pit //
    //////////////////
    dn_entity_t* pit  = dn_createEntitySpecial(&gameData->entityManager, 1, DN_NO_ANIMATION, true, DN_PIT_ASSET, 0,
                                               (vec_t){0xFFFF, 0xFFFF - (72 << DN_DECIMAL_BITS)}, gameData);
    pit->drawFunction = dn_drawPit;

    ///////////////////////////
    // Make the left speaker //
    ///////////////////////////
    dn_createEntitySpecial(&gameData->entityManager, 1, DN_LOOPING_ANIMATION, false, DN_SPEAKER_STAND_ASSET, 0,
                           (vec_t){0xFFFF - (123 << DN_DECIMAL_BITS), 0xFFFF - (20 << DN_DECIMAL_BITS)}, gameData);
    dn_createEntitySpecial(&gameData->entityManager, 6, DN_LOOPING_ANIMATION, false, DN_SPEAKER_ASSET, 3,
                           (vec_t){0xFFFF - (123 << DN_DECIMAL_BITS), 0xFFFF - (20 << DN_DECIMAL_BITS)}, gameData);

    ////////////////////////////
    // Make the right speaker //
    ////////////////////////////
    dn_createEntitySpecial(&gameData->entityManager, 1, DN_LOOPING_ANIMATION, false, DN_SPEAKER_STAND_ASSET, 0,
                           (vec_t){0xFFFF + (122 << DN_DECIMAL_BITS), 0xFFFF - (20 << DN_DECIMAL_BITS)}, gameData);
    dn_createEntitySpecial(&gameData->entityManager, 6, DN_LOOPING_ANIMATION, false, DN_SPEAKER_ASSET, 4,
                           (vec_t){0xFFFF + (124 << DN_DECIMAL_BITS), 0xFFFF - (20 << DN_DECIMAL_BITS)}, gameData)
        ->flipped
        = true;

    ///////////////////
    // Make the board//
    ///////////////////
    dn_entity_t* board
        = dn_createEntitySimple(&gameData->entityManager, DN_GROUND_TILE_ASSET, (vec_t){0xFFFF, 0xFFFF}, gameData);
    dn_boardData_t* boardData     = (dn_boardData_t*)board->data;
    boardData->impactPos          = (dn_boardPos_t){2, 2};
    gameData->entityManager.board = board;
    for (uint8_t i = 0; i < DN_BOARD_SIZE; i++)
    {
        // boardData->tiles[1][i].rewards = 1;
        boardData->tiles[2][i].rewards = 1;
        // boardData->tiles[3][i].rewards = 1;
    }

    ////////////////////
    // Make the albums//
    ////////////////////
    dn_entity_t* albums
        = dn_createEntitySpecial(&gameData->entityManager, 0, DN_NO_ANIMATION, true, DN_ALBUM_ASSET, 0,
                                 (vec_t){0xFFFF, 63823}, gameData); // Y is 0xFFFF - (107 << DN_DECIMAL_BITS)
    albums->data = heap_caps_calloc(1, sizeof(dn_albumData_t), MALLOC_CAP_SPIRAM);
    memset(albums->data, 0, sizeof(dn_albumData_t));
    albums->dataType               = DN_ALBUMS_DATA;
    albums->drawFunction           = dn_drawAlbums;
    gameData->entityManager.albums = albums;

    // p1 album
    dn_entity_t* album1 = dn_createEntitySimple(&gameData->entityManager, DN_ALBUM_ASSET, (vec_t){0xFFFF - 1280, 63311},
                                                gameData); // Y is 0xFFFF - (139 << DN_DECIMAL_BITS)
    // creative commons album
    dn_entity_t* ccAlbum
        = dn_createEntitySimple(&gameData->entityManager, DN_ALBUM_ASSET, (vec_t){0xFFFF, 63311}, gameData);
    // p2 album
    dn_entity_t* album2
        = dn_createEntitySimple(&gameData->entityManager, DN_ALBUM_ASSET, (vec_t){0xFFFF + 1280, 63311}, gameData);

    uint8_t roll1 = dn_randomInt(0, 15);
    uint8_t roll2 = dn_randomInt(0, 15);
    while (roll2 == roll1)
    {
        roll2 = dn_randomInt(0, 15);
    }

    for (uint8_t blueTrack = 0; blueTrack < 4; blueTrack++)
    {
        if (starterAlbums[roll1][blueTrack] != cTransparent)
        {
            dn_addTrackToAlbum(album1, dn_colorToTrackCoords(starterAlbums[roll1][blueTrack]), DN_BLUE_TRACK);
        }
        if (starterAlbums[roll2][blueTrack] != cTransparent)
        {
            dn_addTrackToAlbum(album2, dn_colorToTrackCoords(starterAlbums[roll2][blueTrack]), DN_BLUE_TRACK);
        }
    }

    ((dn_albumData_t*)album1->data)->timer  = 10 << 20;
    ((dn_albumData_t*)ccAlbum->data)->timer = 11 << 20;
    ((dn_albumData_t*)album2->data)->timer  = 12 << 20;

    // p2's album is upside down
    ((dn_albumData_t*)album2->data)->rot = 180;

    ((dn_albumsData_t*)gameData->entityManager.albums->data)->p1Album              = album1;
    ((dn_albumsData_t*)gameData->entityManager.albums->data)->creativeCommonsAlbum = ccAlbum;
    ((dn_albumsData_t*)gameData->entityManager.albums->data)->p2Album              = album2;

    ///////////////////
    // Make the units//
    ///////////////////
    // p1 king
    dn_assetIdx_t assetIdx = dn_getAssetIdx(gameData->characterSets[0], DN_KING, DN_UP);
    dn_boardPos_t boardPos = {2, 4};
    boardData->p1Units[0]
        = dn_createEntitySimple(&gameData->entityManager, assetIdx, dn_boardToWorldPos(boardPos), gameData);
    boardData->tiles[boardPos.y][boardPos.x].unit = boardData->p1Units[0]; // Set the unit on the tile
    boardData->p1Units[0]->paused                 = false;
    // p1 pawns
    assetIdx = dn_getAssetIdx(gameData->characterSets[0], DN_PAWN, DN_UP);
    boardPos = (dn_boardPos_t){0, 4};
    boardData->p1Units[1]
        = dn_createEntitySimple(&gameData->entityManager, assetIdx, dn_boardToWorldPos(boardPos), gameData);
    boardData->tiles[boardPos.y][boardPos.x].unit = boardData->p1Units[1]; // Set the unit on the tile
    boardPos                                      = (dn_boardPos_t){1, 4};
    boardData->p1Units[2]
        = dn_createEntitySimple(&gameData->entityManager, assetIdx, dn_boardToWorldPos(boardPos), gameData);
    boardData->tiles[boardPos.y][boardPos.x].unit = boardData->p1Units[2]; // Set the unit on the tile
    boardPos                                      = (dn_boardPos_t){3, 4};
    boardData->p1Units[3]
        = dn_createEntitySimple(&gameData->entityManager, assetIdx, dn_boardToWorldPos(boardPos), gameData);
    boardData->tiles[boardPos.y][boardPos.x].unit = boardData->p1Units[3]; // Set the unit on the tile
    boardPos                                      = (dn_boardPos_t){4, 4};
    boardData->p1Units[4]
        = dn_createEntitySimple(&gameData->entityManager, assetIdx, dn_boardToWorldPos(boardPos), gameData);
    boardData->tiles[boardPos.y][boardPos.x].unit = boardData->p1Units[4]; // Set the unit on the tile

    // p2 king
    assetIdx = dn_getAssetIdx(gameData->characterSets[1], DN_KING, DN_DOWN);
    boardPos = (dn_boardPos_t){2, 0};
    boardData->p2Units[0]
        = dn_createEntitySimple(&gameData->entityManager, assetIdx, dn_boardToWorldPos(boardPos), gameData);
    boardData->tiles[boardPos.y][boardPos.x].unit = boardData->p2Units[0]; // Set the unit on the tile
    boardData->p2Units[0]->paused                 = false;
    // p2 pawns
    assetIdx = dn_getAssetIdx(gameData->characterSets[1], DN_PAWN, DN_DOWN);
    boardPos = (dn_boardPos_t){0, 0};
    boardData->p2Units[1]
        = dn_createEntitySimple(&gameData->entityManager, assetIdx, dn_boardToWorldPos(boardPos), gameData);
    boardData->tiles[boardPos.y][boardPos.x].unit = boardData->p2Units[1]; // Set the unit on the tile
    boardPos                                      = (dn_boardPos_t){1, 0};
    boardData->p2Units[2]
        = dn_createEntitySimple(&gameData->entityManager, assetIdx, dn_boardToWorldPos(boardPos), gameData);
    boardData->tiles[boardPos.y][boardPos.x].unit = boardData->p2Units[2]; // Set the unit on the tile
    boardPos                                      = (dn_boardPos_t){3, 0};
    boardData->p2Units[3]
        = dn_createEntitySimple(&gameData->entityManager, assetIdx, dn_boardToWorldPos(boardPos), gameData);
    boardData->tiles[boardPos.y][boardPos.x].unit = boardData->p2Units[3]; // Set the unit on the tile
    boardPos                                      = (dn_boardPos_t){4, 0};
    boardData->p2Units[4]
        = dn_createEntitySimple(&gameData->entityManager, assetIdx, dn_boardToWorldPos(boardPos), gameData);
    boardData->tiles[boardPos.y][boardPos.x].unit = boardData->p2Units[4]; // Set the unit on the tile

    /////////////////////////////
    // Make the pit foreground //
    /////////////////////////////
    dn_entity_t* pitForeground = dn_createEntitySpecial(&gameData->entityManager, 0, DN_NO_ANIMATION, true, DN_NO_ASSET,
                                                        0, (vec_t){0xffff, 0xffff - (57 << DN_DECIMAL_BITS)}, gameData);
    pitForeground->drawFunction = dn_drawPitForeground;

    //////////////////////////
    // Make the swap button //
    //////////////////////////
    dn_entity_t* swapButton = dn_createEntitySpecial(
        &gameData->entityManager, 0, DN_LOOPING_ANIMATION, true, DN_SWAP_ASSET, 3,
        (vec_t){0xFFFF - (103 << DN_DECIMAL_BITS), 0xFFFF + (25 << DN_DECIMAL_BITS)}, gameData);
    swapButton->updateFunction = dn_updateSwapButton;
    swapButton->drawFunction   = dn_drawSwapButton;

    //////////////////////////
    // Make the skip button //
    //////////////////////////
    dn_entity_t* skipButton
        = dn_createEntitySpecial(&gameData->entityManager, 0, DN_LOOPING_ANIMATION, true, DN_SKIP_ASSET, 3,
                                 (vec_t){0xFFFF + (74 << DN_DECIMAL_BITS), 0xFFFF + (25 << DN_DECIMAL_BITS)}, gameData);
    skipButton->updateFunction = dn_updateSkipButton;
    skipButton->drawFunction   = dn_drawSkipButton;

    /////////////////////////
    // Make the playerTurn //
    /////////////////////////
    dn_entity_t* playerTurn = dn_createEntitySpecial(&gameData->entityManager, 0, DN_NO_ANIMATION, true, DN_NO_ASSET, 0,
                                                     (vec_t){0xffff, 0xffff}, gameData);
    playerTurn->drawFunction = dn_drawPlayerTurn;

    /////////////////////
    // Make the curtain//
    /////////////////////
    dn_createEntitySimple(&gameData->entityManager, DN_CURTAIN_ASSET, (vec_t){0xFFFF, 0xFFFF}, gameData);
}

/**
 * @brief Load sprites needed in this UI
 *
 * @param
 */
static void dn_initializeCharacterSelect(void)
{
    dn_loadAsset(DN_ALPHA_DOWN_WSG, 1, &gameData->assets[DN_ALPHA_DOWN_ASSET]);
    dn_loadAsset(DN_ALPHA_UP_WSG, 1, &gameData->assets[DN_ALPHA_UP_ASSET]);
    dn_loadAsset(DN_BUCKET_HAT_DOWN_WSG, 1, &gameData->assets[DN_BUCKET_HAT_DOWN_ASSET]);
    dn_loadAsset(DN_BUCKET_HAT_UP_WSG, 1, &gameData->assets[DN_BUCKET_HAT_UP_ASSET]);
    dn_loadAsset(DN_KING_WSG, 1, &gameData->assets[DN_KING_ASSET]);
    dn_loadAsset(DN_PAWN_WSG, 1, &gameData->assets[DN_PAWN_ASSET]);
    dn_loadAsset(DN_GROUND_TILE_WSG, 1, &gameData->assets[DN_GROUND_TILE_ASSET]);
    ///////////////////////////////
    // Make the character select //
    ///////////////////////////////
    dn_entity_t* characterSelect = dn_createEntitySpecial(&gameData->entityManager, 0, DN_NO_ANIMATION, true,
                                                          DN_NO_ASSET, 0, (vec_t){0xFFFF, 0xFFFF}, gameData);
    characterSelect->data        = heap_caps_calloc(1, sizeof(dn_characterSelectData_t), MALLOC_CAP_SPIRAM);
    memset(characterSelect->data, 0, sizeof(dn_characterSelectData_t));
    characterSelect->dataType          = DN_CHARACTER_SELECT_DATA;
    dn_characterSelectData_t* cData    = (dn_characterSelectData_t*)characterSelect->data;
    bool selectDiamondShapeInit[9 * 5] = {
        false, false, true, false, false, false, true, true, false, false, false, true,  true, true,  false,
        true,  true,  true, true,  false, true,  true, true, true,  true,  true,  true,  true, true,  false,
        false, true,  true, true,  false, false, true, true, false, false, false, false, true, false, false,
    };
    memcpy(cData->selectDiamondShape, selectDiamondShapeInit, sizeof(selectDiamondShapeInit));
    characterSelect->updateFunction = dn_updateCharacterSelect;
    characterSelect->drawFunction   = dn_drawCharacterSelect;
}

static void dn_freeAssets(void)
{
    for (int i = 0; i < NUM_ASSETS; i++)
    {
        if (gameData->assets[i].allocated)
        {
            for (int frameIdx = 0; frameIdx < gameData->assets[i].numFrames; frameIdx++)
            {
                freeWsg(&gameData->assets[i].frames[frameIdx]);
            }
            free(gameData->assets[i].frames);
            gameData->assets[i].allocated = false;
        }
    }
}
