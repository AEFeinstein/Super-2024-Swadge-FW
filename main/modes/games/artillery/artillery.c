//==============================================================================
// Includes
//==============================================================================

#include "artillery.h"
#include "artillery_game.h"
#include "artillery_phys_objs.h"
#include "artillery_p2p.h"
#include "artillery_paint.h"
#include "artillery_game_over.h"
#include "artillery_help.h"
#include "mainMenu.h"

//==============================================================================
// Defines
//==============================================================================

#define WORLD_WIDTH  (TFT_WIDTH * 4)
#define WORLD_HEIGHT (TFT_HEIGHT * 4)
#define GROUND_LEVEL ((3 * WORLD_HEIGHT) / 4)

#define DEFAULT_GRAV_X 0.0f
#define DEFAULT_GRAV_Y 98.0f

//==============================================================================
// Function Declarations
//==============================================================================

void artilleryEnterMode(void);
void artilleryExitMode(void);
void artilleryMainLoop(int64_t elapsedUs);
void artilleryBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);
void artilleryEspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi);
void artilleryEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status);

bool artilleryModeMenuCb(const char* label, bool selected, uint32_t value);
bool artilleryGameMenuCb(const char* label, bool selected, uint32_t value);

//==============================================================================
// Const Variables
//==============================================================================

const char ART_TAG[] = "ART";

const char load_ammo[]   = "Load Ammo";
const char drive[]       = "Drive";
const char look_around[] = "Look Around";

const struct
{
    const char* text;
    artilleryGameState_t nextState;
} menuEntries[] = {
    {
        .text      = look_around,
        .nextState = AGS_LOOK,
    },
    {
        .text      = drive,
        .nextState = AGS_MOVE,
    },
    {
        .text      = load_ammo,
        .nextState = AGS_MENU,
    },
    {
        .text      = "Adjust Shot",
        .nextState = AGS_ADJUST,
    },
    {
        .text      = "Fire!",
        .nextState = AGS_FIRE,
    },
};

static const char str_passAndPlay[]     = "Pass and Play";
static const char str_wirelessConnect[] = "Wireless Connect";
static const char str_cpuPractice[]     = "CPU Practice";
static const char str_paintSelect[]     = "Paint Shop";
static const char str_help[]            = "Help!";
static const char str_exit[]            = "Exit";

static const char modeName[] = "Vector Tanks";

//==============================================================================
// Variables
//==============================================================================

swadgeMode_t artilleryMode = {
    .modeName                 = modeName,
    .wifiMode                 = ESP_NOW,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = artilleryEnterMode,
    .fnExitMode               = artilleryExitMode,
    .fnMainLoop               = artilleryMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = artilleryBackgroundDrawCallback,
    .fnEspNowRecvCb           = artilleryEspNowRecvCb,
    .fnEspNowSendCb           = artilleryEspNowSendCb,
    .fnAdvancedUSB            = NULL,
    .fnDacCb                  = NULL,
    .fnAddToSwadgePassPacket  = NULL,
    .trophyData               = NULL,
};

artilleryData_t* ad;

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief This function is called when this mode is started. It should initialize variables and start the mode.
 */
