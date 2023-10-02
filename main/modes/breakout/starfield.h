#ifndef _STARFIELD_H_
#define _STARFIELD_H_

//==============================================================================
// Includes
//==============================================================================
#include <stdint.h>
#include <stdbool.h>
#include "palette.h"

//==============================================================================
// Defines
//==============================================================================
#define NUM_STARS 92

//==============================================================================
// Structs
//==============================================================================
typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;

    paletteColor_t color;
} star_t;

typedef struct 
{
    star_t stars[NUM_STARS];
    bool randomColors;
} starfield_t;

//==============================================================================
// Function Prototypes
//==============================================================================
void initializeStarfield(starfield_t *self, bool randomColors);
void updateStarfield(starfield_t *self, int32_t scale);
int randomInt(int lowerBound, int upperBound);
void drawStarfield(starfield_t *self);

#endif
