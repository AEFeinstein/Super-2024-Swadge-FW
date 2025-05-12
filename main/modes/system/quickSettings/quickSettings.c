/**
 * @file quickSettings.c
 * @author dylwhich (dylan@whichard.com)
 * @brief A Quick Settings menu mode to be drawn on top of other apps
 * @date 2023-08-14
 *
 */

//==============================================================================
// Includes
//==============================================================================

#include <esp_heap_caps.h>

#include "quickSettings.h"
#include "menu.h"
#include "menu_utils.h"
#include "menuQuickSettingsRenderer.h"
#include "macros.h"
#include "soundFuncs.h"

#include "settingsManager.h"

#include <stdint.h>
#include <stdbool.h>

//==============================================================================
// Defines
//==============================================================================

//==============================================================================
// Enums
//==============================================================================

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    menu_t* menu;                          ///< The menu structure
    menuQuickSettingsRenderer_t* renderer; ///< Renderer for the menu
    font_t font;                           ///< The font used for menu text
    int64_t ledTimer;
    bool showLeds;
#ifdef SW_VOL_CONTROL
    midiFile_t jingle;
#endif
    void* buzzerState;
    led_t ledState[CONFIG_NUM_LEDS];

    wsg_t iconGeneric;
    wsg_t iconSfxOn;
    wsg_t iconSfxOff;
    wsg_t iconBgmOn;
    wsg_t iconBgmOff;
    wsg_t iconLedsOn;
    wsg_t iconLedsOff;
    wsg_t iconTftOn;
    wsg_t iconTftOff;

#ifdef SW_VOL_CONTROL
    int32_t lastOnSfxValue;
    int32_t minSfxValue;
    int32_t prevSfxValue;

    int32_t lastOnBgmValue;
    int32_t minBgmValue;
    int32_t prevBgmValue;
#endif

    int32_t lastOnLedsValue;
    int32_t minLedsValue;

    int32_t lastOnTftValue;
    int32_t minTftValue;

    paletteColor_t* frozenScreen;
} quickSettingsMenu_t;

//==============================================================================
// Function Prototypes
//==============================================================================

static int32_t setupQuickSettingParams(const settingParam_t* bounds, int32_t currentValue, int32_t* onValue,
                                       int32_t* minValue);
static void quickSettingsMainLoop(int64_t elapsedUs);
static void quickSettingsEnterMode(void);
static void quickSettingsExitMode(void);
static void quickSettingsMenuCb(const char* label, bool selected, uint32_t settingVal);
static int32_t quickSettingsFlipValue(const char* label, int32_t value);
static void quickSettingsOnChange(const char* label, int32_t value);
static int32_t quickSettingsMenuFlipItem(const char* label);
static void quickSettingsSetLeds(int64_t elapsedUs);

//==============================================================================
// Strings
//==============================================================================

static const char quickSettingsName[] = "Settings";

static const char quickSettingsLeds[] = "LED Brightness";
#ifdef SW_VOL_CONTROL
static const char quickSettingsSfx[] = "SFX Volume";
static const char quickSettingsBgm[] = "Music Volume";
#endif
const char quickSettingsBacklight[] = "Screen Brightness: ";

static const char quickSettingsLedsOff[] = "LEDs Off";
static const char quickSettingsLedsMax[] = "LED Brightness: Max";
#ifdef SW_VOL_CONTROL
static const char quickSettingsSfxMuted[] = "SFX Muted";
static const char quickSettingsSfxMax[]   = "SFX Volume: Max";
static const char quickSettingsBgmMuted[] = "Music Muted";
static const char quickSettingsBgmMax[]   = "Music Volume: Max";
#endif
const char quickSettingsBacklightOff[]        = "Screen Backlight Off";
static const char quickSettingsBacklightMax[] = "Screen Brightness: Max";

//==============================================================================
// Variables
//==============================================================================

/// The Swadge mode for Quick Settings
swadgeMode_t quickSettingsMode = {
    .modeName                 = quickSettingsName,
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = quickSettingsEnterMode,
    .fnExitMode               = quickSettingsExitMode,
    .fnMainLoop               = quickSettingsMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = NULL,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
};

