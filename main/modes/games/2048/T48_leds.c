/**
 * @file T48_leds.c
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief LED helpers for 2048
 * @version 1.0.0
 * @date 2024-08-16
 *
 * @copyright Copyright (c) 2024
 *
 */

#include "T48_leds.h"

void t48DimLEDs(t48_t* t48)
{
    for (uint8_t i = 0; i < CONFIG_NUM_LEDS; i++)
    {
        // Red
        if (t48->leds[i].r < 6)
        {
            t48->leds[i].r = 0;
        }
        else if (t48->leds[i].r == 0)
        {
            // Do nothing
        }
        else
        {
            t48->leds[i].r -= 6;
        }
        // Green
        if (t48->leds[i].g < 6)
        {
            t48->leds[i].g = 0;
        }
        else if (t48->leds[i].g == 0)
        {
            // Do nothing
        }
        else
        {
            t48->leds[i].g -= 6;
        }
        // Blue
        if (t48->leds[i].b < 6)
        {
            t48->leds[i].b = 0;
        }
        else if (t48->leds[i].b == 0)
        {
            // Do nothing
        }
        else
        {
            t48->leds[i].b -= 6;
        }
    }
    setLeds(t48->leds, CONFIG_NUM_LEDS);
}

void t48SetRGB(t48_t* t48, uint8_t idx, led_t color)
{
    t48->leds[idx] = color;
}

void t48LightLEDs(t48_t* t48, t48Direction_t dir, led_t color)
{
    switch (dir)
    {
        case DOWN:
            t48SetRGB(t48, 0, color);
            t48SetRGB(t48, 1, color);
            t48SetRGB(t48, 6, color);
            t48SetRGB(t48, 7, color);
            break;
        case UP:
            t48SetRGB(t48, 2, color);
            t48SetRGB(t48, 3, color);
            t48SetRGB(t48, 4, color);
            t48SetRGB(t48, 5, color);
            break;
        case LEFT:
            t48SetRGB(t48, 0, color);
            t48SetRGB(t48, 1, color);
            t48SetRGB(t48, 2, color);
            t48SetRGB(t48, 3, color);
            break;
        case RIGHT:
            t48SetRGB(t48, 4, color);
            t48SetRGB(t48, 5, color);
            t48SetRGB(t48, 6, color);
            t48SetRGB(t48, 7, color);
            break;
        case ALL:
            t48SetRGB(t48, 0, color);
            t48SetRGB(t48, 1, color);
            t48SetRGB(t48, 2, color);
            t48SetRGB(t48, 3, color);
            t48SetRGB(t48, 4, color);
            t48SetRGB(t48, 5, color);
            t48SetRGB(t48, 6, color);
            t48SetRGB(t48, 7, color);
            break;
    }
}

led_t t48GetLEDColors(t48_t* t48)
{
    uint32_t maxVal = 0;
    for (uint8_t i = 0; i < T48_BOARD_SIZE; i++)
    {
        if (maxVal < t48->board[i].val)
        {
            maxVal = t48->board[i].val;
        }
    }
    led_t col = {0};
    switch (maxVal)
    {
        case 2:
            // Green
            col.g = 128;
            return col;
        case 4:
            // Pink
            col.r = 200;
            col.g = 150;
            col.b = 150;
            return col;
        case 8:
            // Cyan
            col.g = 255;
            col.b = 255;
            return col;
        case 16:
            // Red
            col.r = 255;
            return col;
        case 32:
            // Blue
            col.b = 255;
            return col;
        case 64:
            // Yellow
            col.r = 128;
            col.g = 128;
            return col;
        case 128:
            // Blue
            col.b = 255;
            return col;
        case 256:
            // Orange
            col.r = 255;
            col.g = 165;
            return col;
        case 512:
            // Dark Pink
            col.r = 255;
            col.g = 64;
            col.b = 64;
            return col;
        case 1024:
            // Pink
            col.r = 255;
            col.g = 128;
            col.b = 128;
            return col;
        case 2048:
            // Yellow
            col.r = 255;
            col.g = 255;
            return col;
        case 4096:
            // Purple
            col.r = 200;
            col.b = 200;
            return col;
        case 8192:
            // Mauve
            col.r = 255;
            col.b = 64;
            return col;
        case 16384:
            // Red
            col.r = 255;
            return col;
        case 32768:
            // Green
            col.g = 255;
            return col;
        case 65535:
            // Dark Blue
            col.b = 255;
            return col;
        default:
            col.r = 255;
            col.g = 128;
            col.b = 128;
            return col;
    }
}

led_t t48RandColor()
{
    led_t col = {0};
    col.r     = 128 + (esp_random() % 127);
    col.g     = 128 + (esp_random() % 127);
    col.b     = 128 + (esp_random() % 127);
    return col;
}

void t48RandLEDs(t48_t* t48)
{
    t48->timer -= 1;
    if (t48->timer <= 0)
    {
        t48->timer = T48_LED_TIMER;
        t48LightLEDs(t48, esp_random() % 5, t48RandColor());
    }
}

paletteColor_t t48Rainbow(t48_t* t48)
{
    uint8_t hue = t48->hue++;
    uint8_t sat = 255;
    uint8_t val = 255;
    return paletteHsvToHex(hue, sat, val);
}