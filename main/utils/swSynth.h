#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "hdw-dac.h"

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    SHAPE_SINE,
    SHAPE_SAWTOOTH,
    SHAPE_TRIANGLE,
    SHAPE_SQUARE,
} oscillatorShape_t;

//==============================================================================
// Unions
//==============================================================================

typedef union
{
    uint32_t accum32; /** The oscillator value is accumulated in a 32 bit integer */
    uint8_t bytes[4]; /** The index into the sine table is accessed as a single byte */
} oscAccum_t;

//==============================================================================
// Typedefs
//==============================================================================

typedef int8_t (*waveFunc_t)(uint16_t idx);

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    waveFunc_t waveFunc;
    oscAccum_t accumulator;
    int32_t stepSize;
    uint32_t tVol;
    uint32_t cVol;
} synthOscillator_t;

//==============================================================================
// Function prototypes
//==============================================================================

void swSynthInitOscillator(synthOscillator_t* osc, oscillatorShape_t shape, uint8_t volume);
void swSynthSetShape(synthOscillator_t* osc, oscillatorShape_t shape);
void swSynthSetFreq(synthOscillator_t* osc, uint32_t freq);
uint8_t swSynthMixOscillators(synthOscillator_t* oscs, uint16_t numOscs);
void swSynthSetVolume(synthOscillator_t* osc, uint8_t volume);