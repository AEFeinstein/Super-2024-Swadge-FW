/*! \mainpage Swadge 2024
 *
 * Generated on \showdate "%A, %B %-d, %H:%M:%S"
 *
 * \section intro_sec Introduction
 *
 * Welcome to the Swadge 2024 API documentation! Here you will find information on how to use all hardware features of a
 * Swadge and write a Swadge mode. A Swadge is <a href="https://www.magfest.org/">Magfest's</a> electronic swag-badges.
 * They play games and make music and shine bright and do all sorts of cool things.
 *
 * The Swadge GitHub repository can be found at <a
 * href="https://github.com/AEFeinstein/Super-2024-Swadge-FW">https://github.com/AEFeinstein/Super-2024-Swadge-FW</a>.
 * The corresponding hardware repository for the Super Magfest 2024 Swadge can be found at <a
 * href="https://github.com/AEFeinstein/Super-2024-Swadge-HW">https://github.com/AEFeinstein/Super-2024-Swadge-HW</a>.
 *
 * This is living documentation, so if you notice that something is incorrect or incomplete, please fix or complete it,
 * and submit a pull request!
 *
 * Most discussions happen in the Magfest Slack, in the \#circuitboards channel. If you are interested in joining and
 * contributing to this project, email circuitboards@magfest.org.
 *
 * General Swadge design principles <a
 * href="https://docs.google.com/document/d/1TzzatyRWp9t26YWF3qUlOg7NFgmsueL-0suqyDT98IE/edit">can be found here</a>.
 *
 * \section start Where to Start
 *
 * If you're just starting Swadge development, you're already at the right place to start! Here's a good sequence of
 * pages to read from here.
 *
 * -# First, follow the guide to \ref setup. This will walk you through setting up the toolchain and compiling the
 * firmware and emulator.
 * -# Next, read about the basics of a Swadge Mode at \ref swadge2024.h.
 * -# Once you understand the basics of a Swadge Mode, check out the \ref swadge_mode_example to see a simple mode in
 * action.
 * -# After you grasp the example, you can go deeper and read the full \ref apis to understand the full capability of
 * the Swadge firmware.
 * -# When you're ready to make a contribution, read the \ref contribution_guide first to see how to do it in the most
 * productive way.
 * -# If you want to bring a mode forward from last year's Swadge, take a look at \ref porting.
 * -# Finally, if you want to do lower level or \c component programming, read the \ref espressif_doc to understand the
 * full capability of the ESP32-S2 chip.
 *
 * \section swadge_mode_example Swadge Mode Example
 *
 * The <a
 * href="https://github.com/AEFeinstein/Super-2024-Swadge-FW/blob/fdce1624625a5888a1866c84c56ac994a58ae1cb/main/modes/pong/pong.c">Pong
 * mode</a> is written to be a relatively simple example of a Swadge mode. It is well commented, demonstrates a handful
 * of features, and uses good design patterns. Look out for things like:
 * - How the \c pong_t struct contains all variables the mode needs, is allocated when the mode is entered, and is freed
 * when the mode exits
 * - How immutable strings are declared <tt>static const</tt>
 * - How a \ref menu.h "menu" is initialized in \c pongEnterMode(), updated and drawn in \c pongMainLoop(), and
 * deinitialized in \c pongExitMode()
 * - How \ref font.h "fonts", \ref wsg.h "WSG", and \ref hdw-bzr.h "song" assets are loaded in \c pongEnterMode() and
 * freed in \c pongExitMode()
 *     - How fonts and WSGs are drawn in \c pongDrawField()
 *     - How background music is used in \c pongResetGame() and sound effects are used in \c pongUpdatePhysics()
 * - How a background is drawn in \c pongBackgroundDrawCallback()
 * - How LEDs are lit in \c pongDrawField()
 * - How the main loop runs depending on the screen currently being displayed in \c pongMainLoop() and \c pongGameLoop()
 * - How an accumulating timer pattern is used in \c pongFadeLeds()
 * - How user input is handled in \c pongControlPlayerPaddle()
 * - How the game state is updated in \c pongUpdatePhysics()
 *
 * \section apis API Reference
 *
 * What follows are all the APIs available to write Swadge modes. If something does not exist, and it would be
 * beneficial to multiple Swadge modes, please contribute both the firmware and API documentation. It's a team effort!
 *
 * \subsection swadge_mode_api Swadge Mode APIs
 *
 * - swadge2024.h: Write a mode. This is a good starting place
 * - menu.h and menuLogbookRenderer.h: Make and render a menu within a mode
 *
 * \subsection hw_api Hardware APIs
 *
 * - hdw-battmon.h: Learn how to check the battery voltage!
 * - hdw-btn.h: Learn how to use both push and touch button input!
 * - hdw-bzr.h: Learn how to use the buzzer!
 * - hdw-imu.h: Learn how to use the inertial measurement unit!
 * - hdw-led.h: Learn how to use the LEDs!
 * - hdw-mic.h: Learn how to use the microphone!
 * - hdw-temperature.h: Learn how to use the temperature sensor!
 * - hdw-usb.h: Learn how to be a USB HID Gamepad!
 *     - advanced_usb_control.h: Use USB for application development!
 *
 * \subsection nwk_api Network APIs
 *
 * - hdw-esp-now.h: Broadcast and receive messages. This is fast and unreliable.
 * - p2pConnection.h: Connect to another Swadge and exchange messages. This is slower and more reliable.
 *
 * \subsection sw_api Persistent Memory APIs
 *
 * - hdw-nvs.h: Learn how to save and load persistent runtime data!
 * - hdw-spiffs.h: Learn how to load and use assets from the SPIFFS partition! These file types have their own loaders:
 *     - spiffs_font.h: Load font bitmaps
 *     - spiffs_wsg.h: Load WSG images
 *     - spiffs_song.h: Load SNG songs
 *     - spiffs_json.h: Load JSON
 *     - spiffs_txt.h: Load plaintext
 *
 * \subsection gr_api Graphics APIs
 *
 * - hdw-tft.h: Learn how to use the TFT!
 * - palette.h: Learn about available colors!
 * - color_utils.h: Learn about color manipulation!
 * - fill.h: Learn how to fill areas on the screen!
 * - shapes.h: Learn how to draw shapes and curves on the screen!
 * - wsg.h: Learn how to draw sprites on the screen!
 * - font.h: Learn how to draw text on the screen!
 *
 * \subsection oth_api Other Useful APIs
 *
 * - linked_list.h: A basic data structure
 * - trigonometry.h: Fast math based on look up tables
 * - vector2d.h: Basic math for 2D vectors
 * - geometry.h: Basic math for 2D shapes, like collision checks
 * - macros.h: Convenient macros like MIN() and MAX()
 * - settingsManager.h: Set and get persistent settings for things like screen brightness
 *
 * \section espressif_doc Espressif Documentation
 *
 * The Swadge uses an ESP32-S2 micro-controller with firmware built on IDF 5.0. The goal of this project is to enable
 * developers to write modes and games for the Swadge without going too deep into Espressif's API. However, if you're
 * doing system development or writing a mode that requires a specific hardware peripheral, this Espressif documentation
 * is useful:
 * - <a href="https://docs.espressif.com/projects/esp-idf/en/v5.1.1/esp32s2/api-reference/index.html">ESP-IDF API
 * Reference</a>
 * - <a href="https://www.espressif.com/sites/default/files/documentation/esp32-s2_datasheet_en.pdf">ESP32-­S2 Series
 * Datasheet</a>
 * - <a
 * href="https://www.espressif.com/sites/default/files/documentation/esp32-s2_technical_reference_manual_en.pdf">ESP32­-S2
 * Technical Reference Manual</a>
 */

