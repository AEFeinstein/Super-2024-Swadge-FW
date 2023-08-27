/*! \file trigonometry.h
 *
 * \section trigonometry_design Design Philosophy
 *
 * Floating point or double precision trigonometry is slow.
 * Look up tables are fast!
 * These functions do a good job approximating sine, cosine, and tangent.
 *
 * The arguments are in degrees, not radians, and they return (1024 * the trigonometric function).
 *
 * \section trigonometry_usage Usage
 *
 * Call getSin1024(), getCos1024(), or getTan1024() with a degree between 0 and 359 inclusive.
 * If the input degree is out of bounds, the result will be indeterminate.
 *
 * The ::sin1024 and ::tan1024 arrays contain the first 91 elements of the respective functions and can be used directly
 * if you're careful.
 *
 * \section trigonometry_example Example
 *
 * \code{.c}
 * printf("%d\n", getSin1024(180));
 * \endcode
 */

#ifndef _TRIGONOMETRY_H_
#define _TRIGONOMETRY_H_

#include <stdint.h>

extern const int16_t sin1024[91];
extern const uint16_t tan1024[91];

int16_t getSin1024(int16_t degree);
int16_t getCos1024(int16_t degree);
int32_t getTan1024(int16_t degree);
int16_t getAtan2(int32_t y, int32_t x);

#endif