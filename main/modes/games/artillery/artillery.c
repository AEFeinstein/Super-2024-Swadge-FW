//==============================================================================
// Includes
//==============================================================================

#include "artillery.h"
#include "artillery_game.h"
#include "artillery_phys_objs.h"

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

void artilleryInitGame(void);

//==============================================================================
// Const Variables
//==============================================================================

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

const struct
{
    const char* text;
    artilleryAmmoType_t ammo;
} ammoEntries[] = {
    {
        .text = "Normal Shot",
        .ammo = AMMO_NORMAL,
    },
    {
        .text = "Big Explosion",
        .ammo = AMMO_BIG_EXPLODE,
    },
    {
        .text = "Three Shot",
        .ammo = AMMO_THREE,
    },
    {
        .text = "Five Shot",
        .ammo = AMMO_FIVE,
    },
    {
        .text = "Sniper",
        .ammo = AMMO_SNIPER,
    },
    {
        .text = "Machine Gun",
        .ammo = AMMO_MACHINE_GUN,
    },
    {
        .text = "Bouncy Shot",
        .ammo = AMMO_BOUNCY,
    },
    {
        .text = "Jackhammer",
        .ammo = AMMO_JACKHAMMER,
    },
    {
        .text = "Hill Maker",
        .ammo = AMMO_HILL_MAKER,
    },
    {
        .text = "Jump Jets",
        .ammo = AMMO_JUMP,
    },
};

static const char str_passAndPlay[]     = "Pass and Play";
static const char str_wirelessConnect[] = "Wireless Connect";
static const char str_cpuPractice[]     = "CPU Practice";
static const char str_help[]            = "Help!";

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
    setFrameRateUs(1000000 / 60);

    ad = heap_caps_calloc(1, sizeof(artilleryData_t), MALLOC_CAP_8BIT);

    ad->modeMenu  = initMenu(modeName, artilleryModeMenuCb);
    ad->mRenderer = initMenuManiaRenderer(NULL, NULL, NULL);

    addSingleItemToMenu(ad->modeMenu, str_passAndPlay);
    addSingleItemToMenu(ad->modeMenu, str_wirelessConnect);
    addSingleItemToMenu(ad->modeMenu, str_cpuPractice);
    addSingleItemToMenu(ad->modeMenu, str_help);

    ad->mState = AMS_MENU;
}

/**
 * @brief This function is called when the mode is exited. It should free any allocated memory.
 */
