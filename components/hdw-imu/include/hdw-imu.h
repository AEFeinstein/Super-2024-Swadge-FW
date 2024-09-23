/*! \file hdw-imu.h
 *
 * \section imu_design Design Philosophy
 *
 * Originally Swadges were planned to use a LSM6DSL and a QMC6308, however, because the batteries are so close to the
 * magnetometer, the quality of the data was low enough we decided to proceed with with a LSM6DSL-only IMU.
 *
 * Unlike the accelerometer process, the IMU fuses the gyroscope and accelerometer data from the LMS6DSL.  By fusing
 * both sensors, we are able to produce a quaternion to represent the rotation of the swadge.  The idea is that
 * we run the IMU at 208 Hz, and we use the hardware FIFO built into the LSM6DSL to queue up events.  Then, every
 * frame, we empty out the FIFO.
 *
 * \section imu_usage Usage
 *
 * The core system will call initAccelerometer() and deInitAccelerometer() appropriately. And you can at any point
 * call any of the proper IMU / accel functions.
 *
 * accelSetRegistersAndReset() \b must be called when entering a Swadge mode that uses the IMU.
 *
 * accelIntegrate() \b should be called periodically. This can be done either in the mode's main loop or in the
 * background draw callback. If this is called from the background draw callback, make sure to only call it once per
 * frame, since multiple callbacks are called per-frame.
 *
 * The functions you can use to get acceleration data are:
 *  - accelGetAccelVecRaw()
 *  - accelGetQuaternion()
 *
 * \section imu_example Example
 *
 * \code{.c}
 * // Declare variables to receive rotation
 * float q[4];
 *
 * // Get the current rotation
 * if(ESP_OK == accelGetQuaternion( q ))
 * {
 *     // Print data to debug logs
 *     printf( "%f %f %f %f\n", q[0], q[1], q[2], q[3] );
 * }
 * \endcode
 */

#ifndef _HDW_IMU_H_
#define _HDW_IMU_H_

#include <stdint.h>

#include <hal/gpio_types.h>
#include <soc/gpio_num.h>
#include <esp_err.h>

#include "quaternions.h"

typedef struct
{
    int32_t temp;
    uint32_t computetime;
    uint32_t performCal; // 1 if expecting a zero cal.

    // Quats are wxyz.
    // You can take a vector, in controller space, rotate by this quat, and you get it in world space.
    float fqQuatLast[4]; // Delta
    float fqQuat[4];     // Absolute

    // The last raw accelerometer (NOT FUSED)
    float fvLastAccelRaw[3];

    // Bias for all of the euler angles.
    float fvBias[3];

    // Used for calibration
    float fvDeviation[3];
    float fvAverage[3];

    uint32_t sampCount;

    // For debug
    int lastreadr;
    int32_t gyroaccum[3];
    int16_t gyrolast[3];
    int16_t accellast[3];
    float fCorrectLast[3];
} LSM6DSLData;

extern LSM6DSLData LSM6DSL;

esp_err_t initAccelerometer(gpio_num_t sda, gpio_num_t scl, gpio_pullup_t pullup);
esp_err_t deInitAccelerometer(void);
esp_err_t accelGetAccelVecRaw(int16_t* x, int16_t* y, int16_t* z);
esp_err_t accelGetOrientVec(int16_t* x, int16_t* y, int16_t* z);
esp_err_t accelGetQuaternion(float* q);
esp_err_t accelIntegrate(void);
esp_err_t accelPerformCal(void);
esp_err_t accelGetSteeringAngleDegrees(int16_t* xcomp, int16_t* ycomp);
float accelGetStdDevInCal(void);
void accelSetRegistersAndReset(void);

#endif
