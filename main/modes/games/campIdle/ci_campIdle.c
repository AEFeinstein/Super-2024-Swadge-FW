//==============================================================================
// Includes
//==============================================================================

// Main
#include "ci_campIdle.h"

// Camp
#include "ci_genericData.h"
#include "ci_menu.h"
#include "ci_nvs.h"
#include "ci_recipeData.h"
#include "ci_workbench.h"

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
    ccd = (ciCampData_t*)heap_caps_calloc(1, sizeof(ciCampData_t), MALLOC_CAP_8BIT);
    
    // Load assets
    ccd->uiImages = (wsg_t*)heap_caps_calloc(ARRAY_SIZE(uiImages), sizeof(wsg_t), MALLOC_CAP_8BIT);
    for (int idx = 0; idx < ARRAY_SIZE(uiImages); idx++)
    {
        loadWsg(uiImages[idx], &ccd->uiImages[idx], true);
    }
    loadFont(IBM_VGA_8_FONT, &ccd->smallFont, true);
    loadFont(RODIN_EB_FONT, &ccd->largeText, true);
    
    // Init subcomponents
    clear(&ccd->cft.craftQueue);
    ciLoadCraftFromNVS(&ccd->cft);
    ciInitInventory(&ccd->inv);
    ciInitWorkbenchImages(&ccd->wbd);
    ciLoadWorkbenchFromNVS(&ccd->wbd);
    
    // Load Addt'l items from NVS
    int outVal = 0;
    readNamespaceNvs32(ciNVSKeys[CI_NVS_NAMESPACE], ciNVSKeys[CI_NVS_SAVED_UNITS], &outVal);
    ccd->timerUnits += outVal;
    // ciInitCraftTimer(ccd); FIXME: Load from NVS
    // Start
    ciInitState(ccd, CI_SPLASH);
}

static void campExitMode()
{
    // Save everything
    writeNamespaceNvs32(ciNVSKeys[CI_NVS_NAMESPACE], ciNVSKeys[CI_NVS_SAVED_UNITS], ccd->timerUnits);
    ciSaveCraftToNVS(&ccd->cft);
    // Clear
    clear(&ccd->cft.craftQueue);
    ciFreeWorkbenchImages(&ccd->wbd);
    ciFreeInventory(&ccd->inv);
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
            ciDrawItemPanel(&ccd->inv, &ccd->largeText, &ccd->smallFont, ccd->selection);
            break;
        }
        case CI_CRAFTING:
        {
            if (ciRunCraft(ccd))
            {
                ciInitState(ccd, CI_MENU);
            }
            break;
        }
        case CI_CRAFTING_PREP:
        {
            ciRunCraftSelection(ccd);
            break;
        }
        default:
        {
            buttonEvt_t evt;
            while (checkButtonQueueWrapper(&evt))
            {
            }
            break;
        }
    }
    if (ccd->cft.craftQueue.first != NULL || ccd->foraging)
    {
        ccd->timerUs += elapsedUs;
        if (ccd->timerUs == UNIT)
        {
            ccd->timerUs = 0;
            ccd->timerUnits += 1;
        }
        while (ccd->cft.craftQueue.first != NULL
               && recipeList[(intptr_t)ccd->cft.craftQueue.first->val].time <= ccd->timerUnits)
        {
            ciCraft(&ccd->cft, &ccd->inv, &ccd->timerUnits);
        }
        // TODO: Add forage
    }
}

