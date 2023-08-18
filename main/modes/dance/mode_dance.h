/*
 * mode_dance.h
 *
 *  Created on: Nov 10, 2018
 *      Author: adam
 */

#ifndef MODE_DANCE_H_
#define MODE_DANCE_H_

#include "swadge2024.h"

typedef void (*ledDance)(uint32_t, uint32_t, bool);

typedef struct
{
    ledDance func;
    uint32_t arg;
    char* name;
} ledDanceArg;

extern swadgeMode_t modeDance;
extern const ledDanceArg ledDances[];

void danceComet(uint32_t tElapsedUs, uint32_t arg, bool reset);
void danceFlashlight(uint32_t tElapsedUs, uint32_t arg, bool reset);
void danceRise(uint32_t tElapsedUs, uint32_t arg, bool reset);
void dancePulse(uint32_t tElapsedUs, uint32_t arg, bool reset);
void danceSmoothRainbow(uint32_t tElapsedUs, uint32_t arg, bool reset);
void danceSharpRainbow(uint32_t tElapsedUs, uint32_t arg, bool reset);
void danceRainbowSolid(uint32_t tElapsedUs, uint32_t arg, bool reset);
void danceBinaryCounter(uint32_t tElapsedUs, uint32_t arg, bool reset);
void danceFire(uint32_t tElapsedUs, uint32_t arg, bool reset);
void dancePoliceSiren(uint32_t tElapsedUs, uint32_t arg, bool reset);
void dancePureRandom(uint32_t tElapsedUs, uint32_t arg, bool reset);
void danceRandomDance(uint32_t tElapsedUs, uint32_t arg, bool reset);
void danceChristmas(uint32_t tElapsedUs, uint32_t arg, bool reset);
void danceNone(uint32_t tElapsedUs, uint32_t arg, bool reset);

uint8_t getNumDances(void);

#endif /* MODE_DANCE_H_ */
