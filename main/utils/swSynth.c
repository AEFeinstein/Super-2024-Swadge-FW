//==============================================================================
// Includes
//==============================================================================

#include <inttypes.h>
#include <stdio.h>
#include "esp_attr.h"
#include "hdw-dac.h"
#include "swSynth.h"
#include "macros.h"
#include "fp_math.h"

//==============================================================================
// Constant variables
//==============================================================================

/**
 * @brief Table of 256 8-bit signed values for a sine wave
 *
 * \code{.c}
 * for(int i = 0; i < 256; i++)
 * {
 *     printf("%d, ", (int)round((255*( (sin((i * 2 * M_PI) / 256) + 1) / 2.0f  )) - 128));
 * }
 * @endcode
 */
static const int8_t DRAM_ATTR sinTab[] = {
    -1,   3,    6,    9,    12,   15,   18,   21,   24,   27,   30,   34,   37,   39,   42,   45,   48,   51,   54,
    57,   60,   62,   65,   68,   70,   73,   75,   78,   80,   83,   85,   87,   90,   92,   94,   96,   98,   100,
    102,  104,  106,  107,  109,  110,  112,  113,  115,  116,  117,  118,  120,  121,  122,  122,  123,  124,  125,
    125,  126,  126,  126,  127,  127,  127,  127,  127,  127,  127,  126,  126,  126,  125,  125,  124,  123,  122,
    122,  121,  120,  118,  117,  116,  115,  113,  112,  110,  109,  107,  106,  104,  102,  100,  98,   96,   94,
    92,   90,   87,   85,   83,   80,   78,   75,   73,   70,   68,   65,   62,   60,   57,   54,   51,   48,   45,
    42,   39,   37,   34,   30,   27,   24,   21,   18,   15,   12,   9,    6,    3,    0,    -4,   -7,   -10,  -13,
    -16,  -19,  -22,  -25,  -28,  -31,  -35,  -38,  -40,  -43,  -46,  -49,  -52,  -55,  -58,  -61,  -63,  -66,  -69,
    -71,  -74,  -76,  -79,  -81,  -84,  -86,  -88,  -91,  -93,  -95,  -97,  -99,  -101, -103, -105, -107, -108, -110,
    -111, -113, -114, -116, -117, -118, -119, -121, -122, -123, -123, -124, -125, -126, -126, -127, -127, -127, -128,
    -128, -128, -128, -128, -128, -128, -127, -127, -127, -126, -126, -125, -124, -123, -123, -122, -121, -119, -118,
    -117, -116, -114, -113, -111, -110, -108, -107, -105, -103, -101, -99,  -97,  -95,  -93,  -91,  -88,  -86,  -84,
    -81,  -79,  -76,  -74,  -71,  -69,  -66,  -63,  -61,  -58,  -55,  -52,  -49,  -46,  -43,  -40,  -38,  -35,  -31,
    -28,  -25,  -22,  -19,  -16,  -13,  -10,  -7,   -4,
};

/**
 * @brief Table of 256 8-bit signed values for a triangle wave
 *
 * \code{.c}
 * for(int i = 0; i < 256; i++)
 * {
 *     if(i < 64)
 *     {
 *         printf("%d, ", i * 2);
 *     }
 *     else if (i == 64)
 *     {
 *         printf("%d, ", 127);
 *     }
 *     else if (i <= 192)
 *     {
 *         printf("%d, ", 256 - (i * 2));
 *     }
 *     else
 *     {
 *         printf("%d, ", (i * 2) - 512);
 *     }
 * }
 * @endcode
 */
