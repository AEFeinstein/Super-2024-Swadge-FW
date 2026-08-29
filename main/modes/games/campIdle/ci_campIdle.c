//==============================================================================
// Includes
//==============================================================================

// Core
#include "ci_campIdle.h"

// Subcomponents
#include "ci_genericData.h"
#include "ci_items.h"
#include "ci_draw.h"

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

// Game States
static void doSplash(void);
static void doMenu(void);

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
    ciInitInventory(ccd);
}

static void campExitMode()
{
    ciFreeInventory(ccd);
    free(ccd);
}

static void campMainLoop(int64_t elapsedUs)
{
    switch (ccd->state)
    {
        case CI_SPLASH:
        {
            doSplash();
            break;
        }
        case CI_MENU:
        {
            doMenu();
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

static void doSplash()
{
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        if (evt.down && (evt.button & PB_A))
        {
            ccd->state = CI_MENU;
        }
    }
    ciDrawSplash(ccd);
}

static void doMenu()
{
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
    }
    ciDrawMenu(ccd);
}