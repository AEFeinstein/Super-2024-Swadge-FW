//==============================================================================
// Includes
//==============================================================================

#include "hdw-nvs.h"
#include "hdw-bzr.h"
#include "hdw-tft.h"
#include "settingsManager.h"

//==============================================================================
// Defines
//==============================================================================

#define MAX_LED_BRIGHTNESS  7
#define MAX_TFT_BRIGHTNESS  7
#define MAX_MIC_GAIN        7
#define MAX_SCREENSAVER     6
#define DEFAULT_SCREENSAVER 2

//==============================================================================
// Variables
//==============================================================================

const char KEY_MUTE_BGM[]    = "mutebgm";
const char KEY_MUTE_SFX[]    = "mutesfx";
const char KEY_TFT_BRIGHT[]  = "bright";
const char KEY_MIC[]         = "mic";
const char KEY_LED_BRIGHT[]  = "led";
const char KEY_CC_MODE[]     = "ccm";
const char KEY_TEST[]        = "test";
const char KEY_SCREENSAVER[] = "scrnsvrtm";

static struct
{
    bool bgmIsMuted;
    bool sfxIsMuted;
    int32_t tftBrightness;
    int32_t ledBrightness;
    int32_t micGain;
    int32_t screensaverTime;
    colorchordMode_t colorchordMode;

    bool bgmIsMutedRead;
    bool sfxIsMutedRead;
    bool tftBrightnessRead;
    bool ledBrightnessRead;
    bool micGainRead;
    bool screensaverTimeRead;
    bool colorchordModeRead;
} settingsRam;

// Static function prototypes
static bool setScreensaverSetting(int32_t setting);
static int32_t getScreensaverSetting(void);

//==============================================================================
// Functions
//==============================================================================

/**
 * @return true if the buzzer is muted, false if it is not
 */
bool getBgmIsMuted(void)
{
    // If it's already in RAM, return that
    if (settingsRam.bgmIsMutedRead)
    {
        return settingsRam.bgmIsMuted;
    }
    else
    {
        int32_t muted = false;
        // Try reading the value
        if (false == readNvs32(KEY_MUTE_BGM, &muted))
        {
            // Value didn't exist, so write the default
            setBgmIsMuted(muted);
        }
        // Save to RAM as well
        settingsRam.bgmIsMuted     = muted;
        settingsRam.bgmIsMutedRead = true;
        // Return the read value
        return (bool)muted;
    }
}

/**
 * Set if the buzzer is muted or not
 *
 * @param isMuted true to mute the buzzer, false to turn it on
 * @return true if the setting was saved, false if it was not
 */
bool setBgmIsMuted(bool isMuted)
{
    // Write the value
    settingsRam.bgmIsMuted     = isMuted;
    settingsRam.bgmIsMutedRead = true;
    bzrSetBgmIsMuted(isMuted);
    return writeNvs32(KEY_MUTE_BGM, isMuted);
}

/**
 * @return true if the buzzer is muted, false if it is not
 */
bool getSfxIsMuted(void)
{
    // If it's already in RAM, return that
    if (settingsRam.sfxIsMutedRead)
    {
        return settingsRam.sfxIsMuted;
    }
    else
    {
        int32_t muted = false;
        // Try reading the value
        if (false == readNvs32(KEY_MUTE_SFX, &muted))
        {
            // Value didn't exist, so write the default
            setSfxIsMuted(muted);
        }
        // Save to RAM as well
        settingsRam.sfxIsMuted     = muted;
        settingsRam.sfxIsMutedRead = true;
        // Return the read value
        return (bool)muted;
    }
}

/**
 * Set if the buzzer is muted or not
 *
 * @param isMuted true to mute the buzzer, false to turn it on
 * @return true if the setting was saved, false if it was not
 */
bool setSfxIsMuted(bool isMuted)
{
    // Write the value
    settingsRam.sfxIsMuted     = isMuted;
    settingsRam.sfxIsMutedRead = true;
    bzrSetSfxIsMuted(isMuted);
    return writeNvs32(KEY_MUTE_SFX, isMuted);
}

