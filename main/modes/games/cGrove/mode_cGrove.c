/**
 * @file mode_cGrove.c
 * @author Jeremy Stintzcum (Jeremy.Stintzcum@gmail.com)
 * @brief A small game similar to the chao garden from the Sonic seres by SEGA
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

//==============================================================================
// Consts
//==============================================================================

static const char cGroveTitle[] = "Chowa Grove"; // Game title

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
 * @brief Loads the appropriate WSGs, sounds etc for the current mode.
 * 
 */
static void cg_loadMode(void);

/**
 * @brief Menu callback for the main Chowa Grove menu
 * 
 * @param label 
 * @param selected 
 * @param settingVal 
 */
static void cg_menuCB(const char* label, bool selected, uint32_t settingVal);

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
    cg = calloc(1, sizeof(cGrove_t));
    setFrameRateUs(CG_FRAMERATE);

    // Load a font
    loadFont("ibm_vga8.font", &cg->menuFont, false);

    // Init
    cg->state = CG_SPAR; // FIXME: Load into main menu
}

static void cGroveExitMode(void)
{
    // Unload sub modes
    switch (cg->state)
    {
        case CG_SPAR:
        {   
            cg_deInitSpar();
            break;
        }
        case CG_GROVE:
        {   
            cg_deInitGrove(cg);
            break;
        }
        default:
        {
            break;
        }
    }

    // Fonts
    freeFont(&cg->menuFont);

    // Main
    free(cg);
}

static void cGroveMainLoop(int64_t elapsedUs)
{
    if (!cg->loaded)
    {
        cg_loadMode();
    }

    switch (cg->state)
    {
        case CG_MAIN_MENU:
        {
            // Menu
            break;
        }
        case CG_GROVE:
        {
            // Grove
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

static void cg_loadMode()
{
    switch (cg->state)
    {
        case CG_SPAR:
        {   
            cg_initSpar(cg);
            break;
        }
        case CG_GROVE:
        {   
            cg_initGrove(cg);
            break;
        }
        default:
        {
            break;
        }
    }
    cg->loaded = true;
}

static void cg_menuCB(const char* label, bool selected, uint32_t settingVal)
{

}