//==============================================================================
// Includes
//==============================================================================

#include <stdio.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_lcd_panel_ops.h>
#include <esp_heap_caps.h>
#include <esp_log.h>
#include <esp_lcd_panel_interface.h>
#include <driver/spi_master.h>
#include <driver/gpio.h>

#include "hdw-tft.h"

// #define PROC_PROFILE

#ifdef PROC_PROFILE
void uart_tx_one_char(char c);
static inline uint32_t get_cCount()
{
    uint32_t cCount;
    asm volatile("rsr %0,cCount" : "=a"(cCount));
    return cCount;
}
#endif

//==============================================================================
// Defines
//==============================================================================

/// Swap the upper and lower bytes in a 16-bit word
#define SWAP(x) ((x >> 8) | (x << 8))

/**
 * @brief The number of parallel lines used in a SPI transfer
 *
 * To speed up transfers, every SPI transfer sends a bunch of lines. This define
 * specifies how many. More means more memory use, but less overhead for setting
 * up and finishing transfers. Make sure TFT_HEIGHT is dividable by this.
 */
#define PARALLEL_LINES 16

/// The GPIO level to turn the backlight on
#define LCD_BK_LIGHT_ON_LEVEL 1
/// The GPIO level to turn the backlight off
#define LCD_BK_LIGHT_OFF_LEVEL 0

/// The number of bits in an LCD control command
#define LCD_CMD_BITS 8
/// The number of bits for an LCD control command's parameter
#define LCD_PARAM_BITS 8

/* Screen-specific configurations */
#if defined(CONFIG_ST7735_160x80)
    #define LCD_PIXEL_CLOCK_HZ (40 * 1000 * 1000)
    #define X_OFFSET           1
    #define Y_OFFSET           26
    #define SWAP_XY            true
    #define MIRROR_X           false
    #define MIRROR_Y           true
#elif defined(CONFIG_ST7735_128x160)
// Mixture of docs + experimentation
// This is the RB027D25N05A / RB017D14N05A (Actually the ST7735S, so in between a ST7735 and ST7789)
    #define LCD_PIXEL_CLOCK_HZ (40 * 1000 * 1000)
    #define X_OFFSET           0
    #define Y_OFFSET           0
    #define SWAP_XY            true
    #define MIRROR_X           false
    #define MIRROR_Y           true
#elif defined(CONFIG_ST7789_240x135)
    #define LCD_PIXEL_CLOCK_HZ (80 * 1000 * 1000)
    #define X_OFFSET           40
    #define Y_OFFSET           52
    #define SWAP_XY            true
    #define MIRROR_X           false
    #define MIRROR_Y           true
#elif defined(CONFIG_ST7789_240x240)
    #define LCD_PIXEL_CLOCK_HZ (80 * 1000 * 1000)
    #define X_OFFSET           0
    #define Y_OFFSET           80
    #define SWAP_XY            false
    #define MIRROR_X           true
    #define MIRROR_Y           true
#elif defined(CONFIG_GC9307_240x280)
// A beautiful rounded edges LCD RB017A1505A
    #define LCD_PIXEL_CLOCK_HZ (80 * 1000 * 1000)
    #define X_OFFSET           20
    #define Y_OFFSET           0
    #define SWAP_XY            true
    #define MIRROR_X           true
    #define MIRROR_Y           true
#else
    #error "Please pick a screen size"
#endif

#define NUM_S_LINES 2

//==============================================================================
// Variables
//==============================================================================

static spi_host_device_t tftSpiHost;

static esp_lcd_panel_handle_t panel_handle = NULL;
static paletteColor_t* pixels              = NULL;
static uint16_t* s_lines[NUM_S_LINES]      = {0};

static ledc_timer_t tftLedcTimer;
static ledc_channel_t tftLedcChannel;
static gpio_num_t tftBacklightPin;
static bool tftBacklightIsPwm;
static uint8_t tftBacklightIntensity;

static esp_lcd_panel_io_handle_t tft_io_handle = NULL;

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Set TFT Backlight brightness. setTftBrightnessSetting() should be called instead if the new volume should be
 * persistent through a reboot.
 *
 * @param intensity The brightness, 0 to MAX_TFT_BRIGHTNESS
 *
 * @return value is 0 if OK nonzero if error.
 */
esp_err_t setTFTBacklightBrightness(uint8_t intensity)
{
    // Save the value we can pass back into this function
    tftBacklightIntensity = intensity;

    intensity
        = (CONFIG_TFT_MIN_BRIGHTNESS + (((CONFIG_TFT_MAX_BRIGHTNESS - CONFIG_TFT_MIN_BRIGHTNESS) * intensity) / 7));

    esp_err_t e;
    if (intensity > CONFIG_TFT_MAX_BRIGHTNESS)
    {
        return ESP_ERR_INVALID_ARG;
    }
    e = ledc_set_duty(LEDC_LOW_SPEED_MODE, tftLedcChannel, 255 - intensity);
    if (e)
    {
        return e;
    }
    return ledc_update_duty(LEDC_LOW_SPEED_MODE, tftLedcChannel);
}

