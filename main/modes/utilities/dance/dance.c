/*
 * dance.c
 *
 *  Created on: Nov 10, 2018
 *      Author: adam
 */

//==============================================================================
// Includes
//==============================================================================

#include <esp_sleep.h>
#include <esp_heap_caps.h>

#include "dance.h"
#include "settingsManager.h"
#include "mainMenu.h"

//==============================================================================
// Defines
//==============================================================================

#define RGB_2_ARG(r, g, b) ((((r) & 0xFF) << 16) | (((g) & 0xFF) << 8) | (((b) & 0xFF)))
#define ARG_R(arg)         (((arg) >> 16) & 0xFF)
#define ARG_G(arg)         (((arg) >> 8) & 0xFF)
#define ARG_B(arg)         (((arg) >> 0) & 0xFF)

#define DANCE_SPEED_MULT 8

// Sleep the TFT after 5s
#define TFT_TIMEOUT_US 5000000

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    uint32_t danceIdx;
    uint32_t danceSpeed;

    bool resetDance;
    bool blankScreen;

    uint32_t buttonPressedTimer;

    menu_t* menu;
    menuManiaRenderer_t* menuRenderer;

    const char** danceNames;
    int32_t* danceVals;
} danceMode_t;

//==============================================================================
// Prototypes
//==============================================================================

static void danceEnterMode(void);
static void danceExitMode(void);
static void danceMainLoop(int64_t elapsedUs);
static uint32_t danceRand(uint32_t bound);
static void danceMenuCb(const char* label, bool selected, uint32_t value);

//==============================================================================
// Const Variables
//==============================================================================

static const char danceName[]      = "Light Dances";
static const char str_exit[]       = "Exit";
static const char str_brightness[] = "Brightness";

static const char str_speed[]    = "Speed: ";
static const char* speedLabels[] = {
    "1/8x", "1/6x", "1/4x", "1/3x", "1/2x", "1x", "1.5x", "2x", "4x",
};
static const int32_t speedVals[] = {
    64, // 1/8x
    48, // 1/6x
    32, // 1/4x
    24, // 1/3x
    16, // 1/2x
    8,  // 1x
    6,  // 1.5x
    4,  // 2x
    2,  // 4x
};

const ledDanceArg ledDances[] = {
    {.func = danceComet, .arg = RGB_2_ARG(0, 0, 0), .name = "Comet RGB"},
    {.func = danceComet, .arg = RGB_2_ARG(0xFF, 0, 0), .name = "Comet R"},
    {.func = danceComet, .arg = RGB_2_ARG(0, 0xFF, 0), .name = "Comet G"},
    {.func = danceComet, .arg = RGB_2_ARG(0, 0, 0xFF), .name = "Comet B"},
    {.func = danceRise, .arg = RGB_2_ARG(0, 0, 0), .name = "Rise RGB"},
    {.func = danceRise, .arg = RGB_2_ARG(0xFF, 0, 0), .name = "Rise R"},
    {.func = danceRise, .arg = RGB_2_ARG(0, 0xFF, 0), .name = "Rise G"},
    {.func = danceRise, .arg = RGB_2_ARG(0, 0, 0xFF), .name = "Rise B"},
    {.func = dancePulse, .arg = RGB_2_ARG(0, 0, 0), .name = "Pulse RGB"},
    {.func = dancePulse, .arg = RGB_2_ARG(0xFF, 0, 0), .name = "Pulse R"},
    {.func = dancePulse, .arg = RGB_2_ARG(0, 0xFF, 0), .name = "Pulse G"},
    {.func = dancePulse, .arg = RGB_2_ARG(0, 0, 0xFF), .name = "Pulse B"},
    {.func = danceSharpRainbow, .arg = 0, .name = "Rainbow Sharp"},
    {.func = danceSmoothRainbow, .arg = 20000, .name = "Rainbow Slow"},
    {.func = danceSmoothRainbow, .arg = 4000, .name = "Rainbow Fast"},
    {.func = danceRainbowSolid, .arg = 0, .name = "Rainbow Solid"},
    {.func = danceFire, .arg = RGB_2_ARG(0xFF, 51, 0), .name = "Fire R"},
    {.func = danceFire, .arg = RGB_2_ARG(0, 0xFF, 51), .name = "Fire G"},
    {.func = danceFire, .arg = RGB_2_ARG(51, 0, 0xFF), .name = "Fire B"},
    {.func = danceBinaryCounter, .arg = 0, .name = "Binary"},
    {.func = dancePoliceSiren, .arg = 0, .name = "Siren"},
    {.func = dancePureRandom, .arg = 0, .name = "Random LEDs"},
    {.func = danceChristmas, .arg = 1, .name = "Holiday 1"},
    {.func = danceChristmas, .arg = 0, .name = "Holiday 2"},
    {.func = danceFlashlight, .arg = 0, .name = "Flashlight"},
    {.func = danceNone, .arg = 0, .name = "None"},
    {.func = danceRandomDance, .arg = 0, .name = "Shuffle All"},
};

//==============================================================================
// Variables
//==============================================================================

swadgeMode_t danceMode = {
    .modeName        = danceName,
    .fnEnterMode     = danceEnterMode,
    .fnExitMode      = danceExitMode,
    .fnMainLoop      = danceMainLoop,
    .wifiMode        = NO_WIFI,
    .fnEspNowRecvCb  = NULL,
    .fnEspNowSendCb  = NULL,
    .fnAudioCallback = NULL,
    .overrideUsb     = false,
};

