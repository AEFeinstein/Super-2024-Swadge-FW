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
 */

#include <stdio.h>

#include "esp_timer.h"
#include "esp_log.h"

#include "hdw-btn.h"
#include "hdw-tft.h"

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

        if(drawScreen)
        {
            clearPxTft();
            for(uint16_t y = 0; y < TFT_HEIGHT; y++)
            {
                for(uint16_t x = 0; x < TFT_WIDTH; x++)
                {
                    if(x < TFT_WIDTH / 3)
                    {
                        setPxTft(x, y, c500);
                    }
                    else if (x < (2 * TFT_WIDTH) / 3)
                    {
                        setPxTft(x, y, c050);
                    }
                    else
                    {
                        setPxTft(x, y, c005);
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
