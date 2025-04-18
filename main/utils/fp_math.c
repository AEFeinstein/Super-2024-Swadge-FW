//==============================================================================
// Includes
//==============================================================================

#include "fp_math.h"

//==============================================================================
// Lookup Tables
//==============================================================================

/**
 * @brief q24_8 lookup table to convert from a ratio between X and Y vectors to the normalized length of the X vector
 *
 * Equivalent to "f(x) = x / sqrt((x * x) + 1)"
 * Inputs are in the range [0x00, 0xFF], equivalent to [0, 1]
 * Outputs are in the range [0x00, 0xB5], equivalent to [0, sqrt(2)]
 */
static const uint8_t ratioToXLut[] = {
    0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11,
    0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24,
    0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x32, 0x33, 0x34, 0x35, 0x36,
    0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x3F, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x47,
    0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4E, 0x4F, 0x50, 0x51, 0x52, 0x53, 0x54, 0x54, 0x55, 0x56, 0x57, 0x58,
    0x59, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5D, 0x5E, 0x5F, 0x60, 0x61, 0x61, 0x62, 0x63, 0x64, 0x65, 0x65, 0x66, 0x67,
    0x68, 0x68, 0x69, 0x6A, 0x6B, 0x6B, 0x6C, 0x6D, 0x6E, 0x6E, 0x6F, 0x70, 0x71, 0x71, 0x72, 0x73, 0x73, 0x74, 0x75,
    0x76, 0x76, 0x77, 0x78, 0x78, 0x79, 0x7A, 0x7A, 0x7B, 0x7C, 0x7C, 0x7D, 0x7E, 0x7E, 0x7F, 0x80, 0x80, 0x81, 0x82,
    0x82, 0x83, 0x83, 0x84, 0x85, 0x85, 0x86, 0x87, 0x87, 0x88, 0x88, 0x89, 0x8A, 0x8A, 0x8B, 0x8B, 0x8C, 0x8D, 0x8D,
    0x8E, 0x8E, 0x8F, 0x8F, 0x90, 0x91, 0x91, 0x92, 0x92, 0x93, 0x93, 0x94, 0x94, 0x95, 0x95, 0x96, 0x97, 0x97, 0x98,
    0x98, 0x99, 0x99, 0x9A, 0x9A, 0x9B, 0x9B, 0x9C, 0x9C, 0x9D, 0x9D, 0x9E, 0x9E, 0x9F, 0x9F, 0xA0, 0xA0, 0xA0, 0xA1,
    0xA1, 0xA2, 0xA2, 0xA3, 0xA3, 0xA4, 0xA4, 0xA5, 0xA5, 0xA5, 0xA6, 0xA6, 0xA7, 0xA7, 0xA8, 0xA8, 0xA9, 0xA9, 0xA9,
    0xAA, 0xAA, 0xAB, 0xAB, 0xAB, 0xAC, 0xAC, 0xAD, 0xAD, 0xAD, 0xAE, 0xAE, 0xAF, 0xAF, 0xAF, 0xB0, 0xB0, 0xB1, 0xB1,
    0xB1, 0xB2, 0xB2, 0xB2, 0xB3, 0xB3, 0xB3, 0xB4, 0xB4, 0xB5,
};

/**
 * @brief q24_8 Lookup table to find the complimentary Y vector for a normalized X vector
 *
 * Equivalent to "f(x) = sqrt(1 - (x * x))"
 * Inputs are in the range [0x00, 0xB5], equivalent to [0, sqrt(2)] (outputs of ratioToXLut[])
 * Outputs are in the range [0xB5, 0x100], equivalent to [0, 1]
 *
 * Values of 0x00 in this table must be treated as 0x0100. This is so values can be 8 bit
 */