/**
 * @brief Initialize a TFT display and return it through a pointer arg
 *
 * @param spiHost The SPI host to use for this display
 * @param sclk    The GPIO for the SCLK pin
 * @param mosi    The GPIO for the MOSI pin
 * @param dc      The GPIO for the TFT SPI data or command selector pin
 * @param cs      The GPIO for the chip select pin
 * @param rst     The GPIO for the RESET pin
 * @param backlight The GPIO used to PWM control the backlight
 * @param isPwmBacklight true to set up the backlight as PWM, false to have it be on/off
 * @param ledcChannel The LEDC channel to use for the PWM backlight
 * @param ledcTimer The LEDC timer to use for the PWM backlight
 */
void initTFT(spi_host_device_t spiHost, gpio_num_t sclk, gpio_num_t mosi, gpio_num_t dc, gpio_num_t cs, gpio_num_t rst,
             gpio_num_t backlight, bool isPwmBacklight, ledc_channel_t ledcChannel, ledc_timer_t ledcTimer, uint8_t brightness)
{
    tftSpiHost        = spiHost;
    tftBacklightPin   = backlight;
    tftBacklightIsPwm = isPwmBacklight;
    tftLedcChannel    = ledcChannel;
    tftLedcTimer      = ledcTimer;

    spi_bus_config_t busCfg = {
        .sclk_io_num     = sclk,
        .mosi_io_num     = mosi,
        .miso_io_num     = -1,
        .quadwp_io_num   = -1,
        .quadhd_io_num   = -1,
        .max_transfer_sz = PARALLEL_LINES * TFT_WIDTH * 2 + 8,
    };
    // Initialize the SPI bus
    ESP_ERROR_CHECK(spi_bus_initialize(spiHost, &busCfg, SPI_DMA_CH_AUTO));

    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num       = dc,
        .cs_gpio_num       = cs,
        .pclk_hz           = LCD_PIXEL_CLOCK_HZ,
        .lcd_cmd_bits      = LCD_CMD_BITS,
        .lcd_param_bits    = LCD_PARAM_BITS,
        .spi_mode          = 0,
        .trans_queue_depth = 10,
    };

    // Attach the LCD to the SPI bus
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)spiHost, &io_config, &tft_io_handle));

    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = rst,
        .color_space    = ESP_LCD_COLOR_SPACE_RGB,
        .bits_per_pixel = 16,
    };
    // Initialize the LCD configuration

#if defined(CONFIG_ST7735_160x80)
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7735(tft_io_handle, &panel_config, &panel_handle));
#elif defined(CONFIG_ST7789_240x135) || defined(CONFIG_ST7789_240x240) || defined(CONFIG_ST7735_128x160) \
    || defined(CONFIG_GC9307_240x280)
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(tft_io_handle, &panel_config, &panel_handle));
#else
    #error "Please pick a screen size"
#endif

    // Reset the display
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));

    // Initialize LCD panel
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));

    // Allocate memory for the pixel buffers
    for (int i = 0; i < NUM_S_LINES; i++)
    {
        s_lines[i] = heap_caps_malloc(TFT_WIDTH * PARALLEL_LINES * sizeof(uint16_t), MALLOC_CAP_DMA);
        assert(s_lines[i] != NULL);
    }

    // Config the TFT
    esp_lcd_panel_swap_xy(panel_handle, SWAP_XY);
    esp_lcd_panel_mirror(panel_handle, MIRROR_X, MIRROR_Y);
    esp_lcd_panel_set_gap(panel_handle, X_OFFSET, Y_OFFSET);

#if defined(CONFIG_GC9307_240x280)
    esp_lcd_panel_invert_color(panel_handle, false);
    // NOTE: the following call would override settings set by esp_lcd_panel_swap_xy() and esp_lcd_panel_mirror()
    // Both of the prior functions write to the 0x36 register
    esp_lcd_panel_io_tx_param(tft_io_handle, 0x36, (uint8_t[]){0xE8}, 1); // MX, MY, RGB mode  (MAD CTL)
    esp_lcd_panel_io_tx_param(tft_io_handle, 0x35, (uint8_t[]){0x00}, 1); // "tear effect" testing sync pin.
