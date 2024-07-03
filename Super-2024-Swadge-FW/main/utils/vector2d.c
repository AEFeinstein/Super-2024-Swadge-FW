#include "trigonometry.h"
#include "vector2d.h"

/**
 * @brief Add two vectors and return the resulting vector
 *
 * @param left One vector to add
 * @param right The other vector to add
 * @return The sum of both vectors
 */
vec_t addVec2d(vec_t left, vec_t right)
{
    vec_t result = {
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
vec_t subVec2d(vec_t left, vec_t right)
{
    vec_t result = {
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
vec_t mulVec2d(vec_t vector, int32_t scalar)
{
    vec_t result = {
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
vec_t divVec2d(vec_t vector, int32_t scalar)
{
    vec_t result = {
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
int32_t dotVec2d(vec_t left, vec_t right)
{
    return (left.x * right.x) + (left.y * right.y);
}

/**
 * @brief Rotate a vector by a number of degrees and return the result
 *
 * @param vector The vector to rotate
 * @param degree The angle to rotate clockwise by, in degrees
 * @return The rotated vector
 */
vec_t rotateVec2d(vec_t vector, int32_t degree)
{
    while (degree < 0)
    {
        degree += 360;
    }
    while (degree > 359)
    {
        degree -= 360;
    }

    int16_t sin  = getSin1024(degree);
    int16_t cos  = getCos1024(degree);
    int32_t oldX = vector.x;
    int32_t oldY = vector.y;

    vec_t result = {
        .x = ((oldX * cos) - (oldY * sin)) / 1024,
        .y = ((oldX * sin) + (oldY * cos)) / 1024,
    };
    return result;
}

/**
 * @brief Return the squared magnitude of the given vector. The square root is not used because it is slow.
 *
 * @param vector The vector to get the squared magnitude for
 * @return The squared magnitude of the vector
 */
int32_t sqMagVec2d(vec_t vector)
{
    return (vector.x * vector.x) + (vector.y * vector.y);
}
