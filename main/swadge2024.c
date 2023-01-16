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
 *
 * \section example Example Swadge Mode
 * 
 *     int main(void)
 *     {
 *         return 0;
 *     }
 */

#include <stdio.h>

#include "esp_timer.h"
#include "esp_log.h"

#include "hdw-btn.h"
#include "hdw-tft.h"

// #include "tinyusb.h"
// #include "advanced_usb_control.h"

/**
 * @brief TODO doxygen something
 * 
 */
void app_main(void)
{
    ESP_LOGE("MAIN", "boot");

    // Init timers
    esp_timer_init();

    // const tinyusb_config_t tusb_cfg = {
    //     .device_descriptor = NULL,
    //     .string_descriptor = NULL,
    //     .external_phy = false,
    //     .configuration_descriptor = hid_configuration_descriptor,
    // };
    // tinyusb_driver_install(&tusb_cfg);

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
            true);       // PWM backlight

    bool drawScreen = false;
    while(1)
    {
        paletteColor_t color = c000;

        buttonEvt_t evt;
        if(checkButtonQueue(&evt))
        {
            drawScreen = evt.down;
        }

        if(drawScreen)
        {
            for(uint16_t y = 0; y < TFT_HEIGHT; y++)
            {
                for(uint16_t x = 0; x < TFT_WIDTH; x++)
                {
                    setPxTft(x, y, color);
                    color = (color + 1) % cTransparent;
                }
            }
        }
        else
        {
            clearPxTft();
        }

        drawDisplayTft(false, NULL);
    }
}
