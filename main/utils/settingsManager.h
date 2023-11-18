/*! \file settingsManager.h
 *
 * \section settingsManager_design Design Philosophy
 *
 * Settings are integers that control hardware peripherals and are persistent across reboots.
 * Each setting has a bounded range and a default value.
 *
 * The range and default value is stored in a settingParam_t, which does not change and should be declared \c const to
 * compile into ROM. The setting is read from NVS and stored a setting_t in RAM so that modes and peripherals can
 * quickly access it without disk operations. The DECL_SETTING() macro exists to easily and consistently declare the
 * required variables for a setting.
 *
 * Settings should immediately modify the hardware peripheral they control when changed.
 *
 * An example of a setting is TFT brightness. The brightness is an integer (0-MAX_TFT_BRIGHTNESS), which controls a
 * hardware peripheral (the TFT), and is persistent across reboots (once the brightness is set by the user, it should
 * stay that way).
 *
 * Settings rely heavily on, and can be thought of a wrapper around, hdw-nvs.h.
 *
 * Settings can be easily added to menus with the function addSettingsItemToMenu(). Be sure to set the setting in the
 * ::menuCb callback according to the new \c value.
 *
 * \section settingsManager_usage Usage
 *
 * Setting variables should be declared using DECL_SETTING() and giving the macro the minimum, maximum, and default
 * values.
 *
 * Settings should be modified using the \c static functions readSetting(), incSetting(), decSetting(), and
 * setSetting().
 *
 * Each setting should have it's own set of functions to be called from other files. For instance,
 * getTftBrightnessSetting() is used to get the current TFT brightness level, getTftBrightnessSettingBounds() is used to
 * get the setting bounds for menu construction, and setTftBrightnessSetting() is used to set a new TFT brightness
 * value, which is used immediately.
 *
 * readAllSettings() is called during system initialization to read all settings into RAM and set hardware peripherals
 * accordingly.
 *
 * \section settingsManager_example Example
 *
 * Adding a setting to a menu:
 * \code{.c}
 * // Declare the label
 * static const char tftSettingLabel[] = "TFT";
 *
 * // Add the TFT settings item to the menu. Initialize it by getting the parameters and current value
 * addSettingsItemToMenu(menu, tftSettingLabel, getTftBrightnessSettingBounds(), getTftBrightnessSetting());
 * \endcode
 *
 * Setting a setting from a menu callback
 * \code{.c}
 * // Make sure the label matches
 * if (tftSettingLabel == label)
 * {
 *     // Set the setting to the reported value.
 *     // This will call setTFTBacklightBrightness(), setting the actual brightness as well
 *     setTftBrightnessSetting(settingVal);
 * }
 * \endcode
 */

#ifndef _SETTINGS_MANAGER_H_
#define _SETTINGS_MANAGER_H_

//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include <stdbool.h>

#include "colorchordTypes.h"

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    GAMEPAD_TOUCH_L_STICK_SETTING,
    GAMEPAD_TOUCH_R_STICK_SETTING,
} gamepadTouch_t;

typedef enum
{
    SHOW_SECRETS,
    HIDE_SECRETS,
} showSecrets_t;

//==============================================================================
// Structs
//==============================================================================

/**
 * @brief Immutable data for a setting, including minimum, maximum, and default values, and the NVS key
 */
typedef struct
{
    const int32_t min; ///< The minimum value for this setting, inclusive
    const int32_t max; ///< The maximum value for this setting, inclusive
    const int32_t def; ///< The default value for this setting, must be between min and max
    const char* key;   ///< The NVS key for this setting
} settingParam_t;

//==============================================================================
// Function Prototypes
//==============================================================================

void readAllSettings(void);

uint16_t getBgmVolumeSetting(void);
const settingParam_t* getBgmVolumeSettingBounds(void);
bool setBgmVolumeSetting(uint16_t);

uint16_t getSfxVolumeSetting(void);
const settingParam_t* getSfxVolumeSettingBounds(void);
bool setSfxVolumeSetting(uint16_t);

uint8_t getTftBrightnessSetting(void);
const settingParam_t* getTftBrightnessSettingBounds(void);
bool setTftBrightnessSetting(uint8_t newVal);

uint8_t getLedBrightnessSetting(void);
const settingParam_t* getLedBrightnessSettingBounds(void);
bool setLedBrightnessSetting(uint8_t brightness);
bool incLedBrightnessSetting(void);
bool decLedBrightnessSetting(void);

uint8_t getMicGainSetting(void);
const settingParam_t* getMicGainSettingBounds(void);
bool setMicGainSetting(uint8_t newGain);
bool decMicGainSetting(void);
bool incMicGainSetting(void);

uint16_t getScreensaverTimeSetting(void);
const settingParam_t* getScreensaverTimeSettingBounds(void);
bool setScreensaverTimeSetting(uint16_t val);

colorchordMode_t getColorchordModeSetting(void);
bool setColorchordModeSetting(colorchordMode_t);

bool getTestModePassedSetting(void);
bool setTestModePassedSetting(bool status);

bool getTutorialCompletedSetting(void);
bool setTutorialCompletedSetting(bool status);

bool getGamepadAccelSetting(void);
const settingParam_t* getGamepadAccelSettingBounds(void);
bool setGamepadAccelSetting(bool status);

gamepadTouch_t getGamepadTouchSetting(void);
const settingParam_t* getGamepadTouchSettingBounds(void);
bool setGamepadTouchSetting(gamepadTouch_t status);

showSecrets_t getShowSecretsMenuSetting(void);
const settingParam_t* getShowSecretsMenuSettingBounds(void);
bool setShowSecretsMenuSetting(showSecrets_t status);

bool getFlipSwadgeSetting(void);
const settingParam_t* getFlipSwadgeSettingBounds(void);
bool setFlipSwadgeSetting(bool val);

#endif