//==============================================================================
// Includes
//==============================================================================

#include <esp_system.h>
#include <esp_timer.h>
#include <esp_log.h>
#include <esp_sleep.h>
#include <rom/usb/usb_persist.h>
#include <rom/usb/chip_usb_dw_wrapper.h>
#include <soc/rtc_cntl_reg.h>

#include "advanced_usb_control.h"
#include "shapes.h"
#include "swadge2024.h"

#include "factoryTest.h"
#include "lumberjack.h"
#include "mainMenu.h"
#include "quickSettings.h"

//==============================================================================
// Defines
//==============================================================================

// Define RTC_DATA_ATTR if it doesn't exist
#ifndef RTC_DATA_ATTR
    #define RTC_DATA_ATTR
#endif

//==============================================================================
// Variables
//==============================================================================

/// @brief The current Swadge mode
static swadgeMode_t* cSwadgeMode = &mainMenuMode;

/// @brief A pending Swadge mode to use after a deep sleep
static RTC_DATA_ATTR swadgeMode_t* pendingSwadgeMode = NULL;

/// @brief Flag set if the quick settings should be shown synchronously
static bool shouldShowQuickSettings = false;
/// @brief Flag set if the quick settings should be hidden synchronously
static bool shouldHideQuickSettings = false;
/// @brief A pointer to the Swadge mode under the quick settings
static swadgeMode_t* modeBehindQuickSettings = NULL;

