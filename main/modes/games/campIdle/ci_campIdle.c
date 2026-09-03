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

const cnfsFileIdx_t uiImages[] = {
    CI_ARROW_LONG_WSG,
};

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
    ccd           = (ciCampData_t*)heap_caps_calloc(1, sizeof(ciCampData_t), MALLOC_CAP_8BIT);
    ccd->uiImages = (wsg_t*)heap_caps_calloc(ARRAY_SIZE(uiImages), sizeof(wsg_t), MALLOC_CAP_8BIT);
    for (int idx = 0; idx < ARRAY_SIZE(uiImages); idx++)
    {
        loadWsg(uiImages[idx], &ccd->uiImages[idx], true);
    }
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
    for (int idx = 0; idx < ARRAY_SIZE(uiImages); idx++)
    {
        freeWsg(&ccd->uiImages[idx]);
    }
    free(ccd->uiImages);
    free(ccd);
}

static void campMainLoop(int64_t elapsedUs)
{
    switch (ccd->state)
    {
        case CI_SPLASH:
        {
            ciRunSplash(ccd, elapsedUs);
            break;
        }
        case CI_MENU:
        {
            ciRunMenu(ccd);
            break;
        }
        case CI_ENCYC:
        {
            ciRunEncyclopedia(ccd);
            break;
        }
        case CI_ENCYC_DESC:
        {
            buttonEvt_t evt;
            while (checkButtonQueueWrapper(&evt))
            {
                if (evt.down)
                {
                    ccd->state = CI_ENCYC;
                }
            }
            ciDrawItemPanel(ccd, ccd->selection);
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