/// All state information for the Quick Settings mode. This whole struct is heap_caps_calloc()'d and heap_caps_free()'d
/// so that Quick Settings is only using memory while it is active
quickSettingsMenu_t* quickSettings = NULL;

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Helper function to easily setup the quick settings options values.
 *
 * @param bounds The setting bounds to use, with min for the "off" value
 * @param currentValue The current setting value, to be used as the "on" value if larger than the min
 * @param[out] onValue Pointer to write the "on" value for this setting
 * @param[out] minValue Pointer to write the minimum value for this setting
 * @return int32_t Returns the current setting value for convenience
 */
static int32_t setupQuickSettingParams(const settingParam_t* bounds, int32_t currentValue, int32_t* onValue,
                                       int32_t* minValue)
{
    *minValue = bounds->min;
    if (currentValue <= bounds->min)
    {
        // If the setting is already off / at the minimum, we need it to be something, so use the max
        *onValue = bounds->max;
    }
    else
    {
        // If the setting is any other value, use it for last "on" value
        *onValue = currentValue;
    }

    return currentValue;
}

/**
 * @brief Enter Quick Settings mode, allocate required memory, and initialize required variables
 *
 */
static void quickSettingsEnterMode(void)
{
    // Allocate and clear all memory for this mode. All the variables are contained in a single struct for convenience.
    // heap_caps_calloc() is used instead of heap_caps_malloc() because heap_caps_calloc() also initializes the
    // allocated memory to zeros.
    quickSettings = heap_caps_calloc(1, sizeof(quickSettingsMenu_t), MALLOC_CAP_8BIT);

    // Allocate background image and copy framebuffer into it
    quickSettings->frozenScreen = heap_caps_calloc(TFT_WIDTH * TFT_HEIGHT, sizeof(paletteColor_t), MALLOC_CAP_SPIRAM);
    memcpy(quickSettings->frozenScreen, getPxTftFramebuffer(), sizeof(paletteColor_t) * TFT_HEIGHT * TFT_WIDTH);

    // Save the buzzer state and pause it
    quickSettings->buzzerState = soundSave();

    // Pause the sound
    soundPause();

    // Save the LED state
    memcpy(quickSettings->ledState, getLedState(), sizeof(led_t) * CONFIG_NUM_LEDS);

    // Clear the LEDs
    led_t leds[CONFIG_NUM_LEDS] = {0};
    setLeds(leds, ARRAY_SIZE(leds));

    // Load a font
    loadFont(IBM_VGA_8_FONT, &quickSettings->font, true);

#ifdef SW_VOL_CONTROL
    // Load the buzzer song
    loadMidiFile(JINGLE_MID, &quickSettings->jingle, true);
#endif
    // Load graphics
    // Use SPI because we're not the only mode, I guess?
    loadWsg(DEFAULT_SETTING_WSG, &quickSettings->iconGeneric, true);
    loadWsg(LEDS_ENABLED_WSG, &quickSettings->iconLedsOn, true);
    loadWsg(LEDS_DISABLED_WSG, &quickSettings->iconLedsOff, true);
#ifdef SW_VOL_CONTROL
    loadWsg(MUSIC_ENABLED_WSG, &quickSettings->iconBgmOn, true);
    loadWsg(MUSIC_DISABLED_WSG, &quickSettings->iconBgmOff, true);
    loadWsg(SFX_ENABLED_WSG, &quickSettings->iconSfxOn, true);
    loadWsg(SFX_DISABLED_WSG, &quickSettings->iconSfxOff, true);
#endif
    loadWsg(BACKLIGHT_ENABLED_WSG, &quickSettings->iconTftOn, true);
    loadWsg(BACKLIGHT_DISABLED_WSG, &quickSettings->iconTftOff, true);

    // Initialize the menu
    quickSettings->menu                  = initMenu(quickSettingsName, quickSettingsMenuCb);
    quickSettings->renderer              = initMenuQuickSettingsRenderer(&quickSettings->font);
    quickSettings->renderer->defaultIcon = &quickSettings->iconGeneric;

    // Set up the values we'll use for the settings -- keep the current value if we toggle, or the max
    // If we get an independent mute setting we can just use that instead and not worry about it
    const settingParam_t* ledsBounds = getLedBrightnessSettingBounds();
    const settingParam_t* tftBounds  = getTftBrightnessSettingBounds();
#ifdef SW_VOL_CONTROL
    const settingParam_t* sfxBounds = getSfxVolumeSettingBounds();
    const settingParam_t* bgmBounds = getBgmVolumeSettingBounds();
#endif

    // Calculate the settings' minimums and set up their "on" values for toggling
    int32_t ledsValue = setupQuickSettingParams(ledsBounds, getLedBrightnessSetting(), &quickSettings->lastOnLedsValue,
                                                &quickSettings->minLedsValue);
    int32_t tftValue  = setupQuickSettingParams(tftBounds, getTftBrightnessSetting(), &quickSettings->lastOnTftValue,
                                                &quickSettings->minTftValue);
#ifdef SW_VOL_CONTROL
    int32_t sfxValue = setupQuickSettingParams(sfxBounds, getSfxVolumeSetting(), &quickSettings->lastOnSfxValue,
                                               &quickSettings->minSfxValue);
    int32_t bgmValue = setupQuickSettingParams(bgmBounds, getBgmVolumeSetting(), &quickSettings->lastOnBgmValue,
                                               &quickSettings->minBgmValue);

    quickSettings->prevSfxValue = sfxValue;
    quickSettings->prevBgmValue = bgmValue;
#endif
    quickSettings->showLeds = true;

    // Add the actual items to the menu
    addSettingsItemToMenu(quickSettings->menu, quickSettingsLeds, ledsBounds, ledsValue);
    addSettingsItemToMenu(quickSettings->menu, quickSettingsBacklight, tftBounds, tftValue);
#ifdef SW_VOL_CONTROL
    addSettingsItemToMenu(quickSettings->menu, quickSettingsSfx, sfxBounds, sfxValue);
    addSettingsItemToMenu(quickSettings->menu, quickSettingsBgm, bgmBounds, bgmValue);
#endif

    // Customize the icons and labels for all the quick settings items
    quickSettingsRendererCustomizeOption(quickSettings->renderer, quickSettingsLeds, &quickSettings->iconLedsOn,
                                         &quickSettings->iconLedsOff, quickSettingsLedsMax, quickSettingsLedsOff);
    quickSettingsRendererCustomizeOption(quickSettings->renderer, quickSettingsBacklight, &quickSettings->iconTftOn,
                                         &quickSettings->iconTftOff, quickSettingsBacklightMax,
                                         quickSettingsBacklightOff);
#ifdef SW_VOL_CONTROL
    quickSettingsRendererCustomizeOption(quickSettings->renderer, quickSettingsSfx, &quickSettings->iconSfxOn,
                                         &quickSettings->iconSfxOff, quickSettingsSfxMax, quickSettingsSfxMuted);
    quickSettingsRendererCustomizeOption(quickSettings->renderer, quickSettingsBgm, &quickSettings->iconBgmOn,
                                         &quickSettings->iconBgmOff, quickSettingsBgmMax, quickSettingsBgmMuted);
#endif
}