danceMode_t* danceState;

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Enter the LED dance mode and allocate resources
 */
void danceEnterMode(void)
{
    danceState = calloc(1, sizeof(danceMode_t));

    danceState->danceIdx   = 0;
    danceState->danceSpeed = DANCE_SPEED_MULT;

    danceState->resetDance  = true;
    danceState->blankScreen = false;

    danceState->buttonPressedTimer = 0;

    danceState->menu         = initMenu(danceName, danceMenuCb);
    danceState->menuRenderer = initMenuManiaRenderer(NULL, NULL, NULL);
    setManiaLedsOn(danceState->menuRenderer, false);
    static const paletteColor_t shadowColors[] = {
        c430, c431, c442, c543, c554, c555, c554, c543, c442, c431,
    };
    led_t offLed = {0};
    recolorMenuManiaRenderer(danceState->menuRenderer, // Pango palette!
                             c430, c542, c111,         // titleBgColor, titleTextColor, textOutlineColor
                             c045,                     // bgColor
                             c542, c541,               // outerRingColor, innerRingColor
                             c111, c455,               // rowColor, rowTextColor
                             shadowColors, ARRAY_SIZE(shadowColors), offLed);

    // Add dances to the menu
    danceState->danceNames = heap_caps_calloc(ARRAY_SIZE(ledDances), sizeof(char*), MALLOC_CAP_SPIRAM);
    danceState->danceVals  = heap_caps_calloc(ARRAY_SIZE(ledDances), sizeof(int32_t), MALLOC_CAP_SPIRAM);
    for (int32_t dIdx = 0; dIdx < ARRAY_SIZE(ledDances); dIdx++)
    {
        danceState->danceNames[dIdx] = ledDances[dIdx].name;
        danceState->danceVals[dIdx]  = dIdx;
    }
    const settingParam_t danceParam = {
        .min = 0,
        .max = ARRAY_SIZE(ledDances) - 1,
    };
    addSettingsOptionsItemToMenu(danceState->menu, NULL, danceState->danceNames, danceState->danceVals,
                                 ARRAY_SIZE(ledDances), &danceParam, 0);

    // Add brightness to the menu
    addSettingsItemToMenu(danceState->menu, str_brightness, getLedBrightnessSettingBounds(), getLedBrightnessSetting());

    // Add speed to the menu
    const settingParam_t speedParam = {
        .min = speedVals[0],
        .max = speedVals[ARRAY_SIZE(speedVals) - 1],
    };
    addSettingsOptionsItemToMenu(danceState->menu, str_speed, speedLabels, speedVals, ARRAY_SIZE(speedVals),
                                 &speedParam, speedVals[5]);

    // Add exit to the menu
    addSingleItemToMenu(danceState->menu, str_exit);
}

/**
 * @brief Exit the LED dance mode and free resources
 */
void danceExitMode(void)
{
    if (danceState->blankScreen)
    {
        // Turn the screen on
        enableTFTBacklight();
        setTFTBacklightBrightness(getTftBrightnessSetting());
    }
    deinitMenuManiaRenderer(danceState->menuRenderer);
    deinitMenu(danceState->menu);

    free(danceState->danceNames);
    free(danceState->danceVals);
    free(danceState);
    danceState = NULL;
}

/**
 * @brief Run the main loop for the LED dance mode
 *
 * @param elapsedUs The time elapsed since the last invocation of this function, in microseconds
 */
void danceMainLoop(int64_t elapsedUs)
{
    // Poll for button inputs
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        // Reset this on any button event
        danceState->buttonPressedTimer = 0;

        // This button press will wake the display, so don't process it
        if (danceState->blankScreen)
        {
            return;
        }

        danceState->menu = menuButton(danceState->menu, evt);
    }

    // Light the LEDs!
    ledDances[danceState->danceIdx].func(elapsedUs * DANCE_SPEED_MULT / danceState->danceSpeed,
                                         ledDances[danceState->danceIdx].arg, danceState->resetDance);
    danceState->resetDance = false;

    // If the screen is blank
    if (danceState->blankScreen)
    {
        // If a button has been pressed recently
        if (danceState->buttonPressedTimer < TFT_TIMEOUT_US)
        {
            // Turn the screen on
            enableTFTBacklight();
            setTFTBacklightBrightness(getTftBrightnessSetting());
            danceState->blankScreen = false;
            // Draw to it
            drawMenuMania(danceState->menu, danceState->menuRenderer, elapsedUs);
        }
    }
    else
    {
        // Check if it should be blanked
        danceState->buttonPressedTimer += elapsedUs;
        if (danceState->buttonPressedTimer >= TFT_TIMEOUT_US)
        {
            disableTFTBacklight();
            danceState->blankScreen = true;
        }
        else
        {
            // Screen is not blank, draw to it
            drawMenuMania(danceState->menu, danceState->menuRenderer, elapsedUs);
        }
    }

    // Only sleep with a blank screen, otherwise the screen flickers
    if (danceState->blankScreen)
    {
        // Wait for any LED transactions to finish, otherwise RMT will do weird things during CPU sleep
        flushLeds();
        /* Light sleep for 40ms (see DEFAULT_FRAME_RATE_US).
         * The longer the sleep, the choppier the LED animations.
         * Sleeping longer than the current framerate will look worse
         */
        esp_sleep_enable_timer_wakeup(DEFAULT_FRAME_RATE_US);
        esp_light_sleep_start();
    }
}

