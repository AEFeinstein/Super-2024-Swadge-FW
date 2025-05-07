/*! \file imu_utils.h
 *
 * \section imu_utils_design Design Philosophy
 *
 * Streaming IMU data may be tricky to interpret for something simple like shake detection. This utility provides a
 * simple interface for shake detection. It should not be used when full orientation data is needed.
 *
 * \section imu_utils_usage Usage
 *
 * Call checkForShake() frequently, like from a Swadge Mode's main loop. You do not need to call accelIntegrate() or
 * accelGetAccelVecRaw() elsewhere. When it returns \c true, then the shake state has changed and the \c isShook
 * argument can be checked for the current shake state.
 *
 * \section imu_utils_example Example
 *
 * \code{.c}
 *
 * // These variables are static to preserve their value between calls.
 * // Ideally they are contained in a Swadge Mode struct
 * static vec3d_t lastOrientation;
 * static list_t shakeHistory;
 * static bool isShook;
 *
 * if (checkForShake(&lastOrientation, &shakeHistory, &isShook))
 * {
 *     if (isShook)
 *     {
 *         ESP_LOGD("IMU", "Shake detected!");
 *     }
 *     else
 *     {
 *         ESP_LOGD("IMU", "Shake stopped");
 *     }
 * }
 *
 * \endcode
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "linked_list.h"

/**
 * @brief A general purpose 3D vector
 */
typedef struct
{
    int16_t x; ///< The x component of the vector
    int16_t y; ///< The y component of the vector
    int16_t z; ///< The z component of the vector
} vec3d_t;

bool checkForShake(vec3d_t* lastOrientation, list_t* shakeHistory, bool* isShook);