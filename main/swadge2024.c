/*! \mainpage Swadge 2024
 *
 * \section intro_sec Introduction
 *
 * So you're here to write some Swadge code, eh?
 *
 * \section espressif_doc Espressif Documentation
 *
 * The Swadge uses an ESP32-S2 microcontroller with firmware built on IDF 5.0. The goal of this project is to enable
 * developers to write modes and games for the Swadge without going too deep into Espressif's API. However, if you're
 * doing system development or writing a mdoe that requires a specific hardware peripheral, this Espressif documentation
 * is useful:
 * - <a href="https://docs.espressif.com/projects/esp-idf/en/v5.0/esp32s2/api-reference/index.html">ESP-IDF API
 * Reference</a>
 * - <a href="https://www.espressif.com/sites/default/files/documentation/esp32-s2_datasheet_en.pdf">ESP32-­S2 Series
 * Datasheet</a>
 * - <a
 * href="https://www.espressif.com/sites/default/files/documentation/esp32-s2_technical_reference_manual_en.pdf">ESP32­-S2
 * Technical Reference Manual</a>
 *
 * \section install_sec Installation
 *
 * \subsection linux_env Linux Environment
 *
 * Penguins
 *
 * \subsection win_env Windows Environment
 *
 * DEVELOPERS! DEVELOPERS! DEVELOPERS!
 *
 * \section hw_api Hardware APIs
 *
 * Physical things you interact with
 *
 * - hdw-btn.c: Learn how to use both push and touch button input!
 * - hdw-tft.c: Learn how to use the TFT!
 * - hdw-bzr.c: Learn how to use the buzzer!
 * - hdw-accel.c: Learn how to use the accelerometer!
 * - hdw-led.c: Learn how to use the LEDs!
 * - hdw-mic.c: Learn how to use the microphone!
 * - hdw-temperature.c: Learn how to use the temperature sensor!
 * - hdw-usb.c: Learn how to be a USB HID Gamepad!
 * - hdw-esp-now.c, p2pconnection.c: Learn how to communicate with other Swadges!
 *
 * \section sw_api Software APIs
 *
 * Digital things you interact with
 *
 * - hdw-spiffs.c: Learn how to load and use assets from the SPIFFS partition!
 * - hdw-nvs.c: Learn how to save and load persistent runtime data!
 *
 * \section gr_api Graphics APIs
 *
 * Be an artist!
 *
 * - font.c: Learn how to draw text to the screen!
 * - bresenham.c: Learn how to draw shapes to the screen!
 * - cndraw.c: Learn how to draw differenter shapes to the screen!
 * - fill.c: Learn how to fill areas on the screen!
 *
 * \section oth_api Other Useful APIs
 *
 * Maybe I should organize these better
 *
 * - color_utils.c: Do stuff with color, both LED and TFT
 * - linked_list.c: A basic data structure
 */

#include <stdio.h>

#include <esp_timer.h>
#include <esp_log.h>

#include "hdw-btn.h"
#include "hdw-tft.h"
#include "hdw-bzr.h"
#include "hdw-accel.h"
#include "hdw-led.h"
#include "hdw-mic.h"
#include "hdw-temperature.h"
#include "hdw-spiffs.h"
#include "hdw-usb.h"
#include "hdw-esp-now.h"
#include "hdw-nvs.h"

#include "font.h"
#include "bresenham.h"
#include "cndraw.h"
#include "fill.h"
#include "wsg.h"

#include "macros.h"
#include "swadgeMode.h"
#include "demoMode.h"

swadgeMode_t* modes[] = {
    &demoMode,
};
swadgeMode_t* cSwadgeMode;

/// 25 FPS by default
static uint32_t frameRateUs = 40000;

static void swadgeModeEspNowRecvCb(const uint8_t* mac_addr, const char* data, uint8_t len, int8_t rssi);
static void swadgeModeEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status);

/**
 * @brief TODO doxygen something
 *
 */
