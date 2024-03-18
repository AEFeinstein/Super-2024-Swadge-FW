//==============================================================================
// Includes
//==============================================================================

#include <inttypes.h>
#include <stdio.h>
#include "esp_attr.h"
#include "swSynth.h"

//==============================================================================
// Constant variables
//==============================================================================

/*
for(int i = 0; i < 256; i++)
{
    printf("%d, ", (int)round((255*( (sin((i * 2 * M_PI) / 256) + 1) / 2.0f  )) - 128));
}
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
 * @brief TODO
 *
 * @param idx
 * @return int8_t
 */
static int8_t sineGen(uint16_t idx)
{
    return sinTab[idx];
}

/**
 * @brief TODO
 *
 * @param idx
 * @return int8_t
 */
static int8_t squareGen(uint16_t idx)
{
    return sinTab[idx] >= 0 ? 64 : -64;
}

/**
 * @brief TODO
 *
 * @param idx
 * @return int8_t
 */
static int8_t sawtoothGen(uint16_t idx)
{
    return idx - 128;
}

/**
 * @brief TODO
 *
 * @param idx
 * @return int8_t
 */
static int8_t triangleGen(uint16_t idx)
{
    return triTab[idx];
}

/**
 * @brief TODO
 *
 * @param osc
 * @param shape
 * @param volume
 */
void swSynthInitOscillator(synthOscillator_t* osc, oscillatorShape_t shape, uint8_t volume)
{
    osc->accumulator.accum32 = 0;
    osc->cVol                = 0;
    osc->stepSize            = 0;
    swSynthSetVolume(osc, volume);
    swSynthSetShape(osc, shape);
}

/**
 * @brief TODO
 *
 * @param osc
 * @param shape
 */
void swSynthSetShape(synthOscillator_t* osc, oscillatorShape_t shape)
{
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
    }
}

/**
 * @brief TODO
 *
 * @param osc
 * @param freq
 */
void swSynthSetFreq(synthOscillator_t* osc, uint32_t freq)
{
    osc->stepSize = ((uint64_t)(sizeof(sinTab) * freq) << 16) / (AUDIO_SAMPLE_RATE_HZ);
}

/**
 * @brief TODO
 *
 * @param osc
 * @param volume 255 is loudest, 0 is off
 */
void swSynthSetVolume(synthOscillator_t* osc, uint8_t volume)
{
    osc->tVol = volume;
}

/**
 * @brief TODO
 *
 * @param oscs
 * @param numOscs
 * @return uint8_t
 */
uint8_t swSynthMixOscillators(synthOscillator_t* oscs[], uint16_t numOscs)
{
    int32_t sample               = 0;
    int32_t numOscillatorsActive = 0;
    for (int32_t oscIdx = 0; oscIdx < numOscs; oscIdx++)
    {
        synthOscillator_t* osc = oscs[oscIdx];
        osc->accumulator.accum32 += osc->stepSize;

        if (osc->cVol != osc->tVol)
        {
            if (osc->cVol < osc->tVol)
            {
                osc->cVol++;
            }
            else
            {
                osc->cVol--;
            }
        }

        if (osc->cVol > 0)
        {
            sample += ((osc->waveFunc(osc->accumulator.bytes[2]) * osc->cVol) / 256);
            numOscillatorsActive++;
        }
        // printf("  %ld %ld\n", oscIdx, sample);
    }
    if (0 == numOscillatorsActive)
    {
        return 127;
    }
    // printf("%ld\n", (sample / numOscillatorsActive) + 128);
    return (sample / numOscs) + 128;
}
