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

// Includes
#include "mode_cGrove.h"

//==============================================================================
// Consts
//==============================================================================

static const char cGroveTitle[] = "Chowa Grove";            // Game title

//==============================================================================
// Function declarations
//==============================================================================

static void cGroveEnterMode(void);
static void cGroveExitMode(void);
static void cGroveMainLoop(int64_t elapsedUs);

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

static cGrove_t* grove = NULL;

//==============================================================================
// Functions
//==============================================================================

static void cGroveEnterMode(void)
{
    // Mode memory allocation
    grove = calloc(1, sizeof(cGrove_t));

    // Load a font
    loadFont("logbook.font", &grove->menuFont, false);

    // Load images
    loadWsg("cgBigBoulder.wsg", &grove->gardenSpr[0], true);
    loadWsg("cgHandCursor.wsg", &grove->cursors[0], true);
    loadWsg("cgBall.wsg", &grove->items[0], true);

    // Init
    cgInitGarden(grove);
}

static void cGroveExitMode(void)
{
    // WSGs
    freeWsg(&grove->cursors[0]);
    freeWsg(&grove->gardenSpr[0]);
    //Fonts
    freeFont(&grove->menuFont); 
    // Main
    free(grove);
}

static void cGroveMainLoop(int64_t elapsedUs) 
{
    //grove
    cgRunGarden(grove);
}