#elif defined(CONFIG_ST7735_128x160)
    esp_lcd_panel_io_tx_param(tft_io_handle, 0xB1, (uint8_t[]){0x05, 0x3C, 0x3C}, 3);
    esp_lcd_panel_io_tx_param(tft_io_handle, 0xB2, (uint8_t[]){0x05, 0x3C, 0x3C}, 3);
    esp_lcd_panel_io_tx_param(tft_io_handle, 0xB3, (uint8_t[]){0x05, 0x3C, 0x3C, 0x05, 0x3C, 0x3C}, 6);
    esp_lcd_panel_io_tx_param(tft_io_handle, 0xB4, (uint8_t[]){0x00}, 1); // 00 Dot inversion,  //07 column inversion
    esp_lcd_panel_io_tx_param(tft_io_handle, 0x36, (uint8_t[]){0xa0}, 1); // MX, MY, RGB mode  (MAD CTL)
    esp_lcd_panel_io_tx_param(
        tft_io_handle, 0xE0,
        (uint8_t[]){0x04, 0x22, 0x07, 0x0A, 0x2E, 0x30, 0x25, 0x2A, 0x28, 0x26, 0x2E, 0x3A, 0x00, 0x01, 0x03, 0x13},
        16);
    esp_lcd_panel_io_tx_param(
        tft_io_handle, 0xE1,
        (uint8_t[]){0x04, 0x16, 0x06, 0x0D, 0x2D, 0x26, 0x23, 0x27, 0x27, 0x25, 0x2D, 0x3B, 0x00, 0x01, 0x04, 0x13},
        16);
    esp_lcd_panel_io_tx_param(tft_io_handle, 0x20, (uint8_t[]){0}, 0); // buffer color inversion
#else
    esp_lcd_panel_invert_color(panel_handle, true);
#endif

    // Enable the backlight
    enableTFTBacklight();

    if (NULL == pixels)
    {
        pixels = (paletteColor_t*)malloc(sizeof(paletteColor_t) * TFT_HEIGHT * TFT_WIDTH);
    }
}

/**
 * @brief Deinitialize the TFT display
 */
void deinitTFT(void)
{
    disableTFTBacklight();

    esp_lcd_panel_del(panel_handle);
    esp_lcd_panel_io_del(tft_io_handle);
    spi_bus_free(tftSpiHost);

    for (int i = 0; i < NUM_S_LINES; i++)
    {
        free(s_lines[i]);
    }
    free(pixels);
}

/**
 * @brief Return the pixel framebuffer, which is (TFT_WIDTH * TFT_HEIGHT) pixels
 * in row order, starting from the top left. This can be used t directly modify
 * individual pixels without calling ::setPxTft()
 *
 * @return The pixel framebuffer
 */
paletteColor_t* getPxTftFramebuffer(void)
{
    return pixels;
}

/**
 * @brief Disable the backlight (for power down)
 *
 */
void disableTFTBacklight(void)
{
#if defined(CONFIG_GC9307_240x280)
    // Display OFF
    esp_lcd_panel_io_tx_param(tft_io_handle, 0x28, NULL, 0);
    // Enter sleep mode
    esp_lcd_panel_io_tx_param(tft_io_handle, 0x10, NULL, 0);
#endif

    ledc_stop(LEDC_LOW_SPEED_MODE, tftLedcChannel, 0);
    gpio_reset_pin(tftBacklightPin);
    gpio_set_level(tftBacklightPin, 0);
}

/**
 * @brief Enable the backlight
 *
 */
void enableTFTBacklight(void)
{
#if defined(CONFIG_GC9307_240x280)
    // Exit sleep mode
    esp_lcd_panel_io_tx_param(tft_io_handle, 0x11, NULL, 0);
    // Display ON
    esp_lcd_panel_io_tx_param(tft_io_handle, 0x29, NULL, 0);
#endif

    if (false == tftBacklightIsPwm)
    {
        // Binary backlight
        gpio_config_t bk_gpio_config = {
            .mode         = GPIO_MODE_OUTPUT,
            .pin_bit_mask = 1ULL << tftBacklightPin,
        };
        // Initialize the GPIO of backlight
        ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));
        ESP_ERROR_CHECK(gpio_set_level(tftBacklightPin, LCD_BK_LIGHT_ON_LEVEL));
    }
    else
    {
        // PWM Backlight
        ledc_timer_config_t ledc_config_timer = {
            .speed_mode      = LEDC_LOW_SPEED_MODE,
            .duty_resolution = LEDC_TIMER_8_BIT,
            .freq_hz         = 50000,
            .timer_num       = tftLedcTimer,
            .clk_cfg         = LEDC_AUTO_CLK,
        };
        ESP_ERROR_CHECK(ledc_timer_config(&ledc_config_timer));
        ledc_channel_config_t ledc_config_backlight = {
            .gpio_num   = tftBacklightPin,
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .channel    = tftLedcChannel,
            .timer_sel  = tftLedcTimer,
            .duty       = 255, // Disable to start.
        };
        ESP_ERROR_CHECK(ledc_channel_config(&ledc_config_backlight));
        setTFTBacklightBrightness(tftBacklightIntensity);
    }
}

