#include "danceNetwork.h"
#include "dn_typedef.h"
#include "dn_p2p.h"
#include "mainMenu.h"

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
static void dn_drawScene(void);
static void dn_drawTiles(void);
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
static const char dn_MultiStr[]       = "Wireless Play";
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

static const led_t utttLedMenuColor = {
    .r = 0x66,
    .g = 0x00,
    .b = 0x66,
};

dn_gameData_t* gameData;

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
    loadWsg(DN_GROUND_TILE_WSG, &gameData->sprites.groundTile, true);
    loadWsg(DN_ALPHA_DOWN_WSG, &gameData->sprites.alphaDown, true);
    loadWsg(DN_ALPHA_UP_WSG, &gameData->sprites.alphaUp, true);

    // Initialize a menu renderer
    gameData->menuRenderer = initMenuManiaRenderer(&gameData->font_righteous, NULL, &gameData->font_rodin);
    // Color the menu my way
    static const paletteColor_t shadowColors[] = {
        c500, c050, c005, c550, c505, c055, c200, c020, c002, c220,
    };

    recolorMenuManiaRenderer(gameData->menuRenderer, //
                             c202, c540, c000,  // titleBgColor, titleTextColor, textOutlineColor
                             c315,              // bgColor
                             c213, c035,        // outerRingColor, innerRingColor
                             c000, c555,        // rowColor, rowTextColor
                             shadowColors, ARRAY_SIZE(shadowColors), utttLedMenuColor);

    // Initialize the main menu
    gameData->menu = initMenu(dn_Name, dn_MenuCb);
    addSingleItemToMenu(gameData->menu, dn_MultiStr);
    addSingleItemToMenu(gameData->menu, dn_PassAndPlayStr);

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
}

static void dn_MainLoop(int64_t elapsedUs)
{
    buttonEvt_t evt;
    while(checkButtonQueueWrapper(&evt))
    {
        if(evt.down)
        {
            if(evt.button == PB_UP && gameData->selection[1] > 0)
            {
                gameData->selection[1]--;
                gameData->tiles[gameData->selection[1]][gameData->selection[0]].yOffset = (TFT_HEIGHT >> 2) << DECIMAL_BITS;
                gameData->tiles[gameData->selection[1]][gameData->selection[0]].yVel = -700;
                gameData->alphaFaceDir = 2; //face up
            }
            else if(evt.button == PB_DOWN && gameData->selection[1] < BOARD_SIZE - 1)
            {
                gameData->selection[1]++;
                gameData->tiles[gameData->selection[1]][gameData->selection[0]].yOffset = (TFT_HEIGHT >> 2) << DECIMAL_BITS;
                gameData->tiles[gameData->selection[1]][gameData->selection[0]].yVel = -700;
                gameData->alphaFaceDir = 0; //face down
            }
            else if(evt.button == PB_LEFT && gameData->selection[0] > 0)
            {
                gameData->selection[0]--;
                gameData->tiles[gameData->selection[1]][gameData->selection[0]].yOffset = (TFT_HEIGHT >> 2) << DECIMAL_BITS;
                gameData->tiles[gameData->selection[1]][gameData->selection[0]].yVel = -700;
                gameData->alphaFaceDir = 1; //face left
            }
            else if(evt.button == PB_RIGHT && gameData->selection[0] < BOARD_SIZE - 1)
            {
                gameData->selection[0]++;
                gameData->tiles[gameData->selection[1]][gameData->selection[0]].yOffset = (TFT_HEIGHT >> 2) << DECIMAL_BITS;
                gameData->tiles[gameData->selection[1]][gameData->selection[0]].yVel = -700;
                gameData->alphaFaceDir = 3; //face right
            }
        }
    }

    //perform hooke's law on neighboring tiles
    for (int y = 0; y < BOARD_SIZE; y++)
    {
        for (int x = 0; x < BOARD_SIZE; x++)
        {
            // Get the current tile
            dn_tileData_t* tile = &gameData->tiles[y][x];
            int8_t dampen = 3;
            if(x == gameData->selection[0] && y == gameData->selection[1])
            {
                //the selected tile approaches a particular offset
                tile->yVel += (((int16_t)(((TFT_HEIGHT >> 2) << DECIMAL_BITS) - tile->yOffset)) / 3);
            }
            else
            {
                //all unselected tiles approach neighboring tiles
                if (y > gameData->selection[1])
                {
                    tile->yVel += (((int16_t)(gameData->tiles[y - 1][x].yOffset - tile->yOffset)) / 1);
                    dampen += y - gameData->selection[1];
                }
                if (y < gameData->selection[1])
                {
                    tile->yVel += (((int16_t)(gameData->tiles[y + 1][x].yOffset - tile->yOffset)) / 1);
                    dampen += gameData->selection[1] - y;
                }
                if (x > gameData->selection[0])
                {
                    tile->yVel += (((int16_t)(gameData->tiles[y][x - 1].yOffset - tile->yOffset)) / 1);
                    dampen += x - gameData->selection[0];
                }
                if (x < gameData->selection[0])
                {
                    tile->yVel += (((int16_t)(gameData->tiles[y][x + 1].yOffset - tile->yOffset)) / 1);
                    dampen += gameData->selection[0] - x;
                }
            }

            tile->yVel /= dampen;

            // Update position with smaller time step
            uint16_t newYOffset = tile->yOffset + tile->yVel * (elapsedUs >> 14);
            // If the the yOffset would wrap around
            if(((tile->yOffset & 0x8000) && !(newYOffset & 0x8000) && tile->yVel > 0) ||
                (!(tile->yOffset & 0x8000) && (newYOffset & 0x8000) && tile->yVel < 0))
            {
                //print a message
                ESP_LOGI("Dance Network", "Tile %d,%d yOffset hit the limit", x, y);
                // Set yVel to 0
                tile->yVel = 0;
            }
            else{
                tile->yOffset  = newYOffset;
            }
        }
    }
    dn_drawScene();
}