static const uint8_t complNormLut[] = {
    0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFD, 0xFD, 0xFD, 0xFD, 0xFD, 0xFD,
    0xFD, 0xFD, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xF9,
    0xF9, 0xF9, 0xF9, 0xF8, 0xF8, 0xF8, 0xF8, 0xF7, 0xF7, 0xF7, 0xF7, 0xF6, 0xF6, 0xF6, 0xF5, 0xF5, 0xF5, 0xF5, 0xF4,
    0xF4, 0xF4, 0xF3, 0xF3, 0xF3, 0xF2, 0xF2, 0xF2, 0xF1, 0xF1, 0xF1, 0xF0, 0xF0, 0xF0, 0xEF, 0xEF, 0xEE, 0xEE, 0xEE,
    0xED, 0xED, 0xEC, 0xEC, 0xEC, 0xEB, 0xEB, 0xEA, 0xEA, 0xE9, 0xE9, 0xE9, 0xE8, 0xE8, 0xE7, 0xE7, 0xE6, 0xE6, 0xE5,
    0xE5, 0xE4, 0xE4, 0xE3, 0xE3, 0xE2, 0xE2, 0xE1, 0xE1, 0xE0, 0xDF, 0xDF, 0xDE, 0xDE, 0xDD, 0xDD, 0xDC, 0xDB, 0xDB,
    0xDA, 0xDA, 0xD9, 0xD8, 0xD8, 0xD7, 0xD6, 0xD6, 0xD5, 0xD5, 0xD4, 0xD3, 0xD2, 0xD2, 0xD1, 0xD0, 0xD0, 0xCF, 0xCE,
    0xCD, 0xCD, 0xCC, 0xCB, 0xCA, 0xCA, 0xC9, 0xC8, 0xC7, 0xC7, 0xC6, 0xC5, 0xC4, 0xC3, 0xC2, 0xC2, 0xC1, 0xC0, 0xBF,
    0xBE, 0xBD, 0xBC, 0xBB, 0xBA, 0xB9, 0xB8, 0xB7, 0xB7, 0xB6, 0xB5,
};

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Quickly normalize a q24_8 vector, in-place
 *
 * @param xp The X component of the vector, normalized in-place
 * @param yp The Y component of the vector, normalized in-place
 */
void fastNormVec(q24_8* xp, q24_8* yp)
{
    // Save pointer values to local
    q24_8 x = *xp;
    q24_8 y = *yp;
    // Local variables for the normalized length
    q24_8 nx, ny;

    // Adjust signs to be positive for LUTs
    bool xIsPos = true;
    if (x < 0)
    {
        xIsPos = false;
        x      = -x;
    }
    bool yIsPos = true;
    if (y < 0)
    {
        yIsPos = false;
        y      = -y;
    }

    // Fixed point division bitshifts up, so make sure the inputs are small enough to not disturb the sign bit
    while (x > 0x007FFFFF || y > 0x007FFFFF)
    {
        // Dividing both components by two won't change the direction
        x /= 2;
        y /= 2;
    }

    // Check special cases
    if (x == 0)
    {
        nx = 0;
        ny = TO_FX(1);
    }
    else if (y == 0)
    {
        nx = TO_FX(1);
        ny = 0;
    }
    // Do ratio math
    else if (x <= y)
    {
        // x is <= y, so the ratio will always be <= 1 (0x0100 or less in q24_8)
        q24_8 ratio = DIV_FX(x, y);
        nx          = ratioToXLut[ratio];

        // Treat 0x00 as 0x0100
        if (complNormLut[nx])
        {
            ny = complNormLut[nx];
        }
        else
        {
            ny = 0x100;
        }
    }
    else if (y < x)
    {
        // y is less than x, so the ratio will always be less than 1 (8 bits or fewer)
        q24_8 ratio = DIV_FX(y, x);
        ny          = ratioToXLut[ratio];

        // Treat 0x00 as 0x0100
        if (complNormLut[ny])
        {
            nx = complNormLut[ny];
        }
        else
        {
            nx = 0x100;
        }
    }

    // Reapply signs
    if (!xIsPos)
    {
        nx = -(nx);
    }
    if (!yIsPos)
    {
        ny = -(ny);
    }

    // Return values
    *xp = nx;
    *yp = ny;
}

/**
 * @brief Normalize and return a vector
 *
 * @param vec The vector to normalize
 * @return The normalized vector
 */
vec_q24_8 fpvNorm(vec_q24_8 vec)
{
    vec_q24_8 normalized = vec;
    fastNormVec(&normalized.x, &normalized.y);
    return normalized;
}

/**
 * @brief Compute the dot product of two vectors
 *
 * @param a A vector to dot
 * @param b The other vector to dot
 * @return The dot product of the two vectors
 */