void artilleryEnterMode(void)
{
    setFrameRateUs(PHYS_TIME_STEP_US);

    ad = heap_caps_calloc(1, sizeof(artilleryData_t), MALLOC_CAP_8BIT);

    // Initialize mode menu
    ad->modeMenu = initMenu(modeName, artilleryModeMenuCb);
    addSingleItemToMenu(ad->modeMenu, str_passAndPlay);
    addSingleItemToMenu(ad->modeMenu, str_wirelessConnect);
    addSingleItemToMenu(ad->modeMenu, str_cpuPractice);
    addSingleItemToMenu(ad->modeMenu, str_paintSelect);
    addSingleItemToMenu(ad->modeMenu, str_help);
    addSingleItemToMenu(ad->modeMenu, str_exit);

    ad->blankMenu = initMenu(str_paintSelect, NULL);

    // Initialize mode menu renderer
    ad->mRenderer = initMenuMegaRenderer(NULL, NULL, NULL);
    ad->scoreFont = ad->mRenderer->menuFont;

    // Initialize in-game menu
    ad->gameMenu = initMenu(NULL, artilleryGameMenuCb);
    for (int mIdx = 0; mIdx < ARRAY_SIZE(menuEntries); mIdx++)
    {
        if (load_ammo == menuEntries[mIdx].text)
        {
            ad->gameMenu = startSubMenu(ad->gameMenu, load_ammo);

            uint16_t numAmmos;
            const artilleryAmmoAttrib_t* ammos = getAmmoAttributes(&numAmmos);
            for (int aIdx = 0; aIdx < numAmmos; aIdx++)
            {
                addSingleItemToMenu(ad->gameMenu, ammos[aIdx].name);
            }
            ad->gameMenu = endSubMenu(ad->gameMenu);
        }
        else
        {
            addSingleItemToMenu(ad->gameMenu, menuEntries[mIdx].text);
        }
    }

    // Initialize in-game menu renderer
    ad->smRenderer = initMenuSimpleRenderer(NULL, c005, c111, c555, 5);

    // Initialize p2p
    p2pInitialize(&ad->p2p, 0x76, artillery_p2pConCb, artillery_p2pMsgRxCb, -70);

    // Start on the mode menu
    ad->mState = AMS_MENU;

    // Set the touchpad as untouched
    ad->tpLastPhi = INT32_MIN;

    // Load tank color
    artilleryPaintLoadColor(ad);
}

/**
 * @brief This function is called when the mode is exited. It should free any allocated memory.
 */
void artilleryExitMode(void)
{
    // Deinit physics
    deinitPhys(ad->phys);

    // Deinit menus
    deinitMenuSimpleRenderer(ad->smRenderer);
    deinitMenuMegaRenderer(ad->mRenderer);
    deinitMenu(ad->modeMenu);
    deinitMenu(ad->blankMenu);
    deinitMenu(ad->gameMenu);

    // Deinit p2p
    p2pDeinit(&ad->p2p);
    while (ad->p2pQueue.first)
    {
        heap_caps_free(pop(&ad->p2pQueue));
    }

    // Free everything
    heap_caps_free(ad);
}

/**
 * @brief This function is called from the main loop. It's pretty quick, but the timing may be inconsistent.
 *
 * @param elapsedUs The time elapsed since the last time this function was called. Use this value to determine when
 * it's time to do things
 */
void artilleryMainLoop(int64_t elapsedUs)
{
    bool barrelChanged = false;
    buttonEvt_t evt    = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        switch (ad->mState)
        {
            default:
            case AMS_MENU:
            {
                ad->modeMenu = menuButton(ad->modeMenu, evt);
                break;
            }
            case AMS_CONNECTING:
            {
                if (evt.down && PB_B == evt.button)
                {
                    // Cancel the connection
                    p2pRestart(&ad->p2p);
                }
                break;
            }
            case AMS_GAME:
            {
                barrelChanged = artilleryGameInput(ad, evt);
                break;
            }
            case AMS_HELP:
            {
                artilleryHelpInput(ad, &evt);
                break;
            }
            case AMS_PAINT:
            {
                artilleryPaintInput(ad, &evt);
                break;
            }
            case AMS_GAME_OVER:
            {
                artilleryGameOverInput(ad, &evt);
                break;
            }
        }
    }

    switch (ad->mState)
    {
        default:
        case AMS_MENU:
        {
            drawMenuMega(ad->modeMenu, ad->mRenderer, elapsedUs);
            break;
        }
        case AMS_CONNECTING:
        {
            // Draw background
            drawMenuMega(ad->blankMenu, ad->mRenderer, elapsedUs);

            // Draw connection text
            font_t* f      = ad->mRenderer->menuFont;
            int16_t tWidth = textWidth(f, ad->conStr) + 1;
            drawTextShadow(f, c555, c000, ad->conStr, (TFT_WIDTH - tWidth) / 2, 135 - (f->height / 2));

            // Check for packets to transmit
            artilleryCheckTxQueue(ad);
            break;
        }
        case AMS_GAME:
        {
            artilleryGameLoop(ad, elapsedUs, barrelChanged);
            artilleryCheckTxQueue(ad);
            break;
        }
        case AMS_HELP:
        {
            artilleryHelpLoop(ad, elapsedUs);
            break;
        }
        case AMS_PAINT:
        {
            artilleryPaintLoop(ad, elapsedUs);
            break;
        }
        case AMS_GAME_OVER:
        {
            artilleryGameOverLoop(ad, elapsedUs);
            break;
        }
    }
}

