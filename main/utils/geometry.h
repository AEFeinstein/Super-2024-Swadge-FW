/*! \file geometry.h
 *
 * \section geometry_design Design Philosophy
 *
 * These utility functions are can be used for geometric math.
 *
 * \section geometry_usage Usage
 *
 * No initialization or deinitialization is required. Each function will not modify the given shapes. See the functions
 * below for what is provided and how to use them.
 *
 * \section geometry_example Example
 *
 * \code{.c}
 * circle_t circ = {
 *     .x      = 2,
 *     .y      = 5,
 *     .radius = 8,
 * };
 * rectangle_t rect = {
 *     .x      = -2,
 *     .y      = -4,
 *     .width  = 6,
 *     .height = 9,
 * };
 * if (circleRectIntersection(circ, rect))
 * {
 *     printf("Shapes intersect\n");
 * }
 * else
 * {
 *     printf("Shapes do not intersect\n");
 * }
 * \endcode
 */

#ifndef _GEOMETRY_H_
#define _GEOMETRY_H_

#include <stdint.h>
#include <stdbool.h>
#include "vector2d.h"

/**
 * @brief Signed integer representation of a circle
 */
typedef struct
{
    vec_t pos;      ///< The position of the center of the circle
    int32_t radius; ///< The radius of the circle
} circle_t;

/**
 * @brief Signed integer representation of a rectangle
 */
typedef struct
{
    vec_t pos;      ///< The position the top left corner of the rectangle
    int32_t width;  ///< The width of the rectangle
    int32_t height; ///< The height of the rectangle
} rectangle_t;

/**
 * @brief Signed integer representation of a line segment
 *
 */
typedef struct
{
    vec_t p1; ///< One end point of the line segment
    vec_t p2; ///< The other end point of the line segment
} line_t;

bool circleCircleIntersection(circle_t circle1, circle_t circle2);
bool circlePointIntersection(circle_t circle, vec_t point);
bool circleRectIntersection(circle_t circle, rectangle_t rect);
bool circleLineIntersection(circle_t circle, line_t line);
bool rectRectIntersection(rectangle_t rect1, rectangle_t rect2);

#endif