/**
 * @brief Callback for the menu options
 *
 * @param label The menu option that was selected or changed
 * @param selected True if the option was selected, false if it was only changed
 * @param value The setting value for this operation
 */
void danceMenuCb(const char* label, bool selected, uint32_t value)
{
    if (selected && str_exit == label)
    {
        // Exit to the main menu
        switchToSwadgeMode(&mainMenuMode);
    }
    else if (str_brightness == label)
    {
        setLedBrightnessSetting(value);
    }
    else if (str_speed == label)
    {
        danceState->danceSpeed = value;
    }
    else if (NULL == label) // Dance names are label-less
    {
        if (danceState->danceIdx != value)
        {
            danceState->danceIdx   = value;
            danceState->resetDance = true;
        }
    }
}

/**
 * @return The number of different dances
 */
uint8_t getNumDances(void)
{
    return ARRAY_SIZE(ledDances);
}

/**
 * Get a random number from a range.
 *
 * This isn't true-random, unless bound is a power of 2. But it's close enough?
 * The problem is that rand() returns a number between [0, 2^64), and the
 * size of the range may not be even divisible by bound
 *
 * For what it's worth, this is what Arduino's random() does. It lies!
 *
 * @param bound An upper bound of the random range to return
 * @return A number in the range [0,bound), which does not include bound
 */
uint32_t danceRand(uint32_t bound)
{
    if (bound == 0)
    {
        return 0;
    }
    return rand() % bound;
}

//==============================================================================
// Animation functions
//==============================================================================

/**
 * Rotate a single white LED around the swadge
 *
 * @param tElapsedUs The time elapsed since last call, in microseconds
 * @param reset      true to reset this dance's variables
 */
void danceComet(uint32_t tElapsedUs, uint32_t arg, bool reset)
{
    static int32_t ledCount            = 0;
    static uint8_t rainbow             = 0;
    static int32_t msCount             = 0;
    static uint32_t tAccumulated       = 0;
    static led_t leds[CONFIG_NUM_LEDS] = {{0}};

    if (reset)
    {
        ledCount     = 0;
        rainbow      = 0;
        msCount      = 80;
        tAccumulated = 2000;
        memset(leds, 0, sizeof(leds));
        return;
    }

    bool ledsUpdated = false;

    tAccumulated += tElapsedUs;
    while (tAccumulated >= 2000)
    {
        tAccumulated -= 2000;
        for (uint8_t i = 0; i < CONFIG_NUM_LEDS; i++)
        {
            if (leds[i].r > 0)
            {
                leds[i].r--;
            }
            if (leds[i].g > 0)
            {
                leds[i].g--;
            }
            if (leds[i].b > 0)
            {
                leds[i].b--;
            }
        }
        msCount++;

        if (msCount % 10 == 0)
        {
            rainbow++;
        }

        if (msCount >= 80)
        {
            if (0 == arg)
            {
                int32_t color    = EHSVtoHEXhelper(rainbow, 0xFF, 0xFF, false);
                leds[ledCount].r = (color >> 0) & 0xFF;
                leds[ledCount].g = (color >> 8) & 0xFF;
                leds[ledCount].b = (color >> 16) & 0xFF;
            }
            else
            {
                leds[ledCount].r = ARG_R(arg);
                leds[ledCount].g = ARG_G(arg);
                leds[ledCount].b = ARG_B(arg);
            }
            ledCount = (ledCount + 1) % CONFIG_NUM_LEDS;
            msCount  = 0;
        }
        ledsUpdated = true;
    }

    if (ledsUpdated)
    {
        setLeds(leds, CONFIG_NUM_LEDS);
    }
}

/**
 * Blink all LEDs red for on for 500ms, then off for 500ms
 *
 * @param tElapsedUs The time elapsed since last call, in microseconds
 * @param reset      true to reset this dance's variables
 */
void dancePulse(uint32_t tElapsedUs, uint32_t arg, bool reset)
{
    static uint8_t ledVal        = 0;
    static uint8_t randColor     = 0;
    static bool goingUp          = true;
    static uint32_t tAccumulated = 0;

    if (reset)
    {
        ledVal       = 0;
        randColor    = 0;
        goingUp      = true;
        tAccumulated = 5000;
        return;
    }

    // Declare some LEDs, all off
    led_t leds[CONFIG_NUM_LEDS] = {{0}};
    bool ledsUpdated            = false;

    tAccumulated += tElapsedUs;
    while (tAccumulated >= 5000)
    {
        tAccumulated -= 5000;

        if (goingUp)
        {
            ledVal++;
            if (255 == ledVal)
            {
                goingUp = false;
            }
        }
        else
        {
            ledVal--;
            if (0 == ledVal)
            {
                goingUp   = true;
                randColor = danceRand(256);
            }
        }

        for (int i = 0; i < CONFIG_NUM_LEDS; i++)
        {
            if (0 == arg)
            {
                int32_t color = EHSVtoHEXhelper(randColor, 0xFF, 0xFF, false);
                leds[i].r     = (ledVal * ((color >> 0) & 0xFF) >> 8);
                leds[i].g     = (ledVal * ((color >> 8) & 0xFF) >> 8);
                leds[i].b     = (ledVal * ((color >> 16) & 0xFF) >> 8);
            }
            else
            {
                leds[i].r = (ledVal * ARG_R(arg)) >> 8;
                leds[i].g = (ledVal * ARG_G(arg)) >> 8;
                leds[i].b = (ledVal * ARG_B(arg)) >> 8;
            }
        }
        ledsUpdated = true;
    }

    if (ledsUpdated)
    {
        setLeds(leds, CONFIG_NUM_LEDS);
    }
}

