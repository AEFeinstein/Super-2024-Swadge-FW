/*! \file vector2d.h
 *
 * \section vector_design Design Philosophy
 *
 * These utility functions are can be used for signed integer vector 2D math. VecFltor math is useful for physics
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
 * vecFl_t vecOne = {
 *     .x = 3,
 *     .y = 5,
 * };
 * vecFl_t vecTwo = {
 *     .x = 8,
 *     .y = 2,
 * };
 * vecFl_t vecSum = addVecFl2d(vecOne, vecTwo);
 * \endcode
 */

#ifndef _VECTOR_FL_2D_H_
#define _VECTOR_FL_2D_H_

#include <stdint.h>

/**
 * @brief A 2D vector with signed integer X and Y components
 */
typedef struct
{
    float x; ///< The signed integer X component of the vector
    float y; ///< The signed integer Y component of the vector
} vecFl_t;

vecFl_t addVecFl2d(vecFl_t left, vecFl_t right);
vecFl_t subVecFl2d(vecFl_t left, vecFl_t right);
vecFl_t mulVecFl2d(vecFl_t vector, float scalar);
vecFl_t divVecFl2d(vecFl_t vector, float scalar);

float dotVecFl2d(vecFl_t left, vecFl_t right);
vecFl_t rotateVecFl2d(vecFl_t vector, float degree);
float sqMagVecFl2d(vecFl_t vector);
vecFl_t normVecFl2d(vecFl_t in);

#endif