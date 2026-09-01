//==============================================================================
// Includes
//==============================================================================

// Core
#include "ci_campIdle.h"

// Subcomponents
#include "ci_genericData.h"
#include "ci_items.h"
#include "ci_menu.h"

//==============================================================================
// Consts
//==============================================================================

const char campModeName[] = "Cozy Camping";

//==============================================================================
// Function Declarations
//==============================================================================

// Main
static void campEnterMode(void);
static void campExitMode(void);
static void campMainLoop(int64_t elapsedUs);

//==============================================================================
// Variables
//==============================================================================

swadgeMode_t campIdleMode = {
    .modeName                 = campModeName,
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = campEnterMode,
    .fnExitMode               = campExitMode,
    .fnMainLoop               = campMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = NULL,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
};

ciCampData_t* ccd;

//==============================================================================
// Functions
//==============================================================================

static void campEnterMode()
{
    ccd = (ciCampData_t*)heap_caps_calloc(1, sizeof(ciCampData_t), MALLOC_CAP_8BIT);
    loadFont(IBM_VGA_8_FONT, &ccd->smallFont, true);
    loadFont(RODIN_EB_FONT, &ccd->largeText, true);
    ciInitInventory(ccd);
    ciInitSplash(ccd);
}

static void campExitMode()
{
    ciFreeInventory(ccd);
    freeFont(&ccd->largeText);
    freeFont(&ccd->smallFont);
    free(ccd);
}

static void campMainLoop(int64_t elapsedUs)
{
    switch (ccd->state)
    {
        case CI_SPLASH:
        {
            ciRunSplash(ccd);
            break;
        }
        case CI_MENU:
        {
            ciRunMenu(ccd);
            break;
        }
        default:
        {
            buttonEvt_t evt;
            while (checkButtonQueueWrapper(&evt))
            {
                // Allow backing out
            }
            break;
        }
    }
}