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

#include "quickSettings.h"
#include "menu.h"
#include "menu_utils.h"
#include "menuQuickSettingsRenderer.h"
#include "macros.h"
#include "hdw-bzr.h"

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
    song_t jingle;
    void* buzzerState;
    led_t ledState[CONFIG_NUM_LEDS + 1];

    wsg_t iconGeneric;
    wsg_t iconSfxOn;
    wsg_t iconSfxOff;
    wsg_t iconBgmOn;
    wsg_t iconBgmOff;
    wsg_t iconLedsOn;
    wsg_t iconLedsOff;
    wsg_t iconTftOn;
    wsg_t iconTftOff;

    int32_t lastOnSfxValue;
    int32_t minSfxValue;
    int32_t prevSfxValue;

    int32_t lastOnBgmValue;
    int32_t minBgmValue;
    int32_t prevBgmValue;

    int32_t lastOnLedsValue;
    int32_t minLedsValue;

    int32_t lastOnTftValue;
    int32_t minTftValue;
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

static const char quickSettingsLeds[]      = "LED Brightness";
static const char quickSettingsSfx[]       = "SFX Volume";
static const char quickSettingsBgm[]       = "Music Volume";
static const char quickSettingsBacklight[] = "Screen Brightness: ";

static const char quickSettingsLedsOff[]      = "LEDs Off";
static const char quickSettingsLedsMax[]      = "LED Brightness: Max";
static const char quickSettingsSfxMuted[]     = "SFX Muted";
static const char quickSettingsSfxMax[]       = "SFX Volume: Max";
static const char quickSettingsBgmMuted[]     = "Music Muted";
static const char quickSettingsBgmMax[]       = "Music Volume: Max";
static const char quickSettingsBacklightOff[] = "Screen Backlight Off";
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

/// All state information for the Quick Settings mode. This whole struct is calloc()'d and free()'d
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
    // calloc() is used instead of malloc() because calloc() also initializes the allocated memory to zeros.
    quickSettings = calloc(1, sizeof(quickSettingsMenu_t));

    // Save the buzzer state and pause it
    quickSettings->buzzerState = bzrSave();

    // Save the LED state
    getLedState(quickSettings->ledState, CONFIG_NUM_LEDS + 1);

    // Clear the LEDs
    led_t leds[CONFIG_NUM_LEDS] = {0};
    setLeds(leds, ARRAY_SIZE(leds));

    // Load a font
    loadFont("ibm_vga8.font", &quickSettings->font, true);

    // Load the buzzer song
    loadSong("jingle.sng", &quickSettings->jingle, true);

    // Load graphics
    // Use SPI because we're not the only mode, I guess?
    loadWsg("defaultSetting.wsg", &quickSettings->iconGeneric, true);
    loadWsg("ledsEnabled.wsg", &quickSettings->iconLedsOn, true);
    loadWsg("ledsDisabled.wsg", &quickSettings->iconLedsOff, true);
    loadWsg("musicEnabled.wsg", &quickSettings->iconBgmOn, true);
    loadWsg("musicDisabled.wsg", &quickSettings->iconBgmOff, true);
    loadWsg("sfxEnabled.wsg", &quickSettings->iconSfxOn, true);
    loadWsg("sfxDisabled.wsg", &quickSettings->iconSfxOff, true);
    loadWsg("backlightEnabled.wsg", &quickSettings->iconTftOn, true);
    loadWsg("backlightDisabled.wsg", &quickSettings->iconTftOff, true);

    // Initialize the menu
    quickSettings->menu                  = initMenu(quickSettingsName, quickSettingsMenuCb);
    quickSettings->renderer              = initMenuQuickSettingsRenderer(&quickSettings->font);
    quickSettings->renderer->defaultIcon = &quickSettings->iconGeneric;

    // Set up the values we'll use for the settings -- keep the current value if we toggle, or the max
    // If we get an independent mute setting we can just use that instead and not worry about it
    const settingParam_t* ledsBounds = getLedBrightnessSettingBounds();
    const settingParam_t* tftBounds  = getTftBrightnessSettingBounds();
    const settingParam_t* sfxBounds  = getSfxVolumeSettingBounds();
    const settingParam_t* bgmBounds  = getBgmVolumeSettingBounds();

    // Calculate the settings' minimums and set up their "on" values for toggling
    int32_t ledsValue = setupQuickSettingParams(ledsBounds, getLedBrightnessSetting(), &quickSettings->lastOnLedsValue,
                                                &quickSettings->minLedsValue);
    int32_t tftValue  = setupQuickSettingParams(tftBounds, getTftBrightnessSetting(), &quickSettings->lastOnTftValue,
                                                &quickSettings->minTftValue);
    int32_t sfxValue  = setupQuickSettingParams(sfxBounds, getSfxVolumeSetting(), &quickSettings->lastOnSfxValue,
                                                &quickSettings->minSfxValue);
    int32_t bgmValue  = setupQuickSettingParams(bgmBounds, getBgmVolumeSetting(), &quickSettings->lastOnBgmValue,
                                                &quickSettings->minBgmValue);

    quickSettings->prevSfxValue = sfxValue;
    quickSettings->prevBgmValue = bgmValue;
    quickSettings->showLeds     = true;

    // Add the actual items to the menu
    addSettingsItemToMenu(quickSettings->menu, quickSettingsLeds, ledsBounds, ledsValue);
    addSettingsItemToMenu(quickSettings->menu, quickSettingsBacklight, tftBounds, tftValue);
    addSettingsItemToMenu(quickSettings->menu, quickSettingsSfx, sfxBounds, sfxValue);
    addSettingsItemToMenu(quickSettings->menu, quickSettingsBgm, bgmBounds, bgmValue);

    // Customize the icons and labels for all the quick settings items
    quickSettingsRendererCustomizeOption(quickSettings->renderer, quickSettingsLeds, &quickSettings->iconLedsOn,
                                         &quickSettings->iconLedsOff, quickSettingsLedsMax, quickSettingsLedsOff);
    quickSettingsRendererCustomizeOption(quickSettings->renderer, quickSettingsBacklight, &quickSettings->iconTftOn,
                                         &quickSettings->iconTftOff, quickSettingsBacklightMax,
                                         quickSettingsBacklightOff);
    quickSettingsRendererCustomizeOption(quickSettings->renderer, quickSettingsSfx, &quickSettings->iconSfxOn,
                                         &quickSettings->iconSfxOff, quickSettingsSfxMax, quickSettingsSfxMuted);
    quickSettingsRendererCustomizeOption(quickSettings->renderer, quickSettingsBgm, &quickSettings->iconBgmOn,
                                         &quickSettings->iconBgmOff, quickSettingsBgmMax, quickSettingsBgmMuted);
}

