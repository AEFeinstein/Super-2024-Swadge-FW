/**
 * @file mode_cGrove.c
 * @author Jeremy Stintzcum (Jeremy.Stintzcum@gmail.com)
 * @brief A small game similar to the chao garden from the Sonic series by SEGA
 * @version 1.0
 * @date 2024-09-07
 *
 * @copyright Copyright (c) 2024
 *
 */

//==============================================================================
// Includes
//==============================================================================

#include "mode_cGrove.h"
#include "cg_Chowa.h"
#include "cg_Grove.h"
#include "cg_Spar.h"
#include "textEntry.h"
#include <esp_random.h>
#include <esp_heap_caps.h>

//==============================================================================
// Defines
//==============================================================================

#define CG_FRAMERATE 16667
#define SECOND       1000000

//==============================================================================
// Consts
//==============================================================================

static const char cGroveTitle[] = "Chowa Grove"; // Game title

static const char* cGroveMenuNames[]   = {"Play with Chowa", "Spar", "Settings"};
static const char* cGroveSettingOpts[] = {"Grove Touch Scroll: ", "Online: ", "Show Item Text: ", "Show Chowa Names: "};
static const char* const cGroveEnabledOptions[] = {"Enabled", "Disabled"};
static const int32_t cGroveEnabledVals[]        = {true, false};
static const char* cGroveResetData[]
    = {"Reset all game data", "Are you sure you want to reset to factory? All data will be lost.",
       "Press 'Start' to permanently erase all data. Press any other key to return to menu."};

static const char* cGroveTitleSprites[] = {"cg_cloud.wsg",          "cg_sky.wsg",
                                           "cg_title_1.wsg",        "cg_title_2.wsg",
                                           "cg_title_middle_1.wsg", "cg_title_middle_2.wsg",
                                           "cg_title_middle_3.wsg", "cg_title_middle_4.wsg"};

//==============================================================================
// Function declarations
//==============================================================================

/**
 * @brief Constructs the mode
 *
 */
static void cGroveEnterMode(void);

/**
 * @brief Deconstructs the mode
 *
 */
static void cGroveExitMode(void);

/**
 * @brief Main loop of Chowa Grove
 *
 * @param elapsedUs
 */
static void cGroveMainLoop(int64_t elapsedUs);

/**
 * @brief Menu callback for the main Chowa Grove menu
 *
 * @param label
 * @param selected
 * @param settingVal
 */
static void cg_menuCB(const char* label, bool selected, uint32_t settingVal);

/**
 * @brief Draw title screen
 *
 * @param elapsedUs Time since I last dunked my head in a bucket of acid
 */
static void cg_titleScreen(int64_t elapsedUs);

//==============================================================================
// Variables
//==============================================================================

swadgeMode_t cGroveMode = {
    .modeName                 = cGroveTitle,
    .wifiMode                 = ESP_NOW,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = cGroveEnterMode,
    .fnExitMode               = cGroveExitMode,
    .fnMainLoop               = cGroveMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = NULL,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
    .fnDacCb                  = NULL,
};

static cGrove_t* cg = NULL;

static char cgTextPrompt[] = "Enter your username:";

//==============================================================================
// Functions
//==============================================================================

