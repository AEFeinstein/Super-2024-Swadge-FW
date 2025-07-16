//==============================================================================
// Includes
//==============================================================================

#include "artillery.h"
#include "artillery_game.h"

//==============================================================================
// Function Declarations
//==============================================================================

void artilleryEnterMode(void);
void artilleryExitMode(void);
void artilleryMainLoop(int64_t elapsedUs);
void artilleryBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);
void artilleryEspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi);
void artilleryEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status);
bool artilleryGameMenuCb(const char* label, bool selected, uint32_t value);

//==============================================================================
// Variables
//==============================================================================

swadgeMode_t artilleryMode = {
    .modeName                 = "Artillery",
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
// Const Variables
//==============================================================================

const char load_ammo[] = "Load Ammo";

const struct
{
    const char* text;
    artilleryGameState_t nextState;
} menuEntries[] = {
    {
        .text      = "Look Around",
        .nextState = AGS_LOOK,
    },
    {
        .text      = "Drive",
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
                // TODO top level menu
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
            // TODO top level menu
            break;
        }
        case AMS_GAME:
        {
            // TODO simulate and draw game
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
