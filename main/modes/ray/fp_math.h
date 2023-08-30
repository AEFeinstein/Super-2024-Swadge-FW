#ifndef _FP_MATH_H_
#define _FP_MATH_H_

#include <stdint.h>

typedef int32_t q24_8;  // 24 bits integer, 8 bits fraction
typedef int32_t q16_16; // 16 bits integer, 16 bits fraction
typedef int32_t q8_24;  // 8 bits integer, 24 bits fraction

#define FRAC_BITS        8
#define Q24_8_DECI_MASK  ((1 << FRAC_BITS) - 1)
#define Q24_8_WHOLE_MASK (~Q24_8_DECI_MASK)

#define Q16_16_FRAC_BITS  16
#define Q16_16_DECI_MASK  ((1 << Q16_16_FRAC_BITS) - 1)
#define Q16_16_WHOLE_MASK (~Q16_16_DECI_MASK)

#define Q8_24_FRAC_BITS  24
#define Q8_24_DECI_MASK  ((1 << Q8_24_FRAC_BITS) - 1)
#define Q8_24_WHOLE_MASK (~Q8_24_DECI_MASK)

//==============================================================================
// Fixed Point Math Functions
//==============================================================================

// Switch to use macros or inline functions
#define FP_MATH_DEFINES

#ifdef FP_MATH_DEFINES

    #define TO_FX(in)    ((in) << FRAC_BITS)
    #define FROM_FX(in)  ((in) >> FRAC_BITS)
    #define ADD_FX(a, b) ((a) + (b))
    #define SUB_FX(a, b) ((a) - (b))
    #define MUL_FX(a, b) (((a) * (b)) >> FRAC_BITS)
    #define DIV_FX(a, b) (((a) << FRAC_BITS) / (b))
    #define FLOOR_FX(a)  ((a) & (~((1 << FRAC_BITS) - 1)))

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
    // TODO could be simpler without rounding
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
//     return (1000 * (int64_t)(ABS(in) & ((1 << FRAC_BITS) - 1))) / (1 << FRAC_BITS);
// }
#endif

#endif