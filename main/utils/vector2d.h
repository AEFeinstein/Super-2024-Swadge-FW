/*! \file vector2d.h
 *
 * \section vector_design Design Philosophy
 *
 * These utility functions are can be used for signed integer vector 2D math. Vector math is useful for physics
 * calculations.
 *
 * \section vector_usage Usage
 *
 * No initialization or deinitialization is required. Each function will return a new vector and will not modify the
 * vectors given as arguments. See the functions below for what is provided and how to use them.
 *
 * \section vector_example Example
 *
 * \code{.c}
 * vec_t vecOne = {
 *     .x = 3,
 *     .y = 5,
 * };
 * vec_t vecTwo = {
 *     .x = 8,
 *     .y = 2,
 * };
 * vec_t vecSum = addVec2d(vecOne, vecTwo);
 * \endcode
 */

#ifndef _VECTOR_2D_H_
#define _VECTOR_2D_H_

#include <stdint.h>

/**
 * @brief A 2D vector with signed integer X and Y components
 */
typedef struct
{
    int32_t x; ///< The signed integer X component of the vector
    int32_t y; ///< The signed integer Y component of the vector
} vec_t;

vec_t addVec2d(vec_t left, vec_t right);
vec_t subVec2d(vec_t left, vec_t right);
vec_t mulVec2d(vec_t vector, int32_t scalar);
vec_t divVec2d(vec_t vector, int32_t scalar);

int32_t dotVec2d(vec_t left, vec_t right);
vec_t rotateVec2d(vec_t vector, int32_t angle);
int32_t sqMagVec2d(vec_t vector);

#endif