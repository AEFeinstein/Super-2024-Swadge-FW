//==============================================================================
// Includes
//==============================================================================

#include <esp_random.h>

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

    // Split the table into zones
    createTableZones(pinball);

    // Create random balls
    createRandomBalls(pinball, 10);

    // Create random walls
    createRandomWalls(pinball, 50);

    // Create random bumpers
    createRandomBumpers(pinball, 10);
}

/**
 * @brief Tear down the pinball mode
 *
 */
static void pinExitMode(void)
{
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

    // Only check physics once per frame
    p->frameTimer += elapsedUs;
    while (p->frameTimer >= PIN_US_PER_FRAME)
    {
        p->frameTimer -= PIN_US_PER_FRAME;
        updatePinballPhysicsFrame(pinball);
    }

    // Always draw foreground to prevent flicker
    pinballDrawForeground(pinball);
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
    // TODO is this causing flickering b/c it's drawn faster than the background?
    pinballDrawBackground(pinball, x, y, w, h);
}
