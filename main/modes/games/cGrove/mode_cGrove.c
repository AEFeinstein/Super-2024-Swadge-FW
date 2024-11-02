/**
 * @file mode_cGrove.c
 * @author Jeremy Stintzcum (Jeremy.Stintzcum@gmail.com)
 * @brief A small game similar to the chao garden from the Sonic series by SEGA
 * @version 0.1
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

//==============================================================================
// Defines
//==============================================================================

#define CG_FRAMERATE 16667
#define SECOND       1000000

//==============================================================================
// Consts
//==============================================================================

static const char cGroveTitle[] = "Chowa Grove"; // Game title

static const char* cGroveMenuNames[]   = {"Play with Chowa", "Spar", "Race", "Perform", "Player Profiles", "Settings"};
static const char* cGroveSettingOpts[] = {"Grove Touch Scroll: ", "Online: "};
static const char* const cGroveEnabledOptions[] = {"Enabled", "Disabled"};
static const uint32_t cGroveEnabledVals[]       = {true, false};

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

//==============================================================================
// Functions
//==============================================================================

static void cGroveEnterMode(void)
{
    // Mode memory allocation
    // FIXME: Emulator doesn't accept this?
    // cg = heap_caps_calloc(1, sizeof(cGrove_t), MALLOC_CAP_SPIRAM);
    cg = calloc(1, sizeof(cGrove_t));
    setFrameRateUs(CG_FRAMERATE);

    // Load a font
    loadFont("ibm_vga8.font", &cg->menuFont, true);

    // Load title screen
    cg->title = calloc(ARRAY_SIZE(cGroveTitleSprites), sizeof(wsg_t));
    for (int32_t idx = 0; idx < ARRAY_SIZE(cGroveTitleSprites); idx++)
    {
        loadWsg(cGroveTitleSprites[idx], &cg->title[idx], true);
    }

    // Menu
    cg->menu                                   = initMenu(cGroveTitle, cg_menuCB);
    cg->renderer                               = initMenuManiaRenderer(NULL, NULL, NULL);
    static const paletteColor_t shadowColors[] = {c110, c210, c220, c320, c330, c430, c330, c320, c220, c210};
    led_t ledColor                             = {.r = 128, .g = 128, .b = 0};
    recolorMenuManiaRenderer(cg->renderer, c115, c335, c000, c110, c003, c004, c220, c335, shadowColors,
                             ARRAY_SIZE(shadowColors), ledColor);
    addSingleItemToMenu(cg->menu, cGroveMenuNames[0]);     // Go to Grove
    addSingleItemToMenu(cg->menu, cGroveMenuNames[1]);     // Go to Spar
    addSingleItemToMenu(cg->menu, cGroveMenuNames[2]);     // Go to Race
    addSingleItemToMenu(cg->menu, cGroveMenuNames[3]);     // Go to Performance
    addSingleItemToMenu(cg->menu, cGroveMenuNames[4]);     // View player profiles
    cg->menu = startSubMenu(cg->menu, cGroveMenuNames[5]); // Settings
    // FIXME: Load values from NVM
    // TODO: Add more settings
    addSettingsOptionsItemToMenu(cg->menu, cGroveSettingOpts[0], cGroveEnabledOptions, &cGroveEnabledVals,
                                 ARRAY_SIZE(cGroveEnabledOptions), getScreensaverTimeSettingBounds(),
                                 0); // Enable/disable touch controls
    addSettingsOptionsItemToMenu(cg->menu, cGroveSettingOpts[1], cGroveEnabledOptions, &cGroveEnabledVals,
                                 ARRAY_SIZE(cGroveEnabledOptions), getScreensaverTimeSettingBounds(),
                                 0); // Enable/disable online functions
    cg->menu = endSubMenu(cg->menu);

    // Init
    cg->state       = CG_MAIN_MENU;
    cg->titleActive = true;

    //FIXME: test
    cg->chowa[0].active = true;
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

    // WSGs
    for (uint8_t i = 0; i < ARRAY_SIZE(cGroveTitleSprites); i++)
    {
        freeWsg(&cg->title[i]);
    }
    free(cg->title);

    // Fonts
    freeFont(&cg->menuFont);

    // Main
    free(cg);
}

static void cGroveMainLoop(int64_t elapsedUs)
{
    // Draw title screen
    if (cg->titleActive)
    {
        buttonEvt_t evt = {0};
        while (checkButtonQueueWrapper(&evt))
        {
            if (evt.down)
            {
                cg->titleActive = false;
            }
        }
        cg_titleScreen(elapsedUs);
        return;
    }

    // Unload old assets if they're not needed
    if (cg->unload)
    {
        // Resetting back to the menu
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
            case CG_RACE:
            {
                break;
            }
            case CG_PERFORMANCE:
            {
                break;
            }

            default:
            {
                // Something went wrong
                break;
            }
        }

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
        case CG_RACE:
        {
            // Race
            break;
        }
        case CG_PERFORMANCE:
        {
            // Performance
            break;
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
            cg_initGrove(cg);
            cg->state = CG_GROVE;
        }
        else if (label == cGroveMenuNames[1])
        {
            // Start Sparring
            cg_initSpar(cg);
            cg->state = CG_SPAR;
        }
        else if (label == cGroveMenuNames[2])
        {
            // Start Racing
            // TODO: Racing
        }
        else if (label == cGroveMenuNames[3])
        {
            // Start Performing
            // TODO: Performances
        }
        else if (label == cGroveMenuNames[4])
        {
            // View saved players
            // TODO: View recently interacted players
        }
        else
        {
            // Something went wrong
        }
    }
    else if (label == cGroveSettingOpts[0])
    {
        // Grove C stick or buttons
        cg->touch = settingVal;
    }
    else if (label == cGroveSettingOpts[1])
    {
        // Online on or off
        cg->online = settingVal;
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
    cg->animFrame = (cg->animFrame + 1) % 32;
    cg->titleFrame = (cg->titleFrame + 1) % 64;

    // Draw
    drawWsgSimple(&cg->title[1], 0, 0);
    drawWsgSimpleHalf(&cg->title[0], cg->cloudPos.x, -30);
    drawWsgSimpleHalf(&cg->title[4 + (cg->animFrame >> 3)], 0, 84);
    drawWsgSimpleHalf(&cg->title[2 + (cg->titleFrame >> 5)], 0, 0);
}