static const int8_t DRAM_ATTR triTab[] = {
    0,    2,    4,    6,    8,    10,   12,   14,   16,   18,   20,   22,   24,   26,   28,   30,   32,   34,   36,
    38,   40,   42,   44,   46,   48,   50,   52,   54,   56,   58,   60,   62,   64,   66,   68,   70,   72,   74,
    76,   78,   80,   82,   84,   86,   88,   90,   92,   94,   96,   98,   100,  102,  104,  106,  108,  110,  112,
    114,  116,  118,  120,  122,  124,  126,  127,  126,  124,  122,  120,  118,  116,  114,  112,  110,  108,  106,
    104,  102,  100,  98,   96,   94,   92,   90,   88,   86,   84,   82,   80,   78,   76,   74,   72,   70,   68,
    66,   64,   62,   60,   58,   56,   54,   52,   50,   48,   46,   44,   42,   40,   38,   36,   34,   32,   30,
    28,   26,   24,   22,   20,   18,   16,   14,   12,   10,   8,    6,    4,    2,    0,    -2,   -4,   -6,   -8,
    -10,  -12,  -14,  -16,  -18,  -20,  -22,  -24,  -26,  -28,  -30,  -32,  -34,  -36,  -38,  -40,  -42,  -44,  -46,
    -48,  -50,  -52,  -54,  -56,  -58,  -60,  -62,  -64,  -66,  -68,  -70,  -72,  -74,  -76,  -78,  -80,  -82,  -84,
    -86,  -88,  -90,  -92,  -94,  -96,  -98,  -100, -102, -104, -106, -108, -110, -112, -114, -116, -118, -120, -122,
    -124, -126, -128, -126, -124, -122, -120, -118, -116, -114, -112, -110, -108, -106, -104, -102, -100, -98,  -96,
    -94,  -92,  -90,  -88,  -86,  -84,  -82,  -80,  -78,  -76,  -74,  -72,  -70,  -68,  -66,  -64,  -62,  -60,  -58,
    -56,  -54,  -52,  -50,  -48,  -46,  -44,  -42,  -40,  -38,  -36,  -34,  -32,  -30,  -28,  -26,  -24,  -22,  -20,
    -18,  -16,  -14,  -12,  -10,  -8,   -6,   -4,   -2,
};

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Get an 8-bit signed sample from a 256 point sine wave.
 *
 * @param idx The index to get, must be between 0 and 255
 * @return A signed 8-bit sample of a sine wave
 */
static int8_t sineGen(uint16_t idx, void* data __attribute__((unused)))
{
    return sinTab[idx];
}

/**
 * @brief Get an 8-bit signed sample from a 256 point square wave.
 *
 * This has a smaller amplitude than the other waves because it is naturally louder
 *
 * @param idx The index to get, must be between 0 and 255
 * @return A signed 8-bit sample of a square wave
 */
static int8_t squareGen(uint16_t idx, void* data __attribute__((unused)))
{
    return (idx >= 128) ? 64 : -64;
}

/**
 * @brief Get an 8-bit signed sample from a 256 point sawtooth wave.
 *
 * @param idx The index to get, must be between 0 and 255
 * @return A signed 8-bit sample of a sawtooth wave
 */
static int8_t sawtoothGen(uint16_t idx, void* data __attribute__((unused)))
{
    return (idx - 128);
}

/**
 * @brief Get an 8-bit signed sample from a 256 point triangle wave.
 *
 * @param idx The index to get, must be between 0 and 255
 * @return A signed 8-bit sample of a triangle wave
 */
static int8_t triangleGen(uint16_t idx, void* data __attribute__((unused)))
{
    return triTab[idx];
}

/**
 * @brief Get an 8-bit signed random noise sample
 *
 * See https://en.wikipedia.org/wiki/Linear-feedback_shift_register#Fibonacci_LFSRs
 *
 * @param idx Unused
 * @return A random signed 8-bit sample
 */
static int8_t noiseGen(uint16_t idx __attribute__((unused)), void* data __attribute__((unused)))
{
    /* Static variable persists between function calls */
    static uint16_t shiftReg = 0xACE1u;

    /* taps: 16 14 13 11; feedback polynomial: x^16 + x^14 + x^13 + x^11 + 1 */
    uint16_t bit = ((shiftReg >> 0) ^ (shiftReg >> 2) ^ (shiftReg >> 3) ^ (shiftReg >> 5)) & 1u;
    shiftReg     = (shiftReg >> 1) | (bit << 15);

    /* This will return as an 8-bit signed value */
    return shiftReg;
}

/**
 * @brief Initialize a software synthesizer oscillator
 *
 * @param osc The oscillator to initialize
 * @param shape The shape of the wave to generate
 * @param freq The frequency of the wave to generate, in hertz
 * @param volume The volume (amplitude) of the wave to generate
 */
void swSynthInitOscillator(synthOscillator_t* osc, oscillatorShape_t shape, uint32_t freq, uint8_t volume)
{
    osc->accumulator.accum32 = 0;
    osc->cVol                = 0;
    osc->stepSize            = 0;
    osc->waveFuncData        = NULL;
    swSynthSetShape(osc, shape);
    swSynthSetFreq(osc, freq);
    swSynthSetVolume(osc, volume);
}

/**
 * @brief Initialize a software synthesizer oscillator
 *
 * @param osc The oscillator to initialize
 * @param waveFunc The wave function to use
 * @param waveData Custom data to pass into the wave function
 * @param freq The frequency of the wave to generate, in hertz
 * @param volume The volume (amplitude) of the wave to generate
 */
void swSynthInitOscillatorWave(synthOscillator_t* osc, waveFunc_t waveFunc, void* waveData, uint32_t freq, uint8_t volume)
{
    osc->accumulator.accum32 = 0;
    osc->cVol                = 0;
    osc->stepSize            = 0;
    swSynthSetWaveFunc(osc, waveFunc, waveData);
    swSynthSetFreq(osc, freq);
    swSynthSetVolume(osc, volume);
}