q24_8 fpvDot(vec_q24_8 a, vec_q24_8 b)
{
    return ((a.x * b.x) + (a.y * b.y)) >> FRAC_BITS;
}

/**
 * @brief Compute the squared magnitude of a vector
 *
 * @param a The vector to compute the squared magnitude of
 * @return The squared magnitude of the vector
 */
q24_8 fpvSqMag(vec_q24_8 a)
{
    return ((a.x * a.x) + (a.y * a.y)) >> FRAC_BITS;
}

/**
 * @brief Add two vectors
 *
 * @param a A vector to add
 * @param b The other vector to add
 * @return The sum of the two input vectors
 */
vec_q24_8 fpvAdd(vec_q24_8 a, vec_q24_8 b)
{
    vec_q24_8 sum = {
        .x = a.x + b.x,
        .y = a.y + b.y,
    };
    return sum;
}

/**
 * @brief Subtract two vectors
 *
 * @param a A vector to subtract from
 * @param b The other vector to subtract
 * @return The difference between the two input vectors
 */
vec_q24_8 fpvSub(vec_q24_8 a, vec_q24_8 b)
{
    vec_q24_8 diff = {
        .x = a.x - b.x,
        .y = a.y - b.y,
    };
    return diff;
}

/**
 * @brief Multiply a vector by a scalar
 *
 * @param vec A vector multiply
 * @param scalar A scalar to multiply the vector by
 * @return The scaled vector
 */
vec_q24_8 fpvMulSc(vec_q24_8 vec, q24_8 scalar)
{
    vec_q24_8 mul = {
        .x = MUL_FX(vec.x, scalar),
        .y = MUL_FX(vec.y, scalar),
    };
    return mul;
}

/**
 * @brief Divide a vector by a scalar
 *
 * @param vec A vector divide
 * @param scalar A scalar to divide the vector by
 * @return The scaled vector
 */
vec_q24_8 fpvDivSc(vec_q24_8 vec, q24_8 scalar)
{
    vec_q24_8 div = {
        .x = DIV_FX(vec.x, scalar),
        .y = DIV_FX(vec.y, scalar),
    };
    return div;
}

/**
 * @brief Convert a q24_8 to floating point
 *
 * @param fx Fixed point input
 * @return Floating point output
 */
float fixToFloat(q24_8 fx)
{
    float fl = (float)(fx / (1 << FRAC_BITS));

    if (fx >= 0)
    {
        float decimal = 0.5f;
        for (int i = 7; i >= 0; i--)
        {
            if (fx & (1 << i))
            {
                fl += (decimal);
            }
            decimal /= 2;
        }
    }
    else
    {
        if (0 != (fx & 0xFF))
        {
            fl -= 1 / 256.0f;
            float decimal = 0.5f;
            for (int i = 7; i >= 0; i--)
            {
                if (!(fx & (1 << i)))
                {
                    fl -= (decimal);
                }
                decimal /= 2;
            }
        }
    }
    return fl;
}

#ifdef LUT_GEN_FUNCS

    #include <stdlib.h>
    #include <stdio.h>
    #include <math.h>

typedef struct
{
    int32_t x;
    int32_t y;
    float err;
} testResult_t;

/**
 * @brief Convert a floating point to q24_8
 *
 * @param fl Floating point input
 * @return Fixed point output
 */
q24_8 floatToFix(float fl)
{
    int sign = (fl >= 0) ? 1 : -1;
    fl *= sign;

    q24_8 fx = (q24_8)((int32_t)fl * (1 << FRAC_BITS));

    float deciPart = fl - (int)fl;

    float deciBit = 0.5f;
    for (int i = 0; i < 8; i++)
    {
        if (deciPart >= deciBit)
        {
            deciPart -= deciBit;
            fx = fx | (0x80 >> i);
        }
        deciBit /= 2;
    }
    return fx * sign;
}

/**
 * @brief Function to convert from a ratio between X and Y vectors to the normalized length of the X vector
 *
 * @param x The ratio between X and Y vector lengths
 * @return float The normalized X vector
 */
float ratioToX(float r)
{
    return r / sqrt((r * r) + 1);
}

