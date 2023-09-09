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

#define LOGFIRE false

#define NUM_DIGITS 8
#define NUM_PUSHY_COLORS 11 // 0-9 and "off"

#define IDLE_SECONDS_UNTIL_SAVE 3
#define SAVE_AT_MOD 100

#define SHUFFLE_AT_MOD 1000

#define HUE_STEP 28
#define SATURATION 255
#define BRIGHTNESS 255

#define EFFECT_MAX 200

#define FIRE_TIMER_MS 100
#define FIREWINDOWS 100

//==============================================================================
// Enums
//==============================================================================

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    font_t sevenSegment;      ///< The font used in the game

    char eights[NUM_DIGITS + 1];
    uint16_t eightsWidth;

    uint32_t counter;           ///< The score for the game
    uint32_t lastSaveCounter;   ///< The last score that was saved

    uint32_t allFireCounts[FIREWINDOWS];
    uint32_t fireCounter;
    uint32_t fireWindowCount;
    int64_t fireWindowStartUs;
    int64_t buttonPushedMillis;

    int64_t usSinceLastInput; ///< Microseconds since the last save
    uint16_t btnState;        ///< The button state

    int64_t rainbowTimer;
    int64_t weedTimer;
    bool rainbowDigits[8];
    bool weedDigits[8];
    float weedHue;
    int64_t ledFadeTimer;     ///< The timer to fade LEDs

    paletteColor_t colors[NUM_PUSHY_COLORS];        ///< 
    uint8_t rainbowHues[NUM_PUSHY_COLORS]; ///< 

    led_t boxleds[CONFIG_NUM_LEDS];
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

static const char pushyName[] = "Pushy Kawaii Go";
static const char pushyCounterKey[] = "pk_counter";
static const char rainbowStr[] = "69";
static const char weedStr[] = "420";

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
    pushy->eightsWidth = textWidth(&pushy->sevenSegment, pushy->eights);

    // Load score from NVS
    readNvs32(pushyCounterKey, (int32_t*) &pushy->counter);

    // Initialize fire variables
    pushy->fireWindowStartUs = esp_timer_get_time();

    // Initialize weed and rainbow variables
    pushy->rainbowTimer = EFFECT_MAX;
    pushy->weedTimer = EFFECT_MAX;

    // Initialize default color values
    pushy->colors[0] = paletteHsvToHex(0, 0, BRIGHTNESS);               // white
    pushy->colors[1] = paletteHsvToHex(HUE_STEP * 0, SATURATION, BRIGHTNESS); // red
    pushy->colors[2] = paletteHsvToHex(HUE_STEP * 1, SATURATION, BRIGHTNESS); // orange
    pushy->colors[3] = paletteHsvToHex(HUE_STEP * 2, SATURATION, BRIGHTNESS); // yellow
    pushy->colors[4] = paletteHsvToHex(HUE_STEP * 3, SATURATION, BRIGHTNESS); // lime green
    pushy->colors[5] = paletteHsvToHex(HUE_STEP * 4, SATURATION, BRIGHTNESS); // green
    pushy->colors[6] = paletteHsvToHex(HUE_STEP * 5, SATURATION, BRIGHTNESS); // aqua-ish
    pushy->colors[7] = c025; //paletteHsvToHex(HUE_STEP * 6, SATURATION, BRIGHTNESS); // blue
    pushy->colors[8] = paletteHsvToHex(HUE_STEP * 7, SATURATION, BRIGHTNESS); // purpley
    pushy->colors[9] = paletteHsvToHex(HUE_STEP * 8, SATURATION, BRIGHTNESS); // pinkish
    pushy->colors[10] = paletteHsvToHex(0, 0, 55);                      // grey

    pushy->rainbowHues[0] = 23 * 0;
    pushy->rainbowHues[1] = 23 * 1;
    pushy->rainbowHues[2] = 23 * 2;
    pushy->rainbowHues[3] = 23 * 3;
    pushy->rainbowHues[4] = 23 * 4;
    pushy->rainbowHues[5] = 23 * 5;
    pushy->rainbowHues[6] = 23 * 6;
    pushy->rainbowHues[7] = 23 * 7;
    pushy->rainbowHues[8] = 23 * 8;
    pushy->rainbowHues[9] = 23 * 9;
    pushy->rainbowHues[10] = 23 * 10;

    shuffleColors();
}

/**
 * This function is called when the mode is exited. It deinitializes variables and frees all memory.
 */