/**
 * @brief Set a software synthesizer oscillator's shape
 *
 * @param osc The oscillator to set the shape for
 * @param shape The shape to set (sine, square, sawtooth, triangle, or noise)
 */
void swSynthSetShape(synthOscillator_t* osc, oscillatorShape_t shape)
{
    osc->waveFuncData = NULL;
    switch (shape)
    {
        case SHAPE_SINE:
        {
            osc->waveFunc = sineGen;
            break;
        }
        case SHAPE_SAWTOOTH:
        {
            osc->waveFunc = sawtoothGen;
            break;
        }
        case SHAPE_SQUARE:
        {
            osc->waveFunc = squareGen;
            break;
        }
        case SHAPE_TRIANGLE:
        {
            osc->waveFunc = triangleGen;
            break;
        }
        case SHAPE_NOISE:
        {
            osc->waveFunc = noiseGen;
            break;
        }
    }
}

void swSynthSetWaveFunc(synthOscillator_t* osc, waveFunc_t waveFunc, void* waveFuncData)
{
    osc->waveFunc = waveFunc;
    osc->waveFuncData = waveFuncData;
}

/**
 * @brief Set the frequency of an oscillator
 *
 * @param osc The oscillator to set the frequency for
 * @param freq The frequency to set
 */
void swSynthSetFreq(synthOscillator_t* osc, uint32_t freq)
{
    osc->stepSize = ((uint64_t)(ARRAY_SIZE(sinTab) * freq) << 16) / (DAC_SAMPLE_RATE_HZ);
}

/**
 * @brief Set the frequency of an oscillator with 16 bits of decimal precision
 *
 * @param osc The oscillator to set the frequency for
 * @param freq The frequency to set, as a fixed-point value with 16 bits of decimal precision
 */
void swSynthSetFreqPrecise(synthOscillator_t* osc, uq16_16 freq)
{
    // equivalent to ((ARRAY_SIZE(sinTab) * freq) / DAC_SAMPLE_RATE_HZ except without overflow
    osc->stepSize = freq / (DAC_SAMPLE_RATE_HZ >> 8);
}

/**
 * @brief Set the volume (amplitude) of an oscillator
 *
 * @param osc The oscillator to set the volume for
 * @param volume The volume, 255 is loudest, 0 is off
 */
void swSynthSetVolume(synthOscillator_t* osc, uint8_t volume)
{
    osc->tVol = volume;
}

/**
 * @brief Increment a set of oscillators by one step each, mix together the resulting samples, and return the single
 * mixed sample
 *
 * @param oscillators An array of pointers to oscillators to step and mix together
 * @param numOscillators The number of oscillators to step and mix
 * @return The mixed unsigned 8-bit output sample
 */
uint8_t swSynthMixOscillators(synthOscillator_t* oscillators[], uint16_t numOscillators)
{
    // Return the 8-bit unsigned sample
    return (swSynthSumOscillators(oscillators, numOscillators) / numOscillators) + 128;
}

/**
 * @brief Increment and mix together a set of oscillators like swSynthMixOscillators(), but returns the
 * intermediate sample sum rather than the average, to allow mixing in other sources without losing precision.
 *
 * The caller must divide this value by the number of oscillators (plus the number of other sources) then add
 * 128 to the result to convert it to an unsigned 8-bit value.
 *
 * @param oscillators An array of oscillator pointers
 * @param numOscillators The number of members in oscillators
 * @return int32_t The signed sum of all oscillator samples
 */
int32_t swSynthSumOscillators(synthOscillator_t* oscillators[], uint16_t numOscillators)
{
    // Start off with an empty sample. It's 32-bit for math but will be returned as 8-bit
    int32_t sample = 0;
    // For each oscillator
    for (int32_t oscIdx = 0; oscIdx < numOscillators; oscIdx++)
    {
        synthOscillator_t* osc = oscillators[oscIdx];
        // Step the oscillator's accumulator
        osc->accumulator.accum32 += osc->stepSize;

        // If the oscillator's current volume doesn't match the target volume
        if (osc->cVol != osc->tVol)
        {
            // Either increment or decrement it, depending
            if (osc->cVol < osc->tVol)
            {
                osc->cVol++;
            }
            else
            {
                osc->cVol--;
            }
        }

        // Mix this oscillator's output into the sample
        sample += ((osc->waveFunc(osc->accumulator.bytes[2], osc->waveFuncData) * ((int32_t)osc->cVol)) / 256);
    }

    return sample;
}
