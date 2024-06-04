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
 * circleFl_t circ = {
 *     .pos.x  = 2,
 *     .pos.y  = 5,
 *     .radius = 8,
 * };
 * rectangleFl_t rect = {
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

#ifndef _GEOMETRY_FL_H_
#define _GEOMETRY_FL_H_

#include <stdint.h>
#include <stdbool.h>
#include <float.h>
#include "vectorFl2d.h"

#define EPS 0.0001f

/**
 * @brief Signed integer representation of a circle
 */
typedef struct
{
    vecFl_t pos;  ///< The position of the center of the circle
    float radius; ///< The radius of the circle
} circleFl_t;

/**
 * @brief Signed integer representation of a rectangle
 */
typedef struct
{
    vecFl_t pos;  ///< The position the top left corner of the rectangle
    float width;  ///< The width of the rectangle
    float height; ///< The height of the rectangle
} rectangleFl_t;

/**
 * @brief Signed integer representation of a line segment
 *
 */
typedef struct
{
    vecFl_t p1; ///< One end point of the line segment
    vecFl_t p2; ///< The other end point of the line segment
} lineFl_t;

bool circleCircleFlIntersection(circleFl_t circle1, circleFl_t circle2, vecFl_t* collisionVec);
bool circlePointFlIntersection(circleFl_t circle, vecFl_t point, vecFl_t* collisionVec);
bool circleRectFlIntersection(circleFl_t circle, rectangleFl_t rect, vecFl_t* collisionVec);
bool circleLineFlIntersection(circleFl_t circle, lineFl_t line, vecFl_t* cpOnLine, vecFl_t* collisionVec);
int16_t circleLineFlIntersectionPoints(circleFl_t circle, lineFl_t line, vecFl_t* intersection_1,
                                       vecFl_t* intersection_2);
bool rectRectFlIntersection(rectangleFl_t rect1, rectangleFl_t rect2, vecFl_t* collisionVec);
bool rectLineFlIntersection(rectangleFl_t rect, lineFl_t line, vecFl_t* collisionVec);
bool lineLineFlIntersection(lineFl_t line1, lineFl_t line2);
vecFl_t infLineIntersectionPoint(lineFl_t a, lineFl_t b);

#endif