/**
 * This function is called when the mode is exited. It deinitializes variables and frees all memory.
 */
static void quickSettingsExitMode(void)
{
    // Deinitialize the menu
    deinitMenu(quickSettings->menu);
    deinitMenuQuickSettingsRenderer(quickSettings->renderer);

    // Free the buzzer song
    freeSong(&quickSettings->jingle);

    bzrStop(true);

    // Restore the buzzer state and resume it
    bzrRestore(quickSettings->buzzerState);

    // Restore the LED state
    setLeds(quickSettings->ledState, CONFIG_NUM_LEDS + 1);

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

    free(quickSettings);
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
    else if (label == quickSettingsSfx)
    {
        return (value <= quickSettings->minSfxValue) ? quickSettings->lastOnSfxValue : quickSettings->minSfxValue;
    }
    else if (label == quickSettingsBgm)
    {
        return (value <= quickSettings->minBgmValue) ? quickSettings->lastOnBgmValue : quickSettings->minBgmValue;
    }

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
        bzrStop(true);
        setLedBrightnessSetting(value);
        if (value > quickSettings->minLedsValue)
        {
            quickSettings->lastOnLedsValue = value;
        }

        quickSettings->showLeds = true;
    }
    else if (label == quickSettingsBacklight)
    {
        bzrStop(true);
        setTftBrightnessSetting(value);
        if (value > quickSettings->minTftValue)
        {
            quickSettings->lastOnTftValue = value;
        }
    }
    else if (label == quickSettingsSfx)
    {
        if (value != quickSettings->prevSfxValue)
        {
            bzrStop(true);
            setSfxVolumeSetting(value);
            bzrPlaySfx(&quickSettings->jingle, BZR_STEREO);
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
            bzrStop(true);
            setBgmVolumeSetting(value);
            bzrPlayBgm(&quickSettings->jingle, BZR_STEREO);
            quickSettings->prevBgmValue = value;
        }

        if (value > quickSettings->minBgmValue)
        {
            quickSettings->lastOnBgmValue = value;
        }
    }
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