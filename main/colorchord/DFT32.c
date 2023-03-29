// Copyright 2015 <>< Charles Lohr under the ColorChord License.

//==============================================================================
// Includes
//==============================================================================

#include "DFT32.h"
#include <string.h>

#ifndef CCEMBEDDED
    #include <stdlib.h>
    #include <stdio.h>
    #include <math.h>
static float* goutbins;
#endif

//==============================================================================
// Constant data
//==============================================================================

// A table of precomputed sin() values.  Ranging -1500 to +1500
// If we increase this, it may cause overflows elsewhere in code.
const int16_t Ssinonlytable[256] = {
    0,     36,    73,    110,   147,   183,   220,   256,   292,   328,   364,   400,   435,   470,   505,   539,
    574,   607,   641,   674,   707,   739,   771,   802,   833,   863,   893,   922,   951,   979,   1007,  1034,
    1060,  1086,  1111,  1135,  1159,  1182,  1204,  1226,  1247,  1267,  1286,  1305,  1322,  1339,  1355,  1371,
    1385,  1399,  1412,  1424,  1435,  1445,  1455,  1463,  1471,  1477,  1483,  1488,  1492,  1495,  1498,  1499,
    1500,  1499,  1498,  1495,  1492,  1488,  1483,  1477,  1471,  1463,  1455,  1445,  1435,  1424,  1412,  1399,
    1385,  1371,  1356,  1339,  1322,  1305,  1286,  1267,  1247,  1226,  1204,  1182,  1159,  1135,  1111,  1086,
    1060,  1034,  1007,  979,   951,   922,   893,   863,   833,   802,   771,   739,   707,   674,   641,   607,
    574,   539,   505,   470,   435,   400,   364,   328,   292,   256,   220,   183,   147,   110,   73,    36,
    0,     -36,   -73,   -110,  -146,  -183,  -219,  -256,  -292,  -328,  -364,  -399,  -435,  -470,  -505,  -539,
    -573,  -607,  -641,  -674,  -706,  -739,  -771,  -802,  -833,  -863,  -893,  -922,  -951,  -979,  -1007, -1034,
    -1060, -1086, -1111, -1135, -1159, -1182, -1204, -1226, -1247, -1267, -1286, -1305, -1322, -1339, -1355, -1371,
    -1385, -1399, -1412, -1424, -1435, -1445, -1454, -1463, -1471, -1477, -1483, -1488, -1492, -1495, -1498, -1499,
    -1500, -1499, -1498, -1495, -1492, -1488, -1483, -1477, -1471, -1463, -1455, -1445, -1435, -1424, -1412, -1399,
    -1385, -1371, -1356, -1339, -1322, -1305, -1286, -1267, -1247, -1226, -1204, -1182, -1159, -1135, -1111, -1086,
    -1060, -1034, -1007, -979,  -951,  -923,  -893,  -863,  -833,  -802,  -771,  -739,  -707,  -674,  -641,  -608,
    -574,  -540,  -505,  -470,  -435,  -400,  -364,  -328,  -292,  -256,  -220,  -183,  -147,  -110,  -73,   -37,
};

/** The above table was created using the following code:
#include <math.h>
#include <stdio.h>
#include <stdint.h>

int16_t Ssintable[256]; //Actually, just [sin].

int main()
{
    int i;
    for( i = 0; i < 256; i++ )
    {
        Ssintable[i] = (int16_t)((sinf( i / 256.0 * 6.283 ) * 1500.0));
    }

    printf( "const int16_t Ssinonlytable[256] = {" );
    for( i = 0; i < 256; i++ )
    {
        if( !(i & 0x7 ) )
        {
            printf( "\n\t" );
        }
        printf( "%6d," ,Ssintable[i] );
    }
    printf( "};\n" );
} */

//==============================================================================
// Functions
//==============================================================================

// From:
// http://stackoverflow.com/questions/1100090/looking-for-an-efficient-integer-square-root-algorithm-for-arm-thumb2
//   for sqrt approx but also suggestion for quick norm approximation that would work in this DFT

#if APPROXNORM != 1
/**
 * \brief    Fast Square root algorithm, with rounding
 *
 * This does arithmetic rounding of the result. That is, if the real answer
 * would have a fractional part of 0.5 or greater, the result is rounded up to
 * the next integer.
 *      - SquareRootRounded(2) --> 1
 *      - SquareRootRounded(3) --> 2
 *      - SquareRootRounded(4) --> 2
 *      - SquareRootRounded(6) --> 2
 *      - SquareRootRounded(7) --> 3
 *      - SquareRootRounded(8) --> 3
 *      - SquareRootRounded(9) --> 3
 *
 * \param[in] a_nInput - unsigned integer for which to find the square root
 *
 * \return Integer square root of the input value.
 */
