//==============================================================================
// Includes
//==============================================================================

#include "powerMeasure.h"

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
    menu_t* menu;
    menuManiaRenderer_t* renderer;
    font_t ibm;
    bool isMeasuring;
} powerMeasure_t;

//==============================================================================
// Function Declarations
//==============================================================================

void powerMeasureEnterMode(void);
void powerMeasureExitMode(void);
void powerMeasureMainLoop(int64_t elapsedUs);
void powerMeasureEspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi);
void powerMeasureEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status);
void powerMeasureMenuCb(const char* label, bool selected, uint32_t value);
void powerMeasureAudioCallback(uint16_t* samples, uint32_t sampleCnt);

void startLedMeasurement(void);
void turnPeripheralsOff(void);
void turnPeripheralsOn(void);

//==============================================================================
// Constant Variables
//==============================================================================

const char powerMeasureName[] = "Power Measure";

const char pmMenuLeds[]   = "LEDs";
const char pmMenuTft[]    = "TFT";
const char pmMenuSpk[]    = "Speaker";
const char pmMenuAccel[]  = "Accelerometer";
const char pmMenuMic[]    = "Microphone";
const char pmMenuEspNow[] = "ESP-NOW TX";
const char pmMenuLSleep[] = "Light Sleep";
const char pmMenuDSleep[] = "Deep Sleep";

//==============================================================================
// Variables
//==============================================================================

swadgeMode_t powerMeasureMode = {
    .modeName                 = powerMeasureName,
    .wifiMode                 = ESP_NOW,
    .overrideUsb              = false,
    .usesAccelerometer        = true,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = powerMeasureEnterMode,
    .fnExitMode               = powerMeasureExitMode,
    .fnMainLoop               = powerMeasureMainLoop,
    .fnAudioCallback          = powerMeasureAudioCallback,
    .fnBackgroundDrawCallback = NULL,
    .fnEspNowRecvCb           = powerMeasureEspNowRecvCb,
    .fnEspNowSendCb           = powerMeasureEspNowSendCb,
    .fnAdvancedUSB            = NULL,
    .fnDacCb                  = NULL,
};

static powerMeasure_t* pm;

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief This function is called when this mode is started. It should initialize variables and start the mode.
 */
void powerMeasureEnterMode(void)
{
    // Allocate memory
    pm = heap_caps_calloc(1, sizeof(powerMeasure_t), MALLOC_CAP_8BIT);

    // Load fonts
    loadFont("ibm_vga8.font", &pm->ibm, true);

    // Turn off LEDs
    led_t leds[CONFIG_NUM_LEDS] = {0};
    setLeds(leds, CONFIG_NUM_LEDS);

    // Setup menu
    pm->renderer = initMenuManiaRenderer(NULL, NULL, NULL);
    setManiaLedsOn(pm->renderer, false);
    pm->menu = initMenu(powerMeasureMode.modeName, powerMeasureMenuCb);

    addSingleItemToMenu(pm->menu, pmMenuLeds);
    addSingleItemToMenu(pm->menu, pmMenuTft);
    addSingleItemToMenu(pm->menu, pmMenuSpk);
    addSingleItemToMenu(pm->menu, pmMenuAccel);
    addSingleItemToMenu(pm->menu, pmMenuMic);
    addSingleItemToMenu(pm->menu, pmMenuEspNow);
    addSingleItemToMenu(pm->menu, pmMenuLSleep);
    addSingleItemToMenu(pm->menu, pmMenuDSleep);
}

/**
 * @brief This function is called when the mode is exited. It should free any allocated memory.
 */
void powerMeasureExitMode(void)
{
    // Free font
    freeFont(&pm->ibm);

    // Deinit menu
    deinitMenuManiaRenderer(pm->renderer);
    deinitMenu(pm->menu);

    // Free everything
    heap_caps_free(pm);
}