static void cGroveEnterMode(void)
{
    // Mode memory allocation
    cg = heap_caps_calloc(1, sizeof(cGrove_t), MALLOC_CAP_SPIRAM);
    setFrameRateUs(CG_FRAMERATE);

    // Load Chowa WSGs
    cg_initChowaWSGs(cg);
    // Load title screen
    cg->title = calloc(ARRAY_SIZE(cGroveTitleSprites), sizeof(wsg_t));
    for (int32_t idx = 0; idx < ARRAY_SIZE(cGroveTitleSprites); idx++)
    {
        loadWsg(cGroveTitleSprites[idx], &cg->title[idx], true);
    }
    // Multi-use
    loadWsg("cg_Arrow.wsg", &cg->arrow, true);

    // Load a font
    loadFont("cg_font_body_thin.font", &cg->menuFont, true);
    loadFont("cg_font_body.font", &cg->largeMenuFont, true);
    loadFont("cg_heading.font", &cg->titleFont, true);
    makeOutlineFont(&cg->titleFont, &cg->titleFontOutline, true);

    // Load Midis
    loadMidiFile("Chowa_Menu.mid", &cg->menuBGM, true);

    // Load settings
    size_t blobLen;
    readNvsBlob(cgNVSKeys[2], NULL, &blobLen);
    if (!readNvsBlob(cgNVSKeys[2], &cg->settings, &blobLen))
    {
        cg->settings.touch      = true;
        cg->settings.online     = false;
        cg->settings.itemText   = true;
        cg->settings.chowaNames = true;
    }

    // Menu
    cg->menu                                   = initMenu(cGroveTitle, cg_menuCB);
    cg->renderer                               = initMenuManiaRenderer(&cg->titleFont, NULL, &cg->menuFont);
    static const paletteColor_t shadowColors[] = {c001, c002, c002, c003, c013, c014, c013, c003, c002, c001};
    led_t ledColor                             = {.r = 0, .g = 200, .b = 200};
    recolorMenuManiaRenderer(cg->renderer, c111, c430, c445, c045, c542, c430, c111, c445, shadowColors,
                             ARRAY_SIZE(shadowColors), ledColor);
    addSingleItemToMenu(cg->menu, cGroveMenuNames[0]);     // Go to Grove
    addSingleItemToMenu(cg->menu, cGroveMenuNames[1]);     // Go to Spar
    cg->menu = startSubMenu(cg->menu, cGroveMenuNames[2]); // Settings
    addSettingsOptionsItemToMenu(cg->menu, cGroveSettingOpts[0], cGroveEnabledOptions, cGroveEnabledVals,
                                 ARRAY_SIZE(cGroveEnabledOptions), getScreensaverTimeSettingBounds(),
                                 cg->settings.touch); // Enable/disable touch controls
    addSettingsOptionsItemToMenu(cg->menu, cGroveSettingOpts[1], cGroveEnabledOptions, cGroveEnabledVals,
                                 ARRAY_SIZE(cGroveEnabledOptions), getScreensaverTimeSettingBounds(),
                                 cg->settings.online); // Enable/disable online functions
    addSettingsOptionsItemToMenu(cg->menu, cGroveSettingOpts[2], cGroveEnabledOptions, cGroveEnabledVals,
                                 ARRAY_SIZE(cGroveEnabledOptions), getScreensaverTimeSettingBounds(),
                                 cg->settings.itemText); // Enable/disable Item names
    addSettingsOptionsItemToMenu(cg->menu, cGroveSettingOpts[3], cGroveEnabledOptions, cGroveEnabledVals,
                                 ARRAY_SIZE(cGroveEnabledOptions), getScreensaverTimeSettingBounds(),
                                 cg->settings.chowaNames); // Enable/disable Chowa names
    addSingleItemToMenu(cg->menu, cGroveResetData[0]);
    cg->menu = endSubMenu(cg->menu);

    // Initialize Text Entry
    textEntryInit(&cg->menuFont, CG_MAX_STR_LEN, cg->buffer);
    textEntrySetBGTransparent();
    textEntrySetShadowboxColor(true, c111);
    textEntrySetNounMode();

    // Init State
    cg->state       = CG_MAIN_MENU;
    cg->titleActive = true;

    // If first run, do tutorial and get players name
    readNvsBlob(cgNVSKeys[0], NULL, &blobLen);
    if (!readNvsBlob(cgNVSKeys[0], &cg->player, &blobLen))
    {
        cg->state = CG_FIRST_RUN;
        textEntrySetPrompt(cgTextPrompt);
    }

    // Adjust Audio to use correct instruments
    cg->mPlayer       = globalMidiPlayerGet(MIDI_BGM);
    cg->mPlayer->loop = true;
    midiGmOn(cg->mPlayer);

    // Init Chowa
    readNvsBlob(cgNVSKeys[1], NULL, &blobLen);
    if (!readNvsBlob(cgNVSKeys[1], &cg->chowa, &blobLen))
    {
        for (int i = 0; i < CG_MAX_CHOWA - 1; i++)
        {
            cg->chowa[i].active = false;
            cg->chowa[i].type   = CG_KING_DONUT;
            for (int idx = 0; idx < CG_STAT_COUNT; idx++)
            {
                cg->chowa[i].stats[idx] = esp_random() % 255;
            }
            switch (esp_random() % 4)
            {
                case 0:
                    cg->chowa[i].mood = CG_HAPPY;
                    break;
                case 1:
                    cg->chowa[i].mood = CG_SAD;
                    break;
                case 2:
                    cg->chowa[i].mood = CG_ANGRY;
                    break;
                case 3:
                    cg->chowa[i].mood = CG_CONFUSED;
                    break;
            }
            cg->chowa[i].playerAffinity = 101;
            char buffer[32];
            snprintf(buffer, sizeof(buffer) - 1, "Chowa%d", i);
            strcpy(cg->chowa[i].name, buffer);
            strcpy(cg->chowa[i].owner, cg->player);
        }
        writeNvsBlob(cgNVSKeys[1], &cg->chowa, sizeof(cgChowa_t) * CG_MAX_CHOWA);
    }
    globalMidiPlayerPlaySong(&cg->menuBGM, MIDI_BGM);
}

