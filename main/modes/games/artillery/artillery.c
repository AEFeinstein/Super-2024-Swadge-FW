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

const char str_load_ammo[]   = "Load Ammo";
const char str_drive[]       = "Drive";
const char str_look_around[] = "Look Around";
const char str_adjust[]      = "Adjust Shot";
const char str_fire[]        = "Fire!";

const struct
{
    const char* text;
    artilleryGameState_t nextState;
} menuEntries[] = {
    {
        .text      = str_look_around,
        .nextState = AGS_LOOK,
    },
    {
        .text      = str_drive,
        .nextState = AGS_MOVE,
    },
    {
        .text      = str_load_ammo,
        .nextState = AGS_MENU,
    },
    {
        .text      = str_adjust,
        .nextState = AGS_ADJUST,
    },
    {
        .text      = str_fire,
        .nextState = AGS_FIRE,
    },
};

const char str_passAndPlay[]     = "Pass and Play";
const char str_wirelessConnect[] = "Wireless Connect";
const char str_cpuPractice[]     = "CPU Practice";
const char str_paintSelect[]     = "Paint Shop";
const char str_help[]            = "Help!";
const char str_exit[]            = "Exit";

const char artilleryModeName[] = "Vector Tanks";

// List of trophies
const trophyData_t artilleryTrophies[] = {
    {
        // RoyalSampler
        .title       = "Royal Sampler",
        .description = "Try firing all the different ammo",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_CHECKLIST,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 0x1FFF, // Thirteen ammos in ammoAttributes[]
    },
    {
        // HittingYourself
        .title       = "Stop Hitting Yourself",
        .description = "Damage yourself with a shot you fired",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 1, // For trigger type, set to one
    },
    {
        // ToTheMoon
        .title       = "To The Moon",
        .description = "Bounce off the top of the sky",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 1, // For trigger type, set to one
    },
    {
        // PassAndPlay
        .title       = "Pass Master",
        .description = "Play 10 Pass and Play games",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_ADDITIVE,
        .difficulty  = TROPHY_DIFF_MEDIUM,
        .maxVal      = 10,
    },
    {
        // P2P
        .title       = "Wireless Master",
        .description = "Play 10 Wireless Connect games",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_ADDITIVE,
        .difficulty  = TROPHY_DIFF_MEDIUM,
        .maxVal      = 10,
    },
    {
        // Skynet
        .title       = "Take That Skynet",
        .description = "You defeated the CPU player",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_HARD,
        .maxVal      = 1, // For trigger type, set to one
    },
    {
        // Sniper
        .title       = "Sniper No Sniping",
        .description = "Hit another player with the sniper ammo",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_HARD,
        .maxVal      = 1, // For trigger type, set to one
    },
};

// Individual mode settings
const trophySettings_t artilleryTrophySettings = {
    .drawFromBottom   = false,
    .staticDurationUs = DRAW_STATIC_US * 6,
    .slideDurationUs  = DRAW_SLIDE_US,
    .namespaceKey     = artilleryModeName,
};

// This is passed to the swadgeMode_t
const trophyDataList_t artilleryTrophyData = {
    .settings = &artilleryTrophySettings,
    .list     = artilleryTrophies,
    .length   = ARRAY_SIZE(artilleryTrophies),
};

//==============================================================================
// Variables
//==============================================================================