static uint16_t SquareRootRounded(uint32_t a_nInput)
{
    uint32_t op  = a_nInput;
    uint32_t res = 0;
    uint32_t one
        = 1uL << 30; // The second-to-top bit is set: use 1u << 14 for uint16_t type; use 1uL<<30 for uint32_t type

    // "one" starts at the highest power of four <= than the argument.
    while (one > op)
    {
        one >>= 2;
    }

    while (one != 0)
    {
        if (op >= res + one)
        {
            op  = op - (res + one);
            res = res + 2 * one;
        }
        res >>= 1;
        one >>= 2;
    }

    /* Do arithmetic rounding to nearest integer */
    if (op > res)
    {
        res++;
    }

    return res;
}
#endif

/**
 * @brief TODO
 *
 * @param dd
 */
void UpdateOutputBins32(dft32_data* dd)
{
    int i;
    int32_t* ipt = &dd->Sdatspace32BOut[0];
    for (i = 0; i < FIXBINS; i++)
    {
        int32_t isps = *(ipt++); // keep 32 bits
        int32_t ispc = *(ipt++);
        // take absolute values
        isps       = isps < 0 ? -isps : isps;
        ispc       = ispc < 0 ? -ispc : ispc;
        int octave = i / FIXBPERO;

        // If we are running DFT32 on regular ColorChord, then we will need to
        // also update goutbins[]... But if we're on embedded systems, we only
        // update dd->embeddedbins32.
#ifndef CCEMBEDDED
        // convert 32 bit precision isps and ispc to floating point
        float mux   = ((float)isps * (float)isps) + ((float)ispc * (float)ispc);
        goutbins[i] = sqrtf(mux) / 65536.0; // scale by 2^16
        // reasonable (but arbitrary attenuation)
        goutbins[i] /= (78 << DFTIIR) * (1 << octave);
#endif

#if APPROXNORM == 1
        // using full 32 bit precision for isps and ispc
        uint32_t rmux = isps > ispc ? isps + (ispc >> 1) : ispc + (isps >> 1);
        rmux          = rmux >> 16; // keep most significant 16 bits
#else
        // use the most significant 16 bits of isps and ispc when squaring
        // since isps and ispc are non-negative right bit shifing is well defined
        uint32_t rmux = ((isps >> 16) * (isps >> 16)) + ((ispc > 16) * (ispc >> 16));
        rmux          = SquareRootRounded(rmux);
#endif

        // bump up all outputs here, so when we nerf it by bit shifting by
        // octave we don't lose a lot of detail.
        rmux = rmux << 1;

        dd->embeddedbins32[i] = rmux >> octave;
    }
}

/**
 * @brief TODO
 *
 * @param dd
 * @param sample
 */
static void HandleInt(dft32_data* dd, int16_t sample)
{
    int i;

    uint8_t oct = dd->Sdo_this_octave[dd->Swhichoctaveplace];
    dd->Swhichoctaveplace++;
    dd->Swhichoctaveplace &= BINCYCLE - 1;

    for (i = 0; i < OCTAVES; i++)
    {
        dd->Saccum_octavebins[i] += sample;
    }

    if (oct > 128)
    {
        // Special: This is when we can update everything.
        // This gets run once out of every (1<<OCTAVES) times.
        //  which is half as many samples
        // It handles updating part of the DFT.
        // It should happen at the very first call to HandleInit
        int32_t* bins    = &dd->Sdatspace32B[0];
        int32_t* binsOut = &dd->Sdatspace32BOut[0];

        for (i = 0; i < FIXBINS; i++)
        {
            // First for the SIN then the COS.
            int32_t val  = *(bins);
            *(binsOut++) = val;
            *(bins++) -= val >> DFTIIR;

            val          = *(bins);
            *(binsOut++) = val;
            *(bins++) -= val >> DFTIIR;
        }
        return;
    }

    if ((oct * FIXBPERO * 2) < (FIXBINS * 2) && (oct <= OCTAVES))
    {
        // process a filtered sample for one of the octaves
        uint16_t* dsA              = &dd->Sdatspace32A[oct * FIXBPERO * 2];
        int32_t* dsB               = &dd->Sdatspace32B[oct * FIXBPERO * 2];
        int16_t filteredsample     = dd->Saccum_octavebins[oct] >> (OCTAVES - oct);
        dd->Saccum_octavebins[oct] = 0;

        for (i = 0; i < FIXBPERO; i++)
        {
            uint16_t adv     = *(dsA++);
            uint8_t localipl = *(dsA) >> 8;
            *(dsA++) += adv;

            *(dsB++) += (Ssinonlytable[localipl] * filteredsample);
            // Get the cosine (1/4 wavelength out-of-phase with sin)
            localipl += 64;
            *(dsB++) += (Ssinonlytable[localipl] * filteredsample);
        }
    }
}