/**
 * Rotate a single white LED around the swadge
 *
 * @param tElapsedUs The time elapsed since last call, in microseconds
 * @param reset      true to reset this dance's variables
 */
void danceRise(uint32_t tElapsedUs, uint32_t arg, bool reset)
{
    static int16_t levels[CONFIG_NUM_LEDS / 2] = {0, -256, -512};
    static bool rising[CONFIG_NUM_LEDS / 2]    = {true, true, true};
    static uint8_t angle                       = 0;
    static uint32_t tAccumulated               = 0;
    static uint8_t ledRemap[CONFIG_NUM_LEDS]   = {1, 0, 2, 3, 4, 5, 7, 6};

    if (reset)
    {
        for (uint8_t i = 0; i < CONFIG_NUM_LEDS / 2; i++)
        {
            levels[i] = i * -256;
            rising[i] = true;
        }
        angle        = 0;
        tAccumulated = 800;
        return;
    }

    bool ledsUpdated            = false;
    led_t leds[CONFIG_NUM_LEDS] = {{0}};

    tAccumulated += tElapsedUs;
    while (tAccumulated >= 800)
    {
        tAccumulated -= 800;

        if (true == rising[0] && 0 == levels[0])
        {
            angle = danceRand(256);
        }

        for (uint8_t i = 0; i < CONFIG_NUM_LEDS / 2; i++)
        {
            if (rising[i])
            {
                levels[i]++;
                if (levels[i] == 255)
                {
                    rising[i] = false;
                }
            }
            else
            {
                levels[i]--;
                if (levels[i] == -512)
                {
                    rising[i] = true;
                }
            }
        }

        if (0 == arg)
        {
            int32_t color = EHSVtoHEXhelper(angle, 0xFF, 0xFF, false);
            for (uint8_t i = 0; i < CONFIG_NUM_LEDS / 2; i++)
            {
                if (levels[i] > 0)
                {
                    leds[ledRemap[i]].r = (levels[i] * ((color >> 0) & 0xFF) >> 8);
                    leds[ledRemap[i]].g = (levels[i] * ((color >> 8) & 0xFF) >> 8);
                    leds[ledRemap[i]].b = (levels[i] * ((color >> 16) & 0xFF) >> 8);

                    leds[ledRemap[CONFIG_NUM_LEDS - 1 - i]].r = (levels[i] * ((color >> 0) & 0xFF) >> 8);
                    leds[ledRemap[CONFIG_NUM_LEDS - 1 - i]].g = (levels[i] * ((color >> 8) & 0xFF) >> 8);
                    leds[ledRemap[CONFIG_NUM_LEDS - 1 - i]].b = (levels[i] * ((color >> 16) & 0xFF) >> 8);
                }
            }
        }
        else
        {
            for (uint8_t i = 0; i < CONFIG_NUM_LEDS / 2; i++)
            {
                if (levels[i] > 0)
                {
                    leds[ledRemap[i]].r = (levels[i] * ARG_R(arg)) >> 8;
                    leds[ledRemap[i]].g = (levels[i] * ARG_G(arg)) >> 8;
                    leds[ledRemap[i]].b = (levels[i] * ARG_B(arg)) >> 8;

                    leds[ledRemap[CONFIG_NUM_LEDS - 1 - i]].r = (levels[i] * ARG_R(arg)) >> 8;
                    leds[ledRemap[CONFIG_NUM_LEDS - 1 - i]].g = (levels[i] * ARG_G(arg)) >> 8;
                    leds[ledRemap[CONFIG_NUM_LEDS - 1 - i]].b = (levels[i] * ARG_B(arg)) >> 8;
                }
            }
        }
        ledsUpdated = true;
    }

    if (ledsUpdated)
    {
        setLeds(leds, CONFIG_NUM_LEDS);
    }
}

/**
 * Smoothly rotate a color wheel around the swadge
 *
 * @param tElapsedUs The time elapsed since last call, in microseconds
 * @param reset      true to reset this dance's variables
 */
void danceSmoothRainbow(uint32_t tElapsedUs, uint32_t arg, bool reset)
{
    static uint32_t tAccumulated = 0;
    static uint8_t ledCount      = 0;

    if (reset)
    {
        ledCount     = 0;
        tAccumulated = arg;
        return;
    }

    // Declare some LEDs, all off
    led_t leds[CONFIG_NUM_LEDS] = {{0}};
    bool ledsUpdated            = false;

    tAccumulated += tElapsedUs;
    while (tAccumulated >= arg)
    {
        tAccumulated -= arg;
        ledsUpdated = true;

        ledCount--;

        uint8_t i;
        for (i = 0; i < CONFIG_NUM_LEDS; i++)
        {
            int16_t angle  = ((((i * 256) / CONFIG_NUM_LEDS)) + ledCount) % 256;
            uint32_t color = EHSVtoHEXhelper(angle, 0xFF, 0xFF, false);

            leds[i].r = (color >> 0) & 0xFF;
            leds[i].g = (color >> 8) & 0xFF;
            leds[i].b = (color >> 16) & 0xFF;
        }
    }
    // Output the LED data, actually turning them on
    if (ledsUpdated)
    {
        setLeds(leds, CONFIG_NUM_LEDS);
    }
}

