/**
 * @file pushy.c
 * @author Brycey92
 * @brief A port of Socks' Pushy Kawaii 2
 * @link https://cults3d.com/en/3d-model/game/pushy-kawaii-v2
 * @date 2023-09-07
 */

//==============================================================================
// Includes
//==============================================================================

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "color_utils.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "hdw-led.h"
#include "hdw-nvs.h"

#include "pushy.h"

//==============================================================================
// Defines
//==============================================================================

// clang-format off

#define LOGFIRE              false
#define LOGPUSHY             false
#define VIDEO                false
#define GENERATE_COLOR_WHEEL false

#if VIDEO
#define PUSHY_BPM             80
#define PUSHY_HOLD_BEATS      0.5
#define PUSHY_HOLD_US         ((int64_t)(PUSHY_HOLD_BEATS     / (PUSHY_BPM / 60.0) * 1000 * 1000))

#define PUSHY_69_AT_BEATS     8
#define PUSHY_420_AT_BEATS    16
#define PUSHY_6969_AT_BEATS   24
#define PUSHY_42069_AT_BEATS  32
#define PUSHY_69420_AT_BEATS  36

#define PUSHY_BEAT_DROP_AT_US ((int64_t)((15 + 35776 / 44100.0) * 1000 * 1000))
#define PUSHY_69_AT_US        ((int64_t)(PUSHY_69_AT_BEATS    / (PUSHY_BPM / 60.0) * 1000 * 1000 + PUSHY_BEAT_DROP_AT_US))
#define PUSHY_420_AT_US       ((int64_t)(PUSHY_420_AT_BEATS   / (PUSHY_BPM / 60.0) * 1000 * 1000 + PUSHY_BEAT_DROP_AT_US))
#define PUSHY_6969_AT_US      ((int64_t)(PUSHY_6969_AT_BEATS  / (PUSHY_BPM / 60.0) * 1000 * 1000 + PUSHY_BEAT_DROP_AT_US))
#define PUSHY_42069_AT_US     ((int64_t)(PUSHY_42069_AT_BEATS / (PUSHY_BPM / 60.0) * 1000 * 1000 + PUSHY_BEAT_DROP_AT_US))
#define PUSHY_69420_AT_US     ((int64_t)(PUSHY_69420_AT_BEATS / (PUSHY_BPM / 60.0) * 1000 * 1000 + PUSHY_BEAT_DROP_AT_US))
#endif

#define NUM_DIGITS        8
#define NUM_PUSHY_COLORS 11 // 0-9 and "off"

#define IDLE_SECONDS_UNTIL_SAVE   3
#define SAVE_AT_MOD             100

#define SHUFFLE_AT_MOD 1000

// reserve a color for white, one for "off"/grey, and evenly spread the remaining colors across the rainbow
#define HUE_STEP         (255 / (NUM_PUSHY_COLORS - 2))
// reserve a color for "off"/grey, and evenly spread the remaining colors across the rainbow
#define RAINBOW_HUE_STEP (255 / (NUM_PUSHY_COLORS - 1))
#define SATURATION        255
#define BRIGHTNESS        255
// clang-format on

#define EFFECT_MAX 200

#define FIRE_TIMER_MS 100
#define FIREWINDOWS   100

//==============================================================================
// Enums
//==============================================================================

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    // clang-format off
    font_t sevenSegment;                     ///< The font used in the game

    char eights[NUM_DIGITS + 1];             ///< A string of '8's to draw behind the score as "unlit" seven-segment displays
    uint16_t eightsWidth;                    ///< The width of the string of '8's, in pixels

    uint32_t counter;                        ///< The score for the game
    uint32_t lastSaveCounter;                ///< The last score that was saved

    uint32_t allFireCounts[FIREWINDOWS];
    uint32_t fireCounter;
    uint32_t fireWindowCount;
    int64_t fireWindowStartUs;
    int64_t buttonPushedUs;                  ///< Microseconds since the last button push
    uint16_t btnState;                       ///< The button state

    int64_t rainbowTimer;                    ///< 0 if no digits should be rainbow, or EFFECT_MAX if any digits should
    int64_t weedTimer;                       ///< 0 if no digits should be weed colored, or EFFECT_MAX if any digits should
    bool rainbowDigits[NUM_DIGITS];          ///< A bitmap of digits that should be rainbow, from most significant digit at [0] to least significant digit at [NUM_DIGITS]
    bool weedDigits[NUM_DIGITS];             ///< A bitmap of digits that should be weed colored, from most significant digit at [0] to least significant digit at [NUM_DIGITS]
    float weedHue;                           ///< The hue to display on digits that are weed colored
