//==============================================================================
// Includes
//==============================================================================

#include "mainMenu.h"
#include "menu.h"
#include "swadge2024.h"

#include "demoMode.h"
#include "pong.h"

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
    mainMenu->menu = initMenu(mainMenuName, &mainMenu->ibm, mainMenuCb);

    // Add single items
    addSingleItemToMenu(mainMenu->menu, demoMode.modeName);
    addSingleItemToMenu(mainMenu->menu, pongMode.modeName);
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
    while (checkButtonQueue(&evt))
    {
        mainMenu->menu = menuButton(mainMenu->menu, evt);
    }

    // Draw the menu
    drawMenu(mainMenu->menu);
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
    }
}
