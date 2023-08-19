//==============================================================================
// Includes
//==============================================================================

#include "trigonometry.h"

//==============================================================================
// Macros
//==============================================================================

/// Approximates arctan(y / x) to within 1 degree, but only when 0 <= (y / x) <= 1
#define ARCTAN_APPROX(y, x) (-y * (47 * y - 182 * x) / 3 / x / x)

//==============================================================================
// Constant data
//==============================================================================

/// A table of the first 91 values of (1024 * sin(x)). Only 91 values are necessary because getSin1024() and
/// getCos1024() handle symmetry to calculate the full wave
const int16_t sin1024[91] = {
    0,   18,  36,   54,   71,   89,   107,  125,  143,  160,  178,  195,  213,  230,  248,  265, 282, 299, 316,
    333, 350, 367,  384,  400,  416,  433,  449,  465,  481,  496,  512,  527,  543,  558,  573, 587, 602, 616,
    630, 644, 658,  672,  685,  698,  711,  724,  737,  749,  761,  773,  784,  796,  807,  818, 828, 839, 849,
    859, 868, 878,  887,  896,  904,  912,  920,  928,  935,  943,  949,  956,  962,  968,  974, 979, 984, 989,
    994, 998, 1002, 1005, 1008, 1011, 1014, 1016, 1018, 1020, 1022, 1023, 1023, 1024, 1024,
};

/// A table of the first 91 values of (1024 * tan(x)). Only 91 values are necessary because getTan1024() handles
/// symmetry to calculate the full wave
const uint16_t tan1024[91] = {
    0,    18,   36,   54,   72,   90,    108,   126,   144,   162,   181,   199,  218,  236,  255,  274,
    294,  313,  333,  353,  373,  393,   414,   435,   456,   477,   499,   522,  544,  568,  591,  615,
    640,  665,  691,  717,  744,  772,   800,   829,   859,   890,   922,   955,  989,  1024, 1060, 1098,
    1137, 1178, 1220, 1265, 1311, 1359,  1409,  1462,  1518,  1577,  1639,  1704, 1774, 1847, 1926, 2010,
    2100, 2196, 2300, 2412, 2534, 2668,  2813,  2974,  3152,  3349,  3571,  3822, 4107, 4435, 4818, 5268,
    5807, 6465, 7286, 8340, 9743, 11704, 14644, 19539, 29324, 58665, 65535,
};

//==============================================================================
// Static Function Prototypes
//==============================================================================

static int16_t innerAtan2(int32_t y, int32_t x);

//==============================================================================
// Functions
//==============================================================================

/**
 * Integer sine function
 *
 * @param degree The degree, between 0 and 359
 * @return The sine of the degree, between -1024 and 1024
 */
int16_t getSin1024(int16_t degree)
{
    if (degree >= 180)
    {
        // 180 -> 359
        if (degree <= 270)
        {
            // 180 -> 270
            return -sin1024[degree - 180];
        }
        else
        {
            // 271 -> 359
            return -sin1024[360 - degree];
        }
    }
    else
    {
        // 0 -> 179
        if (degree <= 90)
        {
            // 0 -> 90
            return sin1024[degree];
        }
        else
        {
            // 91 -> 179
            return sin1024[180 - degree];
        }
    }
}

/**
 * Integer cosine function
 *
 * @param degree The degree, between 0 and 359
 * @return The cosine of the degree, between -1024 and 1024
 */
int16_t getCos1024(int16_t degree)
{
    if (degree >= 180)
    {
        // 180 -> 359
        if (degree <= 270)
        {
            // 180 -> 270
            return -sin1024[270 - degree];
        }
        else
        {
            // 271 -> 359
            return sin1024[degree - 270];
        }
    }
    else
    {
        // 0 -> 179
        if (degree <= 90)
        {
            // 0 -> 90
            return sin1024[90 - degree];
        }
        else
        {
            // 91 -> 179
            return -sin1024[degree - 90];
        }
    }
}

/**
 * Integer tangent function
 *
 * @param degree The degree, between 0 and 359
 * @return The tangent of the degree, scaled by 1024
 */
int32_t getTan1024(int16_t degree)
{
    if (degree >= 180)
    {
        // 180 -> 359
        if (degree <= 270)
        {
            // 180 -> 270
            return tan1024[degree - 180];
        }
        else
        {
            // 271 -> 359
            return -tan1024[360 - degree];
        }
    }
    else
    {
        // 0 -> 179
        if (degree <= 90)
        {
            // 0 -> 90
            return tan1024[degree];
        }
        else
        {
            // 91 -> 179
            return -tan1024[180 - degree];
        }
    }
}

/**
 * @brief Static helper function to make sure the ::ARCTAN_APPROX macro
 * is only called with parameters inside its domain.
 */
static int16_t innerAtan2(int32_t y, int32_t x)
{
    int8_t sig = 1;

    // Handle flipping the sign to make sure everything fits
    if (x < 0 && y < 0)
    {
        // Flip both X and Y but not the result
        x = -x;
        y = -y;
    }
    else if (x < 0)
    {
        // Flip X and result
        x = -x;
        sig = -1;
    }
    else if (y < 0)
    {
        // Flip Y and result
        y = -y;
        sig = -1;
    }

    if (y > x)
    {
        // y > x means (y/x) > 1, so instead
        return sig * (90 - ARCTAN_APPROX(x, y));
    }
    else
    {
        return sig * ARCTAN_APPROX(y, x);
    }
}

/**
 * @brief Calculate the angle of the line from the origin to (x, y), in degrees
 *
 * If both x and y are 0, atan2 is undefined and will return 0.
 *
 * @param y The Y coordinate of the point to calculate the angle to
 * @param x The X coordinate of the point to calculate the angle to
 * @return int16_t The angle degree, between 0 and 359.
 */
int16_t getAtan2(int32_t y, int32_t x)
{
    if (x == 0 && y == 0)
    {
        return 0;
    }
    else if (y > 0)
    {
        return ((90 - innerAtan2(x, y)) + 360) % 360;
    }
    else if (y < 0)
    {
        return ((270 - innerAtan2(x, y)) + 360) % 360;
    }
    else if (x < 0)
    {
        return (180 + innerAtan2(y, x)) % 360;
    }
    else
    {
        return innerAtan2(y, x);
    }
}