/// 25 FPS by default
static uint32_t frameRateUs = DEFAULT_FRAME_RATE_US;

/// @brief Timer to return to the main menu
static int64_t timeExitPressed = 0;

//==============================================================================
// Function declarations
//==============================================================================

static void swadgeModeEspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len,
                                   int8_t rssi);
static void swadgeModeEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status);
static void setSwadgeMode(void* swadgeMode);
static void initOptionalPeripherals(void);

//==============================================================================
// Functions
//==============================================================================

/**
 * The main entry point for firmware. This initializes the system and runs the main loop
 */
void app_main(void)
{
    // Init NVS. Do this first to get test mode status and crashwrap logs
    initNvs(true);

    // Read settings from NVS
    readAllSettings();

    // If test mode was passed
    if (getTestModePassedSetting())
    {
        // Show the main menu
        cSwadgeMode = &mainMenuMode;
    }
    else
    {
        // Otherwise enter test mode
        cSwadgeMode = &factoryTestMode;
    }

    // If the ESP woke from sleep, and there is a pending Swadge Mode
    if ((ESP_SLEEP_WAKEUP_TIMER == esp_sleep_get_wakeup_cause()) && (NULL != pendingSwadgeMode))
    {
        // Use the pending mode
        cSwadgeMode       = pendingSwadgeMode;
        pendingSwadgeMode = NULL;
    }

    // Init USB if not overridden by the mode. This also sets up USB printf
    if (false == cSwadgeMode->overrideUsb)
    {
        initUsb(setSwadgeMode, cSwadgeMode->fnAdvancedUSB);
    }

    // Check for prior crash info and install crash wrapper
    checkAndInstallCrashwrap();

    // Init timers
    esp_timer_init();

    // Init SPIFFS file system
    initSpiffs();

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
        TOUCH_PAD_NUM13, // GPIO_NUM_13
        TOUCH_PAD_NUM14, // GPIO_NUM_14
    };
    initButtons(pushButtons, sizeof(pushButtons) / sizeof(pushButtons[0]), touchPads,
                sizeof(touchPads) / sizeof(touchPads[0]));

    // Init buzzer. This must be called before initMic()
    initBuzzer(GPIO_NUM_40, LEDC_TIMER_0, LEDC_CHANNEL_0, //
               GPIO_NUM_42, LEDC_TIMER_1, LEDC_CHANNEL_1, getBgmVolumeSetting(), getSfxVolumeSetting());

    // Init TFT, use a different LEDC channel than buzzer
    initTFT(SPI2_HOST,
            GPIO_NUM_36,                // sclk
            GPIO_NUM_37,                // mosi
            GPIO_NUM_21,                // dc
            GPIO_NUM_34,                // cs
            GPIO_NUM_38,                // rst
            GPIO_NUM_35,                // backlight
            true,                       // PWM backlight
            LEDC_CHANNEL_2,             // Channel to use for PWM backlight
            LEDC_TIMER_2,               // Timer to use for PWM backlight
            getTftBrightnessSetting()); // TFT Brightness

    initShapes();

    // Initialize the RGB LEDs
    initLeds(GPIO_NUM_39, GPIO_NUM_18, getLedBrightnessSetting());

    // Initialize optional peripherals, depending on the mode's requests
    initOptionalPeripherals();

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
            // This must have the same number of elements as the bounds in mic_param
            const uint16_t micGains[] = {
                32, 45, 64, 90, 128, 181, 256, 362,
            };

            uint16_t micAmp = micGains[getMicGainSetting()];
            uint16_t adcSamples[ADC_READ_LEN / SOC_ADC_DIGI_RESULT_BYTES];
            uint32_t sampleCnt = 0;
            while (0 < (sampleCnt = loopMic(adcSamples, ARRAY_SIZE(adcSamples))))
            {
                // Run all samples through an IIR filter
                for (uint32_t i = 0; i < sampleCnt; i++)
                {
                    static uint32_t samp_iir = 0;

                    int32_t sample  = adcSamples[i];
                    samp_iir        = samp_iir - (samp_iir >> 9) + sample;
                    int32_t newSamp = (sample - (samp_iir >> 9));
                    newSamp         = newSamp * micAmp;
                    newSamp         = CLAMP(newSamp, -32768, 32767);
                    adcSamples[i]   = newSamp;
                }
                cSwadgeMode->fnAudioCallback(adcSamples, sampleCnt);
            }
        }

        // Check for buzzer callback flags from the ISR
        bzrCheckSongDone();

        if (NO_WIFI != cSwadgeMode->wifiMode)
        {
            checkEspNowRxQueue();
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

            // If the menu button is being held
            if (0 != timeExitPressed)
            {
                // Figure out for how long
                int64_t tHeldUs = esp_timer_get_time() - timeExitPressed;
                // If it has been held for more than the exit time
                if (tHeldUs > EXIT_TIME_US)
                {
                    // Reset the count
                    timeExitPressed = 0;
                    // Return to the main menu
                    switchToSwadgeMode(&mainMenuMode);
                }
                else
                {
                    // Draw 'progress' bar for exiting. This is done right before the TFT is drawn
                    int16_t numPx = (tHeldUs * TFT_WIDTH) / EXIT_TIME_US;
                    fillDisplayArea(0, TFT_HEIGHT - 10, numPx, TFT_HEIGHT, c333);
                }
            }

            // If quick settings should be shown or hidden, do that before drawing the TFT
            if (shouldShowQuickSettings)
            {
                // Lower the flag
                shouldShowQuickSettings = false;
                // Pause the buzzer
                bzrPause();
                // Save the current mode
                modeBehindQuickSettings = cSwadgeMode;
                cSwadgeMode             = &quickSettingsMode;
                // Show the quick settings
                quickSettingsMode.fnEnterMode();
            }
            else if (shouldHideQuickSettings)
            {
                // Lower the flag
                shouldHideQuickSettings = false;
                // Hide the quick settings
                quickSettingsMode.fnExitMode();
                // Restore the mode
                cSwadgeMode = modeBehindQuickSettings;
                // Resume the buzzer
                bzrResume();
            }

            // Draw to the TFT
            drawDisplayTft(cSwadgeMode->fnBackgroundDrawCallback);
        }

        // If the mode should be switched, do it now
        if (NULL != pendingSwadgeMode)
        {
            // We have to do this otherwise the backlight can glitch
            disableTFTBacklight();

            // Prevent bootloader on reboot if rebooting from originally bootloaded instance
            REG_WRITE(RTC_CNTL_OPTION1_REG, 0);

            // Only an issue if originally coming from bootloader. This is actually a ROM function.
            // It prevents the USB from glitching out on the reboot after the reboot after coming
            // out of bootloader
            chip_usb_set_persist_flags(USBDC_PERSIST_ENA);

            // Go to sleep. pendingSwadgeMode will be used after waking up
            esp_sleep_enable_timer_wakeup(1);
            esp_deep_sleep_start();
        }

        // Yield to let the rest of the RTOS run
        taskYIELD();
    }

    // Deinitialize the swadge mode
    if (NULL != cSwadgeMode->fnExitMode)
    {
        cSwadgeMode->fnExitMode();
    }

    deinitSystem();
}