void app_main(void)
{
    cSwadgeMode = modes[0];

    // Init timers
    esp_timer_init();

    // Init SPIFFS file system
    initSpiffs();

    // Init NVS
    initNvs(true);

    // Init buttons and touch pads
    gpio_num_t pushButtons[] = {
        GPIO_NUM_0,  // Up
        GPIO_NUM_4,  // Down
        GPIO_NUM_2,  // Left
        GPIO_NUM_1,  // Right
        GPIO_NUM_16, // A
        GPIO_NUM_15, // B
        GPIO_NUM_8,  // Start
        GPIO_NUM_5   // Select
    };
    touch_pad_t touchPads[] = {
        TOUCH_PAD_NUM9,  // GPIO_NUM_9
        TOUCH_PAD_NUM10, // GPIO_NUM_10
        TOUCH_PAD_NUM11, // GPIO_NUM_11
        TOUCH_PAD_NUM12, // GPIO_NUM_12
        TOUCH_PAD_NUM13  // GPIO_NUM_13
    };
    initButtons(pushButtons, sizeof(pushButtons) / sizeof(pushButtons[0]), touchPads,
                sizeof(touchPads) / sizeof(touchPads[0]));

    // Init mic if it is used by the mode
    if (NULL != cSwadgeMode->fnAudioCallback)
    {
        initMic(GPIO_NUM_7);
        startMic();
    }

    // Init buzzer
    initBuzzer(GPIO_NUM_40, LEDC_TIMER_0, LEDC_CHANNEL_0, false, false);

    // Init TFT, use a different LEDC channel than buzzer
    initTFT(SPI2_HOST,
            GPIO_NUM_36,     // sclk
            GPIO_NUM_37,     // mosi
            GPIO_NUM_21,     // dc
            GPIO_NUM_34,     // cs
            GPIO_NUM_38,     // rst
            GPIO_NUM_35,     // backlight
            true,            // PWM backlight
            LEDC_CHANNEL_1); // Channel to use for PWM backlight

    // Initialize the RGB LEDs
    initLeds(GPIO_NUM_39);

    // Init USB if not overridden by the mode
    if (false == cSwadgeMode->overrideUsb)
    {
        initUsb();
    }

    // Init esp-now if requested by the mode
    if ((ESP_NOW == cSwadgeMode->wifiMode) || (ESP_NOW_IMMEDIATE == cSwadgeMode->wifiMode))
    {
        initEspNow(&swadgeModeEspNowRecvCb, &swadgeModeEspNowSendCb, GPIO_NUM_NC, GPIO_NUM_NC, UART_NUM_MAX, ESP_NOW);
    }

    // Init accelerometer
    qma7981_init(I2C_NUM_0,
                 GPIO_NUM_3,  // SDA
                 GPIO_NUM_41, // SCL
                 GPIO_PULLUP_DISABLE, 1000000, QMA_RANGE_2G, QMA_BANDWIDTH_1024_HZ);

    // Init the temperature sensor
    initTemperatureSensor();

    // Initialize the loop timer
    static int64_t tLastLoopUs = 0;
    tLastLoopUs                = esp_timer_get_time();

    // Initialize the swadge mode
    if (NULL != cSwadgeMode->fnEnterMode)
    {
        cSwadgeMode->fnEnterMode();
    }

    // Run the main loop, forever!
    while (true)
    {
        // Track the elapsed time between loop calls
        int64_t tNowUs     = esp_timer_get_time();
        int64_t tElapsedUs = tNowUs - tLastLoopUs;
        tLastLoopUs        = tNowUs;

        // Process ADC samples
        if (NULL != cSwadgeMode->fnAudioCallback)
        {
            uint16_t adcSamps[ADC_READ_LEN / SOC_ADC_DIGI_RESULT_BYTES];
            uint32_t sampleCnt = 0;
            while (0 < (sampleCnt = loopMic(adcSamps, ARRAY_SIZE(adcSamps))))
            {
                // Run all samples through an IIR filter
                for (uint32_t i = 0; i < sampleCnt; i++)
                {
                    static uint32_t samp_iir = 0;

                    int32_t sample  = adcSamps[i];
                    samp_iir        = samp_iir - (samp_iir >> 9) + sample;
                    int32_t newsamp = (sample - (samp_iir >> 9));
                    newsamp         = CLAMP(newsamp, -32768, 32767);
                    adcSamps[i]     = newsamp;
                }
                cSwadgeMode->fnAudioCallback(adcSamps, sampleCnt);
            }
        }

        // Only draw to the TFT every frameRateUs
        static uint64_t tAccumDraw = 0;
        tAccumDraw += tElapsedUs;
        if (tAccumDraw >= frameRateUs)
        {
            // Decrement the accumulation
            tAccumDraw -= frameRateUs;

            // Call the mode's main loop
            if (NULL != cSwadgeMode->fnMainLoop)
            {
                // Keep track of the time between main loop calls
                static uint64_t tLastMainLoopCall = 0;
                if (0 == tLastMainLoopCall)
                {
                    tLastMainLoopCall = tNowUs;
                }
                cSwadgeMode->fnMainLoop(tNowUs - tLastMainLoopCall);
                tLastMainLoopCall = tNowUs;
            }

            // Draw to the TFT
            drawDisplayTft(cSwadgeMode->fnBackgroundDrawCallback);
        }

        // Yield to let the rest of the RTOS run
        taskYIELD();
    }

    // Deinitialize the swadge mode
    if (NULL != cSwadgeMode->fnExitMode)
    {
        cSwadgeMode->fnExitMode();
    }
}

/**
 * Callback from ESP NOW to the current Swadge mode whenever a packet is
 * received. It routes through user_main.c, which knows what the current mode is
 *
 * @param mac_addr
 * @param data
 * @param len
 * @param rssi
 */
static void swadgeModeEspNowRecvCb(const uint8_t* mac_addr, const char* data, uint8_t len, int8_t rssi)
{
    if (NULL != cSwadgeMode->fnEspNowRecvCb)
    {
        cSwadgeMode->fnEspNowRecvCb(mac_addr, data, len, rssi);
    }
}

/**
 * Callback from ESP NOW to the current Swadge mode whenever a packet is sent
 * It routes through user_main.c, which knows what the current mode is
 *
 * @param mac_addr
 * @param status
 */
static void swadgeModeEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status)
{
    if (NULL != cSwadgeMode->fnEspNowSendCb)
    {
        cSwadgeMode->fnEspNowSendCb(mac_addr, status);
    }
}
