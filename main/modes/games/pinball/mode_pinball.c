//==============================================================================
// Includes
//==============================================================================

#include <esp_random.h>
#include <esp_heap_caps.h>

#include "mode_pinball.h"
#include "pinball_zones.h"
#include "pinball_physics.h"
#include "pinball_draw.h"
#include "pinball_test.h"

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

    pinball->balls    = heap_caps_calloc(MAX_NUM_BALLS, sizeof(pbCircle_t), MALLOC_CAP_SPIRAM);
    pinball->bumpers  = heap_caps_calloc(MAX_NUM_BUMPERS, sizeof(pbCircle_t), MALLOC_CAP_SPIRAM);
    pinball->walls    = heap_caps_calloc(MAX_NUM_WALLS, sizeof(pbLine_t), MALLOC_CAP_SPIRAM);
    pinball->flippers = heap_caps_calloc(MAX_NUM_FLIPPERS, sizeof(pbFlipper_t), MALLOC_CAP_SPIRAM);

    pinball->ballsTouching = heap_caps_calloc(MAX_NUM_BALLS, sizeof(pbTouchRef_t*), MALLOC_CAP_SPIRAM);
    for (uint32_t i = 0; i < MAX_NUM_BALLS; i++)
    {
        pinball->ballsTouching[i] = heap_caps_calloc(MAX_NUM_TOUCHES, sizeof(pbTouchRef_t), MALLOC_CAP_SPIRAM);
    }

    // Split the table into zones
    createTableZones(pinball);

    // Create random balls
    createRandomBalls(pinball, 0);
    pbCreateBall(pinball, (TFT_WIDTH / 2 + 20), (6));

    // Create random walls
    createRandomWalls(pinball, 0);

    // Create random bumpers
    createRandomBumpers(pinball, 0);

    // Create flippers
    createFlipper(pinball, TFT_WIDTH / 2 - 50, TFT_HEIGHT / 2, true);
    createFlipper(pinball, TFT_WIDTH / 2 + 50, TFT_HEIGHT / 2, false);

    // Load font
    loadFont("ibm_vga8.font", &pinball->ibm_vga8, false);
}

/**
 * @brief Tear down the pinball mode
 *
 */
static void pinExitMode(void)
{
    for (uint32_t i = 0; i < MAX_NUM_BALLS; i++)
    {
        free(pinball->ballsTouching[i]);
    }
    free(pinball->ballsTouching);

    free(pinball->balls);
    free(pinball->walls);
    free(pinball->bumpers);
    free(pinball->flippers);
    // Free font
    freeFont(&pinball->ibm_vga8);
    // Free the rest of the state
    free(pinball);
}

/**
 * @brief Run the pinball main loop
 *
 * @param elapsedUs The number of microseconds since this function was last called
 */
static void pinMainLoop(int64_t elapsedUs)
{
    // Make a local copy for speed
    pinball_t* p = pinball;

    // Check all queued button events
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        if (PB_RIGHT == evt.button)
        {
            p->flippers[1].buttonHeld = evt.down;
        }
        else if (PB_LEFT == evt.button)
        {
            p->flippers[0].buttonHeld = evt.down;
        }
    }

    // Only check physics once per frame
    p->frameTimer += elapsedUs;
    while (p->frameTimer >= PIN_US_PER_FRAME)
    {
        p->frameTimer -= PIN_US_PER_FRAME;
        updatePinballPhysicsFrame(pinball);
    }

    // Always draw foreground to prevent flicker
    pinballDrawForeground(pinball);

    // Log frame time for FPS
    p->frameTimesIdx                = (p->frameTimesIdx + 1) % NUM_FRAME_TIMES;
    p->frameTimes[p->frameTimesIdx] = esp_timer_get_time();
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
    pinballDrawBackground(pinball, x, y, w, h);
}