/**
 * @brief TODO
 *
 * @param dd
 * @return int
 */
int SetupDFTProgressive32(dft32_data* dd)
{
    int i;
    int j;

    dd->Sdonefirstrun      = 1;
    dd->Sdo_this_octave[0] = 0xff;
    for (i = 0; i < BINCYCLE - 1; i++)
    {
        // dd->Sdo_this_octave =
        // 255 4 3 4 2 4 3 4 1 4 3 4 2 4 3 4 0 4 3 4 2 4 3 4 1 4 3 4 2 4 3 4 is case for 5 octaves.
        // Initial state is special one, then at step i do octave = dd->Sdo_this_octave with averaged samples from last
        // update of that octave
        // search for "first" zero

        for (j = 0; j <= OCTAVES; j++)
        {
            if (((1 << j) & i) == 0)
            {
                break;
            }
        }
        if (j > OCTAVES)
        {
#ifndef CCEMBEDDED
            fprintf(stderr, "Error: algorithm fault.\n");
            exit(-1);
#endif
            return -1;
        }
        dd->Sdo_this_octave[i + 1] = OCTAVES - j - 1;
    }
    return 0;
}

/**
 * @brief TODO
 *
 * @param dd
 * @param frequencies
 */
void UpdateBins32(dft32_data* dd, const uint16_t* frequencies)
{
    int i;
    int imod = 0;
    for (i = 0; i < FIXBINS; i++, imod++)
    {
        if (imod >= FIXBPERO)
        {
            imod = 0;
        }
        uint16_t freq           = frequencies[imod];
        dd->Sdatspace32A[i * 2] = freq; // / oneoveroctave;
    }
}

/**
 * @brief TODO
 *
 * @param dd
 * @param dat
 */
void PushSample32(dft32_data* dd, int16_t dat)
{
    HandleInt(dd, dat);
    HandleInt(dd, dat);
}

#ifndef CCEMBEDDED

/**
 * @brief TODO
 *
 * @param dd
 * @param frequencies
 */
void UpdateBinsForDFT32(dft32_data* dd, const float* frequencies)
{
    int i;
    for (i = 0; i < FIXBINS; i++)
    {
        float freq              = frequencies[(i % FIXBPERO) + (FIXBPERO * (OCTAVES - 1))];
        dd->Sdatspace32A[i * 2] = (65536.0 / freq); // / oneoveroctave;
    }
}

/**
 * @brief TODO
 *
 * @param dd
 * @param outbins
 * @param frequencies
 * @param bins
 * @param databuffer
 * @param place_in_data_buffer
 * @param size_of_data_buffer
 * @param q
 * @param speedup
 */
void DoDFTProgressive32(dft32_data* dd, float* outbins, float* frequencies, int bins, const float* databuffer,
                        int place_in_data_buffer, int size_of_data_buffer, float q, float speedup)
{
    static float backupbins[FIXBINS];
    int i;
    static int last_place;

    memset(outbins, 0, bins * sizeof(float));
    goutbins = outbins;

    memcpy(outbins, backupbins, FIXBINS * 4);

    if (FIXBINS != bins)
    {
        fprintf(stderr, "Error: Bins was reconfigured.  skippy requires a constant number of bins (%d != %d).\n",
                FIXBINS, bins);
        return;
    }

    if (!dd->Sdonefirstrun)
    {
        SetupDFTProgressive32();
        dd->Sdonefirstrun = 1;
    }

    UpdateBinsForDFT32(frequencies);

    for (i = last_place; i != place_in_data_buffer; i = (i + 1) % size_of_data_buffer)
    {
        int16_t ifr1 = (int16_t)(((databuffer[i])) * 4095);
        HandleInt(ifr1);
        HandleInt(ifr1);
    }

    UpdateOutputBins32();

    last_place = place_in_data_buffer;

    memcpy(backupbins, outbins, FIXBINS * 4);
}

#endif
