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
#include "swadgePass.h"

#define DANCE_IMPLEMENTATION
#include "dance_Comet.h"
// #include "dance_Condiment.h"
#include "dance_SharpRainbow.h"
#include "dance_SmoothRainbow.h"
#include "dance_RainbowSolid.h"
#include "dance_Sweep.h"
#include "dance_Rise.h"
#include "dance_Pulse.h"
#include "dance_Fire.h"
#include "dance_BinaryCounter.h"
#include "dance_PoliceSiren.h"
#include "dance_PureRandom.h"
#include "dance_Christmas.h"
#include "dance_Flashlight.h"
#include "dance_None.h"
#include "dance_RandomDance.h"
#include "portableDance.h"

//==============================================================================
// Defines
//==============================================================================

// Sleep the TFT after 5s
#define TFT_TIMEOUT_US 5000000

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    portableDance_t* dance;

    bool blankScreen;

    uint32_t buttonPressedTimer;

    menu_t* menu;
    menuMegaRenderer_t* menuRenderer;

    const char** danceNames;
    int32_t* danceVals;

    int32_t bcastTimer;
    int32_t listenTimer;
    bool maySleep;

    swadgePassPacket_t packet;
    int32_t swadgePassCount;
} danceMode_t;

//==============================================================================
// Prototypes
//==============================================================================

static void danceEnterMode(void);
static void danceExitMode(void);
static void danceMainLoop(int64_t elapsedUs);
static bool danceMenuCb(const char* label, bool selected, uint32_t value);

static void danceEspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi);
static void danceEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status);

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
static const char nvsNs[] = "light_dances";

const ledDance_t ledDances[] = {
    {.func = danceComet, .arg = RGB_2_ARG(0, 0, 0), .name = "Comet RGB"},
    {.func = danceComet, .arg = RGB_2_ARG(0xFF, 0, 0), .name = "Comet R"},
    {.func = danceComet, .arg = RGB_2_ARG(0, 0xFF, 0), .name = "Comet G"},
    {.func = danceComet, .arg = RGB_2_ARG(0, 0, 0xFF), .name = "Comet B"},
    // {.func = danceCondiment, .arg = RGB_2_ARG(0xFF, 0x00, 0x00), .name = "Ketchup"},
    // {.func = danceCondiment, .arg = RGB_2_ARG(0xFF, 0xFF, 0x00), .name = "Mustard"},
    // {.func = danceCondiment, .arg = RGB_2_ARG(0x00, 0xFF, 0x00), .name = "Relish"},
    // {.func = danceCondiment, .arg = RGB_2_ARG(0xFF, 0xFF, 0xFF), .name = "Mayo"},
    {.func = danceSharpRainbow, .arg = 0, .name = "Rainbow Sharp"},
    {.func = danceSmoothRainbow, .arg = 20000, .name = "Rainbow Slow"},
    {.func = danceSmoothRainbow, .arg = 4000, .name = "Rainbow Fast"},
    {.func = danceRainbowSolid, .arg = 0, .name = "Rainbow Solid"},
    {.func = danceSweep, .arg = RGB_2_ARG(0, 0, 0), .name = "Sweep RGB"},
    {.func = danceSweep, .arg = RGB_2_ARG(0xFF, 0, 0), .name = "Sweep R"},
    {.func = danceSweep, .arg = RGB_2_ARG(0, 0xFF, 0), .name = "Sweep G"},
    {.func = danceSweep, .arg = RGB_2_ARG(0, 0, 0xFF), .name = "Sweep B"},
    {.func = danceRise, .arg = RGB_2_ARG(0, 0, 0), .name = "Rise RGB"},
    {.func = danceRise, .arg = RGB_2_ARG(0xFF, 0, 0), .name = "Rise R"},
    {.func = danceRise, .arg = RGB_2_ARG(0, 0xFF, 0), .name = "Rise G"},
    {.func = danceRise, .arg = RGB_2_ARG(0, 0, 0xFF), .name = "Rise B"},
    {.func = dancePulse, .arg = RGB_2_ARG(0, 0, 0), .name = "Pulse RGB"},
    {.func = dancePulse, .arg = RGB_2_ARG(0xFF, 0, 0), .name = "Pulse R"},
    {.func = dancePulse, .arg = RGB_2_ARG(0, 0xFF, 0), .name = "Pulse G"},
    {.func = dancePulse, .arg = RGB_2_ARG(0, 0, 0xFF), .name = "Pulse B"},
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
    .wifiMode        = ESP_NOW,
    .fnEspNowRecvCb  = danceEspNowRecvCb,
    .fnEspNowSendCb  = danceEspNowSendCb,
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
    // No speaker output for LEDs!
    setDacShutdown(true);

    danceState = heap_caps_calloc(1, sizeof(danceMode_t), MALLOC_CAP_8BIT);

    // Initialize SwadgePass
    initSwadgePassReceiver();
    fillSwadgePassPacket(&danceState->packet);
    danceState->swadgePassCount = 0;

    danceState->dance = initPortableDance(nvsNs);

    danceState->blankScreen = false;

    danceState->buttonPressedTimer = 0;

    danceState->maySleep = true;
    espNowPreLightSleep();

    danceState->menu         = initMenu(danceName, danceMenuCb);
    danceState->menuRenderer = initMenuMegaRenderer(NULL, NULL, NULL);
    setMegaLedsOn(danceState->menuRenderer, false);
    // static const paletteColor_t shadowColors[] = {
    //     c430, c431, c442, c543, c554, c555, c554, c543, c442, c431,
    // };
    // led_t offLed = {0};
    // recolorMenuManiaRenderer(danceState->menuRenderer, // Pango palette!
    //                          c320, c542, c111,         // titleBgColor, titleTextColor, textOutlineColor
    //                          c045,                     // bgColor
    //                          c542, c541,               // outerRingColor, innerRingColor
    //                          c111, c455,               // rowColor, rowTextColor
    //                          shadowColors, ARRAY_SIZE(shadowColors), offLed);

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
                                 ARRAY_SIZE(ledDances), &danceParam, danceState->dance->danceIndex);

    // Add brightness to the menu
    addSettingsItemToMenu(danceState->menu, str_brightness, getLedBrightnessSettingBounds(), getLedBrightnessSetting());

    // Add speed to the menu
    const settingParam_t speedParam = {
        .min = speedVals[0],
        .max = speedVals[ARRAY_SIZE(speedVals) - 1],
    };
    addSettingsOptionsItemToMenu(danceState->menu, str_speed, speedLabels, speedVals, ARRAY_SIZE(speedVals),
                                 &speedParam, danceState->dance->speed);

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
    deinitMenuMegaRenderer(danceState->menuRenderer);
    deinitMenu(danceState->menu);

    deinitSwadgePassReceiver();

    freePortableDance(danceState->dance);

    heap_caps_free(danceState->danceNames);
    heap_caps_free(danceState->danceVals);
    heap_caps_free(danceState);
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
    portableDanceMainLoop(danceState->dance, elapsedUs);

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
            drawMenuMega(danceState->menu, danceState->menuRenderer, elapsedUs);

            // Set the LED eyes. First do a read to nudge it out of deep sleep
            uint32_t var;
            ch32v003ReadMemory((uint8_t*)&var, sizeof(var), 0x08000000);
            // Then reprogram it
            ch32v003RunBinaryAsset(MATRIX_BLINKS_CFUN_BIN);
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

            // Sleep the ch32
            ch32v003RunBinaryAsset(DEEP_SLEEP_CFUN_BIN);
        }
        else
        {
            // Screen is not blank, draw to it
            drawMenuMega(danceState->menu, danceState->menuRenderer, elapsedUs);

            // Uncomment to draw a count of SwadgePasses received this session
            // font_t* f = danceState->menuRenderer->menuFont;
            // char str[16];
            // sprintf(str, "SP: %" PRId32, danceState->swadgePassCount);
            // drawText(f, c000, str, (TFT_WIDTH - textWidth(f, str)) / 2, TFT_HEIGHT - f->height);
        }
    }

    // Only sleep with a blank screen, otherwise the screen flickers
    if (danceState->blankScreen && danceState->maySleep)
    {
        // Wait for any LED transactions to finish, otherwise RMT will do weird things during CPU sleep
        flushLeds();
        /* Light sleep for 100ms
         * The longer the sleep, the choppier the LED animations, but more power saving.
         */
        esp_sleep_enable_timer_wakeup(1000000 / 10);
        esp_light_sleep_start();
    }

    // Count down the RX listening timer if it's active
    if (danceState->listenTimer)
    {
        danceState->listenTimer -= elapsedUs;
        // Check if it elapsed
        if (0 >= danceState->listenTimer)
        {
            danceState->listenTimer = 0;
            // Allow the mode to sleep again
            danceState->maySleep = true;
            // Turn off wifi before sleeping
            espNowPreLightSleep();
        }
    }

    danceState->bcastTimer -= elapsedUs;
    if (danceState->bcastTimer <= 0)
    {
        // Broadcast every 7.5s, +/- 1.875s
        danceState->bcastTimer = (5625000 + (esp_random() % 3750000));
        // Turn on WiFi before transmitting
        espNowPostLightSleep();
        // Transmit a packet
        sendSwadgePass(&danceState->packet);
        // Stay awake after packet transmission
        danceState->maySleep = false;
    }
}