/**
 * @brief Initialize optional hardware peripherals for this Swadge mode
 */
static void initOptionalPeripherals(void)
{
    // Init mic if it is used by the mode
    if (NULL != cSwadgeMode->fnAudioCallback)
    {
        initMic(GPIO_NUM_7);
        startMic();
    }
    else
    {
        initBattmon(GPIO_NUM_6);
    }

    // Init esp-now if requested by the mode
    if ((ESP_NOW == cSwadgeMode->wifiMode) || (ESP_NOW_IMMEDIATE == cSwadgeMode->wifiMode))
    {
        initEspNow(&swadgeModeEspNowRecvCb, &swadgeModeEspNowSendCb, GPIO_NUM_NC, GPIO_NUM_NC, UART_NUM_MAX,
                   cSwadgeMode->wifiMode);
    }

    // Init accelerometer
    if (cSwadgeMode->usesAccelerometer)
    {
        initAccelerometer(GPIO_NUM_3,  // SDA
                          GPIO_NUM_41, // SCL
                          GPIO_PULLUP_ENABLE);
        accelIntegrate();
    }

    // Init the temperature sensor
    if (cSwadgeMode->usesThermometer)
    {
        initTemperatureSensor();
    }
}

/**
 * @brief Deinitialize all components in the system
 */
void deinitSystem(void)
{
    // Deinit the swadge mode
    if (NULL != cSwadgeMode->fnExitMode)
    {
        cSwadgeMode->fnExitMode();
    }

    // Deinitialize everything!
    deinitButtons();
    deinitBuzzer();
    deinitEspNow();
    deinitLeds();
    deinitMic();
    deinitNvs();
    deinitSpiffs();
    deinitTemperatureSensor();
    deinitTFT();
    deinitUsb();
    deinitBattmon();
}

