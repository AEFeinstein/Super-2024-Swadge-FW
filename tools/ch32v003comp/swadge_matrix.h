// Generic header file for all Swadge ch32v003 code for outputting to a matrix.

#ifndef _SWADGE_MATRIX_H
#define _SWADGE_MATRIX_H

#include <stdint.h>
#include "ch32fun.h"
#include <stdio.h>

#define TARGET_FPS 90
#define MAX_INTENSITY 255

void MatrixSetup();
int apsqrt( int i );
void SetPixel( int x, int y, int intensity );
extern uint8_t LEDSets[9*8];
extern const uint16_t Coordmap[8*16];

#endif
