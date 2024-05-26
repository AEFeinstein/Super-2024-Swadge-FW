/*! \file fp_math.h
 *
 * \section fp_math_design Design Philosophy
 *
 * These utility functions are can be used for fixed point decimal math.
 *
 * ::q24_8 has 24 bits of signed integer and 8 bits of decimal. The integer range is from -8388608 to 8388607 and the
 * smallest decimal resolution is 1/256.
 *
 * ::q16_16 and ::q8_24 typedefs are also provided, but function macros aren't. Use with caution if more decimal
 * precision is required.
 *
 * ::vec_q24_8 is a vector type for ::q24_8. Fixed point vector math functions are provided and start with 'fpv.'
 *
 * \section fp_math_usage Usage
 *
 * No initialization or deinitialization is required.
 *
 * \section fp_math_example Example
 *
 * \code{.c}
 * q24_8 a = TO_FX(6);
 * q24_8 b = TO_FX_FRAC(1, 2);
 * q24_8 c = ADD_FX(a, b);
 * printf("%f\n", fixToFloat(c)); // prints 6.500000
 * \endcode
 */

#ifndef _FP_MATH_H_
#define _FP_MATH_H_

#include <stdint.h>
#include <stdbool.h>

typedef int16_t q8_8;   ///< 8 bits integer, 8 bits fraction
typedef int32_t q24_8;  ///< 24 bits integer, 8 bits fraction
typedef int32_t q16_16; ///< 16 bits integer, 16 bits fraction
typedef int32_t q8_24;  ///< 8 bits integer, 24 bits fraction

typedef uint16_t uq8_8;   ///< unsigned 8 bits integer, 8 bits fraction
typedef uint32_t uq24_8;  ///< unsigned 24 bits integer, 8 bits fraction
typedef uint32_t uq16_16; ///< unsigned 16 bits integer, 16 bits fraction
typedef uint32_t uq8_24;  ///< unsigned 8 bits integer, 24 bits fraction

#define FRAC_BITS        8                      ///< 8 fractional bits for q24_8
#define Q24_8_DECI_MASK  ((1 << FRAC_BITS) - 1) ///< A mask for the fractional part of a q24_8
#define Q24_8_WHOLE_MASK (~Q24_8_DECI_MASK)     ///< A mask for the integer part of a q24_8

#define Q16_16_FRAC_BITS  16                            ///< 8 fractional bits for q16_16
#define Q16_16_DECI_MASK  ((1 << Q16_16_FRAC_BITS) - 1) ///< A mask for the fractional part of a q16_16
#define Q16_16_WHOLE_MASK (~Q16_16_DECI_MASK)           ///< A mask for the integer part of a q16_16

#define Q8_24_FRAC_BITS  24                           ///< 8 fractional bits for q8_24
#define Q8_24_DECI_MASK  ((1 << Q8_24_FRAC_BITS) - 1) ///< A mask for the fractional part of a q8_24
#define Q8_24_WHOLE_MASK (~Q8_24_DECI_MASK)           ///< A mask for the integer part of a q8_24

/// @brief A 2D vector with q24_8 numbers
typedef struct
{
    q24_8 x; ///< The X component of the vector
    q24_8 y; ///< The Y component of the vector
} vec_q24_8;

//==============================================================================
// Fixed Point Math Functions
//==============================================================================

void fastNormVec(q24_8* xp, q24_8* yp);

vec_q24_8 fpvAdd(vec_q24_8 a, vec_q24_8 b);
vec_q24_8 fpvSub(vec_q24_8 a, vec_q24_8 b);
vec_q24_8 fpvMulSc(vec_q24_8 vec, q24_8 scalar);
vec_q24_8 fpvDivSc(vec_q24_8 vec, q24_8 scalar);

q24_8 fpvDot(vec_q24_8 a, vec_q24_8 b);
q24_8 fpvSqMag(vec_q24_8 a);
vec_q24_8 fpvNorm(vec_q24_8 vec);

float fixToFloat(q24_8 fx);

// Switch to use macros or inline functions
#define FP_MATH_DEFINES

#ifdef FP_MATH_DEFINES

    /**
     * @brief Convert an integer to a q24_8
     * @param in The integer to convert to q24_8
     * @return A q24_8
     */
    #define TO_FX(in) ((in) << FRAC_BITS)

    /**
     * @brief Convert a q24_8 to an integer
     * @param in The q24_8 to convert to integer
     * @return An integer
     */
    #define FROM_FX(in) ((in) >> FRAC_BITS)

    /**
     * @brief Add two q24_8 numbers together
     * @param a An operand
     * @param b The other operand
     * @return The sum
     */
    #define ADD_FX(a, b) ((a) + (b))

    /**
     * @brief Subtract a q24_8 from another
     * @param a The number to subtract from
     * @param b The number to subtract
     * @return The difference
     */
    #define SUB_FX(a, b) ((a) - (b))

    /**
     * @brief Multiply two q24_8
     * @param a An operand
     * @param b The other operand
     * @return The result
     */
    #define MUL_FX(a, b) (((a) * (b)) >> FRAC_BITS)

    /**
     * @brief Divide a q24_8 by another
     * @param a The numerator
     * @param b The denominator
     * @return The result
     */
    #define DIV_FX(a, b) (((a) << FRAC_BITS) / (b))

    /**
     * @brief Find the floor of a q24_8
     * @param a The number to floor
     * @return The floor of the input
     */
    #define FLOOR_FX(a) ((a) & (~((1 << FRAC_BITS) - 1)))

    /**
     * @brief Convert an integer fraction to q24_8
     * @param num The numerator
     * @param denom The denominator
     * @return The fraction in q24_8
     */
    #define TO_FX_FRAC(num, denom) DIV_FX(num, denom)

#else

static inline q24_8 TO_FX(uint32_t in)
{
    return (q24_8)(in * (1 << FRAC_BITS));
}

static inline int32_t FROM_FX(q24_8 in)
{
    return in / (1 << FRAC_BITS);
}

static inline q24_8 ADD_FX(q24_8 a, q24_8 b)
{
    return a + b;
}

static inline q24_8 SUB_FX(q24_8 a, q24_8 b)
{
    return a - b;
}

static inline q24_8 MUL_FX(q24_8 a, q24_8 b)
{
    // could be simpler without rounding
    return ((a * b) + (1 << (FRAC_BITS - 1))) / (1 << FRAC_BITS);
}

static inline q24_8 DIV_FX(q24_8 a, q24_8 b)
{
    return ((a * (1 << FRAC_BITS)) / b);
}

static inline q24_8 FLOOR_FX(q24_8 a)
{
    return a & (~((1 << FRAC_BITS) - 1));
}

// #define FMT_FX    "%s%d.%03d"
// #define STR_FX(x) ((x) < 0 ? "-" : ""), ABS(FROM_FX(x)), DEC_PART(x)

// static inline int32_t DEC_PART(q24_8 in)
// {
//     return (1000 * (int32_t)(ABS(in) & ((1 << FRAC_BITS) - 1))) / (1 << FRAC_BITS);
// }
#endif

#endif