/*
 * dance.h
 *
 *  Created on: Nov 10, 2018
 *      Author: adam
 */

#ifndef _DANCE_H_
#define _DANCE_H_

#include "swadge2024.h"

// Helper macros to pack an RGB color into an argument
#define RGB_2_ARG(r, g, b) ((((r) & 0xFF) << 16) | (((g) & 0xFF) << 8) | (((b) & 0xFF)))
#define ARG_R(arg)         (((arg) >> 16) & 0xFF)
#define ARG_G(arg)         (((arg) >> 8) & 0xFF)
#define ARG_B(arg)         (((arg) >> 0) & 0xFF)

/**
 * @brief A function to animate LEDs
 *
 * @param tElapsedUs The time since this was last called
 * @param arg An optional argument such as color, timing, etc.
 * @param reset true to reset state variables, false to run normally
 */
typedef void (*ledDanceFunc)(uint32_t tElapsedUs, uint32_t arg, bool reset);

typedef struct
{
    ledDanceFunc func; ///< A 'main loop' function to animate and set the LEDs
    uint32_t arg;      ///< An optional argument for the \c func, may be color, speed, etc.
    char* name;        ///< The name of this LED dance
} ledDance_t;

extern swadgeMode_t danceMode;
extern const ledDance_t ledDances[];

uint8_t getNumDances(void);
uint32_t danceRand(uint32_t bound);

#endif /* MODE_DANCE_H_ */