/**
 * Callback from ESP NOW to the current Swadge mode whenever a packet is
 * received. It routes through user_main.c, which knows what the current mode is
 *
 * @param esp_now_info Information about the transmission, including The MAC addresses
 * @param data     The data which was received
 * @param len      The length of the data which was received
 * @param rssi     The RSSI for this packet, from 1 (weak) to ~90 (touching)
 */
static void swadgeModeEspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len,
                                   int8_t rssi)
{
    if (NULL != cSwadgeMode->fnEspNowRecvCb)
    {
        cSwadgeMode->fnEspNowRecvCb(esp_now_info, data, len, rssi);
    }
}

/**
 * Callback from ESP NOW to the current Swadge mode whenever a packet is sent
 * It routes through user_main.c, which knows what the current mode is
 *
 * @param mac_addr The MAC address which was transmitted to
 * @param status   The transmission status, either ESP_NOW_SEND_SUCCESS or ESP_NOW_SEND_FAIL
 */
static void swadgeModeEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status)
{
    if (NULL != cSwadgeMode->fnEspNowSendCb)
    {
        cSwadgeMode->fnEspNowSendCb(mac_addr, status);
    }
}

/**
 * @brief Set the current Swadge mode
 *
 * @param swadgeMode The new Swadge mode to execute. If NULL, the first mode is switched to
 */
static void setSwadgeMode(void* swadgeMode)
{
    // If no mode is given, switch to the first
    if (NULL == swadgeMode)
    {
        swadgeMode = &mainMenuMode;
    }

    // Stop the prior mode
    if (cSwadgeMode->fnExitMode)
    {
        cSwadgeMode->fnExitMode();
    }

    // Set and start the new mode
    cSwadgeMode = swadgeMode;
    if (cSwadgeMode->fnEnterMode)
    {
        cSwadgeMode->fnEnterMode();
    }
}

/**
 * Set up variables to synchronously switch the swadge mode in the main loop
 *
 * @param mode A pointer to the mode to switch to
 */
void switchToSwadgeMode(swadgeMode_t* mode)
{
    // Set the framerate back to default
    setFrameRateUs(DEFAULT_FRAME_RATE_US);

    pendingSwadgeMode = mode;
}

/**
 * @brief Switch to the pending Swadge mode without restarting the system
 */
void softSwitchToPendingSwadge(void)
{
    if (pendingSwadgeMode)
    {
        // Exit the current mode
        if (NULL != cSwadgeMode->fnExitMode)
        {
            cSwadgeMode->fnExitMode();
        }

        // Stop the buzzer
        bzrStop(true);

        // Switch the mode pointer
        cSwadgeMode       = pendingSwadgeMode;
        pendingSwadgeMode = NULL;

        // Initialize optional peripherals for this mode
        initOptionalPeripherals();

        // Enter the next mode
        if (NULL != cSwadgeMode->fnEnterMode)
        {
            cSwadgeMode->fnEnterMode();
        }

        // Reenable the TFT backlight
        enableTFTBacklight();
    }
}

/**
 * @brief Service the queue of button events that caused interrupts
 * This only returns a single event, even if there are multiple in the queue
 * This function may be called multiple times in a row to completely empty the queue
 *
 * This is a wrapper for checkButtonQueue() which also monitors the button to return to the main menu
 *
 * @param evt If an event occurred, return it through this argument
 * @return true if an event occurred, false if nothing happened
 */
bool checkButtonQueueWrapper(buttonEvt_t* evt)
{
    // Check the button queue
    bool retval = checkButtonQueue(evt);

    // Check for intercept
    if (retval &&                            // If there was a button press
        (!cSwadgeMode->overrideSelectBtn) && // And PB_SELECT isn't overridden
        (evt->button == PB_SELECT))          // And the button was PB_SELECT
    {
        if (evt->down)
        {
            // Button was pressed, start the timer
            timeExitPressed = esp_timer_get_time();
        }
        else
        {
            // Button was released, stop the timer
            timeExitPressed = 0;

            // If the mode hasn't exited yet, toggle quick settings
            if (cSwadgeMode != &quickSettingsMode)
            {
                shouldShowQuickSettings = true;
            }
            else
            {
                shouldHideQuickSettings = true;
            }
        }

        // Don't pass this button to the mode
        retval = false;
    }

    // Return if there was an event or not
    return retval;
}

/**
 * @brief Set the framerate, in microseconds
 *
 * @param newFrameRateUs The time between frame draws, in microseconds
 */
void setFrameRateUs(uint32_t newFrameRateUs)
{
    frameRateUs = newFrameRateUs;
}
