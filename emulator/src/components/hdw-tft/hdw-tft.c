//==============================================================================
// Includes
//==============================================================================

#include <string.h>
#include <stdlib.h>

#include "hdw-tft.h"
#include "hdw-tft_emu.h"
#include "emu_main.h"

//==============================================================================
// Const variables
//==============================================================================

static const uint32_t paletteColorsEmu[216] = {
    0x000000FF, 0x000033FF, 0x000066FF, 0x000099FF, 0x0000CCFF, 0x0000FFFF, 0x003300FF, 0x003333FF, 0x003366FF,
    0x003399FF, 0x0033CCFF, 0x0033FFFF, 0x006600FF, 0x006633FF, 0x006666FF, 0x006699FF, 0x0066CCFF, 0x0066FFFF,
    0x009900FF, 0x009933FF, 0x009966FF, 0x009999FF, 0x0099CCFF, 0x0099FFFF, 0x00CC00FF, 0x00CC33FF, 0x00CC66FF,
    0x00CC99FF, 0x00CCCCFF, 0x00CCFFFF, 0x00FF00FF, 0x00FF33FF, 0x00FF66FF, 0x00FF99FF, 0x00FFCCFF, 0x00FFFFFF,
    0x330000FF, 0x330033FF, 0x330066FF, 0x330099FF, 0x3300CCFF, 0x3300FFFF, 0x333300FF, 0x333333FF, 0x333366FF,
    0x333399FF, 0x3333CCFF, 0x3333FFFF, 0x336600FF, 0x336633FF, 0x336666FF, 0x336699FF, 0x3366CCFF, 0x3366FFFF,
    0x339900FF, 0x339933FF, 0x339966FF, 0x339999FF, 0x3399CCFF, 0x3399FFFF, 0x33CC00FF, 0x33CC33FF, 0x33CC66FF,
    0x33CC99FF, 0x33CCCCFF, 0x33CCFFFF, 0x33FF00FF, 0x33FF33FF, 0x33FF66FF, 0x33FF99FF, 0x33FFCCFF, 0x33FFFFFF,
    0x660000FF, 0x660033FF, 0x660066FF, 0x660099FF, 0x6600CCFF, 0x6600FFFF, 0x663300FF, 0x663333FF, 0x663366FF,
    0x663399FF, 0x6633CCFF, 0x6633FFFF, 0x666600FF, 0x666633FF, 0x666666FF, 0x666699FF, 0x6666CCFF, 0x6666FFFF,
    0x669900FF, 0x669933FF, 0x669966FF, 0x669999FF, 0x6699CCFF, 0x6699FFFF, 0x66CC00FF, 0x66CC33FF, 0x66CC66FF,
    0x66CC99FF, 0x66CCCCFF, 0x66CCFFFF, 0x66FF00FF, 0x66FF33FF, 0x66FF66FF, 0x66FF99FF, 0x66FFCCFF, 0x66FFFFFF,
    0x990000FF, 0x990033FF, 0x990066FF, 0x990099FF, 0x9900CCFF, 0x9900FFFF, 0x993300FF, 0x993333FF, 0x993366FF,
    0x993399FF, 0x9933CCFF, 0x9933FFFF, 0x996600FF, 0x996633FF, 0x996666FF, 0x996699FF, 0x9966CCFF, 0x9966FFFF,
    0x999900FF, 0x999933FF, 0x999966FF, 0x999999FF, 0x9999CCFF, 0x9999FFFF, 0x99CC00FF, 0x99CC33FF, 0x99CC66FF,
    0x99CC99FF, 0x99CCCCFF, 0x99CCFFFF, 0x99FF00FF, 0x99FF33FF, 0x99FF66FF, 0x99FF99FF, 0x99FFCCFF, 0x99FFFFFF,
    0xCC0000FF, 0xCC0033FF, 0xCC0066FF, 0xCC0099FF, 0xCC00CCFF, 0xCC00FFFF, 0xCC3300FF, 0xCC3333FF, 0xCC3366FF,
    0xCC3399FF, 0xCC33CCFF, 0xCC33FFFF, 0xCC6600FF, 0xCC6633FF, 0xCC6666FF, 0xCC6699FF, 0xCC66CCFF, 0xCC66FFFF,
    0xCC9900FF, 0xCC9933FF, 0xCC9966FF, 0xCC9999FF, 0xCC99CCFF, 0xCC99FFFF, 0xCCCC00FF, 0xCCCC33FF, 0xCCCC66FF,
    0xCCCC99FF, 0xCCCCCCFF, 0xCCCCFFFF, 0xCCFF00FF, 0xCCFF33FF, 0xCCFF66FF, 0xCCFF99FF, 0xCCFFCCFF, 0xCCFFFFFF,
    0xFF0000FF, 0xFF0033FF, 0xFF0066FF, 0xFF0099FF, 0xFF00CCFF, 0xFF00FFFF, 0xFF3300FF, 0xFF3333FF, 0xFF3366FF,
    0xFF3399FF, 0xFF33CCFF, 0xFF33FFFF, 0xFF6600FF, 0xFF6633FF, 0xFF6666FF, 0xFF6699FF, 0xFF66CCFF, 0xFF66FFFF,
    0xFF9900FF, 0xFF9933FF, 0xFF9966FF, 0xFF9999FF, 0xFF99CCFF, 0xFF99FFFF, 0xFFCC00FF, 0xFFCC33FF, 0xFFCC66FF,
    0xFFCC99FF, 0xFFCCCCFF, 0xFFCCFFFF, 0xFFFF00FF, 0xFFFF33FF, 0xFFFF66FF, 0xFFFF99FF, 0xFFFFCCFF, 0xFFFFFFFF,
};

