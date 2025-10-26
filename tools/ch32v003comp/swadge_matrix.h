// Generic header file for all Swadge ch32v003 code for outputting to a matrix.

#ifndef _SWADGE_MATRIX_H
#define _SWADGE_MATRIX_H

#include <stdint.h>
#include "ch32fun.h"
#include <stdio.h>

#define TARGET_FPS 90
#define MAX_INTENSITY 255

#define EYE_LED_OFF    0x00
#define EYE_LED_DIM    0x08
#define EYE_LED_BRIGHT 0x10

#define EYE_LED_W 12
#define EYE_LED_H 6

void MatrixSetup();
int apsqrt( int i );
void SetPixel( int x, int y, int intensity );
extern uint8_t LEDSets[9*8];
extern const uint16_t Coordmap[8*16];
int FLASH_WaitForLastOperation(uint32_t Timeout);

#endif