/**
 * Sharply rotate a color wheel around the swadge
 *
 * @param tElapsedUs The time elapsed since last call, in microseconds
 * @param reset      true to reset this dance's variables
 */
void danceSharpRainbow(uint32_t tElapsedUs, uint32_t arg __attribute__((unused)), bool reset)
{
    static int32_t ledCount      = 0;
    static uint32_t tAccumulated = 0;

    if (reset)
    {
        ledCount     = 0;
        tAccumulated = 300000;
        return;
    }

    // Declare some LEDs, all off
    led_t leds[CONFIG_NUM_LEDS] = {{0}};
    bool ledsUpdated            = false;

    tAccumulated += tElapsedUs;
    while (tAccumulated >= 300000)
    {
        tAccumulated -= 300000;
        ledsUpdated = true;

        ledCount = ledCount + 1;
        if (ledCount > CONFIG_NUM_LEDS - 1)
        {
            ledCount = 0;
        }

        uint8_t i;
        for (i = 0; i < CONFIG_NUM_LEDS; i++)
        {
            int16_t angle  = (((i * 256) / CONFIG_NUM_LEDS)) % 256;
            uint32_t color = EHSVtoHEXhelper(angle, 0xFF, 0xFF, false);

            leds[(i + ledCount) % CONFIG_NUM_LEDS].r = (color >> 0) & 0xFF;
            leds[(i + ledCount) % CONFIG_NUM_LEDS].g = (color >> 8) & 0xFF;
            leds[(i + ledCount) % CONFIG_NUM_LEDS].b = (color >> 16) & 0xFF;
        }
    }
    // Output the LED data, actually turning them on
    if (ledsUpdated)
    {
        setLeds(leds, CONFIG_NUM_LEDS);
    }
}

/**
 * Counts up to 256 in binary. At 256, the color is held for ~3s
 * The 'on' color is smoothly iterated over the color wheel. The 'off'
 * color is also iterated over the color wheel, 180 degrees offset from 'on'
 * @param tElapsedUs The time elapsed since last call, in microseconds
 * @param reset      true to reset this dance's variables
 */
void danceBinaryCounter(uint32_t tElapsedUs, uint32_t arg __attribute__((unused)), bool reset)
{
    static int32_t ledCount      = 0;
    static int32_t ledCount2     = 0;
    static bool led_bool         = false;
    static uint32_t tAccumulated = 0;

    if (reset)
    {
        ledCount     = 0;
        ledCount2    = 0;
        led_bool     = false;
        tAccumulated = 300000;
        return;
    }

    // Declare some LEDs, all off
    led_t leds[CONFIG_NUM_LEDS] = {{0}};
    bool ledsUpdated            = false;

    tAccumulated += tElapsedUs;
    while (tAccumulated >= 300000)
    {
        tAccumulated -= 300000;
        ledsUpdated = true;

        ledCount  = ledCount + 1;
        ledCount2 = ledCount2 + 1;
        if (ledCount2 > 75)
        {
            led_bool  = !led_bool;
            ledCount2 = 0;
        }
        if (ledCount > 255)
        {
            ledCount = 0;
        }
        int16_t angle     = ledCount % 256;
        uint32_t colorOn  = EHSVtoHEXhelper(angle, 0xFF, 0xFF, false);
        uint32_t colorOff = EHSVtoHEXhelper((angle + 128) % 256, 0xFF, 0xFF, false);

        uint8_t i;
        uint8_t j;
        for (i = 0; i < CONFIG_NUM_LEDS; i++)
        {
            if (ledCount2 >= (1 << CONFIG_NUM_LEDS))
            {
                leds[i].r = (colorOn >> 0) & 0xFF;
                leds[i].g = (colorOn >> 8) & 0xFF;
                leds[i].b = (colorOn >> 16) & 0xFF;
            }
            else
            {
                if (led_bool)
                {
                    j = CONFIG_NUM_LEDS - 1 - i;
                }
                else
                {
                    j = i;
                }

                if ((ledCount2 >> i) & 1)
                {
                    leds[(j) % CONFIG_NUM_LEDS].r = (colorOn >> 0) & 0xFF;
                    leds[(j) % CONFIG_NUM_LEDS].g = (colorOn >> 8) & 0xFF;
                    leds[(j) % CONFIG_NUM_LEDS].b = (colorOn >> 16) & 0xFF;
                }
                else
                {
                    leds[(j) % CONFIG_NUM_LEDS].r = (colorOff >> 0) & 0xFF;
                    leds[(j) % CONFIG_NUM_LEDS].g = (colorOff >> 8) & 0xFF;
                    leds[(j) % CONFIG_NUM_LEDS].b = (colorOff >> 16) & 0xFF;
                }
            }
        }
    }
    // Output the LED data, actually turning them on
    if (ledsUpdated)
    {
        setLeds(leds, CONFIG_NUM_LEDS);
    }
}

/**
 * Fire pattern. All LEDs are random amount of red, and fifth that of green.
 * The LEDs towards the bottom have a brighter base and more randomness. The
 * LEDs towards the top are dimmer and have less randomness.
 *
 * @param tElapsedUs The time elapsed since last call, in microseconds
 * @param reset      true to reset this dance's variables
 */
