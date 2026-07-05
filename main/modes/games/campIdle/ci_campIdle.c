//==============================================================================
// Includes
//==============================================================================

// Core
#include "ci_campIdle.h"

// Subcomponents
#include "ci_items.h"

//==============================================================================
// Consts
//==============================================================================

const char campModeName[] = "Cozy Camping";
 
//==============================================================================
// Enum
//==============================================================================

typedef enum {
    CI_SPLASH,
    CI_MENU,
    CI_DAY,
    CI_NIGHT,
} ci_state_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    // Current state
    ci_state_t state;

    // Day
    ci_inventory_t inv;
} ci_campData_t;

//==============================================================================
// Function Declarations
//==============================================================================

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

ci_campData_t* ciData;

//==============================================================================
// Functions
//==============================================================================
 
static void campEnterMode()
{
    ciData = (ci_campData_t*)heap_caps_calloc(1, sizeof(ci_campData_t), MALLOC_CAP_8BIT);
    ci_initInv(&ciData->inv);
}
 
static void campExitMode()
{
    free(ciData);
}
 
static void campMainLoop(int64_t elapsedUs)
{
    buttonEvt_t evt;
    switch (ciData->state)
    {
        case CI_SPLASH:
        {
            while (checkButtonQueueWrapper(&evt))
            {
                if (evt.button & PB_A)
                {
                    ciData->state = CI_MENU;
                }
            }
            break;
        }
        case CI_DAY:
        {
            
            break;
        }
        default:
        {
            break;
        }
    }
}