#if VIDEO
    int64_t lockoutUs;
    bool reachedRainbow;
    bool reachedWeed;
    bool reachedDoubleRainbow;               ///< All the way
    int64_t audioTrackUs;
#endif

    paletteColor_t colors[NUM_PUSHY_COLORS]; ///< Colors for each digit 0-9
    uint8_t rainbowHues[NUM_PUSHY_COLORS];   ///< Hues to display on digits that are rainbow

    led_t boxleds[CONFIG_NUM_LEDS];
    // clang-format on
} pushy_t;

//==============================================================================
// Function Prototypes
//==============================================================================

static void pushyMainLoop(int64_t elapsedUs);
static void pushyEnterMode(void);
static void pushyExitMode(void);
static void pushyBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);

static void saveMemory(void);
static void shuffleColors(void);
static void readButton(void);
static void displayCounter(char const* counterStr);
static void checkSubStr(char const* counterStr, char const* const subStr, bool digitBitmap[NUM_DIGITS], int64_t* timer);
static void checkRainbow(char const* counterStr);
static void checkWeed(char const* counterStr);
static void updateEffects(char const* counterStr);
static uint32_t getFireCount(void);
static void displayFire(void);
static void updateFire(void);
void showDigit(uint8_t number, uint8_t colorIndex, uint8_t digitIndexFromLeastSignificant);

//==============================================================================
// Strings
//==============================================================================

/* Design Pattern!
 * These strings are all declared 'const' because they do not change, so that they are placed in ROM, not RAM.
 * Lengths are not explicitly given so the compiler can figure it out.
 */

static const char pushyName[]       = "Pushy Kawaii Go";
static const char pushyCounterKey[] = "pk_counter";
static const char rainbowStr[]      = "69";
static const char weedStr[]         = "420";

//==============================================================================
// Variables
//==============================================================================

/// The Swadge mode for Pushy
swadgeMode_t pushyMode = {
    .modeName                 = pushyName,
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = pushyEnterMode,
    .fnExitMode               = pushyExitMode,
    .fnMainLoop               = pushyMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = pushyBackgroundDrawCallback,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
};

/// All state information for the Pushy mode. This whole struct is calloc()'d and free()'d so that Pushy is only
/// using memory while it is being played
pushy_t* pushy = NULL;

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Enter Pushy mode, allocate required memory, and initialize required variables
 *
 */