/**
 * @brief This function is called when the display driver wishes to update a section of the display.
 *
 * @param x the x coordinate that should be updated
 * @param y the x coordinate that should be updated
 * @param w the width of the rectangle to be updated
 * @param h the height of the rectangle to be updated
 * @param up update number
 * @param upNum update number denominator
 */
void artilleryBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    if (AMS_GAME == ad->mState)
    {
        // If this is the first block
        if (y == 0)
        {
            // Update terrain in the background
            physStepBackground(ad->phys);
        }

        // Always draw the background
        drawPhysBackground(ad->phys, x, y, w, h);
    }
    else
    {
        // Not the game, simply black it out
        fillDisplayArea(x, y, x + w, y + h, c000);
    }
}

/**
 * @brief This function is called whenever an ESP-NOW packet is received.
 *
 * @param esp_now_info Information about the transmission, including The MAC addresses
 * @param data     A pointer to the data received
 * @param len      The length of the data received
 * @param rssi     The RSSI for this packet, from 1 (weak) to ~90 (touching)
 */
void artilleryEspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi)
{
    p2pRecvCb(&ad->p2p, esp_now_info->src_addr, data, len, rssi);
}

/**
 * @brief This function is called whenever an ESP-NOW packet is sent. It is just a status callback whether or not
 * the packet was actually sent. This will be called after calling espNowSend().
 *
 * @param mac_addr The MAC address which the data was sent to
 * @param status   The status of the transmission
 */
void artilleryEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status)
{
    p2pSendCb(&ad->p2p, mac_addr, status);
}

/**
 * @brief Handle callbacks from the mode menu
 *
 * @param label The label selected or scrolled to
 * @param selected true if the label was selected, false otherwise
 * @param value Unused
 * @return true to go up one level after selecting, false to remain on this level
 */
bool artilleryModeMenuCb(const char* label, bool selected, uint32_t value)
{
    if (selected)
    {
        if (str_passAndPlay == label)
        {
            artilleryInitGame(AG_PASS_AND_PLAY, true);
            artillerySwitchToGameState(ad, AGS_MENU);
        }
        else if (str_wirelessConnect == label)
        {
            p2pStartConnection(&ad->p2p);
            ad->mState           = AMS_CONNECTING;
            ad->blankMenu->title = str_wirelessConnect;

            ad->p2pSetColorReceived   = false;
            ad->p2pSetWorldReceived   = false;
            ad->p2pAddTerrainReceived = false;
            ad->p2pCloudsReceived     = false;
        }
        else if (str_cpuPractice == label)
        {
            // TODO implement CPU difficulty
            artilleryInitGame(AG_CPU_PRACTICE, true);
            artillerySwitchToGameState(ad, AGS_MENU);
        }
        else if (str_help == label)
        {
            ESP_LOGI(ART_TAG, "TODO Start help!");
            ad->mState = AMS_HELP;
        }
        else if (str_paintSelect == label)
        {
            // Set title, which may be overwritten by AMS_GAME_OVER
            ad->blankMenu->title = str_paintSelect;
            ad->mState           = AMS_PAINT;
        }
        else if (str_exit == label)
        {
            // Exit to the main menu
            switchToSwadgeMode(&mainMenuMode);
        }
    }
    return false;
}

/**
 * @brief Handle callbacks from the in-game menu
 *
 * @param label The label selected or scrolled to
 * @param selected true if the label was selected, false otherwise
 * @param value Unused
 * @return true to go up one level after selecting, false to remain on this level
 */
