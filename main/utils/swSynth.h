/*! \file swSynth.h
 *
 * \section swSynth_design Design Philosophy
 *
 * This utility code is provided to run and mix oscillators for DAC output (hdw-dac.h). Each oscillator has a shape,
 * frequency, and amplitude. The oscillators assume the output sample rate is ::DAC_SAMPLE_RATE_HZ.
 *
 * When an oscillator's amplitude is changed by swSynthSetVolume(), it will linearly adjust the output volume over time.
 * This avoids clicks and pops caused by discontinuities in the output. Changing the oscillator's frequency will not
 * cause any discontinuities. Changing the oscillator's shape will have a discontinuity and likely have an audible click
 * or pop.
 *
 * \section swSynth_usage Usage
 *
 * Initialize an oscillator with swSynthInitOscillator(). Change the oscillator's properties with swSynthSetShape(),
 * swSynthSetFreq(), or swSynthSetVolume().
 *
 * Call swSynthMixOscillators() to step a set of oscillators, mix their output, and return it for a DAC buffer.
 *
 * \section swSynth_example Example
 *
 * \code{.c}
 * // Create a 440hz triangle wave at max volume
 * synthOscillator_t osc_tri;
 * swSynthInitOscillator(&osc_tri, SHAPE_TRIANGLE, 440, 255);
 *
 * // Create a 576hz sine wave at max volume
 * synthOscillator_t osc_sine;
 * swSynthInitOscillator(&osc_sine, SHAPE_SINE, 576, 255);
 *
 * // Make an array of pointers to the oscillators
 * synthOscillator_t* oscillators[] = {&osc_tri, &osc_sine};
 *
 * // Fill up a sample buffer
 * uint8_t sampleBuf[2048];
 * for (uint32_t i = 0; i < ARRAY_SIZE(sampleBuf); i++)
 * {
 *     sampleBuf[i] = swSynthMixOscillators(oscillators, ARRAY_SIZE(oscillators));
 * }
 * \endcode
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "fp_math.h"

//==============================================================================
// Defines
//==============================================================================

/** The maximum speaker volume */
#define SPK_MAX_VOLUME 255

//==============================================================================
// Enums
//==============================================================================

/**
 * @brief The different wave shapes that can be generated
 */
typedef enum
{
    SHAPE_SINE,     ///< A sine wave
    SHAPE_SAWTOOTH, ///< A sawtooth wave
    SHAPE_TRIANGLE, ///< A triangle wave
    SHAPE_SQUARE,   ///< A square wave
    SHAPE_NOISE,    ///< Random noise from a linear feedback shift register
} oscillatorShape_t;

//==============================================================================
// Unions
//==============================================================================

/**
 * @brief An accumulator used to increment through a wave. A value is accumulated in a 32-bit integer and bits 16->23
 * are used as an index into a 256 point wave
 */
typedef union
{
    uint32_t accum32; ///< The oscillator value is accumulated in a 32 bit integer
    uint8_t bytes[4]; ///< The index into the sine table is accessed as a single byte
} oscAccum_t;

//==============================================================================
// Typedefs
//==============================================================================

/**
 * @brief Function typedef to return a sample from a wave
 *
 * @param idx The index of the wave to get a sample from
 * @return A signed 8-bit sample
 */
typedef int8_t (*waveFunc_t)(uint16_t idx, void* data);

//==============================================================================
// Structs
//==============================================================================

/**
 * @brief A software oscillator with controllable frequency, amplitude, and shape
 */
typedef struct
{
    waveFunc_t waveFunc;    ///< A pointer to the function which generates samples
    void* waveFuncData;     ///< A pointer to pass to the wave function
    oscAccum_t accumulator; ///< An accumulator to increment the wave sample
    int32_t stepSize;       ///< The step that should be added to the accumulator each sample, dependent on frequency
    uint32_t tVol;          ///< The target volume (amplitude)
    uint32_t cVol;          ///< The current volume which smoothly transitions to the target volume
} synthOscillator_t;

//==============================================================================
// Function prototypes
//==============================================================================

void swSynthInitOscillator(synthOscillator_t* osc, oscillatorShape_t shape, uint32_t freq, uint8_t volume);
void swSynthInitOscillatorWave(synthOscillator_t* osc, waveFunc_t waveFunc, void* waveData, uint32_t freq,
                               uint8_t volume);
void swSynthSetShape(synthOscillator_t* osc, oscillatorShape_t shape);
void swSynthSetWaveFunc(synthOscillator_t* osc, waveFunc_t waveFunc, void* waveFuncData);
void swSynthSetFreq(synthOscillator_t* osc, uint32_t freq);
void swSynthSetFreqPrecise(synthOscillator_t* osc, uq16_16 freq);
void swSynthSetVolume(synthOscillator_t* osc, uint8_t volume);
uint8_t swSynthMixOscillators(synthOscillator_t* oscillators[], uint16_t numOscillators);
int32_t swSynthSumOscillators(synthOscillator_t* oscillators[], uint16_t numOscillators);
int8_t swSynthSampleWave(oscillatorShape_t shape, uint8_t idx);