void danceFire(uint32_t tElapsedUs, uint32_t arg, bool reset)
{
    static uint32_t tAccumulated = 0;

    if (reset)
    {
        tAccumulated = 100000;
        return;
    }

    // Declare some LEDs, all off
    led_t leds[CONFIG_NUM_LEDS] = {{0}};
    bool ledsUpdated            = false;

    tAccumulated += tElapsedUs;
    while (tAccumulated >= 75000)
    {
        tAccumulated -= 75000;
        ledsUpdated = true;

        uint8_t randC;

        // Base
        randC     = danceRand(105) + 150;
        leds[1].r = (randC * ARG_R(arg)) / 256;
        leds[1].g = (randC * ARG_G(arg)) / 256;
        leds[1].b = (randC * ARG_B(arg)) / 256;
        randC     = danceRand(105) + 150;
        leds[6].r = (randC * ARG_R(arg)) / 256;
        leds[6].g = (randC * ARG_G(arg)) / 256;
        leds[6].b = (randC * ARG_B(arg)) / 256;

        // Mid-low
        randC     = danceRand(48) + 32;
        leds[0].r = (randC * ARG_R(arg)) / 256;
        leds[0].g = (randC * ARG_G(arg)) / 256;
        leds[0].b = (randC * ARG_B(arg)) / 256;
        randC     = danceRand(48) + 32;
        leds[7].r = (randC * ARG_R(arg)) / 256;
        leds[7].g = (randC * ARG_G(arg)) / 256;
        leds[7].b = (randC * ARG_B(arg)) / 256;

        // Mid-high
        randC     = danceRand(32) + 16;
        leds[2].r = (randC * ARG_R(arg)) / 256;
        leds[2].g = (randC * ARG_G(arg)) / 256;
        leds[2].b = (randC * ARG_B(arg)) / 256;
        randC     = danceRand(32) + 16;
        leds[5].r = (randC * ARG_R(arg)) / 256;
        leds[5].g = (randC * ARG_G(arg)) / 256;
        leds[5].b = (randC * ARG_B(arg)) / 256;

        // Tip
        randC     = danceRand(16) + 4;
        leds[3].r = (randC * ARG_R(arg)) / 256;
        leds[3].g = (randC * ARG_G(arg)) / 256;
        leds[3].b = (randC * ARG_B(arg)) / 256;
        randC     = danceRand(16) + 4;
        leds[4].r = (randC * ARG_R(arg)) / 256;
        leds[4].g = (randC * ARG_G(arg)) / 256;
        leds[4].b = (randC * ARG_B(arg)) / 256;
    }
    if (ledsUpdated)
    {
        setLeds(leds, CONFIG_NUM_LEDS);
    }
}

/**
 * police sirens, flash half red then half blue
 *
 * @param tElapsedUs The time elapsed since last call, in microseconds
 * @param reset      true to reset this dance's variables
 */
void dancePoliceSiren(uint32_t tElapsedUs, uint32_t arg __attribute__((unused)), bool reset)
{
    static int32_t ledCount;
    static uint32_t tAccumulated = 0;

    if (reset)
    {
        ledCount     = 0;
        tAccumulated = 120000;
        return;
    }

    // Declare some LEDs, all off
    led_t leds[CONFIG_NUM_LEDS] = {{0}};
    bool ledsUpdated            = false;

    tAccumulated += tElapsedUs;
    while (tAccumulated >= 120000)
    {
        tAccumulated -= 120000;
        ledsUpdated = true;

        // Skip to the next LED around the swadge
        ledCount = ledCount + 1;
        if (ledCount > CONFIG_NUM_LEDS)
        {
            ledCount = 0;
        }

        uint8_t i;
        if (ledCount < (CONFIG_NUM_LEDS >> 1))
        {
            // red
            for (i = 0; i < (CONFIG_NUM_LEDS >> 1); i++)
            {
                leds[i].r = 0xFF;
                leds[i].g = 0x00;
                leds[i].b = 0x00;
            }
        }
        else
        {
            // blue
            for (i = (CONFIG_NUM_LEDS >> 1); i < CONFIG_NUM_LEDS; i++)
            {
                leds[i].r = 0x00;
                leds[i].g = 0x00;
                leds[i].b = 0xFF;
            }
        }
    }
    // Output the LED data, actually turning them on
    if (ledsUpdated)
    {
        setLeds(leds, CONFIG_NUM_LEDS);
    }
}

/**
 * Turn a random LED on to a random color, one at a time
 *
 * @param tElapsedUs The time elapsed since last call, in microseconds
 * @param reset      true to reset this dance's variables
 */
