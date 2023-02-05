/*! \mainpage Swadge 2024
 *
 * \section intro_sec Introduction
 *
 * So you're here to write some Swadge code, eh?
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
 */

#include <stdio.h>

#include "esp_timer.h"
#include "esp_log.h"

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

static void swadgeModeEspNowRecvCb(const uint8_t* mac_addr, const char* data, uint8_t len, int8_t rssi);
static void swadgeModeEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status);

/**
 * @brief TODO doxygen something
 *
 */
void app_main(void)
{
    ESP_LOGE("MAIN", "boot");

    // Init timers
    esp_timer_init();

    // Init USB
    initUsb();

    // Init esp-now
    espNowInit(&swadgeModeEspNowRecvCb, &swadgeModeEspNowSendCb, GPIO_NUM_NC, GPIO_NUM_NC, UART_NUM_MAX, ESP_NOW);

    // Init SPIFFS file system
    initSpiffs();

    // Init NVS
    initNvs(true);

    font_t ibm;
    loadFont("ibm_vga8.font", &ibm);

    wsg_t king_donut;
    loadWsgSpiRam("kid0.wsg", &king_donut, true);

    // Init buttons
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

    // Init buzzer
    initBuzzer(GPIO_NUM_40, LEDC_TIMER_3, LEDC_CHANNEL_0, false, false);

    // Init mic
    initMic(GPIO_NUM_7);
    startMic();

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

    // Init accelerometer
    qma7981_init(I2C_NUM_0,
                 GPIO_NUM_3,  // SDA
                 GPIO_NUM_41, // SCL
                 GPIO_PULLUP_DISABLE, 1000000, QMA_RANGE_2G, QMA_BANDWIDTH_1024_HZ);

    // Init the temperature sensor
    initTemperatureSensor();

    // static const song_t BlackDog = {
    //     .numNotes      = 28,
    //     .shouldLoop    = false,
    //     .loopStartNote = 0,
    //     .notes = {{.note = E_5, .timeMs = 188}, {.note = G_5, .timeMs = 188},    {.note = G_SHARP_5, .timeMs = 188},
    //               {.note = A_5, .timeMs = 188}, {.note = E_5, .timeMs = 188},    {.note = C_6, .timeMs = 375},
    //               {.note = A_5, .timeMs = 375}, {.note = D_6, .timeMs = 188},    {.note = E_6, .timeMs = 188},
    //               {.note = C_6, .timeMs = 94},  {.note = D_6, .timeMs = 94},     {.note = C_6, .timeMs = 188},
    //               {.note = A_5, .timeMs = 183}, {.note = SILENCE, .timeMs = 10}, {.note = A_5, .timeMs = 183},
    //               {.note = C_6, .timeMs = 375}, {.note = A_5, .timeMs = 375},    {.note = G_5, .timeMs = 188},
    //               {.note = A_5, .timeMs = 183}, {.note = SILENCE, .timeMs = 10}, {.note = A_5, .timeMs = 183},
    //               {.note = D_5, .timeMs = 188}, {.note = E_5, .timeMs = 188},    {.note = C_5, .timeMs = 188},
    //               {.note = D_5, .timeMs = 188}, {.note = A_4, .timeMs = 370},    {.note = SILENCE, .timeMs = 10},
    //               {.note = A_4, .timeMs = 745}},
    // };
    // bzrPlayBgm(&BlackDog);

    // bool drawScreen = false;
    while (1)
    {
        buttonEvt_t evt              = {0};
        static uint32_t lastBtnState = 0;
        while (checkButtonQueue(&evt))
        {
            printf("state: %04X, button: %d, down: %s\n", evt.state, evt.button, evt.down ? "down" : "up");
            lastBtnState = evt.state;
            // drawScreen = evt.down;

            static hid_gamepad_report_t report;
            report.buttons = lastBtnState;
            sendUsbGamepadReport(&report);
        }

        int32_t centerVal, intensityVal;
        if (getTouchCentroid(&centerVal, &intensityVal))
        {
            printf("touch center: %lu, intensity: %lu\n", centerVal, intensityVal);
        }
        else
        {
            printf("no touch\n");
        }

        clearPxTft();
        int numBtns   = 13;
        int drawWidth = TFT_WIDTH / numBtns;
        for (int i = 0; i < numBtns; i++)
        {
            if (lastBtnState & (1 << i))
            {
                for (int w = drawWidth * i; w < drawWidth * (i + 1); w++)
                {
                    for (int h = 0; h < TFT_HEIGHT; h++)
                    {
                        setPxTft(w, h, (i + 5) * 5);
                    }
                }
            }
        }

        drawText(&ibm, c555, "Hello world", 64, 64);

        drawLine(92, 92, 200, 200, c500, 0, 0, 0, 1, 1);
        speedyLine(102, 92, 210, 200, c050);

        drawWsg(&king_donut, 100, 10, false, false, 0);

        printf("%f\n", readTemperatureSensor());

        int16_t a_x, a_y, a_z;
        qma7981_get_accel(&a_x, &a_y, &a_z);

        uint16_t outSamps[ADC_READ_LEN / 2];
        uint32_t sampsRead = loopMic(outSamps, (ADC_READ_LEN / 2));

        // clearPxTft();
        // for (int i = 0; i < sampsRead; i++)
        // {
        //     setPxTft(i, (TFT_HEIGHT - (outSamps[i] >> 4) - 1) % TFT_HEIGHT, c555);
        // }

        // if (drawScreen)
        // {
        //     clearPxTft();
        //     for (uint16_t y = 0; y < TFT_HEIGHT; y++)
        //     {
        //         for (uint16_t x = 0; x < TFT_WIDTH; x++)
        //         {
        //             if (x < TFT_WIDTH / 3)
        //             {
        //                 setPxTft(x, y, (a_x >> 6) % cTransparent);
        //             }
        //             else if (x < (2 * TFT_WIDTH) / 3)
        //             {
        //                 setPxTft(x, y, (a_y >> 6) % cTransparent);
        //             }
        //             else
        //             {
        //                 setPxTft(x, y, (a_z >> 6) % cTransparent);
        //             }
        //         }
        //     }
        // }
        // else
        // {
        //     clearPxTft();
        // }

        drawDisplayTft(NULL);

        led_t leds[CONFIG_NUM_LEDS] = {0};
        for (uint8_t i = 0; i < CONFIG_NUM_LEDS; i++)
        {
            leds[i].r = (255 * ((i + 0) % CONFIG_NUM_LEDS)) / (CONFIG_NUM_LEDS - 1);
            leds[i].g = (255 * ((i + 3) % CONFIG_NUM_LEDS)) / (CONFIG_NUM_LEDS - 1);
            leds[i].b = (255 * ((i + 6) % CONFIG_NUM_LEDS)) / (CONFIG_NUM_LEDS - 1);
        }
        setLeds(leds, CONFIG_NUM_LEDS);
    }
    freeFont(&ibm);
    freeWsg(&king_donut);
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
    // if(NULL != cSwadgeMode->fnEspNowRecvCb)
    // {
    //     cSwadgeMode->fnEspNowRecvCb(mac_addr, data, len, rssi);
    // }
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
    // if(NULL != cSwadgeMode->fnEspNowSendCb)
    // {
    //     cSwadgeMode->fnEspNowSendCb(mac_addr, status);
    // }
}
