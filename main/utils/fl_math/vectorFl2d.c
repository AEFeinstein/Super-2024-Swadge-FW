#include <math.h>
#include "vectorFl2d.h"

/**
 * @brief Add two vectors and return the resulting vector
 *
 * @param left One vector to add
 * @param right The other vector to add
 * @return The sum of both vectors
 */
vecFl_t addVecFl2d(vecFl_t left, vecFl_t right)
{
    vecFl_t result = {
        .x = left.x + right.x,
        .y = left.y + right.y,
    };
    return result;
}

/**
 * @brief Subtract two vectors and return the resulting vector
 *
 * @param left The vector to subtract from
 * @param right The other vector to subtract
 * @return The difference between the vectors
 */
vecFl_t subVecFl2d(vecFl_t left, vecFl_t right)
{
    vecFl_t result = {
        .x = left.x - right.x,
        .y = left.y - right.y,
    };
    return result;
}

/**
 * @brief Multiply a vector by a scalar and return the result
 *
 * @param vector The vector to multiply
 * @param scalar The scalar to multiply by
 * @return The multiplied vector
 */
vecFl_t mulVecFl2d(vecFl_t vector, float scalar)
{
    vecFl_t result = {
        .x = vector.x * scalar,
        .y = vector.y * scalar,
    };
    return result;
}

/**
 * @brief Divide a vector by a scalar and return the result
 *
 * @param vector The vector to divide
 * @param scalar The scalar to divide by
 * @return The divided vector
 */
vecFl_t divVecFl2d(vecFl_t vector, float scalar)
{
    vecFl_t result = {
        .x = vector.x / scalar,
        .y = vector.y / scalar,
    };
    return result;
}

/**
 * @brief Find the dot product of two vectors
 *
 * @param left One vector to dot
 * @param right The other vector to dot
 * @return The dot product of the two vectors
 */
float dotVecFl2d(vecFl_t left, vecFl_t right)
{
    return (left.x * right.x) + (left.y * right.y);
}

/**
 * @brief Rotate a vector by a number of degrees and return the result
 *
 * @param vector The vector to rotate
 * @param degree The angle to rotate clockwise by, in radians
 * @return The rotated vector
 */
vecFl_t rotateVecFl2d(vecFl_t vector, float radians)
{
    int16_t sinR = sin(radians);
    int16_t cosR = cos(radians);
    float oldX   = vector.x;
    float oldY   = vector.y;

    vecFl_t result = {
        .x = (oldX * cosR) - (oldY * sinR),
        .y = (oldX * sinR) + (oldY * cosR),
    };
    return result;
}

/**
 * @brief Return the squared magnitude of the given vector. The square root is not used because it is slow.
 *
 * @param vector The vector to get the squared magnitude for
 * @return The squared magnitude of the vector
 */
float sqMagVecFl2d(vecFl_t vector)
{
    return (vector.x * vector.x) + (vector.y * vector.y);
}

/**
 * @brief Normalize and return a vector
 *
 * @param in The input vector
 * @return The normalized vector
 */
vecFl_t normVecFl2d(vecFl_t in)
{
    float len    = sqrt(sqMagVecFl2d(in));
    vecFl_t norm = {
        .x = in.x / len,
        .y = in.y / len,
    };
    return norm;
}