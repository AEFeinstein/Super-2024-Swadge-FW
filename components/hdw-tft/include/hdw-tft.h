/*! \file hdw-tft.h
 *
 * \section tft_design Design Philosophy
 *
 * TFT code is based on <a
 * href="https://github.com/espressif/esp-idf/tree/v5.1/examples/peripherals/lcd/tjpgd">Espressif's LCD tjpgd
 * example</a>.
 *
 * Each pixel in the frame-buffer is of type ::paletteColor_t.
 * Even though the TFT supports 16 bit color, a 16 bit frame-buffer is too big to have in RAM alongside games and such.
 * Instead, the 8 bit <a href="https://www.rapidtables.com/web/color/Web_Safe.html">Web Safe palette</a> is used, where
 * each RGB channel has six options for a total of 216 colors. The ::paletteColor_t enum has values for all colors in
 * the form of cRGB, where R, G, and B each range from 0 to 5. For example, ::c500 is full red.
 * ::cTransparent is a special value for a transparent pixel.
 *
 * \section tft_usage Usage
 *
 * You don't need to call initTFT() or deinitTFT(). The system does so at the appropriate time.
 * You don't need to call drawDisplayTft() as it is called automatically after each main loop to draw the current
 * frame-buffer to the TFT.
 *
 * clearPxTft() is used to clear the current frame-buffer.
 * This must be called before drawing a new frame, unless you want to draw over the prior one.
 *
 * setPxTft() and getPxTft() are used to set and get individual pixels in the frame-buffer, respectively.
 * These are not often used directly as there are helper functions to draw text, shapes, and sprites.
 *
 * disableTFTBacklight() and enableTFTBacklight() may be called to disable and enable the backlight, respectively.
 * This may be useful if the Swadge mode is trying to save power, or the TFT is not necessary.
 * setTFTBacklightBrightness() is used to set the TFT's brightness. This is usually handled globally by a persistent
 * setting.
 * setTftBrightnessSetting() should be called instead if the brightness change should be persistent through reboots.
 *
 * \section tft_example Example
 *
 * Setting pixels:
 * \code{.c}
 * // Clear the display
 * clearPxTft();
 *
 * // Draw red, green, and blue vertical bars
 * for(uint16_t y = 0; y < TFT_HEIGHT; y++)
 * {
 *     for(uint16_t x = 0; x < TFT_WIDTH; x++)
 *     {
 *         if(x < TFT_WIDTH / 3)
 *         {
 *             setPxTft(x, y, c500);
 *         }
 *         else if (x < (2 * TFT_WIDTH) / 3)
 *         {
 *             setPxTft(x, y, c050);
 *         }
 *         else
 *         {
 *             setPxTft(x, y, c005);
 *         }
 *     }
 * }
 * \endcode
 *
 * Setting the backlight:
 * \code{.c}
 * // Disable the backlight
 * disableTFTBacklight();
 *
 * // Enable the backlight
 * enableTFTBacklight();
 *
 * // Set the backlight to half brightness
 * setTFTBacklightBrightness(128);
 * \endcode
 */

#ifndef _HDW_TFT_H_
#define _HDW_TFT_H_

#include <stdint.h>
#include <stdbool.h>

#include <hal/gpio_types.h>
#include <hal/spi_types.h>
#include <driver/ledc.h>
#include <esp_err.h>

#include "palette.h"

#if defined(CONFIG_ST7735_160x80)
    #define TFT_WIDTH  160
    #define TFT_HEIGHT 80
#elif defined(CONFIG_ST7735_128x160)
    #define TFT_WIDTH  160
    #define TFT_HEIGHT 128
#elif defined(CONFIG_ST7789_240x135)
    #define TFT_WIDTH  240
    #define TFT_HEIGHT 135
#elif defined(CONFIG_ST7789_240x240)
    #define TFT_WIDTH  240
    #define TFT_HEIGHT 240
#elif defined(CONFIG_GC9307_240x280)
    #define TFT_WIDTH  280
    #define TFT_HEIGHT 240
