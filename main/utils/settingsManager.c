//==============================================================================
// Includes
//==============================================================================

#include "hdw-nvs.h"
#include "hdw-bzr.h"
#include "hdw-tft.h"
#include "hdw-mic.h"
#include "hdw-led.h"
#include "macros.h"
#include "settingsManager.h"

//==============================================================================
// Struct
//==============================================================================

/**
 * @brief A setting. The parameters are const, but the value is not
 */
typedef struct
{
    const settingParam_t* param; ///< The setting's immutable data (bounds and NVS key)
    int32_t val;                 ///< The setting's current value, in RAM
} setting_t;

//==============================================================================
// Defines
//==============================================================================

/**
 * @brief Helper macro to declare const parameters for settings, and the variable setting
 * @param NAME the key for this setting, also used in variable names
 * @param mi The minimum value for this setting
 * @param ma The maximum value for this setting
 * @param de The default value for this setting
 */
#define DECL_SETTING(NAME, mi, ma, de)           \
    static const settingParam_t NAME##_param = { \
        .key = #NAME,                            \
        .min = mi,                               \
        .max = ma,                               \
        .def = de,                               \
    };                                           \
    static setting_t NAME##_setting = {          \
        .param = &NAME##_param,                  \
        .val   = de,                             \
    }

//==============================================================================
// Variables
//==============================================================================

DECL_SETTING(test, 0, 1, 0);
DECL_SETTING(bgm, 0, 13, 13);
DECL_SETTING(sfx, 0, 13, 13);
DECL_SETTING(tft_br, 0, 7, 5);
DECL_SETTING(led_br, 0, 8, 5);
DECL_SETTING(mic, 0, 7, 7);
DECL_SETTING(cc_mode, ALL_SAME_LEDS, LINEAR_LEDS, ALL_SAME_LEDS);
DECL_SETTING(scrn_sv, 0, 6, 2);

//==============================================================================
// Static Function Prototypes
//==============================================================================

static bool readSetting(setting_t* setting);
static bool incSetting(setting_t* setting);
static bool decSetting(setting_t* setting);
static bool setSetting(setting_t* setting, uint32_t newVal);

//==============================================================================
// Static Functions
//==============================================================================

/**
 * @brief Internal helper function to read a setting_t's value from NVS to RAM
 *
 * @param setting The setting to read
 * @return true if the setting was read, false if it was not
 */
static bool readSetting(setting_t* setting)
{
    // Read the setting into val
    if (false == readNvs32(setting->param->key, &setting->val))
    {
        // If the read failed, set val to default and write it
        return setSetting(setting, setting->param->def);
    }
    return true;
}

/**
 * @brief Internal helper function to increment a setting_t's value by one and write it to RAM and NVS.
 * This will not increment the value past the setting's max.
 *
 * @param setting The setting to increment by one
 * @return true if the setting was written, false if it was not
 */
static bool incSetting(setting_t* setting)
{
    setting->val = MIN(setting->val + 1, setting->param->max);
    return writeNvs32(setting->param->key, setting->val);
}

/**
 * @brief Internal helper function to decrement a setting_t's value by one and write it to RAM and NVS.
 * This will not decrement the value past the setting's min.
 *
 * @param setting The setting to decrement by one
 * @return true if the setting was written, false if it was not
 */
static bool decSetting(setting_t* setting)
{
    setting->val = MAX(setting->val - 1, setting->param->min);
    return writeNvs32(setting->param->key, setting->val);
}

/**
 * @brief Internal helper function to set a setting_t's value and write it to RAM and NVS.
 * This will not set the value past the setting's min or max.
 *
 * @param setting The setting to increment by one
 * @return true if the setting was written, false if it was not
 */
static bool setSetting(setting_t* setting, uint32_t newVal)
{
    setting->val = CLAMP(newVal, setting->param->min, setting->param->max);
    return writeNvs32(setting->param->key, setting->val);
}

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Read all settings from NVM and set the appropriate hardware peripherals, like TFT and LED brightness
 */