/**
 * @return The tftBrightness level for the TFT, 0-9
 */
int32_t getTftBrightness(void)
{
    // If it's already in RAM, return that
    if (settingsRam.tftBrightnessRead)
    {
        return settingsRam.tftBrightness;
    }
    else
    {
        int32_t tftBrightness = 5;
        // Try reading the value
        if (false == readNvs32(KEY_TFT_BRIGHT, &tftBrightness))
        {
            // Value didn't exist, so write the default
            writeNvs32(KEY_TFT_BRIGHT, tftBrightness);
        }
        // Save to RAM as well
        settingsRam.tftBrightness     = tftBrightness;
        settingsRam.tftBrightnessRead = true;
        // Return the read value
        return tftBrightness;
    }
}

/**
 * Increment the brightness level for the TFT
 *
 * @return true if the setting was saved, false if it was not
 */
bool incTftBrightness(void)
{
    // Increment the value
    uint8_t brightness = (getTftBrightness() + 1) % (MAX_TFT_BRIGHTNESS + 1);

    // Write the value
    bool retVal = writeNvs32(KEY_TFT_BRIGHT, brightness);

    // Save to RAM as well
    settingsRam.tftBrightness     = brightness;
    settingsRam.tftBrightnessRead = true;
    // Set the brightness
    setTFTBacklightBrightness(getTftIntensity());

    return retVal;
}

/**
 * Decrement the brightness level for the TFT
 *
 * @return true if the setting was saved, false if it was not
 */
bool decTftBrightness(void)
{
    // Decrement the value
    uint8_t brightness = getTftBrightness();
    if (brightness == 0)
    {
        brightness = MAX_TFT_BRIGHTNESS;
    }
    else
    {
        brightness--;
    }

    // Write the value
    bool retVal = writeNvs32(KEY_TFT_BRIGHT, brightness);

    // Save to RAM as well
    settingsRam.tftBrightness     = brightness;
    settingsRam.tftBrightnessRead = true;
    // Set the brightness
    setTFTBacklightBrightness(getTftIntensity());

    return retVal;
}

/**
 * @brief Get the Tft Intensity, passed into setTFTBacklightBrightness()
 *
 * @return The TFT intensity
 */
uint8_t getTftIntensity(void)
{
    return (CONFIG_TFT_MIN_BRIGHTNESS
            + (((CONFIG_TFT_MAX_BRIGHTNESS - CONFIG_TFT_MIN_BRIGHTNESS) * getTftBrightness()) / MAX_TFT_BRIGHTNESS));
}

/**
 * @return The Brightness level for the LED, 0-7
 */
int32_t getLedBrightness(void)
{
    // If it's already in RAM, return that
    if (settingsRam.ledBrightnessRead)
    {
        return settingsRam.ledBrightness;
    }
    else
    {
        int32_t ledBrightness = 5;
        // Try reading the value
        if (false == readNvs32(KEY_LED_BRIGHT, &ledBrightness))
        {
            // Value didn't exist, so write the default
            writeNvs32(KEY_LED_BRIGHT, ledBrightness);
        }
        // Save to RAM as well
        settingsRam.ledBrightness     = ledBrightness;
        settingsRam.ledBrightnessRead = true;
        // Return the read value
        return ledBrightness;
    }
}

/**
 * Set the brightness level for the LED, saving to RAM and NVS
 *
 * @param brightness 0 (off) to 7 (max bright)
 * @return true if the setting was saved, false if it was not
 */
bool setAndSaveLedBrightness(uint8_t brightness)
{
    // Bound
    if (brightness > MAX_LED_BRIGHTNESS)
    {
        brightness = MAX_LED_BRIGHTNESS;
    }
    // Adjust the LEDs
    // TODO implement this!!!
    // setLedBrightness(brightness);
    // Save to RAM as well
    settingsRam.ledBrightness     = brightness;
    settingsRam.ledBrightnessRead = true;
    // Write the value
    return writeNvs32(KEY_LED_BRIGHT, brightness);
}