static void pushyEnterMode(void)
{
    // Allocate and clear all memory for this mode. All the variables are contained in a single struct for convenience.
    // calloc() is used instead of malloc() because calloc() also initializes the allocated memory to zeros.
    pushy = calloc(1, sizeof(pushy_t));

    // Load a font
    loadFont("seven_segment.font", &pushy->sevenSegment, false);

    // Initialize string for "unlit" seven-segment displays
    memset(pushy->eights, '8', NUM_DIGITS);
    pushy->eights[NUM_DIGITS] = 0;
    pushy->eightsWidth        = textWidth(&pushy->sevenSegment, pushy->eights);

    // Load score from NVS
    readNvs32(pushyCounterKey, (int32_t*)&pushy->counter);

    // Initialize fire variables
    pushy->fireWindowStartUs = esp_timer_get_time();

    // Initialize weed and rainbow effect variables
    pushy->rainbowTimer = EFFECT_MAX;
    pushy->weedTimer    = EFFECT_MAX;

#if VIDEO
    pushy->audioTrackUs = PUSHY_BEAT_DROP_AT_US;
#endif

#if GENERATE_COLOR_WHEEL
    for (int i = 0; i < 255; i++)
    {
        printf("%" PRIu8 "\n", (uint8_t)paletteHsvToHex(i, 255, 255));
    }
#endif

    // Initialize default color values
    // clang-format off
    pushy->colors[0]  = paletteHsvToHex(           0,          0, BRIGHTNESS); // white
    pushy->colors[1]  = paletteHsvToHex(HUE_STEP * 0, SATURATION, BRIGHTNESS); // red
    pushy->colors[2]  = paletteHsvToHex(HUE_STEP * 1, SATURATION, BRIGHTNESS); // orange
    pushy->colors[3]  = paletteHsvToHex(HUE_STEP * 2, SATURATION, BRIGHTNESS); // yellow
    pushy->colors[4]  = paletteHsvToHex(HUE_STEP * 3, SATURATION, BRIGHTNESS); // lime green
    pushy->colors[5]  = paletteHsvToHex(HUE_STEP * 4, SATURATION, BRIGHTNESS); // green
    pushy->colors[6]  = paletteHsvToHex(HUE_STEP * 5, SATURATION, BRIGHTNESS); // aqua-ish
    pushy->colors[7]  = c025; //paletteHsvToHex(HUE_STEP * 6, SATURATION, BRIGHTNESS); // blue
    pushy->colors[8]  = paletteHsvToHex(HUE_STEP * 7, SATURATION, BRIGHTNESS); // purpley
    pushy->colors[9]  = paletteHsvToHex(HUE_STEP * 8, SATURATION, BRIGHTNESS); // pinkish
    pushy->colors[10] = paletteHsvToHex(0, 0, 55);                             // grey

    pushy->rainbowHues[0]  = RAINBOW_HUE_STEP *  0;
    pushy->rainbowHues[1]  = RAINBOW_HUE_STEP *  1;
    pushy->rainbowHues[2]  = RAINBOW_HUE_STEP *  2;
    pushy->rainbowHues[3]  = RAINBOW_HUE_STEP *  3;
    pushy->rainbowHues[4]  = RAINBOW_HUE_STEP *  4;
    pushy->rainbowHues[5]  = RAINBOW_HUE_STEP *  5;
    pushy->rainbowHues[6]  = RAINBOW_HUE_STEP *  6;
    pushy->rainbowHues[7]  = RAINBOW_HUE_STEP *  7;
    pushy->rainbowHues[8]  = RAINBOW_HUE_STEP *  8;
    pushy->rainbowHues[9]  = RAINBOW_HUE_STEP *  9;
    pushy->rainbowHues[10] = RAINBOW_HUE_STEP * 10; // never actually used, as this is redirected to pushy->colors[10]
    // clang-format on

    shuffleColors();
}

/**
 * This function is called when the mode is exited. It deinitializes variables and frees all memory.
 */
static void pushyExitMode(void)
{
    // Save score to NVS
    if (pushy->lastSaveCounter != pushy->counter)
    {
#if !VIDEO
        writeNvs32(pushyCounterKey, (int32_t)pushy->counter);
#endif
    }

    // Free the font
    freeFont(&pushy->sevenSegment);

    // Free everything else
    free(pushy);
}

/**
 * @brief This function is called periodically and frequently. It will either draw the menu or play the game, depending
 * on which screen is currently being displayed
 *
 * @param elapsedUs The time that has elapsed since the last call to this function, in microseconds. First call is
 * nonzero
 */
