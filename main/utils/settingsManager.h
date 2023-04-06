/*! \file settingsManager.h
 *
 * \section settingsManager_design Design Philosophy
 *
 * TODO
 *
 * \section settingsManager_usage Usage
 *
 * TODO
 *
 * \section settingsManager_example Example
 *
 * \code{.c}
 * TODO
 * \endcode
 */

#ifndef _SETTINGS_MANAGER_H_
#define _SETTINGS_MANAGER_H_

#include <stdint.h>
#include <stdbool.h>

#include "mode_colorchord_types.h"

typedef struct
{
    const int32_t min;
    const int32_t max;
    const int32_t def;
    const char* key;
} settingParam_t;

//==============================================================================
// Function Prototypes
//==============================================================================

void readAllSettings(void);

uint16_t getBgmVolumeSetting(void);
const settingParam_t* getBgmVolumeSettingBounds(void);
uint16_t getBgmVolumeLevelSetting(void);
bool setBgmVolumeSetting(uint16_t);

uint16_t getSfxVolumeSetting(void);
const settingParam_t* getSfxVolumeSettingBounds(void);
uint16_t getSfxVolumeLevelSetting(void);
bool setSfxVolumeSetting(uint16_t);

uint8_t getTftBrightnessSetting(void);
uint8_t getTFTBacklightBrightnessSetting(void);
const settingParam_t* getTftBrightnessSettingBounds(void);
bool setTftBrightnessSetting(uint8_t newVal);

uint8_t getLedBrightnessSetting(void);
const settingParam_t* getLedBrightnessSettingBounds(void);
bool setLedBrightnessSetting(uint8_t brightness);
bool incLedBrightnessSetting(void);
bool decLedBrightnessSetting(void);

uint8_t getMicGainSetting(void);
const settingParam_t* getMicGainSettingBounds(void);
uint16_t getMicGainMultiplierSetting(void);
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

#endif