/**
 * @brief Lookup table to find the complimentary Y vector for a normalized X vector
 *
 * @param x The normalized X vector
 * @return The Y vector
 */
float complNorm(float x)
{
    return sqrt(1 - (x * x));
}

/**
 * @brief Comparator function to sort testResult_t by error
 *
 * @param p1 A testResult_t to compare
 * @param p2 A testResult_t to compare
 * @return A number less than, equal to, or greater than 0 if p1' error greater than, equal to, or less than p2's error
 */
int comparator(const void* p1, const void* p2)
{
    if (((testResult_t*)p1)->err < ((testResult_t*)p2)->err)
    {
        return 1;
    }
    else if (((testResult_t*)p1)->err > ((testResult_t*)p2)->err)
    {
        return -1;
    }
    else
    {
        return ((testResult_t*)p1)->x - ((testResult_t*)p2)->x;
    }
}

/**
 * @brief Main function to generate LUTs and validate errors between floating and fixed point values
 *
 * @param argc unused
 * @param argv unused
 * @return 0
 */
int main(int argc __attribute__((unused)), char** argv __attribute__((unused)))
{
    #ifdef PRINT_LUTS
    printf("uint8_t ratioToXLut[] = {\n");
    for (int i = 0; i <= TO_FX(1); i++)
    {
        printf("  0x%02X,\n", floatToFix(ratioToX(fixToFloat(i))));
    }
    printf("};\n\n");

    printf("uint16_t complNormLut[] = {\n");
    for (int i = 0; i <= ratioToXLut[sizeof(ratioToXLut) - 1]; i++)
    {
        printf("  0x%04X,\n", floatToFix(complNorm(fixToFloat(i))));
    }
    printf("};\n\n");
    #endif

    #define TEST_RANGE 1000
    // Allocate space for test results
    testResult_t* results = heap_caps_calloc(4 * TEST_RANGE * TEST_RANGE, sizeof(testResult_t), MALLOC_CAP_8BIT);
    // Test all vectors in a 1000 x 1000 range
    for (int x = -TEST_RANGE; x < TEST_RANGE; x++)
    {
        // Ignore symmetric vectors
        for (int y = -TEST_RANGE; y < TEST_RANGE; y++)
        {
            if (!(x == 0 && y == 0))
            {
                // Slow, accurate normalization
                float len = sqrt((x * x) + (y * y));
                float fnx = x / len;
                float fny = y / len;

                // Fast, inaccurate normalization
                q24_8 ox = TO_FX(x);
                q24_8 oy = TO_FX(y);
                fastNormVec(&ox, &oy);

                // Error
                float ex     = fnx - fixToFloat(ox);
                float ey     = fny - fixToFloat(oy);
                float errMag = sqrt(ex * ex + ey * ey);

                // Save the result to sort later
                results[(x + TEST_RANGE) * (2 * TEST_RANGE) + (y + TEST_RANGE)].x   = x;
                results[(x + TEST_RANGE) * (2 * TEST_RANGE) + (y + TEST_RANGE)].y   = y;
                results[(x + TEST_RANGE) * (2 * TEST_RANGE) + (y + TEST_RANGE)].err = errMag;
            }
        }
    }

    // Sort all tests by error
    qsort(results, TEST_RANGE * TEST_RANGE, sizeof(testResult_t), comparator);

    // Print the 10 worst offenders
    for (int i = 0; i < 10; i++)
    {
        int32_t x = results[i].x;
        int32_t y = results[i].y;

        // Slow, accurate normalization
        float len = sqrt((x * x) + (y * y));
        float fnx = x / len;
        float fny = y / len;

        // Fast, inaccurate normalization
        q24_8 ox = TO_FX(x);
        q24_8 oy = TO_FX(y);
        fastNormVec(&ox, &oy);

        // Error
        float ex     = fnx - fixToFloat(ox);
        float ey     = fny - fixToFloat(oy);
        float errMag = sqrt(ex * ex + ey * ey);

        printf("(%4d, %4d) => (%8.05f, %8.05f) == (%8.05f, %8.05f), err: %.05f\n", x, y, fnx, fny, fixToFloat(ox),
               fixToFloat(oy), errMag);
    }

    // Clean up
    heap_caps_free(results);
    return 0;
}

#endif
