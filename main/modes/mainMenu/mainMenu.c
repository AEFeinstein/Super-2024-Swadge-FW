//==============================================================================
// Includes
//==============================================================================

#include "mainMenu.h"
#include "menu.h"
#include "swadge2024.h"

#include "demoMode.h"
#include "pong.h"
#include "mode_colorchord.h"
#include "settingsManager.h"

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    menu_t* menu;
    font_t ibm;
} mainMenu_t;

//==============================================================================
// Function Prototypes
//==============================================================================

static void mainMenuEnterMode(void);
static void mainMenuExitMode(void);
static void mainMenuMainLoop(int64_t elapsedUs);
static void mainMenuCb(const char* label, bool selected);

//==============================================================================
// Variables
//==============================================================================

// It's good practice to declare immutable strings as const so they get placed in ROM, not RAM
static const char mainMenuName[] = "Main Menu";

swadgeMode_t mainMenuMode = {
    .modeName                 = mainMenuName,
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = true,
    .usesThermometer          = true,
    .fnEnterMode              = mainMenuEnterMode,
    .fnExitMode               = mainMenuExitMode,
    .fnMainLoop               = mainMenuMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = NULL,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
};

mainMenu_t* mainMenu;

static const char settingsLabel[] = "Settings";

static const char tft1[] = "TFT: 1";
static const char tft2[] = "TFT: 2";
static const char tft3[] = "TFT: 3";
static const char tft4[] = "TFT: 4";
static const char tft5[] = "TFT: 5";
static const char tft6[] = "TFT: 6";
static const char tft7[] = "TFT: 7";
static const char tft8[] = "TFT: 8";

static const char* const tftOpts[] = {
    tft1, tft2, tft3, tft4, tft5, tft6, tft7, tft8,
};

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief TODO
 *
 */
static void mainMenuEnterMode(void)
{
    // Allocate memory for the mode
    mainMenu = calloc(1, sizeof(mainMenu_t));

    // Load a font
    loadFont("ibm_vga8.font", &mainMenu->ibm, false);

    // Allocate the menu
    mainMenu->menu = initMenu(mainMenuName, mainMenuCb);

    // Add single items
    addSingleItemToMenu(mainMenu->menu, demoMode.modeName);
    addSingleItemToMenu(mainMenu->menu, pongMode.modeName);
    addSingleItemToMenu(mainMenu->menu, colorchordMode.modeName);

    mainMenu->menu = startSubMenu(mainMenu->menu, settingsLabel);
    addMultiItemToMenu(mainMenu->menu, tftOpts, ARRAY_SIZE(tftOpts));
    mainMenu->menu = endSubMenu(mainMenu->menu);
}

/**
 * @brief TODO
 *
 */
static void mainMenuExitMode(void)
{
    // Deinit menu
    deinitMenu(mainMenu->menu);

    // Free the font
    freeFont(&mainMenu->ibm);

    // Free mode memory
    free(mainMenu);
}

/**
 * @brief TODO
 *
 * @param elapsedUs
 */
static void mainMenuMainLoop(int64_t elapsedUs)
{
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        mainMenu->menu = menuButton(mainMenu->menu, evt);
    }

    // Draw the menu
    drawMenu(mainMenu->menu, &mainMenu->ibm);
}

/**
 * @brief TODO
 *
 * @param label
 * @param selected
 */
static void mainMenuCb(const char* label, bool selected)
{
    if (selected)
    {
        if (label == demoMode.modeName)
        {
            switchToSwadgeMode(&demoMode);
        }
        else if (label == pongMode.modeName)
        {
            switchToSwadgeMode(&pongMode);
        }
        else if (label == colorchordMode.modeName)
        {
            switchToSwadgeMode(&colorchordMode);
        }
    }
    else
    {
        if ((tft1 == label) || (tft2 == label) || (tft3 == label) || (tft4 == label) || (tft5 == label)
            || (tft6 == label) || (tft7 == label) || (tft8 == label))
        {
            // Set the brightness based on the number in the label
            setTftBrightness(label[5] - '1');
        }
    }
}
