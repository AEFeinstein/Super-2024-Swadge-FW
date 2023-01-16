#ifndef _HDW_TFT_H_
#define _HDW_TFT_H_

#include <stdint.h>

#include "hal/gpio_types.h"
#include "hal/spi_types.h"

#include "palette.h"

#if defined(CONFIG_ST7735_160x80)
    #define TFT_WIDTH         160
    #define TFT_HEIGHT         80
#elif defined(CONFIG_ST7735_128x160)
    #define TFT_WIDTH         160
    #define TFT_HEIGHT        128
#elif defined(CONFIG_ST7789_240x135)
    #define TFT_WIDTH         240
    #define TFT_HEIGHT        135
#elif defined(CONFIG_ST7789_240x240)
    #define TFT_WIDTH         240
    #define TFT_HEIGHT        240
#elif defined(CONFIG_GC9307_240x280)
    #define TFT_WIDTH         280
    #define TFT_HEIGHT        240
#else
    #error "Please pick a screen size"
#endif

typedef void (*fnBackgroundDrawCallback_t)(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);

void initTFT(spi_host_device_t spiHost, gpio_num_t sclk,
             gpio_num_t mosi, gpio_num_t dc, gpio_num_t cs, gpio_num_t rst,
             gpio_num_t backlight, bool isPwmBacklight);
int setTFTBacklight(uint8_t intensity);
void disableTFTBacklight(void);
void enableTFTBacklight(void);

void setPxTft(int16_t x, int16_t y, paletteColor_t px);
paletteColor_t getPxTft(int16_t x, int16_t y);
void clearPxTft(void);
void drawDisplayTft(bool drawDiff, fnBackgroundDrawCallback_t cb);

#endif