/**
 * This function is called when the mode is exited. It deinitializes variables and frees all memory.
 */
static void quickSettingsExitMode(void)
{
    // Deinitialize the menu
    deinitMenu(quickSettings->menu);
    deinitMenuQuickSettingsRenderer(quickSettings->renderer);

#ifdef SW_VOL_CONTROL
    // Free the buzzer song
    unloadMidiFile(&quickSettings->jingle);
#endif

    soundStop(true);

    // Restore the buzzer state, which may be actively playing
    soundRestore(quickSettings->buzzerState);

    // Restore the LED state
    setLeds(quickSettings->ledState, CONFIG_NUM_LEDS);

    // Free the font
    freeFont(&quickSettings->font);

    // Free graphics
    freeWsg(&quickSettings->iconGeneric);
    freeWsg(&quickSettings->iconLedsOn);
    freeWsg(&quickSettings->iconLedsOff);
    freeWsg(&quickSettings->iconBgmOn);
    freeWsg(&quickSettings->iconBgmOff);
    freeWsg(&quickSettings->iconSfxOn);
    freeWsg(&quickSettings->iconSfxOff);
    freeWsg(&quickSettings->iconTftOn);
    freeWsg(&quickSettings->iconTftOff);

    // Free underlying screen
    heap_caps_free(quickSettings->frozenScreen);

    heap_caps_free(quickSettings);
    quickSettings = NULL;
}

