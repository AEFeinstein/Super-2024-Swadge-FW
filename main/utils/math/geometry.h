/*! \file geometry.h
 *
 * \section geometry_design Design Philosophy
 *
 * These utility functions are can be used for geometric math.
 *
 * Much of this file was adapted from https://www.jeffreythompson.org/collision-detection/
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
 *     .pos.x  = 2,
 *     .pos.y  = 5,
 *     .radius = 8,
 * };
 * rectangle_t rect = {
 *     .pos.x  = -2,
 *     .pos.y  = -4,
 *     .width  = 6,
 *     .height = 9,
 * };
 * if (circleRectIntersection(circ, rect, NULL))
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

typedef struct
{
    vec_t base;  ///< The base of the arrow
    vec_t tip;   ///< The tip of the arrow
    vec_t wing1; ///< The base of one of the arrowhead wings
    vec_t wing2; ///< The base of one of the other arrowhead wing
} arrow_t;

bool circleCircleIntersection(circle_t circle1, circle_t circle2, vec_t* collisionVec);
bool circlePointIntersection(circle_t circle, vec_t point, vec_t* collisionVec);
bool circleRectIntersection(circle_t circle, rectangle_t rect, vec_t* collisionVec);
bool circleLineIntersection(circle_t circle, line_t line, vec_t* collisionVec);
bool rectRectIntersection(rectangle_t rect1, rectangle_t rect2, vec_t* collisionVec);
bool rectLineIntersection(rectangle_t rect, line_t line, vec_t* collisionVec);
bool lineLineIntersection(line_t line1, line_t line2);

arrow_t initArrow(vec_t base, vec_t tip, int32_t wingLen);

#endif