/**
 * @brief Set a single pixel in the display, with bounds check
 *
 * @param x The x coordinate of the pixel to set
 * @param y The y coordinate of the pixel to set
 * @param px The color of the pixel to set
 */
void setPxTft(int16_t x, int16_t y, paletteColor_t px)
{
    if (0 <= x && x <= TFT_WIDTH && 0 <= y && y < TFT_HEIGHT && cTransparent != px)
    {
        pixels[y * TFT_WIDTH + x] = px;
    }
}

/**
 * @brief Get a single pixel in the display
 *
 * @param x The x coordinate of the pixel to get
 * @param y The y coordinate of the pixel to get
 * @return paletteColor_t The color of the given pixel, or black if out of bounds
 */
paletteColor_t getPxTft(int16_t x, int16_t y)
{
    if (0 <= x && x <= TFT_WIDTH && 0 <= y && y < TFT_HEIGHT)
    {
        return pixels[y * TFT_WIDTH + x];
    }
    return c000;
}

/**
 * @brief Clear all pixels in the display to black
 */
void clearPxTft(void)
{
    memset(pixels, c000, sizeof(paletteColor_t) * TFT_HEIGHT * TFT_WIDTH);
}

/**
 * @brief Send the current framebuffer to the TFT display over the SPI bus.
 *
 * This function can be called as quickly as possible
 *
 * Because the SPI driver handles transactions in the background, we can
 * calculate the next line while the previous one is being sent.
 *
 * @param fnBackgroundDrawCallback A function pointer to draw backgrounds while the transmission is occurring
 */
void drawDisplayTft(fnBackgroundDrawCallback_t fnBackgroundDrawCallback)
{
    // Indexes of the line currently being sent to the LCD and the line we're calculating
    uint8_t calc_line = 0;

#ifdef PROC_PROFILE
    uint32_t start, mid, final;
    uart_tx_one_char('f');
#endif

    // Send the frame, ping ponging the send buffer
    for (uint16_t y = 0; y < TFT_HEIGHT; y += PARALLEL_LINES)
    {
        // Calculate a line

#ifdef PROC_PROFILE
        start = get_cCount();
#endif

        // Naive approach is ~100k cycles, later optimization at 60k cycles @ 160 MHz
        // If you quad-pixel it, so you operate on 4 pixels at the same time, you can get it down to 37k cycles.
        // Also FYI - I tried going palette-less, it only saved 18k per chunk (1.6ms per frame)
        uint32_t* outColor = (uint32_t*)s_lines[calc_line];
        uint32_t* inColor  = (uint32_t*)&pixels[y * TFT_WIDTH];
        for (uint16_t x = 0; x < TFT_WIDTH / 4 * PARALLEL_LINES; x++)
        {
            uint32_t colors = *(inColor++);
            uint32_t word1  = paletteColors[(colors >> 0) & 0xff] | (paletteColors[(colors >> 8) & 0xff] << 16);
            uint32_t word2  = paletteColors[(colors >> 16) & 0xff] | (paletteColors[(colors >> 24) & 0xff] << 16);
            outColor[0]     = word1;
            outColor[1]     = word2;
            outColor += 2;
        }

#ifdef PROC_PROFILE
        uart_tx_one_char('g');
        mid = get_cCount();
#endif

        uint8_t sending_line = calc_line;
        calc_line            = !calc_line;

        if (y != 0 && fnBackgroundDrawCallback)
        {
            fnBackgroundDrawCallback(0, y, TFT_WIDTH, PARALLEL_LINES, y / PARALLEL_LINES, TFT_HEIGHT / PARALLEL_LINES);
        }

        // (When operating @ 160 MHz)
        // This code takes 35k cycles when y == 0, but
        // this code takes ~~100k~~ 125k cycles when y != 0...
        // NOTE:
        //  *** You have 780us here, to do whatever you want.  For free. ***
        //  You should avoid when y == 0, but that means you get 14 chunks
        //  every frame.
        //
        // This is because esp_lcd_panel_draw_bitmap blocks until the chunk
        // of frames has been sent.

        // Send the calculated data
        esp_lcd_panel_draw_bitmap(panel_handle, 0, y, TFT_WIDTH, y + PARALLEL_LINES, s_lines[sending_line]);

        if (y == 0 && fnBackgroundDrawCallback)
        {
            fnBackgroundDrawCallback(0, y, TFT_WIDTH, PARALLEL_LINES, y / PARALLEL_LINES, TFT_HEIGHT / PARALLEL_LINES);
        }

#ifdef PROC_PROFILE
        final = get_cCount();
        uart_tx_one_char('h');
#endif
    }

#ifdef PROC_PROFILE
    uart_tx_one_char('i');
    // ESP_LOGI( "tft", "%d/%d", mid - start, final - mid );
#endif
}
