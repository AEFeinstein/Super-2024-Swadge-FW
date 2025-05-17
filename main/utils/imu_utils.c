#include <esp_err.h>
#include <string.h>

#include "hdw-imu.h"
#include "macros.h"
#include "imu_utils.h"

// Limits for detecting shakes
#define SHAKE_THRESHOLD  300
#define SHAKE_HYSTERESIS 10

/**
 * @brief Check if a shake was detected. All of the arguments for this function are both inputs and outputs.
 *
 * This will return if the shake state changed, not if it is shaking or not. The argument \c isShook will contain the
 * shake state.
 *
 * This calls accelIntegrate() and accelGetAccelVecRaw(), so neither of those functions needs to be called elsewhere.
 * Because this function calls accelIntegrate(), it should be called relatively frequently.
 *
 * @param lastOrientation The last IMU orientation, should only be set by this function
 * @param shakeHistory A list of shake values to see when the Swadge settles
 * @param isShook true if the Swadge is shaking, false if it is not
 * @return true if the shake state changed (no shake to shake or vice versa), false if it did not
 */
bool checkForShake(vec3d_t* lastOrientation, list_t* shakeHistory, bool* isShook)
{
    // Get the raw IMU value
    if (ESP_OK == accelIntegrate())
    {
        vec3d_t orientation;
        if (ESP_OK == accelGetAccelVecRaw(&orientation.x, &orientation.y, &orientation.z))
        {
            // Get the difference between the last frame and now
            vec3d_t delta = {
                .x = ABS(lastOrientation->x - orientation.x),
                .y = ABS(lastOrientation->y - orientation.y),
                .z = ABS(lastOrientation->z - orientation.z),
            };
            memcpy(lastOrientation, &orientation, sizeof(vec3d_t));

            // Sum the deltas
            int32_t tDelta = delta.x + delta.y + delta.z;

            // Add the value to the history list
            push(shakeHistory, (void*)((intptr_t)tDelta));
            if (shakeHistory->length > SHAKE_HYSTERESIS)
            {
                shift(shakeHistory);
            }

            // If it's not shaking and over the threshold
            if (!(*isShook) && tDelta > SHAKE_THRESHOLD)
            {
                // Mark it as shaking
                *isShook = true;
                // Change occurred, return true
                return true;
            }
            else if (*isShook)
            {
                // If it's shaking, check for stability
                node_t* shakeNode = shakeHistory->first;
                while (shakeNode)
                {
                    if ((intptr_t)shakeNode->val > SHAKE_THRESHOLD)
                    {
                        // Shake in the history, return no change
                        return false;
                    }
                    shakeNode = shakeNode->next;
                }

                // No shake in the history, mark it as not shaking
                *isShook = false;
                // Change occurred, return true
                return true;
            }
        }
    }
    // No change, return false
    return false;
}