void dancePureRandom(uint32_t tElapsedUs, uint32_t arg __attribute__((unused)), bool reset)
{
    static uint32_t tAccumulated = 0;
    static uint8_t randLedMask   = 0;
    static uint32_t randColor    = 0;
    static uint8_t ledVal        = 0;
    static bool ledRising        = true;
    static uint32_t randInterval = 5000;

    if (reset)
    {
        randInterval = 5000;
        tAccumulated = randInterval;
        randLedMask  = 0;
        randColor    = 0;
        ledVal       = 0;
        ledRising    = true;
        return;
    }

    // Declare some LEDs, all off
    led_t leds[CONFIG_NUM_LEDS] = {{0}};
    bool ledsUpdated            = false;

    tAccumulated += tElapsedUs;
    while (tAccumulated >= randInterval)
    {
        tAccumulated -= randInterval;

        if (0 == ledVal)
        {
            randColor    = danceRand(256);
            randLedMask  = danceRand(1 << CONFIG_NUM_LEDS);
            randInterval = 500 + danceRand(4096);
            ledVal++;
        }
        else if (ledRising)
        {
            ledVal++;
            if (255 == ledVal)
            {
                ledRising = false;
            }
        }
        else
        {
            ledVal--;
            if (0 == ledVal)
            {
                ledRising = true;
            }
        }

        ledsUpdated    = true;
        uint32_t color = EHSVtoHEXhelper(randColor, 0xFF, ledVal, false);
        for (uint8_t i = 0; i < CONFIG_NUM_LEDS; i++)
        {
            if ((1 << i) & randLedMask)
            {
                leds[i].r = (color >> 0) & 0xFF;
                leds[i].g = (color >> 8) & 0xFF;
                leds[i].b = (color >> 16) & 0xFF;
            }
        }
    }
    // Output the LED data, actually turning them on
    if (ledsUpdated)
    {
        setLeds(leds, CONFIG_NUM_LEDS);
    }
}

/**
 * Turn on all LEDs and smooth iterate their singular color around the color wheel
 *
 * @param tElapsedUs The time elapsed since last call, in microseconds
 * @param reset      true to reset this dance's variables
 */
void danceRainbowSolid(uint32_t tElapsedUs, uint32_t arg __attribute__((unused)), bool reset)
{
    static int32_t ledCount      = 0;
    static int32_t color_save    = 0;
    static uint32_t tAccumulated = 0;

    if (reset)
    {
        ledCount     = 0;
        color_save   = 0;
        tAccumulated = 70000;
        return;
    }

    // Declare some LEDs, all off
    led_t leds[CONFIG_NUM_LEDS] = {{0}};
    bool ledsUpdated            = false;

    tAccumulated += tElapsedUs;
    while (tAccumulated >= 70000)
    {
        tAccumulated -= 70000;
        ledsUpdated = true;

        ledCount = ledCount + 1;
        if (ledCount > 255)
        {
            ledCount = 0;
        }
        int16_t angle = ledCount % 256;
        color_save    = EHSVtoHEXhelper(angle, 0xFF, 0xFF, false);

        uint8_t i;
        for (i = 0; i < CONFIG_NUM_LEDS; i++)
        {
            leds[i].r = (color_save >> 0) & 0xFF;
            leds[i].g = (color_save >> 8) & 0xFF;
            leds[i].b = (color_save >> 16) & 0xFF;
        }
    }
    // Output the LED data, actually turning them on
    if (ledsUpdated)
    {
        setLeds(leds, CONFIG_NUM_LEDS);
    }
}

/**
 * Turn on all LEDs and Make Purely White
 *
 * @param tElapsedUs The time elapsed since last call, in microseconds
 * @param reset      true to reset this dance's variables
 */
void danceFlashlight(uint32_t tElapsedUs, uint32_t arg __attribute__((unused)), bool reset)
{
    static uint32_t tAccumulated = 0;

    if (reset)
    {
        tAccumulated = 70000;
        return;
    }

    // Declare some LEDs, all off
    led_t leds[CONFIG_NUM_LEDS] = {{0}};
    bool ledsUpdated            = false;

    tAccumulated += tElapsedUs;
    while (tAccumulated >= 70000)
    {
        tAccumulated -= 70000;
        ledsUpdated = true;

        uint8_t i;
        for (i = 0; i < CONFIG_NUM_LEDS; i++)
        {
            leds[i].r = 0xFF;
            leds[i].g = 0xFF;
            leds[i].b = 0xFF;
        }
    }
    // Output the LED data, actually turning them on
    if (ledsUpdated)
    {
        setLeds(leds, CONFIG_NUM_LEDS);
    }
}

/**
 * Pick a random dance mode and call it at its period for 4.5s. Then pick
 * another random dance and repeat
 *
 * @param tElapsedUs The time elapsed since last call, in microseconds
 * @param reset      true to reset this dance's variables
 */
void danceRandomDance(uint32_t tElapsedUs, uint32_t arg __attribute__((unused)), bool reset)
{
    static int32_t random_choice = -1;
    static uint32_t tAccumulated = 0;

    if (reset)
    {
        random_choice = -1;
        tAccumulated  = 4500000;
        return;
    }

    if (-1 == random_choice)
    {
        random_choice = danceRand(getNumDances() - 3); // exclude the random mode, excluding random & none
    }

    tAccumulated += tElapsedUs;
    while (tAccumulated >= 4500000)
    {
        tAccumulated -= 4500000;
        random_choice = danceRand(getNumDances() - 3); // exclude the random & none mode
        ledDances[random_choice].func(0, ledDances[random_choice].arg, true);
    }

    ledDances[random_choice].func(tElapsedUs, ledDances[random_choice].arg, false);
}

/**
 * Holiday lights. Picks random target hues (red or green) or (blue or yellow) and saturations for
 * random LEDs at random intervals, then smoothly iterates towards those targets.
 * All LEDs are shown with a randomness added to their brightness for a little
 * sparkle
 *
 * @param tElapsedUs The time elapsed since last call, in microseconds
 * @param arg        unused
 * @param reset      true to reset this dance's variables
 */