static void pushyMainLoop(int64_t elapsedUs)
{
    pushy->buttonPushedUs += elapsedUs;
#if VIDEO
    pushy->audioTrackUs += elapsedUs;

    uint32_t oldCounter = pushy->counter;

    // clang-format off
    if (pushy->audioTrackUs < (PUSHY_BEAT_DROP_AT_US + PUSHY_HOLD_US))
    {
        pushy->counter = 0;
    }
    else if (pushy->audioTrackUs < PUSHY_69_AT_US)
    {
        pushy->counter = (0 + (69 - 0) * (pushy->audioTrackUs - (PUSHY_BEAT_DROP_AT_US + PUSHY_HOLD_US)) / (double)(PUSHY_69_AT_US - (PUSHY_BEAT_DROP_AT_US + PUSHY_HOLD_US)));
    }
    else if (pushy->audioTrackUs < (PUSHY_69_AT_US + PUSHY_HOLD_US))
    {
        pushy->counter = 69;
    }
    else if (pushy->audioTrackUs < PUSHY_420_AT_US)
    {
        pushy->counter = (69 + (420 - 69) * (pushy->audioTrackUs - (PUSHY_69_AT_US + PUSHY_HOLD_US)) / (double)(PUSHY_420_AT_US - (PUSHY_69_AT_US + PUSHY_HOLD_US)));
    }
    else if (pushy->audioTrackUs < (PUSHY_420_AT_US + PUSHY_HOLD_US))
    {
        pushy->counter = 420;
    }
    else if (pushy->audioTrackUs < PUSHY_6969_AT_US)
    {
        pushy->counter = (420 + (6969 - 420) * ((pushy->audioTrackUs - (PUSHY_420_AT_US + PUSHY_HOLD_US)) / (double)(PUSHY_6969_AT_US - (PUSHY_420_AT_US + PUSHY_HOLD_US))));
    }
    else if (pushy->audioTrackUs < (PUSHY_6969_AT_US + PUSHY_HOLD_US))
    {
        pushy->counter = 6969;
    }
    else if (pushy->audioTrackUs < PUSHY_42069_AT_US)
    {
        pushy->counter = (6969 + (42069 - 6969) * ((pushy->audioTrackUs - (PUSHY_6969_AT_US + PUSHY_HOLD_US)) / (double)(PUSHY_42069_AT_US - (PUSHY_6969_AT_US + PUSHY_HOLD_US))));
    }
    else if (pushy->audioTrackUs < (PUSHY_42069_AT_US + PUSHY_HOLD_US))
    {
        pushy->counter = 42069;
    }
    else if (pushy->audioTrackUs < PUSHY_69420_AT_US)
    {
        pushy->counter = (42069 + (69420 - 42069) * ((pushy->audioTrackUs - (PUSHY_42069_AT_US + PUSHY_HOLD_US)) / (double)(PUSHY_69420_AT_US - (PUSHY_42069_AT_US + PUSHY_HOLD_US))));
    }
    else if (pushy->audioTrackUs < (PUSHY_69420_AT_US + PUSHY_HOLD_US))
    {
        pushy->counter = 69420;
    }
    else
    {
        // Do nothing, continue holding 69420
    }
    // clang-format on

    pushy->fireCounter += pushy->counter - oldCounter;
    pushy->buttonPushedUs = 0;
    if ((uint32_t)(oldCounter / SHUFFLE_AT_MOD) < (uint32_t)(pushy->counter / SHUFFLE_AT_MOD))
    {
        shuffleColors();
    }

    // if (pushy->lockoutUs == 0)
#endif
    {
        readButton();
    }
#if VIDEO
    // else
    // {
    //     pushy->lockoutUs = MAX(pushy->lockoutUs - elapsedUs, 0);
    // }

    // if (!pushy->reachedRainbow && pushy->counter >= 69)
    // {
    //     pushy->lockoutUs = 1 * 1000 * 1000;
    //     pushy->reachedRainbow = true;
    //     pushy->counter = 69;
    // }
    // else if (!pushy->reachedWeed && pushy->counter >= 420)
    // {
    //     pushy->lockoutUs = 5 * 1000 * 1000;
    //     pushy->reachedWeed = true;
    //     pushy->counter = 420;
    // }
    // else if (!pushy->reachedDoubleRainbow && pushy->counter >= 6969)
    // {
    //     pushy->lockoutUs = 5 * 1000 * 1000;
    //     pushy->reachedDoubleRainbow = true;
    //     pushy->counter = 6969;
    // }
#endif

    // If score has changed, save if last input was longer ago than our threshold or if score is a multiple of 100
    if (pushy->lastSaveCounter != pushy->counter && pushy->buttonPushedUs > IDLE_SECONDS_UNTIL_SAVE * 1000 * 1000)
    {
        saveMemory();
    }

    // Draw "unlit" seven-segment displays
    drawText(&pushy->sevenSegment, pushy->colors[NUM_PUSHY_COLORS - 1], pushy->eights,
             (TFT_WIDTH - pushy->eightsWidth) / 2, (TFT_HEIGHT - pushy->sevenSegment.height) / 2);

    // Draw "lit" segments
    char counterStr[NUM_DIGITS + 1];
    snprintf(counterStr, NUM_DIGITS + 1, "%*" PRIu32, NUM_DIGITS, pushy->counter);

    updateEffects(counterStr);
    updateFire();

    displayCounter(counterStr);
    displayFire();

    setLeds(pushy->boxleds, CONFIG_NUM_LEDS);
}

/**
 * This function is called when the display driver wishes to update a
 * section of the display.
 *
 * @param x the x coordinate that should be updated
 * @param y the x coordinate that should be updated
 * @param w the width of the rectangle to be updated
 * @param h the height of the rectangle to be updated
 * @param up update number
 * @param numUp update number denominator
 */
static void pushyBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    // Use TURBO drawing mode to draw individual pixels fast
    SETUP_FOR_TURBO();

    // Draw a grid
    for (int16_t yp = y; yp < y + h; yp++)
    {
        for (int16_t xp = x; xp < x + w; xp++)
        {
            TURBO_SET_PIXEL(xp, yp, c000);
        }
    }
}

