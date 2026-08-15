//==============================================================================
// Includes
//==============================================================================

#include "gottaGo.h"

//==============================================================================
// Defines
//==============================================================================

//==============================================================================
// Consts
//==============================================================================

const char ggModeName[] = "Gotta Go!";

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    SPLASH,
    MENU,
    RULES,
    GAME,
} ggState_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct 
{
    ggState_t state;
} ggData_t;

//==============================================================================
// Functions declarations
//==============================================================================

static void ggEnterMode(void);
static void ggExitMode(void);
static void ggMainLoop(int64_t elapsedUs);

//==============================================================================
// Variables
//==============================================================================

swadgeMode_t gottaGoMode = {
    .modeName = ggModeName,
    .wifiMode = NO_WIFI,
    .overrideUsb = false,
    .usesAccelerometer = false,
    .usesThermometer = false,
    .fnEnterMode = ggEnterMode,
    .fnExitMode = ggExitMode,
    .fnMainLoop = ggMainLoop,
};

ggData_t* ggd;

//==============================================================================
// Functions
//==============================================================================

static void ggEnterMode(void)
{
    ggd = (ggData_t*)heap_caps_calloc(1, sizeof(ggData_t), MALLOC_CAP_8BIT);
}

static void ggExitMode(void)
{
    free(ggd);
}

static void ggMainLoop(int64_t elapsedUs)
{
    buttonEvt_t evt;
    switch (ggd->state)
    {
        default:
        {
            while(checkButtonQueueWrapper(&evt))
            {
                // Allows for backing out of the mode.
            }
        }
    }
}