void readAllSettings(void)
{
    // Read the test mode setting
    readSetting(&test_setting);

    // Read and set buzzer settings
    readSetting(&bgm_setting);
    bzrSetBgmVolume(getBgmVolumeSetting());
    readSetting(&sfx_setting);
    bzrSetSfxVolume(getSfxVolumeSetting());

    // Read and apply TFT settings
    readSetting(&tft_br_setting);
    setTFTBacklightBrightness(getTftBrightnessSetting());

    // Read and apply LED settings
    readSetting(&led_br_setting);
    setLedBrightness(getLedBrightnessSetting());

    // Read the mic setting
    readSetting(&mic_setting);

    // Read the colorchord setting
    readSetting(&cc_mode_setting);

    // Read the screensaver setting
    readSetting(&scrn_sv_setting);
}

//==============================================================================

/**
 * @brief Get the current background music volume setting
 *
 * @return the current background music volume setting
 */
uint16_t getBgmVolumeSetting(void)
{
    return bgm_setting.val;
}

/**
 * @brief Get the bounds for the background volume setting. Useful for initializing settings items in menus.
 *
 * @return the bounds for the background volume setting
 */
const settingParam_t* getBgmVolumeSettingBounds(void)
{
    return &bgm_param;
}

/**
 * @brief Set the current background music volume setting. This calls bzrSetBgmVolume() after writing to NVS.
 *
 * @param vol The new volume setting
 * @return true if the setting was written, false if it wasn't
 */
bool setBgmVolumeSetting(uint16_t vol)
{
    if (setSetting(&bgm_setting, vol))
    {
        bzrSetBgmVolume(getBgmVolumeSetting());
        return true;
    }
    return false;
}

//==============================================================================

/**
 * @brief Get the current sound effects volume setting
 *
 * @return the current sound effects volume setting
 */
uint16_t getSfxVolumeSetting(void)
{
    return sfx_setting.val;
}

/**
 * @brief Get the bounds for the background volume setting. Useful for initializing settings items in menus.
 *
 * @return the bounds for the background volume setting
 */
const settingParam_t* getSfxVolumeSettingBounds(void)
{
    return &sfx_param;
}

/**
 * @brief Set the current sound effects volume setting. This calls bzrSetSfxVolume() after writing to NVS.
 *
 * @param vol The new volume setting
 * @return true if the setting was written, false if it wasn't
 */
bool setSfxVolumeSetting(uint16_t vol)
{
    if (setSetting(&sfx_setting, vol))
    {
        bzrSetSfxVolume(getSfxVolumeSetting());
        return true;
    }
    return false;
}

//==============================================================================

/**
 * @brief Get the current TFT brightness setting
 *
 * @return the current TFT brightness setting
 */
uint8_t getTftBrightnessSetting(void)
{
    return tft_br_setting.val;
}

/**
 * @brief Get the bounds for the TFT brightness setting. Useful for initializing settings items in menus.
 *
 * @return the bounds for the TFT brightness setting
 */
const settingParam_t* getTftBrightnessSettingBounds(void)
{
    return &tft_br_param;
}

/**
 * @brief Set the current TFT brightness setting. This calls setTFTBacklightBrightness() after writing to NVS.
 *
 * @param newVal the new TFT brightness setting
 * @return true if the setting was written, false if it wasn't
 */
bool setTftBrightnessSetting(uint8_t newVal)
{
    if (setSetting(&tft_br_setting, newVal))
    {
        setTFTBacklightBrightness(getTftBrightnessSetting());
        return true;
    }
    return false;
}

//==============================================================================

/**
 * @brief Get the current LED brightness
 *
 * @return the current LED brightness
 */
uint8_t getLedBrightnessSetting(void)
{
    return led_br_setting.val;
}

/**
 * @brief Get the bounds for the LED brightness setting. Useful for initializing settings items in menus.
 *
 * @return the bounds for the LED brightness setting
 */
const settingParam_t* getLedBrightnessSettingBounds(void)
{
    return &led_br_param;
}

/**
 * @brief Set the current LED brightness setting. This calls setLedBrightness() after writing to NVS.
 *
 * @param brightness The new LED brightness setting
 * @return true if the setting was written, false if it was not
 */