/**
 * Increment the brightness level for the LED
 *
 * @return true if the setting was saved, false if it was not
 */
bool incLedBrightness(void)
{
    // Increment the value
    uint8_t brightness = (getLedBrightness() + 1) % (MAX_LED_BRIGHTNESS + 1);
    // Adjust the LEDs
    // TODO implement this!!!
    // setLedBrightness(brightness);
    // Save to RAM as well
    settingsRam.ledBrightness     = brightness;
    settingsRam.ledBrightnessRead = true;
    // Write the value
    return writeNvs32(KEY_LED_BRIGHT, brightness);
}

/**
 * Decrement the brightness level for the LED
 *
 * @return true if the setting was saved, false if it was not
 */
bool decLedBrightness(void)
{
    // Decrement the value
    uint8_t brightness = getLedBrightness();
    if (brightness == 0)
    {
        brightness = MAX_LED_BRIGHTNESS;
    }
    else
    {
        brightness--;
    }
    // Adjust the LEDs
    // TODO implement this!!!
    // setLedBrightness(brightness);
    // Save to RAM as well
    settingsRam.ledBrightness     = brightness;
    settingsRam.ledBrightnessRead = true;
    // Write the value
    return writeNvs32(KEY_LED_BRIGHT, brightness);
}

/**
 * @return The gain setting for the microphone, 0-9
 */
int32_t getMicGain(void)
{
    // If it's already in RAM, return that
    if (settingsRam.micGainRead)
    {
        return settingsRam.micGain;
    }
    else
    {
        int32_t micGain = MAX_MIC_GAIN;
        // Try reading the value
        if (false == readNvs32(KEY_MIC, &micGain))
        {
            // Value didn't exist, so write the default
            writeNvs32(KEY_MIC, micGain);
        }
        // Save to RAM as well
        settingsRam.micGain     = micGain;
        settingsRam.micGainRead = true;
        // Return the read value
        return micGain;
    }
}

/**
 * Increment the microphone gain setting
 *
 * @param newGain The new gain to set, 0-7
 * @return true if the setting was written, false if it was not
 */
bool setMicGain(uint8_t newGain)
{
    // Increment the value
    if (newGain > MAX_MIC_GAIN)
    {
        newGain = MAX_MIC_GAIN;
    }
    // Save to RAM as well
    settingsRam.micGain     = newGain;
    settingsRam.micGainRead = true;
    // Write the value
    return writeNvs32(KEY_MIC, newGain);
}

/**
 * Increment the microphone gain setting
 *
 * @return true if the setting was written, false if it was not
 */
bool incMicGain(void)
{
    // Increment the value
    uint8_t newGain = (getMicGain() + 1) % (MAX_MIC_GAIN + 1);
    // Save to RAM as well
    settingsRam.micGain     = newGain;
    settingsRam.micGainRead = true;
    // Write the value
    return writeNvs32(KEY_MIC, newGain);
}

/**
 * Decrement the microphone gain setting
 *
 * @return true if the setting was written, false if it was not
 */
bool decMicGain(void)
{
    // Decrement the value
    uint8_t newGain = getMicGain();
    if (newGain == 0)
    {
        newGain = MAX_MIC_GAIN;
    }
    else
    {
        newGain--;
    }
    // Save to RAM as well
    settingsRam.micGain     = newGain;
    settingsRam.micGainRead = true;
    // Write the value
    return writeNvs32(KEY_MIC, newGain);
}

/**
 * @return The amplitude for the microphone when reading samples
 */
uint16_t getMicAmplitude(void)
{
    // Using a logarithmic volume control.
    const uint16_t micVols[] = {
        32, 45, 64, 90, 128, 181, 256, 362,
    };
    return micVols[getMicGain()];
}