swadgeMode_t artilleryMode = {
    .modeName                 = artilleryModeName,
    .wifiMode                 = ESP_NOW,
    .overrideUsb              = true,
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
    .trophyData               = &artilleryTrophyData,
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
    ad->modeMenu = initMenu(artilleryModeName, artilleryModeMenuCb);
    addSingleItemToMenu(ad->modeMenu, str_passAndPlay);
    addSingleItemToMenu(ad->modeMenu, str_wirelessConnect);
    addSingleItemToMenu(ad->modeMenu, str_cpuPractice);
    addSingleItemToMenu(ad->modeMenu, str_paintSelect);
    addSingleItemToMenu(ad->modeMenu, str_help);
    addSingleItemToMenu(ad->modeMenu, str_exit);

    ad->blankMenu = initMenu(str_paintSelect, NULL);

    // Load fonts, not to SPIRAM
    loadFont(OXANIUM_FONT, &ad->font_oxanium, false);
    makeOutlineFont(&ad->font_oxanium, &ad->font_oxaniumOutline, false);
    loadFont(PULSE_AUX_FONT, &ad->font_pulseAux, false);
    makeOutlineFont(&ad->font_pulseAux, &ad->font_pulseAuxOutline, false);

    // Initialize mode menu renderer
    ad->mRenderer = initMenuMegaRenderer(&ad->font_oxanium, &ad->font_oxaniumOutline, &ad->font_pulseAux);

    static const paletteColor_t cycle[] = {
        COLOR_GRADIENT_1, COLOR_GRADIENT_2, COLOR_GRADIENT_3, COLOR_GRADIENT_4,
        COLOR_GRADIENT_5, COLOR_GRADIENT_6, COLOR_GRADIENT_7, COLOR_GRADIENT_8,
    };
    recolorMenuMegaRenderer(ad->mRenderer,
                            COLOR_TEXT,              //
                            COLOR_TEXT_SHADOW,       //
                            COLOR_HEXA_BACKGROUND,   //
                            COLOR_BODY_BG,           //
                            COLOR_BODY_ACCENT_DARK,  //
                            COLOR_BODY_ACCENT_LIGHT, //
                            COLOR_BODY_ARROW_BG,     //
                            COLOR_ROW_UNSEL_BG,      //
                            COLOR_ROW_UNSEL_SHADOW,  //
                            COLOR_ROW_SEL_BG,        //
                            COLOR_ROW_SEL_ACCENT,    //
                            COLOR_ROW_SEL_OUTLINE,   //
                            COLOR_ROW_ARROW_BG,      //
                            cycle, ARRAY_SIZE(cycle));

    // Initialize in-game menu
    ad->gameMenu = initMenuRam(NULL, artilleryGameMenuCb, MALLOC_CAP_8BIT);
    for (int mIdx = 0; mIdx < ARRAY_SIZE(menuEntries); mIdx++)
    {
        if (str_load_ammo == menuEntries[mIdx].text)
        {
            ad->gameMenu = startSubMenu(ad->gameMenu, str_load_ammo);
            ad->gameMenu = endSubMenu(ad->gameMenu);
        }
        else
        {
            addSingleItemToMenu(ad->gameMenu, menuEntries[mIdx].text);
        }
    }

    // Initialize in-game menu renderer
    ad->smRenderer = initMenuSimpleRenderer(NULL, COLOR_SIMPLE_MENU_BORDER, COLOR_SIMPLE_MENU_BACKGROUND,
                                            COLOR_SIMPLE_MENU_TEXT, 5);

    // Initialize help menu
    artilleryHelpInit(ad);

    // Initialize p2p
    p2pInitialize(&ad->p2p, 0x76, artillery_p2pConCb, artillery_p2pMsgRxCb, -70);

    // Start on the mode menu
    ad->mState = AMS_MENU;

    // Set the touchpad as untouched
    ad->tpLastPhi = INT32_MIN;

    // Load tank color
    if (false == artilleryPaintLoadColor(ad))
    {
        // No paint yet, load to the paint shop
        ad->blankMenu->title = str_paintSelect;
        ad->mState           = AMS_PAINT;
        setDrawBody(ad->mRenderer, false);
    }

    // Load and initialize sounds
    loadMidiFile(VT_FIGHT_ON_MID, &ad->bgms[0], false);
    loadMidiFile(VT_FUNK_MID, &ad->bgms[1], false);
    loadMidiFile(VT_RISK_MID, &ad->bgms[2], false);
    loadMidiFile(VT_POP_MID, &ad->bgms[3], false);
    globalMidiPlayerGet(MIDI_BGM)->loop = true;
    midiGmOn(globalMidiPlayerGet(MIDI_BGM));
    midiPause(globalMidiPlayerGet(MIDI_BGM), true);

    // Set up SFX (see sfxPlayer)
    midiGmOn(globalMidiPlayerGet(MIDI_SFX));
    midiPause(globalMidiPlayerGet(MIDI_SFX), false);

    // Write ch32 assets
    ch32v003WriteBitmapAsset(EYES_CC, EYES_DEFAULT_GS);
    ch32v003WriteBitmapAsset(EYES_UL, EYES_UL_GS);
    ch32v003WriteBitmapAsset(EYES_UC, EYES_UC_GS);
    ch32v003WriteBitmapAsset(EYES_UR, EYES_UR_GS);
    ch32v003WriteBitmapAsset(EYES_CR, EYES_CR_GS);
    ch32v003WriteBitmapAsset(EYES_DR, EYES_DR_GS);
    ch32v003WriteBitmapAsset(EYES_DC, EYES_DC_GS);
    ch32v003WriteBitmapAsset(EYES_DL, EYES_DL_GS);
    ch32v003WriteBitmapAsset(EYES_CL, EYES_CL_GS);
    ch32v003WriteBitmapAsset(EYES_DEAD, EYES_DEAD_GS);

    // Start idle
    ad->eyeSlot = EYES_CC;
    ch32v003SelectBitmap(ad->eyeSlot);
}

/**
 * @brief This function is called when the mode is exited. It should free any allocated memory.
 */