/**
 * @brief This function is called from the main loop. It's pretty quick, but the timing may be inconsistent.
 *
 * @param elapsedUs The time elapsed since the last time this function was called. Use this value to determine when
 * it's time to do things
 */
void powerMeasureMainLoop(int64_t elapsedUs)
{
    // Process button events
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        pm->menu = menuButton(pm->menu, evt);
    }

    drawMenuMania(pm->menu, pm->renderer, elapsedUs);
}

/**
 * @brief This function is called whenever an ESP-NOW packet is received.
 *
 * @param esp_now_info Information about the transmission, including The MAC addresses
 * @param data     A pointer to the data received
 * @param len      The length of the data received
 * @param rssi     The RSSI for this packet, from 1 (weak) to ~90 (touching)
 */
void powerMeasureEspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi)
{
    return;
}

/**
 * @brief This function is called whenever an ESP-NOW packet is sent. It is just a status callback whether or not
 * the packet was actually sent. This will be called after calling espNowSend().
 *
 * @param mac_addr The MAC address which the data was sent to
 * @param status   The status of the transmission
 */
void powerMeasureEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status)
{
    // Do nothing, don't care about TX status
    return;
}

/**
 * @brief Menu callback function
 *
 * @param label The menu item that was triggered
 * @param selected True if the item was selected, false if it was moved to
 * @param value Unused
 */
void powerMeasureMenuCb(const char* label, bool selected, uint32_t value)
{
    // If the menu item was selected
    if (selected)
    {
        if (pm->isMeasuring)
        {
            turnPeripheralsOn();
            pm->isMeasuring = false;
        }
        else if (label == pmMenuLeds)
        {
            startLedMeasurement();
        }
        else if (label == pmMenuTft)
        {
        }
        else if (label == pmMenuSpk)
        {
        }
        else if (label == pmMenuMic)
        {
        }
        else if (label == pmMenuAccel)
        {
        }
        else if (label == pmMenuEspNow)
        {
        }
        else if (label == pmMenuLSleep)
        {
        }
        else if (label == pmMenuDSleep)
        {
        }
        return;
    }
}

/**
 * @brief This function is called whenever audio samples are read from the microphone (ADC) and are ready for
 * processing. Samples are read at 8KHz. If this function is not NULL, then readBattmon() will not work
 *
 * @param samples A pointer to 12 bit audio samples
 * @param sampleCnt The number of samples read
 */
void powerMeasureAudioCallback(uint16_t* samples, uint32_t sampleCnt)
{
    return;
}

/**
 * @brief Turn everything off
 *
 */
void turnPeripheralsOff(void)
{
    // Turn off LEDs
    led_t leds[CONFIG_NUM_LEDS] = {0};
    setLeds(leds, CONFIG_NUM_LEDS);

    // Turn off TFT
    tftEnterSleepMode(true);

    // Stop the microphone
    stopMic();
    deinitMic();

    // Stop battery monitoring
    deinitBattmon();

    // Stop the speaker
    globalMidiPlayerStop(true);
    deinitGlobalMidiPlayer();
    setDacShutdown(true);
    deinitDac();

    // Power down the accelerometer
    accelPowerDown();

    // Touchpad
    deinitTouchSensor();
}

/**
 * @brief Turn everything on
 *
 */
void turnPeripheralsOn(void)
{
    // Turn on LEDs
    led_t leds[CONFIG_NUM_LEDS] = {0};
    memset(leds, 0x10, sizeof(led_t) * CONFIG_NUM_LEDS);
    setLeds(leds, CONFIG_NUM_LEDS);

    // Turn on TFT
    tftEnterSleepMode(false);

    // Turn on speaker
    switchToSpeaker();

    // Power up the accelerometer
    accelSetRegistersAndReset();

    // TODO Touchpad
    // initTouchSensor();
}

/**
 * @brief Start a sequence to measure power while different LEDs are on
 *
 */
void startLedMeasurement(void)
{
    pm->isMeasuring = true;
    turnPeripheralsOff();
}