void artilleryExitMode(void)
{
    deinitPhys(ad->phys);
    deinitMenuSimpleRenderer(ad->smRenderer);
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
    buttonEvt_t evt = {0};
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
            case AMS_GAME:
            {
                artilleryGameInput(ad, evt);
                break;
            }
        }
    }

    switch (ad->mState)
    {
        default:
        case AMS_MENU:
        {
            drawMenuMania(ad->modeMenu, ad->mRenderer, elapsedUs);
            break;
        }
        case AMS_GAME:
        {
            artilleryGameLoop(ad, elapsedUs);
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
    fillDisplayArea(x, y, x + w, y + h, c000);
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
}

/**
 * @brief TODO
 *
 * @param label
 * @param selected
 * @param value
 * @return true
 * @return false
 */
bool artilleryModeMenuCb(const char* label, bool selected, uint32_t value)
{
    if (selected)
    {
        if (str_passAndPlay == label)
        {
            artilleryInitGame();
        }
        else if (str_wirelessConnect == label)
        {
            // TODO
        }
        else if (str_cpuPractice == label)
        {
            // TODO
        }
        else if (str_help == label)
        {
            // TODO
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
 */
bool artilleryGameMenuCb(const char* label, bool selected, uint32_t value)
{
    if (selected)
    {
        for (int mIdx = 0; mIdx < ARRAY_SIZE(menuEntries); mIdx++)
        {
            if (label == menuEntries[mIdx].text)
            {
                artillerySwitchToState(ad, menuEntries[mIdx].nextState);
                return false;
            }
        }

        for (int aIdx = 0; aIdx < ARRAY_SIZE(ammoEntries); aIdx++)
        {
            if (label == ammoEntries[aIdx].text)
            {
                ad->players[ad->plIdx]->ammo      = ammoEntries[aIdx].ammo;
                ad->players[ad->plIdx]->ammoLabel = label;
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
 * @brief TODO
 *
 */
void artilleryInitGame(void)
{
#define WORLD_WIDTH  (TFT_WIDTH * 2)
#define WORLD_HEIGHT (TFT_HEIGHT * 2)
    ad->phys = initPhys(WORLD_WIDTH, WORLD_HEIGHT, 0, 1e-10);

#ifdef MOUNTAIN
    #define GROUND_LEVEL (WORLD_HEIGHT - 40)
    // Add some ground
    vecFl_t groundPoints[] = {
        {.x = 0, .y = GROUND_LEVEL},
        // {.x = WORLD_WIDTH / 8, .y = WORLD_HEIGHT - 1},
        // {.x = WORLD_WIDTH / 4, .y = 40},
        {.x = WORLD_WIDTH / 4, .y = GROUND_LEVEL},
        {.x = WORLD_WIDTH / 2, .y = WORLD_HEIGHT / 2},
        {.x = 3 * WORLD_WIDTH / 4, .y = GROUND_LEVEL + 20},
        {.x = WORLD_WIDTH, .y = GROUND_LEVEL},
    };
    for (int idx = 0; idx < ARRAY_SIZE(groundPoints) - 1; idx++)
    {
        physAddLine(ad->phys, groundPoints[idx].x, groundPoints[idx].y, groundPoints[idx + 1].x,
                    groundPoints[idx + 1].y, true);
    }
#else
    #define SEG_WIDTH    8
    #define GROUND_LEVEL (WORLD_HEIGHT / 2)
    for (int32_t i = 0; i < WORLD_WIDTH; i += SEG_WIDTH)
    {
        physAddLine(ad->phys, i, GROUND_LEVEL, i + SEG_WIDTH, GROUND_LEVEL, true);
    }
#endif

    // Add some players
#define PLAYER_RADIUS 8

    ad->players[0] = physAddCircle(ad->phys, WORLD_WIDTH / 8, GROUND_LEVEL - PLAYER_RADIUS - 1, PLAYER_RADIUS, CT_TANK);
    ad->players[1]
        = physAddCircle(ad->phys, (7 * WORLD_WIDTH) / 8, GROUND_LEVEL - PLAYER_RADIUS - 1, PLAYER_RADIUS, CT_TANK);

    // Start with a full movement timer
    ad->moveTimerUs = TANK_MOVE_TIME_US;

    // Test circle-line collisions
    // physAddCircle(ad->phys, WORLD_WIDTH / 2 + 16, 30, 8, CT_SHELL);
    // physAddCircle(ad->phys, WORLD_WIDTH / 2 - 16, 30, 8, CT_SHELL);
    // physAddCircle(ad->phys, WORLD_WIDTH / 2, 30, 8, CT_SHELL);

    // Test circle-circle collisions
    // physAddCircle(ad->phys, (3 * WORLD_WIDTH) / 4 + 4, 20, 8, CT_SHELL);
    // physAddCircle(ad->phys, (3 * WORLD_WIDTH) / 4 - 4, 50, 8, CT_SHELL);
    physAddCircle(ad->phys, (3 * WORLD_WIDTH) / 4, 80, 8, CT_OBSTACLE);

    // Initialize in-game menu and renderer
    ad->gameMenu = initMenu(NULL, artilleryGameMenuCb);
    for (int mIdx = 0; mIdx < ARRAY_SIZE(menuEntries); mIdx++)
    {
        if (load_ammo == menuEntries[mIdx].text)
        {
            ad->gameMenu = startSubMenu(ad->gameMenu, load_ammo);
            for (int aIdx = 0; aIdx < ARRAY_SIZE(ammoEntries); aIdx++)
            {
                addSingleItemToMenu(ad->gameMenu, ammoEntries[aIdx].text);
            }
            ad->gameMenu = endSubMenu(ad->gameMenu);
        }
        else
        {
            addSingleItemToMenu(ad->gameMenu, menuEntries[mIdx].text);
        }
    }
    ad->smRenderer = initMenuSimpleRenderer(NULL, c005, c111, c555, 5);

    ad->mState = AMS_GAME;
    artillerySwitchToState(ad, AGS_MENU);
}