bool artilleryGameMenuCb(const char* label, bool selected, uint32_t value)
{
    if (selected)
    {
        // Iterate to see if a menuEntry was selected
        for (int mIdx = 0; mIdx < ARRAY_SIZE(menuEntries); mIdx++)
        {
            if (label == menuEntries[mIdx].text)
            {
                artillerySwitchToGameState(ad, menuEntries[mIdx].nextState);
                return false;
            }
        }

        uint16_t numAmmos;
        const artilleryAmmoAttrib_t* ammos = getAmmoAttributes(&numAmmos);

        // If not, iterate to see if an ammo was selected
        for (int aIdx = 0; aIdx < numAmmos; aIdx++)
        {
            if (label == ammos[aIdx].name)
            {
                ad->players[ad->plIdx]->ammoIdx = aIdx;
                // Return true to pop back to the parent menu
                return true;
            }
        }
    }

    return false;
}

/**
 * @brief Set if the drive option is visible in the game menu
 *
 * @param visible true to show "Drive," false to hide it
 */
void setDriveInMenu(bool visible)
{
    // Return to the top level menu, just in case
    menu_t* menu = ad->gameMenu;
    while (menu->parentMenu)
    {
        menu = menu->parentMenu;
    }

    // Check if drive is already in the menu
    bool driveInMenu = false;
    node_t* mNode    = menu->items->first;
    while (mNode)
    {
        menuItem_t* item = mNode->val;
        if (drive == item->label)
        {
            driveInMenu = true;
            break;
        }
        mNode = mNode->next;
    }

    // Adjust menu as necessary
    if (visible && !driveInMenu)
    {
        insertSingleItemToMenuAfter(ad->gameMenu, drive, look_around);
        ad->smRenderer->numRows++;
    }
    else if (!visible && driveInMenu)
    {
        removeSingleItemFromMenu(ad->gameMenu, drive);
        ad->smRenderer->numRows--;
    }
}

/**
 * @brief Initialize the game, including physics and timers
 *
 * @param gameType The type of game to initialize
 * @param generateTerrain true to generate terrain, false to receive it from a packet later
 */
void artilleryInitGame(artilleryGameType_t gameType, bool generateTerrain)
{
    // For non-wireless games, pick a random color that's not the player's
    if (gameType != AG_WIRELESS)
    {
        int32_t randomOffset = 1 + esp_random() % (artilleryGetNumTankColors() - 1);
        ad->theirColorIdx    = (ad->myColorIdx + randomOffset) % artilleryGetNumTankColors();
    }

    ad->gameType = gameType;

    // Initialize physics, including terrain
    ad->phys = initPhys(WORLD_WIDTH, WORLD_HEIGHT, GROUND_LEVEL, DEFAULT_GRAV_X, DEFAULT_GRAV_Y, generateTerrain);

    if (generateTerrain)
    {
        // Initialize players, including flattening terrain under them
        paletteColor_t colors[4];
        if (gameType == AG_WIRELESS)
        {
            artilleryGetTankColors(ad->theirColorIdx, &colors[0], &colors[1]);
            artilleryGetTankColors(ad->myColorIdx, &colors[2], &colors[3]);
        }
        else
        {
            artilleryGetTankColors(ad->myColorIdx, &colors[0], &colors[1]);
            artilleryGetTankColors(ad->theirColorIdx, &colors[2], &colors[3]);
        }
        physSpawnPlayers(ad->phys, NUM_PLAYERS, ad->players, colors);
    }

    // Start with a full movement timer
    ad->moveTimerUs = TANK_MOVE_TIME_US;

    // Start on turn 1
    ad->turn = 1;

    // Switch to showing the game
    ad->mState = AMS_GAME;

    // Start the game on the game menu
    artillerySwitchToGameState(ad, AGS_WAIT);
}

/**
 * @return A pointer to all the mode data
 */
artilleryData_t* getArtilleryData(void)
{
    return ad;
}