// Save score to NVS
static void saveMemory(void)
{
#if !VIDEO
    writeNvs32(pushyCounterKey, (int32_t)pushy->counter);
#endif
    pushy->lastSaveCounter = pushy->counter;
}

static void shuffleColors(void)
{
#if !VIDEO
    for (uint8_t i = 0; i < NUM_PUSHY_COLORS - 1; i++)
    {
        uint8_t n = esp_random() % (NUM_PUSHY_COLORS - 1);
#else
    for (uint8_t i = (pushy->counter == 0); i < NUM_PUSHY_COLORS - 1; i++)
    {
        uint8_t n = esp_random() % (NUM_PUSHY_COLORS - pushy->counter - 1) + (pushy->counter == 0);
#endif
#if LOGPUSHY
        printf("gonna swap the next two colors: %" PRIu8 ", %" PRIu8 "\n", i, n);
#endif

        paletteColor_t temp = pushy->colors[n];
        pushy->colors[n]    = pushy->colors[i];
        pushy->colors[i]    = temp;
    }
}

static void readButton(void)
{
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        // Save the button state
        pushy->btnState = evt.state;

        // Check if the A button was pressed
        if (evt.down && PB_A == evt.button)
        {
            pushy->counter++;
            pushy->fireCounter++;
            pushy->buttonPushedUs = 0;
            if (pushy->counter % SAVE_AT_MOD == 0)
            {
                saveMemory();
            }
            if (pushy->counter % SHUFFLE_AT_MOD == 0)
            {
                shuffleColors();
            }
        }
    }

#if VIDEO
    if (pushy->btnState & PB_LEFT)
    {
        pushy->counter++;
        pushy->fireCounter++;
        pushy->buttonPushedUs = 0;
        if (pushy->counter % SHUFFLE_AT_MOD == 0)
        {
            shuffleColors();
        }
    }
    if (pushy->btnState & PB_DOWN)
    {
        pushy->counter += 10;
        pushy->fireCounter += 10;
        pushy->buttonPushedUs = 0;
        if (pushy->counter % SHUFFLE_AT_MOD == 0)
        {
            shuffleColors();
        }
    }
    if (pushy->btnState & PB_RIGHT)
    {
        pushy->counter += 100;
        pushy->fireCounter += 100;
        pushy->buttonPushedUs = 0;
        if (pushy->counter % SHUFFLE_AT_MOD == 0)
        {
            shuffleColors();
        }
    }
#endif
}

static void displayCounter(char const* counterStr)
{
#if LOGPUSHY
    printf("Displaying counter\n");
#endif

    for (unsigned int i = 0; i < NUM_DIGITS; i++)
    {
        // As i increases, we move from least-significant digit to most-significant digit
        if ((counterStr[NUM_DIGITS - 1 - i]) != ' ')
        {
            int c = counterStr[NUM_DIGITS - 1 - i] - '0';
            showDigit(c, c, i);
        }
    }
}

static void checkSubStr(char const* counterStr, char const* const subStr, bool digitBitmap[NUM_DIGITS], int64_t* timer)
{
    bool found                = false;
    char const* curCounterStr = counterStr;
    memset(digitBitmap, false, NUM_DIGITS);

    char const* subStrInCounterStr = strstr(curCounterStr, subStr);
    while (NULL != subStrInCounterStr)
    {
        uint8_t startDigit = subStrInCounterStr - counterStr;
        for (uint8_t i = 0; i < strlen(subStr); i++)
        {
            digitBitmap[startDigit + i] = true;
        }

        *timer = 0;
        found  = true;

        // Move curCounterStr to be one after the found str
        curCounterStr = &subStrInCounterStr[1];
        // Continue searching
        subStrInCounterStr = strstr(curCounterStr, subStr);
    }

    if (!found)
    {
        *timer = EFFECT_MAX;
    }
}

static void checkRainbow(char const* counterStr)
{
    checkSubStr(counterStr, rainbowStr, pushy->rainbowDigits, &pushy->rainbowTimer);
}

static void checkWeed(char const* counterStr)
{
    checkSubStr(counterStr, weedStr, pushy->weedDigits, &pushy->weedTimer);
}

