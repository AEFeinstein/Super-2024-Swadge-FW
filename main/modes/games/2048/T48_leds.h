/**
 * @file T48_leds.h
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief LED helpers for 2048
 * @version 1.0.0
 * @date 2024-08-16
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include "mode_2048.h"

//==============================================================================
// Function Prototypes
//==============================================================================

/**
 * @brief Automatically dims LEDs every frame
 *
 * @param t48 Main Game Object
 */
void t48DimLEDs(t48_t* t48);

/**
 * @brief Sets an LED to a color
 *
 * @param t48 Main Game Object
 * @param idx   Index of the LED
 * @param color Color to set LED
 */
void t48SetRGB(t48_t* t48, uint8_t idx, led_t color);

/**
 * @brief Illuminate appropriate LEDs based on an indicated direction
 *
 * @param t48 Main Game Object
 * @param dir   Direction to illuminate LEDs
 * @param color Color to set the LEDs to
 */
void t48LightLEDs(t48_t* t48, t48Direction_t dir, led_t color);

/**
 * @brief Based on the highest block value, set the LED color
 *
 * @param t48 Main Game Object
 * @return led_t Color object send to the LEDs
 */
led_t t48GetLEDColors(t48_t* t48);

/**
 * @brief Get a random bright color for the LEDs
 *
 * @return led_t Color to send to LEDs
 */
led_t t48RandColor(void);

/**
 * @brief Run an random LED program
 *
 * @param t48 Main Game Object
 */
void t48RandLEDs(t48_t* t48);

/**
 * @brief Returns values in hue sequence
 *
 * @param t48 Main Game Object
 * @return paletteColor_t Color to use
 */
paletteColor_t t48Rainbow(t48_t* t48);