#ifndef _FP_MATH_H_
#define _FP_MATH_H_

#include <stdint.h>
#include <stdbool.h>

typedef int16_t q8_8;   // 8 bits integer, 8 bits fraction
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

typedef struct
{
    q24_8 x;
    q24_8 y;
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
vec_q24_8 fpvNorm(vec_q24_8 vec);

float fixToFloat(q24_8 fx);

// Switch to use macros or inline functions
#define FP_MATH_DEFINES

#ifdef FP_MATH_DEFINES

    #define TO_FX(in)              ((in) << FRAC_BITS)
    #define FROM_FX(in)            ((in) >> FRAC_BITS)
    #define ADD_FX(a, b)           ((a) + (b))
    #define SUB_FX(a, b)           ((a) - (b))
    #define MUL_FX(a, b)           (((a) * (b)) >> FRAC_BITS)
    #define DIV_FX(a, b)           (((a) << FRAC_BITS) / (b))
    #define FLOOR_FX(a)            ((a) & (~((1 << FRAC_BITS) - 1)))
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