/**
 * @brief This callback function is called when an item is selected from the menu
 *
 * @param label The item that was selected from the menu
 * @param selected True if the item was selected with the A button, false if this is a multi-item which scrolled to
 * @param settingVal The value of the setting, if the menu item is a settings item
 */
static void quickSettingsMenuCb(const char* label, bool selected, uint32_t settingVal)
{
    if (selected)
    {
        // If we picked A, flip the settings item's selection
        settingVal = quickSettingsMenuFlipItem(label);
    }

    // Now handle the changed setting value
    quickSettingsOnChange(label, settingVal);
}

/**
 * @brief This function is called periodically and frequently. It will either draw the menu or play the game, depending
 * on which screen is currently being displayed
 *
 * @param elapsedUs The time that has elapsed since the last call to this function, in microseconds
 */
static void quickSettingsMainLoop(int64_t elapsedUs)
{
    // Process button events
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        // Re-map the thing so that items go left/right, and options go up/down
        switch (evt.button)
        {
            case PB_UP:
            {
                evt.button = PB_RIGHT;
                break;
            }
            case PB_DOWN:
            {
                evt.button = PB_LEFT;
                break;
            }
            case PB_LEFT:
            {
                evt.button = PB_UP;
                break;
            }
            case PB_RIGHT:
            {
                evt.button = PB_DOWN;
                break;
            }
            case PB_A:
            case PB_B:
            case PB_START:
            case PB_SELECT:
            {
                break;
            }
        }

        // Pass button events to the menu
        quickSettings->menu = menuButton(quickSettings->menu, evt);
    }

    // If the button press didn't cause the menu to deinit
    if (NULL != quickSettings)
    {
        // Draw the background
        memcpy(getPxTftFramebuffer(), quickSettings->frozenScreen, sizeof(paletteColor_t) * TFT_HEIGHT * TFT_WIDTH);

        // Draw the menu
        drawMenuQuickSettings(quickSettings->menu, quickSettings->renderer, elapsedUs);

        // Update the LEDs
        if (quickSettings->showLeds)
        {
            quickSettingsSetLeds(elapsedUs);
        }
        else
        {
            led_t leds[CONFIG_NUM_LEDS] = {0};
            setLeds(leds, ARRAY_SIZE(leds));
        }
    }
}

/**
 * @brief Returns the value that the setting should be set to if toggled from the given value
 *
 * If \c value is equal to the setting's minimum value, the most-recent non-minimum value for that
 * setting is returned. Otherwise, the minimum value is returned.
 *
 * @param label The menu item to get the flipped setting value of
 * @param value The current value of the setting to be flipped
 * @return int32_t The flipped setting value
 */
static int32_t quickSettingsFlipValue(const char* label, int32_t value)
{
    if (label == quickSettingsLeds)
    {
        return (value <= quickSettings->minLedsValue) ? quickSettings->lastOnLedsValue : quickSettings->minLedsValue;
    }
    else if (label == quickSettingsBacklight)
    {
        return (value <= quickSettings->minTftValue) ? quickSettings->lastOnTftValue : quickSettings->minTftValue;
    }
#ifdef SW_VOL_CONTROL
    else if (label == quickSettingsSfx)
    {
        return (value <= quickSettings->minSfxValue) ? quickSettings->lastOnSfxValue : quickSettings->minSfxValue;
    }
    else if (label == quickSettingsBgm)
    {
        return (value <= quickSettings->minBgmValue) ? quickSettings->lastOnBgmValue : quickSettings->minBgmValue;
    }
#endif

    return 0;
}

