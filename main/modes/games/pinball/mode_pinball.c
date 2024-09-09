//==============================================================================
// Includes
//==============================================================================

#include <esp_random.h>
#include <esp_heap_caps.h>

#include "mode_pinball.h"
#include "pinball_game.h"
#include "pinball_physics.h"
#include "pinball_draw.h"

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    pbScene_t scene;
    font_t ibm;
} pinball_t;

//==============================================================================
// Function Prototypes
//==============================================================================

static void pinEnterMode(void);
static void pinExitMode(void);
static void pinMainLoop(int64_t elapsedUs);
static void pinBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);

//==============================================================================
// Variables
//==============================================================================

const char pinballName[] = "Pinball";

swadgeMode_t pinballMode = {
    .modeName                 = pinballName,
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = pinEnterMode,
    .fnExitMode               = pinExitMode,
    .fnMainLoop               = pinMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = pinBackgroundDrawCallback,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
};

pinball_t* pinball;

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Set up the pinball mode
 */
static void pinEnterMode(void)
{
    // Make it fast
    setFrameRateUs(PIN_US_PER_FRAME);

    // Allocate all the memory
    pinball = calloc(sizeof(pinball_t), 1);

    loadFont("ibm_vga8.font", &pinball->ibm, false);

    pbSceneInit(&pinball->scene);
    pbStartBall(&pinball->scene);
}

/**
 * @brief Tear down the pinball mode
 *
 */
static void pinExitMode(void)
{
    freeFont(&pinball->ibm);
    pbSceneDestroy(&pinball->scene);
    free(pinball);
}

/**
 * @brief Run the pinball main loop
 *
 * @param elapsedUs The number of microseconds since this function was last called
 */
static void pinMainLoop(int64_t elapsedUs)
{
    // Handle inputs
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        if (evt.down && PB_START == evt.button)
        {
            pbSceneDestroy(&pinball->scene);
            pbSceneInit(&pinball->scene);
        }
        else
        {
            pbButtonPressed(&pinball->scene, &evt);
        }
    }

    pbSimulate(&pinball->scene, elapsedUs);
    pbGameTimers(&pinball->scene, elapsedUs);
    pbAdjustCamera(&pinball->scene);
    pbSceneDraw(&pinball->scene, &pinball->ibm);
}

/**
 * @brief Draw the background for the pinball game
 *
 * @param x the x coordinate that should be updated
 * @param y the x coordinate that should be updated
 * @param w the width of the rectangle to be updated
 * @param h the height of the rectangle to be updated
 * @param up update number
 * @param numUp update number denominator
 */
static void pinBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
}