static void pushyExitMode(void)
{
    // Save score to NVS
    if(pushy->lastSaveCounter != pushy->counter)
    {
        writeNvs32(pushyCounterKey, (int32_t) pushy->counter);
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
 * @param elapsedUs The time that has elapsed since the last call to this function, in microseconds
 */
static void pushyMainLoop(int64_t elapsedUs)
{
    pushy->usSinceLastInput += elapsedUs;

    readButton();

    // If the score has changed, save if the last input was longer ago than our threshold or if the score is a multiple of 100
    if(pushy->lastSaveCounter != pushy->counter && pushy->usSinceLastInput > IDLE_SECONDS_UNTIL_SAVE * 1000 * 1000)
    {
        saveMemory();
    }

    // Draw "unlit" 7-segment displays
    drawText(&pushy->sevenSegment, pushy->colors[NUM_PUSHY_COLORS - 1], pushy->eights, (TFT_WIDTH - pushy->eightsWidth) / 2, (TFT_HEIGHT - pushy->sevenSegment.height) / 2);

    // Draw "lit" segments
    char counterStr[NUM_DIGITS + 1];
    snprintf(counterStr, NUM_DIGITS + 1, "%*"PRIu32, NUM_DIGITS, pushy->counter);

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
 * @param disp The display to draw to
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
    writeNvs32(pushyCounterKey, (int32_t) pushy->counter);
    pushy->lastSaveCounter = pushy->counter;
}

static void shuffleColors(void)
{
    for (uint8_t i = 0; i < NUM_PUSHY_COLORS - 1; i++)
    {
        uint8_t n = esp_random() % (NUM_PUSHY_COLORS - 1);
        // printf("gonna swap the next two colors: %"PRIu8", %"PRIu8"\n", i, n);
        
        paletteColor_t temp = pushy->colors[n];
        pushy->colors[n] = pushy->colors[i];
        pushy->colors[i] = temp;
    }
}

static void readButton(void)
{
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        // Save the button state
        pushy->btnState = evt.state;

        // Check if the pause button was pressed
        if (evt.down && (PB_A == evt.button))
        {
            pushy->counter++;
            pushy->fireCounter++;
            pushy->usSinceLastInput = 0;
            if(pushy->counter % SAVE_AT_MOD == 0)
            {
                saveMemory();
            }
            if(pushy->counter % SHUFFLE_AT_MOD == 0)
            {
                shuffleColors();
            }
        }
    }
}

static void displayCounter(char const* counterStr)
{
    // printf("Displaying counter\n");
    
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
    memset(digitBitmap, false, NUM_DIGITS);
    
    char const* subStrInCounterStr = strstr(counterStr, subStr);
    if(subStrInCounterStr == NULL)
    {
        *timer = EFFECT_MAX;
        return;
    }

    uint8_t startDigit = subStrInCounterStr - counterStr;
    for (uint8_t i = 0; i < strlen(subStr); i++)
    {
        digitBitmap[startDigit + i] = true;
    }

    *timer = 0;
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
    // printf("Updating effects\n");
    checkRainbow(counterStr);
    checkWeed(counterStr);

    if (pushy->rainbowTimer < EFFECT_MAX)
    {
        for (int i = 0; i < NUM_PUSHY_COLORS; i++)
        {
            pushy->rainbowHues[i] = (pushy->rainbowHues[i] + 2 % 255);
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
    printf("%"PRIu32" ", pushy->allFireCounts[i]);
    }
    printf("\n%"PRIu32"\n", totalFire);
#endif

    return totalFire;
}

static void displayFire(void)
{
    // // printf("Displaying fire\n");
    // for (int i = 0; i < CONFIG_NUM_LEDS; i++)
    // {
    //     pushy->boxleds[i] = LedEHSVtoHEXhelper(0, 200, 200);
    // }
    // setLeds(pushy->boxleds, CONFIG_NUM_LEDS);
    // return;

    int count = getFireCount();

    led_t color = LedEHSVtoHEXhelper((uint8_t)(count / 2.5), 255, 250, true);

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
        pushy->fireCounter = 0;
        pushy->fireWindowCount = (pushy->fireWindowCount + 1) % FIREWINDOWS; // this rotates us through the array
        pushy->fireWindowStartUs = currentUs;
        // printf("Fire count: %"PRIu32"\n", getFireCount());
    }
}

void showDigit(uint8_t number, uint8_t colorIndex, uint8_t digitIndexFromLeastSignificant)
{
    // printf("showing a digit\n");
    
    paletteColor_t color;
    char numberAsStr[2];
    snprintf(numberAsStr, 2, "%1"PRIu8, number);

    if (pushy->weedDigits[NUM_DIGITS - 1 - digitIndexFromLeastSignificant])
    {
        color = paletteHsvToHex((int) pushy->weedHue, SATURATION, BRIGHTNESS);
        // printf("weed digit at %"PRIu8"\n", digitIndexFromLeastSignificant);
        // printf("weed timer is %"PRIi64"\n", pushy->weedTimer);
    }
    else if (pushy->rainbowDigits[NUM_DIGITS - 1 - digitIndexFromLeastSignificant] && colorIndex != NUM_PUSHY_COLORS - 1)
    {
        color = paletteHsvToHex(pushy->rainbowHues[0], SATURATION, BRIGHTNESS);
        // printf("rainbow digit at %"PRIu8"\n", digitIndexFromLeastSignificant);
        // printf("rainbow timer is %"PRIi64"\n", pushy->rainbowTimer);
    }
    else
    {
        color = pushy->colors[colorIndex];
    }

    uint16_t digitWidth = textWidth(&pushy->sevenSegment, "8");
    drawText(&pushy->sevenSegment, color, numberAsStr,
        (TFT_WIDTH - pushy->eightsWidth) / 2 + ((digitWidth + 1) * (NUM_DIGITS - 1 - digitIndexFromLeastSignificant)),
        (TFT_HEIGHT - pushy->sevenSegment.height) / 2);
}