void danceChristmas(uint32_t tElapsedUs, uint32_t arg, bool reset)
{
    static int32_t ledCount                                  = 0;
    static int32_t ledCount2                                 = 0;
    static uint8_t color_hue_save[CONFIG_NUM_LEDS]           = {0};
    static uint8_t color_saturation_save[CONFIG_NUM_LEDS]    = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    static uint8_t current_color_hue[CONFIG_NUM_LEDS]        = {0};
    static uint8_t current_color_saturation[CONFIG_NUM_LEDS] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    static uint8_t target_value[CONFIG_NUM_LEDS]             = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    static uint8_t current_value[CONFIG_NUM_LEDS]            = {0};

    static uint32_t tAccumulated      = 0;
    static uint32_t tAccumulatedValue = 0;

    if (reset)
    {
        ledCount  = 0;
        ledCount2 = 0;
        memset(color_saturation_save, 0xFF, sizeof(color_saturation_save));
        memset(current_color_saturation, 0xFF, sizeof(current_color_saturation));
        memset(target_value, 0xFF, sizeof(target_value));
        memset(current_value, 0x00, sizeof(current_value));
        if (arg)
        {
            memset(color_hue_save, 0, sizeof(color_hue_save));
            memset(current_color_hue, 0, sizeof(current_color_hue)); // All red
        }
        else
        {
            memset(color_hue_save, 171, sizeof(color_hue_save));
            memset(current_color_hue, 171, sizeof(current_color_hue)); // All blue
        }
        tAccumulated      = 0;
        tAccumulatedValue = 0;
        return;
    }

    // Declare some LEDs, all off
    led_t leds[CONFIG_NUM_LEDS] = {{0}};
    bool ledsUpdated            = false;

    // Run a faster loop for LED brightness updates, this gives a twinkling effect
    tAccumulatedValue += tElapsedUs;
    while (tAccumulatedValue > 3500)
    {
        tAccumulatedValue -= 3500;

        uint8_t i;
        for (i = 0; i < CONFIG_NUM_LEDS; i++)
        {
            if (current_value[i] == target_value[i])
            {
                if (0xFF == target_value[i])
                {
                    // Reached full bright, pick new target value
                    target_value[i] = danceRand(64) + 192;
                }
                else
                {
                    // Reached target value, reset target to full bright
                    target_value[i] = 0xFF;
                }
            }
            // Smoothly move to the target value
            else if (current_value[i] > target_value[i])
            {
                current_value[i] -= 1;
            }
            else // if (current_value[i] < target_value[i])
            {
                current_value[i] += 1;
            }
        }
    }

    // Run a slower loop for hue and saturation updates
    tAccumulated += tElapsedUs;
    while (tAccumulated > 7000)
    {
        tAccumulated -= 7000;

        ledCount += 1;
        if (ledCount > ledCount2)
        {
            ledCount         = 0;
            ledCount2        = danceRand(1000) + 50; // 350ms to 7350ms
            int color_picker = danceRand(CONFIG_NUM_LEDS - 1);
            int node_select  = danceRand(CONFIG_NUM_LEDS);

            if (color_picker < 4)
            {
                // Flip some color targets
                if (arg)
                {
                    if (color_hue_save[node_select] == 0) // red
                    {
                        color_hue_save[node_select] = 86; // green
                    }
                    else
                    {
                        color_hue_save[node_select] = 0; // red
                    }
                }
                else
                {
                    if (color_hue_save[node_select] == 171) // blue
                    {
                        color_hue_save[node_select] = 43; // yellow
                    }
                    else
                    {
                        color_hue_save[node_select] = 171; // blue
                    }
                }
                // Pick a random saturation target
                color_saturation_save[node_select] = danceRand(15) + 240;
            }
            else
            {
                // White-ish target
                color_saturation_save[node_select] = danceRand(25);
            }
        }

        uint8_t i;
        for (i = 0; i < CONFIG_NUM_LEDS; i++)
        {
            // Smoothly move hue to the target
            if (current_color_hue[i] > color_hue_save[i])
            {
                current_color_hue[i] -= 1;
            }
            else if (current_color_hue[i] < color_hue_save[i])
            {
                current_color_hue[i] += 1;
            }

            // Smoothly move saturation to the target
            if (current_color_saturation[i] > color_saturation_save[i])
            {
                current_color_saturation[i] -= 1;
            }
            else if (current_color_saturation[i] < color_saturation_save[i])
            {
                current_color_saturation[i] += 1;
            }
        }

        // Calculate actual LED values
        for (i = 0; i < CONFIG_NUM_LEDS; i++)
        {
            leds[i].r
                = (EHSVtoHEXhelper(current_color_hue[i], current_color_saturation[i], current_value[i], false) >> 0)
                  & 0xFF;
            leds[i].g
                = (EHSVtoHEXhelper(current_color_hue[i], current_color_saturation[i], current_value[i], false) >> 8)
                  & 0xFF;
            leds[i].b
                = (EHSVtoHEXhelper(current_color_hue[i], current_color_saturation[i], current_value[i], false) >> 16)
                  & 0xFF;
        }
        ledsUpdated = true;
    }
    // Output the LED data, actually turning them on
    if (ledsUpdated)
    {
        setLeds(leds, CONFIG_NUM_LEDS);
    }
}

/**
 * @brief Blank the LEDs
 *
 * @param tElapsedUs
 * @param arg
 * @param reset
 */
void danceNone(uint32_t tElapsedUs __attribute__((unused)), uint32_t arg __attribute__((unused)), bool reset)
{
    if (reset)
    {
        led_t leds[CONFIG_NUM_LEDS] = {{0}};
        setLeds(leds, CONFIG_NUM_LEDS);
    }
}