static bool setScreensaverSetting(int32_t setting)
{
    if (setting > MAX_SCREENSAVER)
    {
        setting = MAX_SCREENSAVER;
    }
    else if (setting < 0)
    {
        setting = 0;
    }

    settingsRam.screensaverTime     = setting;
    settingsRam.screensaverTimeRead = true;

    return writeNvs32(KEY_SCREENSAVER, setting);
}

static int32_t getScreensaverSetting(void)
{
    if (settingsRam.screensaverTimeRead)
    {
        return settingsRam.screensaverTime;
    }
    else
    {
        int32_t screensaverSetting = DEFAULT_SCREENSAVER;
        if (!readNvs32(KEY_SCREENSAVER, &screensaverSetting))
        {
            setScreensaverSetting(screensaverSetting);
        }

        settingsRam.screensaverTime     = screensaverSetting;
        settingsRam.screensaverTimeRead = true;

        return settingsRam.screensaverTime;
    }
}

/**
 * @return The number of idle seconds before the screensaver activates, or 0 if it's disabled
 */
uint16_t getScreensaverTime(void)
{
    const uint16_t screensaverTimes[] = {
        0, 10, 20, 30, 60, 120, 300,
    };

    return screensaverTimes[getScreensaverSetting()];
}

bool incScreensaverTime(void)
{
    int32_t screensaverSetting = getScreensaverSetting();
    if (screensaverSetting >= MAX_SCREENSAVER)
    {
        screensaverSetting = 0;
    }
    else
    {
        screensaverSetting++;
    }

    return setScreensaverSetting(screensaverSetting);
}

bool decScreensaverTime(void)
{
    int32_t screensaverSetting = getScreensaverSetting();
    if (screensaverSetting <= 0)
    {
        screensaverSetting = MAX_SCREENSAVER;
    }
    else
    {
        screensaverSetting--;
    }

    return setScreensaverSetting(screensaverSetting);
}

/**
 * @return The colorchordMode level for the TFT, LINEAR_LEDS or ALL_SAME_LEDS
 */
colorchordMode_t getColorchordMode(void)
{
    // If it's already in RAM, return that
    if (settingsRam.colorchordModeRead)
    {
        return settingsRam.colorchordMode;
    }
    else
    {
        colorchordMode_t colorchordMode = ALL_SAME_LEDS;
        // Try reading the value
        if (false == readNvs32(KEY_CC_MODE, (int32_t*)&colorchordMode))
        {
            // Value didn't exist, so write the default
            setColorchordMode(colorchordMode);
        }
        // Save to RAM as well
        settingsRam.colorchordMode     = colorchordMode;
        settingsRam.colorchordModeRead = true;
        // Return the read value
        return colorchordMode;
    }
}

/**
 * Set the colorchordMode
 *
 * @param colorchordMode The colorchordMode, ALL_SAME_LEDS or LINEAR_LEDS
 * @return true if the setting was saved, false if it was not
 */
bool setColorchordMode(colorchordMode_t colorchordMode)
{
    // Bound the value, just in case
    if ((ALL_SAME_LEDS != colorchordMode) && (LINEAR_LEDS != colorchordMode))
    {
        colorchordMode = ALL_SAME_LEDS;
    }

    // Save to RAM as well
    settingsRam.colorchordMode     = colorchordMode;
    settingsRam.colorchordModeRead = true;
    // Write the value
    return writeNvs32(KEY_CC_MODE, colorchordMode);
}

/**
 * @return true if the test mode passed
 */
bool getTestModePassed(void)
{
    int32_t pass = false;
    // Try reading the value
    if (false == readNvs32(KEY_TEST, &pass))
    {
        // Value didn't exist, so write the default
        setTestModePassed(false);
    }
    // Return the read value
    return (bool)pass;
}

/**
 * Set the new test mode pass status
 *
 * @param status true if the test passed, false if it did not
 * @return true if the setting was saved, false if it was not
 */
bool setTestModePassed(bool status)
{
    // Write the value
    return writeNvs32(KEY_TEST, status);
}