void artilleryExitMode(void)
{
    // Deinit physics
    deinitPhys(ad->phys);

    // Deinit help menu
    artilleryHelpDeinit(ad);

    // Deinit menus
    deinitMenuSimpleRenderer(ad->smRenderer);
    deinitMenuMegaRenderer(ad->mRenderer);
    deinitMenu(ad->modeMenu);
    deinitMenu(ad->blankMenu);
    deinitMenu(ad->gameMenu);

    // Free fonts
    freeFont(&ad->font_oxanium);
    freeFont(&ad->font_oxaniumOutline);
    freeFont(&ad->font_pulseAux);
    freeFont(&ad->font_pulseAuxOutline);

    // Deinit p2p
    p2pDeinit(&ad->p2p, true);
    while (ad->p2pQueue.first)
    {
        heap_caps_free(pop(&ad->p2pQueue));
    }

    // Deinit music
    for (uint32_t i = 0; i < ARRAY_SIZE(ad->bgms); i++)
    {
        unloadMidiFile(&ad->bgms[i]);
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
            drawTextShadow(f, COLOR_TEXT, COLOR_TEXT_SHADOW, ad->conStr, (TFT_WIDTH - tWidth) / 2,
                           135 - (f->height / 2));

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

    // Run the timer to clear dead eyes
    if (ad->deadEyeTimer > 0)
    {
        ad->deadEyeTimer -= elapsedUs;
        if (ad->deadEyeTimer <= 0)
        {
            // Go back to idle
            ad->eyeSlot = EYES_CC;
            ch32v003SelectBitmap(ad->eyeSlot);
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
            artillerySwitchToGameState(ad, AGS_TOUR);
        }
        else if (str_wirelessConnect == label)
        {
            p2pRestart(&ad->p2p);
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
            artillerySwitchToGameState(ad, AGS_TOUR);
        }
        else if (str_help == label)
        {
            ad->mState = AMS_HELP;
        }
        else if (str_paintSelect == label)
        {
            // Set title, which may be overwritten by AMS_GAME_OVER
            ad->blankMenu->title = str_paintSelect;
            ad->mState           = AMS_PAINT;
            setDrawBody(ad->mRenderer, false);
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
        if (str_drive == item->label)
        {
            driveInMenu = true;
            break;
        }
        mNode = mNode->next;
    }

    // Adjust menu as necessary
    if (visible && !driveInMenu)
    {
        insertSingleItemToMenuAfter(ad->gameMenu, str_drive, str_look_around);
        ad->smRenderer->numRows++;
    }
    else if (!visible && driveInMenu)
    {
        removeSingleItemFromMenu(ad->gameMenu, str_drive);
        ad->smRenderer->numRows--;
    }
}

/**
 * @brief TODO doc
 *
 * @param ad
 */
void setAmmoInMenu(void)
{
    // Return to the top level menu, just in case
    menu_t* menu = ad->gameMenu;
    while (menu->parentMenu)
    {
        menu = menu->parentMenu;
    }

    // Clear out the ammo menu
    menu = menuNavigateToItem(menu, str_load_ammo);
    menu = ((menuItem_t*)menu->currentItem->val)->subMenu;
    removeAllItemsFromMenu(menu);

    // Get a reference to the player
    list_t* availableAmmo = &ad->players[ad->plIdx]->availableAmmo;

    // Get a reference to ammo attributes
    uint16_t numAmmos;
    const artilleryAmmoAttrib_t* ammos = getAmmoAttributes(&numAmmos);

    // Add this player's ammo to the menu
    node_t* aNode = availableAmmo->first;
    while (aNode)
    {
        // Add available ammo to the menu
        addSingleItemToMenu(menu, ammos[(intptr_t)aNode->val].name);
        // Iterate
        aNode = aNode->next;
    }
    menu = menuNavigateToTopItem(menu);

    // Return to the top of the main menu
    while (menu->parentMenu)
    {
        menu = menu->parentMenu;
    }
    menu = menuNavigateToTopItem(menu);
}

/**
 * @brief Navigate to the load ammo menu
 */
void openAmmoMenu(void)
{
    // Return to the top level menu, just in case
    menu_t* menu = ad->gameMenu;
    while (menu->parentMenu)
    {
        menu = menu->parentMenu;
    }

    ad->gameMenu = menuNavigateToItem(ad->gameMenu, str_load_ammo);
    ad->gameMenu = menuSelectCurrentItem(ad->gameMenu);
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

        // Pick random music here
        ad->bgmIdx = esp_random() % ARRAY_SIZE(ad->bgms);
        // Start playing music
        globalMidiPlayerPlaySong(&ad->bgms[ad->bgmIdx], MIDI_BGM);
        globalMidiPlayerGet(MIDI_BGM)->loop = true;
    }

    // Start with a full movement timer
    ad->moveTimerUs = TANK_MOVE_TIME_US;

    // Start on turn 1
    ad->turn = 1;

    // Switch to showing the game
    ad->mState = AMS_GAME;

    // Start the game waiting
    artillerySwitchToGameState(ad, AGS_WAIT);
}

/**
 * @return A pointer to all the mode data
 */
artilleryData_t* getArtilleryData(void)
{
    return ad;
}