static void dn_drawScene(void)
{
    dn_drawTiles();
}

static void dn_drawTiles(void)
{
    // Draw the tiles
    for (int y = 0; y < BOARD_SIZE; y++)
    {
        for (int x = 0; x < BOARD_SIZE; x++)
        {
            int drawX = (TFT_WIDTH >> 1) - (41 >> 1) + (x - y) * (51 >> 1);
            int drawY = (TFT_HEIGHT >> 1) + (TFT_HEIGHT >> 2) - 15 - (gameData->tiles[y][x].yOffset >> DECIMAL_BITS) + (x + y) * (26 >> 1);
            drawWsgSimple(&gameData->sprites.groundTile, drawX, drawY);
        }
    }
    int drawX = (TFT_WIDTH >> 1) - (41 >> 1) + (gameData->selection[0] - gameData->selection[1]) * (51 >> 1);
    int drawY = (TFT_HEIGHT >> 1) + (TFT_HEIGHT >> 2) - 15 - (gameData->tiles[gameData->selection[1]][gameData->selection[0]].yOffset >> DECIMAL_BITS) + (gameData->selection[0] + gameData->selection[1]) * (26 >> 1);
    //Subtract half the width of the image to center it
    drawX += gameData->sprites.alphaDown.w >> 1;
    drawY -= 41;
    switch (gameData->alphaFaceDir)
    {
        case(0)://face down
            drawWsgSimple(&gameData->sprites.alphaDown, drawX, drawY);
            /* code */
            break;
        case(1)://face left
            drawWsg(&gameData->sprites.alphaUp, drawX, drawY, true, false, 0);
            break;
        case(2)://face up
            drawWsgSimple(&gameData->sprites.alphaUp, drawX, drawY);
            /* code */
            break;
        case(3)://face right
            drawWsg(&gameData->sprites.alphaDown, drawX, drawY, true, false, 0);
            break;
        default:
            break;
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
        if (dn_MultiStr == label)
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
    gameData->menuRenderer->baseLedColor = utttLedMenuColor;
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
            break;
        }
        case UI_CHARACTER_SELECT:
        {
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