static void cGroveExitMode(void)
{
    // Unload sub modes
    switch (cg->state)
    {
        case CG_GROVE:
        {
            cg_deInitGrove(cg);
            break;
        }
        case CG_SPAR:
        {
            cg_deInitSpar();
            break;
        }
        default:
        {
            break;
        }
    }

    // Menu
    deinitMenu(cg->menu);
    deinitMenuManiaRenderer(cg->renderer);

    // Audio
    unloadMidiFile(&cg->menuBGM);

    // Fonts
    freeFont(&cg->titleFontOutline);
    freeFont(&cg->menuFont);
    freeFont(&cg->largeMenuFont);
    freeFont(&cg->titleFont);

    // WSGs
    freeWsg(&cg->title[1]); // Sky wsg, only one not freed earlier
    free(cg->title);
    freeWsg(&cg->arrow);
    cg_deInitChowaWSGs(cg);

    // Main
    free(cg);
}

static void cGroveMainLoop(int64_t elapsedUs)
{
    // Draw title screen
    if (cg->titleActive)
    {
        cg_titleScreen(elapsedUs);
        buttonEvt_t evt = {0};
        while (checkButtonQueueWrapper(&evt))
        {
            if (evt.down)
            {
                cg->titleActive = false;
                // Clear out the sprites
                for (uint8_t i = 0; i < ARRAY_SIZE(cGroveTitleSprites); i++)
                {
                    if (i != 1) // Keep sky sprite for later
                    {
                        freeWsg(&cg->title[i]);
                    }
                }
            }
        }
        return;
    }

    // Unload old assets if they're not needed
    if (cg->unload)
    {
        // Resetting back to the menu
        cg->unload = false;
        globalMidiPlayerStop(true);
        switch (cg->state)
        {
            case CG_GROVE:
            {
                cg_deInitGrove(cg);
                break;
            }
            case CG_SPAR:
            {
                cg_deInitSpar();
                break;
            }
            default:
            {
                // Something went wrong
                break;
            }
        }
        midiGmOn(cg->mPlayer);
        globalMidiPlayerPlaySong(&cg->menuBGM, MIDI_BGM);
        cg->state = CG_MAIN_MENU;
    }

    // Switch behaviors based on state
    switch (cg->state)
    {
        case CG_MAIN_MENU:
        {
            // Menu
            buttonEvt_t evt = {0};
            while (checkButtonQueueWrapper(&evt))
            {
                cg->menu = menuButton(cg->menu, evt);
            }
            drawMenuMania(cg->menu, cg->renderer, elapsedUs);
            break;
        }
        case CG_GROVE:
        {
            // Grove
            cg_runGrove(cg, elapsedUs);
            break;
        }
        case CG_SPAR:
        {
            cg_runSpar(elapsedUs);
            break;
        }
        case CG_FIRST_RUN:
        {
            buttonEvt_t evt = {0};
            bool done       = false;
            while (checkButtonQueueWrapper(&evt))
            {
                done = !textEntryInput(evt.down, evt.button);
            }
            if (done)
            {
                textEntrySoftReset();
                strcpy(cg->player, cg->buffer);
                strcpy(cg->buffer, "");
                writeNvsBlob(cgNVSKeys[0], cg->player, sizeof(cg->player));
                cg->state = CG_MAIN_MENU;
            }
            textEntryDraw(elapsedUs);
            break;
        }
        case CG_ERASE:
        {
            buttonEvt_t evt = {0};
            while (checkButtonQueueWrapper(&evt))
            {
                if (evt.down && evt.button & PB_START)
                {
                    // Player name, Chowa
                    eraseNvsKey(cgNVSKeys[0]);
                    eraseNvsKey(cgNVSKeys[1]);
                    eraseNvsKey(cgNVSKeys[2]);

                    // Deactivate all Chowa
                    for (int idx = 0; idx < CG_MAX_CHOWA; idx++)
                    {
                        cg->chowa[idx].active = false;
                    }
                    for (int idx = 0; idx < CG_GROVE_MAX_GUEST_CHOWA; idx++)
                    {
                        cg->guests[idx].active = false;
                    }

                    // Individual modes
                    cg_clearGroveNVSData();

                    // Move to name entry
                    cg->state = CG_FIRST_RUN;
                    textEntrySetPrompt(cgTextPrompt);
                }
                else if (evt.down)
                {
                    cg->state = CG_MAIN_MENU;
                }
            }

            // Draw
            fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);
            int16_t xOff = 16;
            int16_t yOff = 48;
            drawText(&cg->largeMenuFont, c500, cGroveResetData[0], xOff, yOff);
            yOff += 30;
            drawTextWordWrap(&cg->menuFont, c555, cGroveResetData[1], &xOff, &yOff, TFT_WIDTH - 16, 200);
            xOff = 16;
            yOff = 120;
            drawTextWordWrap(&cg->menuFont, c555, cGroveResetData[2], &xOff, &yOff, TFT_WIDTH - 16, TFT_HEIGHT);
        }
        default:
        {
            break;
        }
    }
}

