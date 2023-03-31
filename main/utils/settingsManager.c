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
    const int32_t min;
    const int32_t max;
    const int32_t def;
    const char* key;
} settingParam_t;

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
DECL_SETTING(bgm, 0, 1, 0);
DECL_SETTING(sfx, 0, 1, 0);
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
    readSetting(&test_setting);
    readSetting(&bgm_setting);
    readSetting(&sfx_setting);
    readSetting(&tft_br_setting);
    readSetting(&led_br_setting);
    readSetting(&mic_setting);
    readSetting(&cc_mode_setting);
    readSetting(&scrn_sv_setting);
    // TODO set peripherals based on read settings?
}

//==============================================================================

bool getBgmIsMuted(void)
{
    return bgm_setting.val;
}

bool setBgmIsMuted(bool isMuted)
{
    if (setSetting(&bgm_setting, isMuted))
    {
        bzrSetBgmIsMuted(isMuted);
        return true;
    }
    return false;
}

//==============================================================================

bool getSfxIsMuted(void)
{
    return sfx_setting.val;
}

bool setSfxIsMuted(bool isMuted)
{
    if (setSetting(&sfx_setting, isMuted))
    {
        bzrSetSfxIsMuted(isMuted);
        return true;
    }
    return false;
}

//==============================================================================

int32_t getTftBrightness(void)
{
    return tft_br_setting.val;
}

uint8_t getTftIntensity(void)
{
    return (CONFIG_TFT_MIN_BRIGHTNESS
            + (((CONFIG_TFT_MAX_BRIGHTNESS - CONFIG_TFT_MIN_BRIGHTNESS) * tft_br_setting.val) / tft_br_param.max));
}

bool setTftBrightness(uint8_t newVal)
{
    if (setSetting(&tft_br_setting, newVal))
    {
        setTFTBacklightBrightness(getTftIntensity());
        return true;
    }
    return false;
}

//==============================================================================

int32_t getLedBrightness(void)
{
    return led_br_setting.val;
}

bool setAndSaveLedBrightness(uint8_t brightness)
{
    // TODO set LED
    return setSetting(&led_br_setting, brightness);
}

bool incLedBrightness(void)
{
    // TODO set LED
    return incSetting(&led_br_setting);
}

bool decLedBrightness(void)
{
    // TODO set LED
    return decSetting(&led_br_setting);
}

//==============================================================================

int32_t getMicGain(void)
{
    return mic_setting.val;
}

uint16_t getMicAmplitude(void)
{
    // Using a logarithmic volume control.
    const uint16_t micVols[] = {
        32, 45, 64, 90, 128, 181, 256, 362,
    };
    return micVols[mic_setting.val];
}

bool setMicGain(uint8_t newGain)
{
    // TODO set mic
    return setSetting(&mic_setting, newGain);
}

bool decMicGain(void)
{
    // TODO set mic
    return decSetting(&mic_setting);
}

bool incMicGain(void)
{
    // TODO set mic
    return incSetting(&mic_setting);
}

//==============================================================================

uint16_t getScreensaverTime(void)
{
    return scrn_sv_setting.val;
}

bool incScreensaverTime(void)
{
    return incSetting(&scrn_sv_setting);
}

bool decScreensaverTime(void)
{
    return decSetting(&scrn_sv_setting);
}

//==============================================================================

colorchordMode_t getColorchordMode(void)
{
    return cc_mode_setting.val;
}

bool setColorchordMode(colorchordMode_t newMode)
{
    return setSetting(&cc_mode_setting, newMode);
}

//==============================================================================

bool getTestModePassed(void)
{
    return test_setting.val;
}

bool setTestModePassed(bool status)
{
    return setSetting(&test_setting, status);
}