//==============================================================================
// Variables
//==============================================================================

static paletteColor_t* frameBuffer   = NULL;
static uint32_t* scaledBitmapDisplay = NULL; // 0xRRGGBBAA
static int bitmapWidth               = 0;
static int bitmapHeight              = 0;
static int displayMult               = 1;
static bool tftDisabled              = false;
static uint8_t tftBrightness         = CONFIG_TFT_MAX_BRIGHTNESS;

//==============================================================================
// Functions
//==============================================================================

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
 */
void initTFT(spi_host_device_t spiHost, gpio_num_t sclk, gpio_num_t mosi, gpio_num_t dc, gpio_num_t cs, gpio_num_t rst,
             gpio_num_t backlight, bool isPwmBacklight, ledc_channel_t ledcChannel)
{
    // ARGB pixels
    bitmapWidth  = TFT_WIDTH;
    bitmapHeight = TFT_HEIGHT;

    // Set up underlying bitmap
    if (NULL == frameBuffer)
    {
        frameBuffer = calloc(TFT_WIDTH * TFT_HEIGHT, sizeof(paletteColor_t));
    }

    // This may be setup by the emulator already
    if (NULL == scaledBitmapDisplay)
    {
        displayMult         = 1;
        scaledBitmapDisplay = calloc(TFT_WIDTH * TFT_HEIGHT, sizeof(uint32_t));
    }
}

/**
 * @brief Deinitialize TFT
 *
 */
void deinitTFT(void)
{
    if (frameBuffer)
    {
        free(frameBuffer);
        frameBuffer = NULL;
    }

    if (scaledBitmapDisplay)
    {
        free(scaledBitmapDisplay);
        scaledBitmapDisplay = NULL;
    }
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
    return frameBuffer;
}

/**
 * @brief Disable the backlight (for power down)
 *
 */
void disableTFTBacklight(void)
{
    tftDisabled = true;
    clearPxTft();
}

/**
 * @brief Enable the backlight
 *
 */