static void cg_menuCB(const char* label, bool selected, uint32_t settingVal)
{
    if (selected)
    {
        if (label == cGroveMenuNames[0])
        {
            // Start Grove
            globalMidiPlayerStop(true);
            cg_initGrove(cg);
            cg->state = CG_GROVE;
        }
        else if (label == cGroveMenuNames[1])
        {
            // Start Sparring
            globalMidiPlayerStop(true);
            cg_initSpar(cg);
            cg->state = CG_SPAR;
        }
        else if (label == cGroveResetData[0])
        {
            // Erase data
            cg->state = CG_ERASE;
        }
        else
        {
            // Something went wrong
        }
    }
    else if (label == cGroveSettingOpts[0])
    {
        // Grove C stick or buttons
        cg->settings.touch = settingVal;
        writeNvsBlob(cgNVSKeys[2], &cg->settings, sizeof(cgSettings_t));
    }
    else if (label == cGroveSettingOpts[1])
    {
        // Online on or off
        cg->settings.online = settingVal;
        writeNvsBlob(cgNVSKeys[2], &cg->settings, sizeof(cgSettings_t));
    }
    else if (label == cGroveSettingOpts[2])
    {
        // If Grove items should display text
        cg->settings.itemText = settingVal;
        writeNvsBlob(cgNVSKeys[2], &cg->settings, sizeof(cgSettings_t));
    }
    else if (label == cGroveSettingOpts[3])
    {
        // If Grove items should display text
        cg->settings.chowaNames = settingVal;
        writeNvsBlob(cgNVSKeys[2], &cg->settings, sizeof(cgSettings_t));
    }
}

static void cg_titleScreen(int64_t elapsedUs)
{
    // Update
    cg->timer += elapsedUs;
    if (cg->timer >= SECOND)
    {
        cg->timer = 0;
        cg->cloudPos.x += 1;
        if (cg->cloudPos.x >= TFT_WIDTH)
        {
            cg->cloudPos.x = -cg->title[0].h;
        }
    }
    cg->animFrame  = (cg->animFrame + 1) % 32;
    cg->titleFrame = (cg->titleFrame + 1) % 64;

    // Draw
    drawWsgSimple(&cg->title[1], 0, 0);
    drawWsgSimpleHalf(&cg->title[0], cg->cloudPos.x, -30);
    drawWsgSimpleHalf(&cg->title[4 + (cg->animFrame >> 3)], 0, 84);
    drawWsgSimpleHalf(&cg->title[2 + (cg->titleFrame >> 5)], 0, 0);
}