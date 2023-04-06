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

typedef struct
{
    const settingParam_t* param;
    int32_t val;
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
DECL_SETTING(led_br, 0, 7, 5);
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

static bool incSetting(setting_t* setting)
{
    setting->val = MIN(setting->val + 1, setting->param->max);
    return writeNvs32(setting->param->key, setting->val);
}

static bool decSetting(setting_t* setting)
{
    setting->val = MAX(setting->val - 1, setting->param->min);
    return writeNvs32(setting->param->key, setting->val);
}

static bool setSetting(setting_t* setting, uint32_t newVal)
{
    setting->val = CLAMP(newVal, setting->param->min, setting->param->max);
    return writeNvs32(setting->param->key, setting->val);
}

//==============================================================================
// Functions
//==============================================================================

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
    setTFTBacklightBrightness(getTFTBacklightBrightnessSetting());

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

// TODO move this to the component, not settings manager
const uint16_t volLevels[] = {
    0, 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096,
};

uint16_t getBgmVolumeSetting(void)
{
    return bgm_setting.val;
}

uint16_t getBgmVolumeLevelSetting(void)
{
    return volLevels[bgm_setting.val];
}

const settingParam_t* getBgmVolumeSettingBounds(void)
{
    return &bgm_param;
}

bool setBgmVolumeSetting(uint16_t vol)
{
    if (setSetting(&bgm_setting, vol))
    {
        bzrSetBgmVolume(getBgmVolumeLevelSetting());
        return true;
    }
    return false;
}

//==============================================================================

uint16_t getSfxVolumeSetting(void)
{
    return sfx_setting.val;
}

const settingParam_t* getSfxVolumeSettingBounds(void)
{
    return &sfx_param;
}

uint16_t getSfxVolumeLevelSetting(void)
{
    return volLevels[sfx_setting.val];
}

bool setSfxVolumeSetting(uint16_t vol)
{
    if (setSetting(&sfx_setting, vol))
    {
        bzrSetSfxVolume(getSfxVolumeLevelSetting());
        return true;
    }
    return false;
}

//==============================================================================

uint8_t getTftBrightnessSetting(void)
{
    return tft_br_setting.val;
}

uint8_t getTFTBacklightBrightnessSetting(void)
{
    // TODO move this to the component, not settings manager
    return (CONFIG_TFT_MIN_BRIGHTNESS
            + (((CONFIG_TFT_MAX_BRIGHTNESS - CONFIG_TFT_MIN_BRIGHTNESS) * tft_br_setting.val) / tft_br_param.max));
}

const settingParam_t* getTftBrightnessSettingBounds(void)
{
    return &tft_br_param;
}

bool setTftBrightnessSetting(uint8_t newVal)
{
    if (setSetting(&tft_br_setting, newVal))
    {
        setTFTBacklightBrightness(getTFTBacklightBrightnessSetting());
        return true;
    }
    return false;
}

//==============================================================================

uint8_t getLedBrightnessSetting(void)
{
    return led_br_setting.val;
}

const settingParam_t* getLedBrightnessSettingBounds(void)
{
    return &led_br_param;
}

bool setLedBrightnessSetting(uint8_t brightness)
{
    if (setSetting(&led_br_setting, brightness))
    {
        setLedBrightness(getLedBrightnessSetting());
        return true;
    }
    return false;
}

bool incLedBrightnessSetting(void)
{
    if (incSetting(&led_br_setting))
    {
        setLedBrightness(getLedBrightnessSetting());
        return true;
    }
    return false;
}

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

uint8_t getMicGainSetting(void)
{
    return mic_setting.val;
}

const settingParam_t* getMicGainSettingBounds(void)
{
    return &mic_param;
}

uint16_t getMicGainMultiplierSetting(void)
{
    // Using a logarithmic volume control.
    // TODO move this to the component, not settings manager
    const uint16_t micVols[] = {
        32, 45, 64, 90, 128, 181, 256, 362,
    };
    return micVols[mic_setting.val];
}

bool setMicGainSetting(uint8_t newGain)
{
    return setSetting(&mic_setting, newGain);
}

bool decMicGainSetting(void)
{
    return decSetting(&mic_setting);
}

bool incMicGainSetting(void)
{
    return incSetting(&mic_setting);
}

//==============================================================================

uint16_t getScreensaverTimeSetting(void)
{
    return scrn_sv_setting.val;
}

const settingParam_t* getScreensaverTimeSettingBounds(void)
{
    return &scrn_sv_param;
}

bool setScreensaverTimeSetting(uint16_t val)
{
    return setSetting(&scrn_sv_setting, val);
}

//==============================================================================

colorchordMode_t getColorchordModeSetting(void)
{
    return cc_mode_setting.val;
}

bool setColorchordModeSetting(colorchordMode_t newMode)
{
    return setSetting(&cc_mode_setting, newMode);
}

//==============================================================================

bool getTestModePassedSetting(void)
{
    return test_setting.val;
}

bool setTestModePassedSetting(bool status)
{
    return setSetting(&test_setting, status);
}