void enableTFTBacklight(void)
{
    tftDisabled = false;
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
    if (tftDisabled)
    {
        return;
    }

    if (0 <= x && x < TFT_WIDTH && 0 <= y && y < TFT_HEIGHT)
    {
        frameBuffer[(y * TFT_WIDTH) + x] = px;
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
    if (tftDisabled)
    {
        return c000;
    }

    if (0 <= x && x < TFT_WIDTH && 0 <= y && y < TFT_HEIGHT)
    {
        paletteColor_t px = frameBuffer[(y * TFT_WIDTH) + x];
        return px;
    }
    return c000;
}

/**
 * @brief Clear all pixels in the display to black
 */
void clearPxTft(void)
{
    memset(frameBuffer, c000, sizeof(paletteColor_t) * TFT_HEIGHT * TFT_WIDTH);
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
    if (tftDisabled)
    {
        // Wipe any framebuffer changes
        clearPxTft();
    }

    /* Copy the current framebuffer to memory that won't be modified by the
     * Swadge mode. rawdraw will use this non-changing bitmap to draw
     */
    int16_t y;
    for (y = 0; y < TFT_HEIGHT; y++)
    {
        for (int16_t x = 0; x < TFT_WIDTH; x++)
        {
            for (uint16_t mY = 0; mY < displayMult; mY++)
            {
                for (uint16_t mX = 0; mX < displayMult; mX++)
                {
                    int dstX  = ((x * displayMult) + mX);
                    int dstY  = ((y * displayMult) + mY);
                    int pxIdx = (dstY * (TFT_WIDTH * displayMult)) + dstX;

                    uint32_t color = paletteColorsEmu[frameBuffer[(y * TFT_WIDTH) + x]];

                    uint8_t a = (color)&0xFF;
                    uint8_t r = (color >> 8) & 0xFF;
                    r         = (r * tftBrightness) / CONFIG_TFT_MAX_BRIGHTNESS;
                    uint8_t g = (color >> 16) & 0xFF;
                    g         = (g * tftBrightness) / CONFIG_TFT_MAX_BRIGHTNESS;
                    uint8_t b = (color >> 24) & 0xFF;
                    b         = (b * tftBrightness) / CONFIG_TFT_MAX_BRIGHTNESS;

                    color = (b << 24) | (g << 16) | (r << 8) | (a);

                    scaledBitmapDisplay[pxIdx] = color;
                }
            }
        }

        if ((y & 0xf) == 0 && fnBackgroundDrawCallback && y > 0)
        {
            fnBackgroundDrawCallback(0, y - 16, TFT_WIDTH, 16, (y - 16) / 16, TFT_HEIGHT / 16);
        }
    }

    if (fnBackgroundDrawCallback)
    {
        fnBackgroundDrawCallback(0, y - 16, TFT_WIDTH, 16, (y - 16) / 16, TFT_HEIGHT / 16);
    }
}

/**
 * @brief Set TFT Backlight brightness.
 *
 * @param intensity    Sets the brightness 0-255
 *
 * @return value is 0 if OK nonzero if error.
 */
esp_err_t setTFTBacklightBrightness(uint8_t intensity)
{
    tftBrightness = intensity;
    return ESP_OK;
}

/**
 * Set a multiplier to draw the TFT to the window at
 *
 * @param multiplier The multiplier for the display, no less than 1
 */
void setDisplayBitmapMultiplier(uint8_t multiplier)
{
    displayMult = multiplier;

    // Reallocate scaledBitmapDisplay
    free(scaledBitmapDisplay);
    scaledBitmapDisplay = calloc((multiplier * TFT_WIDTH) * (multiplier * TFT_HEIGHT), sizeof(uint32_t));
}

/**
 * @brief Get a pointer to the display memory.
 *
 * @param width A pointer to return the width of the display through
 * @param height A pointer to return the height of the display through
 * @return A pointer to the bitmap pixels for the display
 */
uint32_t* getDisplayBitmap(uint16_t* width, uint16_t* height)
{
    *width  = (bitmapWidth * displayMult);
    *height = (bitmapHeight * displayMult);
    return scaledBitmapDisplay;
}
