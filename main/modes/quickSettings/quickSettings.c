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

    wsg_t iconGeneric;
    wsg_t iconSfxOn;
    wsg_t iconSfxOff;
    wsg_t iconBgmOn;
    wsg_t iconBgmOff;
    wsg_t iconLedsOn;
    wsg_t iconLedsOff;

    int32_t sfxOptionsValues[2];
    int32_t bgmOptionsValues[2];
    int32_t ledsOptionsValues[2];
} quickSettingsMenu_t;

//==============================================================================
// Function Prototypes
//==============================================================================

static int32_t setupQuickSettingParams(const settingParam_t* bounds, int32_t currentValue, int32_t* optionValues);
static void quickSettingsMainLoop(int64_t elapsedUs);
static void quickSettingsEnterMode(void);
static void quickSettingsExitMode(void);
static void quickSettingsMenuCb(const char* label, bool selected, uint32_t settingVal);
static int32_t quickSettingsMenuFlipItem(const char* label);

//==============================================================================
// Strings
//==============================================================================

static const char quickSettingsName[] = "Settings";

static const char quickSettingsLeds[] = "LEDs ";
static const char quickSettingsSfx[]  = "Sounds ";
static const char quickSettingsBgm[]  = "Music ";

static const char quickSettingsOn[]    = "On";
static const char quickSettingsOff[]   = "Off";
static const char quickSettingsMuted[] = "Muted";

static const char* const quickSettingsOptionsLeds[] = {
    quickSettingsOff,
    quickSettingsOn,
};

static const char* const quickSettingsOptionsAudio[] = {
    quickSettingsMuted,
    quickSettingsOn,
};

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
 * @param optionValues Pointer to an array of 2 values where the values will be written
 * @return int32_t Returns the current setting value for convenience
 */
static int32_t setupQuickSettingParams(const settingParam_t* bounds, int32_t currentValue, int32_t* optionValues)
{
    // Use the minimum and the current value for the toggles
    // This way, we get the current value back after toggling off.
    optionValues[0] = bounds->min;
    optionValues[1] = currentValue;

    // However, if it was already off / at the minimum, we need it to be something, so just use the max
    if (currentValue <= bounds->min)
    {
        optionValues[1] = bounds->max;
    }

    return currentValue;
}

/**
 * @brief Enter Pong mode, allocate required memory, and initialize required variables
 *
 */
static void quickSettingsEnterMode(void)
{
    // Allocate and clear all memory for this mode. All the variables are contained in a single struct for convenience.
    // calloc() is used instead of malloc() because calloc() also initializes the allocated memory to zeros.
    quickSettings = calloc(1, sizeof(quickSettingsMenu_t));

    // Load a font
    loadFont("ibm_vga8.font", &quickSettings->font, true);

    // Load graphics
    // Use SPI because we're not the only mode, I guess?
    loadWsg("defaultSetting.wsg", &quickSettings->iconGeneric, true);
    loadWsg("ledsEnabled.wsg", &quickSettings->iconLedsOn, true);
    loadWsg("ledsDisabled.wsg", &quickSettings->iconLedsOff, true);
    loadWsg("musicEnabled.wsg", &quickSettings->iconBgmOn, true);
    loadWsg("musicDisabled.wsg", &quickSettings->iconBgmOff, true);
    loadWsg("sfxEnabled.wsg", &quickSettings->iconSfxOn, true);
    loadWsg("sfxDisabled.wsg", &quickSettings->iconSfxOff, true);

    // Initialize the menu
    quickSettings->menu     = initMenu(quickSettingsName, quickSettingsMenuCb);
    quickSettings->renderer = initMenuQuickSettingsRenderer(&quickSettings->font);

    // Set up the values we'll use for the settings -- keep the current value if we toggle, or the max
    // If we get an independent mute setting we can just use that instead and not worry about it
    const settingParam_t* ledsBounds = getLedBrightnessSettingBounds();
    const settingParam_t* sfxBounds  = getSfxVolumeSettingBounds();
    const settingParam_t* bgmBounds  = getBgmVolumeSettingBounds();

    int32_t ledsValue
        = setupQuickSettingParams(ledsBounds, getLedBrightnessSetting(), quickSettings->ledsOptionsValues);
    int32_t sfxValue = setupQuickSettingParams(sfxBounds, getSfxVolumeSetting(), quickSettings->sfxOptionsValues);
    int32_t bgmValue = setupQuickSettingParams(bgmBounds, getBgmVolumeSetting(), quickSettings->bgmOptionsValues);

    addSettingsOptionsItemToMenu(quickSettings->menu, quickSettingsLeds, quickSettingsOptionsLeds,
                                 quickSettings->ledsOptionsValues, 2, ledsBounds, ledsValue);
    addSettingsOptionsItemToMenu(quickSettings->menu, quickSettingsSfx, quickSettingsOptionsAudio,
                                 quickSettings->sfxOptionsValues, 2, sfxBounds, sfxValue);
    addSettingsOptionsItemToMenu(quickSettings->menu, quickSettingsBgm, quickSettingsOptionsAudio,
                                 quickSettings->bgmOptionsValues, 2, bgmBounds, bgmValue);

    quickSettingsRendererAddIcon(quickSettings->renderer, quickSettingsLeds, &quickSettings->iconLedsOn,
                                 &quickSettings->iconLedsOff);
    quickSettingsRendererAddIcon(quickSettings->renderer, quickSettingsSfx, &quickSettings->iconSfxOn,
                                 &quickSettings->iconSfxOff);
    quickSettingsRendererAddIcon(quickSettings->renderer, quickSettingsBgm, &quickSettings->iconBgmOn,
                                 &quickSettings->iconBgmOff);
}

/**
 * This function is called when the mode is exited. It deinitializes variables and frees all memory.
 */
static void quickSettingsExitMode(void)
{
    // Deinitialize the menu
    deinitMenu(quickSettings->menu);
    deinitMenuQuickSettingsRenderer(quickSettings->renderer);

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

    free(quickSettings);
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

    if (label == quickSettingsLeds)
    {
        // Set the leds to the setting value
        setLedBrightnessSetting(settingVal);
    }
    else if (label == quickSettingsSfx)
    {
        setSfxVolumeSetting(settingVal);
    }
    else if (label == quickSettingsBgm)
    {
        setBgmVolumeSetting(settingVal);
    }
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
                evt.button = PB_RIGHT;
                break;

            case PB_DOWN:
                evt.button = PB_LEFT;
                break;

            case PB_LEFT:
                evt.button = PB_UP;
                break;

            case PB_RIGHT:
                evt.button = PB_DOWN;
                break;

            case PB_A:
            case PB_B:
            case PB_START:
            case PB_SELECT:
            case TB_0:
            case TB_1:
            case TB_2:
            case TB_3:
            case TB_4:
                break;
        }

        // Pass button events to the menu
        quickSettings->menu = menuButton(quickSettings->menu, evt);
    }

    // Draw the menu
    drawMenuQuickSettings(quickSettings->menu, quickSettings->renderer, elapsedUs);
}

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

        if (item == NULL)
        {
            // Not found, do nothing
            return 0;
        }
    }

    // Item must have been found here
    // Flip the seleted option
    item->currentOpt = item->numOptions - 1 - item->currentOpt;

    // Update the setting
    item->currentSetting = item->settingVals[item->currentOpt];

    return item->currentSetting;
}