static void updateEffects(char const* counterStr)
{
#if LOGPUSHY
    printf("Updating effects\n");
#endif
    checkRainbow(counterStr);
    checkWeed(counterStr);

    if (pushy->rainbowTimer < EFFECT_MAX)
    {
        for (int i = 0; i < NUM_PUSHY_COLORS; i++)
        {
#if !VIDEO
            pushy->rainbowHues[i] = (pushy->rainbowHues[i] + 2 % 255);
#else
            pushy->rainbowHues[i] = (pushy->rainbowHues[i] + 8 % 255);
#endif
        }
    }

    if (pushy->weedTimer < EFFECT_MAX)
    {
        pushy->weedHue = 105;
        // pushy->weedHue -= 0.25;
        // pushy->weedHue = MAX(weedHue, 24);
    }
}

static uint32_t getFireCount(void)
{
    uint32_t totalFire = 0;

    for (uint8_t i = 0; i < FIREWINDOWS; i++)
    {
        totalFire += pushy->allFireCounts[i];
    }

#if LOGFIRE
    printf("Fire subs: ");

    for (uint8_t i = 0; i < FIREWINDOWS; i++)
    {
        printf("%" PRIu32 " ", pushy->allFireCounts[i]);
    }
    printf("\n%" PRIu32 "\n", totalFire);
#endif

    return totalFire;
}

static void displayFire(void)
{
#if LOGPUSHY
    printf("Displaying fire\n");
#endif

    // for (int i = 0; i < CONFIG_NUM_LEDS; i++)
    // {
    //     pushy->boxleds[i] = LedEHSVtoHEXhelper(0, 200, 200);
    // }
    // setLeds(pushy->boxleds, CONFIG_NUM_LEDS);
    // return;

    int count = getFireCount();

    led_t color = LedEHSVtoHEXhelper((uint8_t)(count / 2.5), 255, 255, true);

    for (int i = 0; i < CONFIG_NUM_LEDS; i++)
    {
        if (count > 5)
        {
            pushy->boxleds[i] = color;
        }
        else
        {
            pushy->boxleds[i] = LedEHSVtoHEXhelper(0, 0, 10, true);
        }
    }
}

static void updateFire(void)
{
    int64_t currentUs = esp_timer_get_time();
    if (currentUs - pushy->fireWindowStartUs > FIRE_TIMER_MS * 1000) // defines how long between checks of the windows
    {
        pushy->allFireCounts[pushy->fireWindowCount] = pushy->fireCounter; // how many presses in the current window
        pushy->fireCounter                           = 0;
        pushy->fireWindowCount   = (pushy->fireWindowCount + 1) % FIREWINDOWS; // this rotates us through the array
        pushy->fireWindowStartUs = currentUs;

#if LOGPUSHY
        printf("Fire count: %" PRIu32 "\n", getFireCount());
#endif
    }
}

void showDigit(uint8_t number, uint8_t colorIndex, uint8_t digitIndexFromLeastSignificant)
{
#if LOGPUSHY
    printf("showing a digit\n");
#endif

    // Convert the number to a string
    paletteColor_t color;
    char numberAsStr[4];
    snprintf(numberAsStr, sizeof(numberAsStr), "%1" PRIu8, number);

    // Apply weed and rainbow effects, or if no effects, get the current color for this digit
    if (pushy->weedDigits[NUM_DIGITS - 1 - digitIndexFromLeastSignificant])
    {
        color = paletteHsvToHex((int)pushy->weedHue, SATURATION, BRIGHTNESS);
#if LOGPUSHY
        printf("weed digit at %" PRIu8 "\n", digitIndexFromLeastSignificant);
        printf("weed timer is %" PRIi64 "\n", pushy->weedTimer);
#endif
    }
    else if (pushy->rainbowDigits[NUM_DIGITS - 1 - digitIndexFromLeastSignificant]
             && colorIndex != NUM_PUSHY_COLORS - 1)
    {
        color = paletteHsvToHex(pushy->rainbowHues[0], SATURATION, BRIGHTNESS);
#if LOGPUSHY
        printf("rainbow digit at %" PRIu8 "\n", digitIndexFromLeastSignificant);
        printf("rainbow timer is %" PRIi64 "\n", pushy->rainbowTimer);
#endif
    }
    else
    {
        color = pushy->colors[colorIndex];
    }

    // Draw the digit to the screen
    uint16_t digitWidth = textWidth(&pushy->sevenSegment, "8");
    drawText(&pushy->sevenSegment, color, numberAsStr,
             (TFT_WIDTH - pushy->eightsWidth) / 2
                 + ((digitWidth + 1) * (NUM_DIGITS - 1 - digitIndexFromLeastSignificant)),
             (TFT_HEIGHT - pushy->sevenSegment.height) / 2);
}
