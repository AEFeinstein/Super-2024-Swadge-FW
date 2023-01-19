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
 * - hdw-btn.c: Learn how to use button input!
 * - hdw-tft.c: Learn how to use the TFT!
 * - hdw-bzr.c: Learn how to use the buzzer!
 * - hdw-accel.c: Learn how to use the accelerometer!
 */

#include <stdio.h>

#include "esp_timer.h"
#include "esp_log.h"

#include "hdw-btn.h"
#include "hdw-tft.h"
#include "hdw-bzr.h"
#include "hdw-accel.h"

/**
 * @brief TODO doxygen something
 * 
 */
void app_main(void)
{
    ESP_LOGE("MAIN", "boot");

    // Init timers
    esp_timer_init();

    // Init buttons
    initButtons(8,
                GPIO_NUM_0,
                GPIO_NUM_4,
                GPIO_NUM_2,
                GPIO_NUM_1,
                GPIO_NUM_16,
                GPIO_NUM_15,
                GPIO_NUM_8,
                GPIO_NUM_5);

    // Make sure to use a different timer than initButtons()
    initBuzzer(GPIO_NUM_40, LEDC_TIMER_3, LEDC_CHANNEL_0,
        false, false);

    // Init TFT
    initTFT(SPI2_HOST,
            GPIO_NUM_36, // sclk
            GPIO_NUM_37, // mosi
            GPIO_NUM_21, // dc
            GPIO_NUM_34, // cs
            GPIO_NUM_38, // rst
            GPIO_NUM_35, // backlight
            true,        // PWM backlight
            LEDC_CHANNEL_1); // Channel to use for PWM backlight

    // Init accelerometer
    qma7981_init(I2C_NUM_0,
        GPIO_NUM_3,  // SDA
        GPIO_NUM_41, // SCL
        GPIO_PULLUP_DISABLE, 1000000,
        QMA_RANGE_2G, QMA_BANDWIDTH_1024_HZ);

    static const song_t BlackDog =
    {
        .numNotes = 28,
        .shouldLoop = false,
        .loopStartNote = 0,
        .notes = {
            {.note = E_5, .timeMs = 188},
            {.note = G_5, .timeMs = 188},
            {.note = G_SHARP_5, .timeMs = 188},
            {.note = A_5, .timeMs = 188},
            {.note = E_5, .timeMs = 188},
            {.note = C_6, .timeMs = 375},
            {.note = A_5, .timeMs = 375},
            {.note = D_6, .timeMs = 188},
            {.note = E_6, .timeMs = 188},
            {.note = C_6, .timeMs = 94},
            {.note = D_6, .timeMs = 94},
            {.note = C_6, .timeMs = 188},
            {.note = A_5, .timeMs = 183},
            {.note = SILENCE, .timeMs = 10},
            {.note = A_5, .timeMs = 183},
            {.note = C_6, .timeMs = 375},
            {.note = A_5, .timeMs = 375},
            {.note = G_5, .timeMs = 188},
            {.note = A_5, .timeMs = 183},
            {.note = SILENCE, .timeMs = 10},
            {.note = A_5, .timeMs = 183},
            {.note = D_5, .timeMs = 188},
            {.note = E_5, .timeMs = 188},
            {.note = C_5, .timeMs = 188},
            {.note = D_5, .timeMs = 188},
            {.note = A_4, .timeMs = 370},
            {.note = SILENCE, .timeMs = 10},
            {.note = A_4, .timeMs = 745},
        }
    };
    bzrPlayBgm(&BlackDog);

    bool drawScreen = false;
    while(1)
    {
        buttonEvt_t evt;
        if(checkButtonQueue(&evt))
        {
            printf("state: %04X, button: %d, down: %s\n",
                evt.state, evt.button, evt.down ? "down" : "up");
            drawScreen = evt.down;
        }

        int16_t a_x, a_y, a_z;
        qma7981_get_accel(&a_x, &a_y, &a_z);

        if(drawScreen)
        {
            clearPxTft();
            for(uint16_t y = 0; y < TFT_HEIGHT; y++)
            {
                for(uint16_t x = 0; x < TFT_WIDTH; x++)
                {
                    if(x < TFT_WIDTH / 3)
                    {
                        setPxTft(x, y, (a_x >> 6) % cTransparent);
                    }
                    else if (x < (2 * TFT_WIDTH) / 3)
                    {
                        setPxTft(x, y, (a_y >> 6) % cTransparent);
                    }
                    else
                    {
                        setPxTft(x, y, (a_z >> 6) % cTransparent);
                    }
                }
            }
        }
        else
        {
            clearPxTft();
        }

        drawDisplayTft(NULL);
    }
}
