#ifndef _STARFIELD_H_
#define _STARFIELD_H_

//==============================================================================
// Includes
//==============================================================================
#include <stdint.h>

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

    uint8_t color;
} star_t;

typedef struct 
{
    star_t stars[NUM_STARS]
} starfield_t;

//==============================================================================
// Function Prototypes
//==============================================================================
void initializeStarfield(starfield_t *self);
void updateStarfield(starfield_t *self);
int randomInt(int lowerBound, int upperBound);
void drawStarfield(starfield_t *self);

#endif
