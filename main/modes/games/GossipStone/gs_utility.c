//==============================================================================
// Includes
//==============================================================================
#include "gs_utility.h"
#include "esp_random.h"
#include <math.h>

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Lerp between a and b by amount
 *
 * @param a One of two inputs
 * @param b One of two inputs
 * @param amount Lerp amount from 0 to 30000. 0 returns a, 30000 returns b.
 */
int gs_lerp(int a, int b, uint16_t amount)
{
    return a + ((b - a) * amount) / 30000;
}

// input 0, output 0
// input 30000, output 30000
int16_t gs_logRemap(int16_t x)
{
    if (x <= 0)
        return 0;

    float factor = pow(x / 30000.0, 0.4); // Exponent > 1 flattens early
    return (int16_t)(30000 * factor);
}

// Both inputs are inclusive
int gs_randomInt(int lowerBound, int upperBound)
{
    return esp_random() % (upperBound - lowerBound + 1) + lowerBound;
}

// Use pos 0 to check if the rightmost bit is 1.
bool gs_checkBit(uint32_t var, uint8_t pos)
{
    return (var) & (1 << (pos - 1));
}