bool setLedBrightnessSetting(uint8_t brightness)
{
    if (setSetting(&led_br_setting, brightness))
    {
        setLedBrightness(getLedBrightnessSetting());
        return true;
    }
    return false;
}

/**
 * @brief Increment the LED brightness setting by one. This calls setLedBrightness() after writing to NVS.
 *
 * @return true if the setting was written, false if it was not
 */
bool incLedBrightnessSetting(void)
{
    if (incSetting(&led_br_setting))
    {
        setLedBrightness(getLedBrightnessSetting());
        return true;
    }
    return false;
}

/**
 * @brief Decrement the LED brightness setting by one. This calls setLedBrightness() after writing to NVS.
 *
 * @return true if the setting was written, false if it was not
 */
bool decLedBrightnessSetting(void)
{
    if (decSetting(&led_br_setting))
    {
        setLedBrightness(getLedBrightnessSetting());
        return true;
    }
    return false;
}

//==============================================================================

/**
 * @brief Get the current microphone gain setting
 *
 * @return the current microphone gain setting
 */
uint8_t getMicGainSetting(void)
{
    return mic_setting.val;
}

/**
 * @brief Get the bounds for the microphone gain setting. Useful for initializing settings items in menus.
 *
 * @return the bounds for the microphone gain setting
 */
const settingParam_t* getMicGainSettingBounds(void)
{
    return &mic_param;
}

/**
 * @brief Set the current microphone gain setting. The new value is immediately used when sampling the microphone.
 *
 * @param newGain The new microphone gain setting
 * @return true if the setting was written, false if it wasn't
 */
bool setMicGainSetting(uint8_t newGain)
{
    return setSetting(&mic_setting, newGain);
}

/**
 * @brief Decrement the microphone gain setting by one. The new value is immediately used when sampling the microphone.
 *
 * @return true if the setting was written, false if it was not
 */
bool decMicGainSetting(void)
{
    return decSetting(&mic_setting);
}

/**
 * @brief Decrement the microphone gain setting by one. The new value is immediately used when sampling the microphone.
 *
 * @return true if the setting was written, false if it was not
 */
bool incMicGainSetting(void)
{
    return incSetting(&mic_setting);
}

//==============================================================================

/**
 * @brief Get the current screensaver timeout setting
 *
 * @return the current screensaver timeout setting
 */
uint16_t getScreensaverTimeSetting(void)
{
    return scrn_sv_setting.val;
}

/**
 * @brief Get the bounds for the screensaver timeout setting. Useful for initializing settings items in menus.
 *
 * @return the bounds for the screensaver timeout setting
 */
const settingParam_t* getScreensaverTimeSettingBounds(void)
{
    return &scrn_sv_param;
}

/**
 * @brief Set the current screensaver timeout setting. The new value is immediately used for the screensaver timeout.
 *
 * @param val The new screensaver timeout setting
 * @return true if the setting was written, false if it wasn't
 */
bool setScreensaverTimeSetting(uint16_t val)
{
    // TODO implement screensaver
    return setSetting(&scrn_sv_setting, val);
}

//==============================================================================

/**
 * @brief Get the current Colorchord LED output setting
 *
 * @return the current Colorchord LED output setting
 */
colorchordMode_t getColorchordModeSetting(void)
{
    return cc_mode_setting.val;
}

/**
 * @brief Set the current Colorchord LED output setting. The new value is immediately used when sampling the microphone.
 *
 * @param newMode The new Colorchord LED output setting
 * @return true if the setting was written, false if it wasn't
 */
bool setColorchordModeSetting(colorchordMode_t newMode)
{
    return setSetting(&cc_mode_setting, newMode);
}

//==============================================================================

/**
 * @brief Get the current test mode passed setting
 *
 * @return the current test mode passed setting
 */
bool getTestModePassedSetting(void)
{
    return test_setting.val;
}

/**
 * @brief Set the current test mode passed setting. The new value is immediately used when sampling the microphone.
 *
 * @param status The new test mode passed setting
 * @return true if the setting was written, false if it wasn't
 */
bool setTestModePassedSetting(bool status)
{
    return setSetting(&test_setting, status);
}