#else
    #error "Please pick a screen size"
#endif

/**
 * @brief This is a typedef for a function pointer passed to drawDisplayTft()
 * which will be called to draw a background image while the SPI transfer is
 * occurring. This will be called multiple times to draw multiple areas for
 * the current frame.
 *
 * @param x The X coordinate to start filling in
 * @param y The Y coordinate to start filling in
 * @param w The width of the background area to draw
 * @param h The height of the background area to draw
 * @param up The number of times this function has been called for the current frame (an incrementing number)
 * @param upNum The total number of times this will be called for the current frame
 */
typedef void (*fnBackgroundDrawCallback_t)(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);

void initTFT(spi_host_device_t spiHost, gpio_num_t sclk, gpio_num_t mosi, gpio_num_t dc, gpio_num_t cs, gpio_num_t rst,
             gpio_num_t backlight, bool isPwmBacklight, ledc_channel_t ledcChannel, ledc_timer_t ledcTimer);
void deinitTFT(void);
esp_err_t setTFTBacklightBrightness(uint8_t intensity);
void disableTFTBacklight(void);
void enableTFTBacklight(void);

void setPxTft(int16_t x, int16_t y, paletteColor_t px);
paletteColor_t getPxTft(int16_t x, int16_t y);
paletteColor_t* getPxTftFramebuffer(void);
void clearPxTft(void);
void drawDisplayTft(fnBackgroundDrawCallback_t cb);

#if defined(__XTENSA__)
    /**
     * Initialize a variable to set pixels faster than setPxTft()
     */
    #define SETUP_FOR_TURBO() register uint32_t dispPx = (uint32_t)getPxTftFramebuffer();

    /**
     * Set a single pixel in the display. This does not bounds check.
     * SETUP_FOR_TURBO() must be called before this.
     *
     * 5/4 cycles -- note you can do better if you don't need arbitrary X/Y's.
     */
    #define TURBO_SET_PIXEL(opxc, opy, colorVal)                                                                    \
        asm volatile("mul16u a4, %[width], %[y]\nadd a4, a4, %[px]\nadd a4, a4, %[opx]\ns8i %[val],a4, 0"           \
                     :                                                                                              \
                     : [opx] "a"(opxc), [y] "a"(opy), [px] "a"(dispPx), [val] "a"(colorVal), [width] "a"(TFT_WIDTH) \
                     : "a4");

    /**
     * Set a single pixel in the display. This does checks the TFT's bounds.
     * SETUP_FOR_TURBO() must be called before this.
     *
     * Very tricky:
     *   We do bgeui which checks to make sure 0 <= x < MAX
     *   Other than that, it's basically the same as above.
     */
    #define TURBO_SET_PIXEL_BOUNDS(opxc, opy, colorVal)                                                        \
        asm volatile(                                                                                          \
            "bgeu %[opx], %[width], failthrough%=\nbgeu %[y], %[height], failthrough%=\nmul16u a4, %[width], " \
            "%[y]\nadd a4, a4, %[px]\nadd a4, a4, %[opx]\ns8i %[val],a4, 0\nfailthrough%=:\n"                  \
            :                                                                                                  \
            : [opx] "a"(opxc), [y] "a"(opy), [px] "a"(dispPx), [val] "a"(colorVal), [width] "a"(TFT_WIDTH),    \
              [height] "a"(TFT_HEIGHT)                                                                         \
            : "a4");
#else
    /// @brief Do nothing if this isn't an __XTENSA__ platform
    #define SETUP_FOR_TURBO()
    /// @brief Passthrough call to setPxTft() if this isn't an __XTENSA__ platform
    #define TURBO_SET_PIXEL(opxc, opy, colorVal)        setPxTft(opxc, opy, colorVal)
    /// @brief Passthrough call to setPxTft() if this isn't an __XTENSA__ platform
    #define TURBO_SET_PIXEL_BOUNDS(opxc, opy, colorVal) setPxTft(opxc, opy, colorVal)
#endif

#endif