/**
 * @brief Handles the updating and tracking of the last "on" value for the given menu item
 *
 * @param label The label of the menu item to update
 * @param value The new setting value to be updated to
 */
static void quickSettingsOnChange(const char* label, int32_t value)
{
    quickSettings->showLeds = false;

    if (label == quickSettingsLeds)
    {
        soundStop(true);
        setLedBrightnessSetting(value);
        if (value > quickSettings->minLedsValue)
        {
            quickSettings->lastOnLedsValue = value;
        }

        quickSettings->showLeds = true;
    }
    else if (label == quickSettingsBacklight)
    {
        soundStop(true);
        setTftBrightnessSetting(value);
        if (value > quickSettings->minTftValue)
        {
            quickSettings->lastOnTftValue = value;
        }
    }
#ifdef SW_VOL_CONTROL
    else if (label == quickSettingsSfx)
    {
        if (value != quickSettings->prevSfxValue)
        {
            soundStop(true);
            setSfxVolumeSetting(value);
            soundPlaySfx(&quickSettings->jingle, BZR_STEREO);
            quickSettings->prevSfxValue = value;
        }

        if (value > quickSettings->minSfxValue)
        {
            quickSettings->lastOnSfxValue = value;
        }
    }
    else if (label == quickSettingsBgm)
    {
        if (value != quickSettings->prevBgmValue)
        {
            soundStop(true);
            setBgmVolumeSetting(value);
            soundPlayBgm(&quickSettings->jingle, BZR_STEREO);
            quickSettings->prevBgmValue = value;
        }

        if (value > quickSettings->minBgmValue)
        {
            quickSettings->lastOnBgmValue = value;
        }
    }
#endif
}

/**
 * @brief Toggles the given menu item between its last "on" value and the setting minimum value
 *
 * @param label The label of the menu item to flip
 * @return int32_t The item's new setting value after flipping
 */
static int32_t quickSettingsMenuFlipItem(const char* label)
{
    node_t* node     = quickSettings->menu->currentItem;
    menuItem_t* item = (menuItem_t*)(node->val);

    // Quick short-circuit for the current node
    if (NULL == item || item->label != label)
    {
        // Check all the items otherwise
        node = quickSettings->menu->items->first;
        while (NULL != node)
        {
            item = (menuItem_t*)(node->val);

            if (item->label == label)
            {
                break;
            }

            node = node->next;
        }

        if (NULL == item)
        {
            // Not found, do nothing
            return 0;
        }
    }

    // Item must have been found here
    // Flip the seleted option
    if (item->options)
    {
        item->currentOpt = item->numOptions - 1 - item->currentOpt;

        // Update the setting
        item->currentSetting = item->settingVals[item->currentOpt];
    }
    else
    {
        item->currentSetting = quickSettingsFlipValue(label, item->currentSetting);
    }

    return item->currentSetting;
}

static void quickSettingsSetLeds(int64_t elapsedUs)
{
    int64_t period  = 3600000; // 3.6s
    int64_t rOffset = (period * 2 / 3);
    int64_t gOffset = (period * 1 / 3);
    int64_t bOffset = 0;
    uint8_t rSpeed = 1, gSpeed = 1, bSpeed = 2;

    quickSettings->ledTimer = (quickSettings->ledTimer + elapsedUs) % period;

    led_t leds[CONFIG_NUM_LEDS];
    for (uint8_t i = 0; i < CONFIG_NUM_LEDS; i++)
    {
        leds[i].r = MAX(0, getSin1024((quickSettings->ledTimer * rSpeed + rOffset + i * period / CONFIG_NUM_LEDS)
                                      % period * 360 / period))
                    * 255 / 1024;
        leds[i].g = MAX(0, getSin1024((quickSettings->ledTimer * gSpeed + gOffset + i * period / CONFIG_NUM_LEDS)
                                      % period * 360 / period))
                    * 255 / 1024;
        leds[i].b = MAX(0, getSin1024((quickSettings->ledTimer * bSpeed + bOffset + i * period / CONFIG_NUM_LEDS)
                                      % period * 360 / period))
                    * 255 / 1024;
    }

    setLeds(leds, ARRAY_SIZE(leds));
}