/**
 * @brief Callback for the menu options
 *
 * @param label The menu option that was selected or changed
 * @param selected True if the option was selected, false if it was only changed
 * @param value The setting value for this operation
 * @return true to go up a menu level, false to remain here
 */
bool danceMenuCb(const char* label, bool selected, uint32_t value)
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
        portableDanceSetSpeed(danceState->dance, (int32_t)value);
    }
    else if (NULL == label) // Dance names are label-less
    {
        if (danceState->dance->danceIndex != value)
        {
            portableDanceSetByIndex(danceState->dance, value);
        }
    }
    return false;
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
// ESP-NOW Functions
//==============================================================================

/**
 * @brief Receive an ESP-NOW packet. If the packet is a Swadgepass, it will be saved to NVM
 *
 * @param esp_now_info Information about the transmission, including The MAC addresses
 * @param data The received packet
 * @param len The length of the received packet
 * @param rssi The signal strength of the received packet
 */
static void danceEspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi)
{
    receiveSwadgePass(esp_now_info, data, len, rssi);
    danceState->swadgePassCount++;
}

/**
 * @brief Callback when sending an ESP-NOW packet. This will keep the Swadge awake after transmission.
 *
 * @param mac_addr The MAC address which the data was sent to
 * @param status   The status of the transmission
 */
static void danceEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status)
{
    ESP_LOGI("ESPNOW", "danceEspNowSendCb %s (%d)",
             ESP_NOW_SEND_SUCCESS == status ? "ESP_NOW_SEND_SUCCESS" : "ESP_NOW_SEND_FAIL", status);

    if (ESP_NOW_SEND_SUCCESS == status)
    {
        // Stay awake for 700ms after transmitting
        danceState->listenTimer = 700000;
    }
}
