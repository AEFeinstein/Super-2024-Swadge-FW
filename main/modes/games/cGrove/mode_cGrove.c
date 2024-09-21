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
#include "cg_Garden.h"
#include "cg_Spar.h"

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
    loadFont("ibm_vga8.font", &grove->menuFont, false);

    // Load images
    // Static objects
    loadWsg("cgBigBoulder.wsg", &grove->gardenSpr[0], true);
    // Cursors
    loadWsg("cgHandCursor.wsg", &grove->cursors[0], true);
    // Items
    loadWsg("cgBall.wsg", &grove->items[0], true);
    // Chowa expressions
    loadWsg("cgChowaNeutral.wsg", &grove->chowaExpressions[CG_NEUTRAL], true);
    loadWsg("cgChowaHappy.wsg", &grove->chowaExpressions[CG_HAPPY], true);
    loadWsg("cgChowaWorried.wsg", &grove->chowaExpressions[CG_WORRIED], true);
    
    // Init
    // FIXME: Load and unload when switching to and from a mode 
    cgInitGarden(grove);
    cg_initSpar(grove);

    grove->state = CG_SPAR;
}

static void cGroveExitMode(void)
{    
    // Unload sub modes
    cg_deInitSpar();

    // WSGs
    for (int8_t i = 0; i < CG_CHOWA_EXPRESSION_COUNT; i++)
    {
        freeWsg(&grove->chowaExpressions[i]);
    }
    for (int8_t i = 0; i < CG_GARDEN_CURSORS; i++)
    {
        freeWsg(&grove->cursors[i]);
    }
    for (int8_t i = 0; i < CG_GARDEN_ITEMS_COUNT; i++)
    {
        freeWsg(&grove->items[i]);
    }
    for (int8_t i = 0; i < CG_GARDEN_STATIC_OBJECTS; i++)
    {
        freeWsg(&grove->gardenSpr[i]);
    }
    
    //Fonts
    freeFont(&grove->menuFont); 
    // Main
    free(grove);
}

static void cGroveMainLoop(int64_t elapsedUs) 
{
    switch (grove->state)
    {
        case CG_MAIN_MENU:
        {
            // Menu
            printf("LMAO");
            grove->state = CG_SPAR;
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