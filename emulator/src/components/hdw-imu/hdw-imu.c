//==============================================================================
// Includes
//==============================================================================

#include "hdw-imu.h"
#include "hdw-imu_emu.h"
#include "quaternions.h"
#include "trigonometry.h"
#include "esp_random.h"
#include "emu_args.h"
#include "macros.h"

#define ONE_G 256

#define ACCEL_MIN -512
#define ACCEL_MAX 512

static bool accelInit  = false;
static int16_t _accelX = 0;
static int16_t _accelY = 0;
static int16_t _accelZ = 0;

LSM6DSLData LSM6DSL;

/**
 * @brief Initialize the IMU
 *
 * @param sda The GPIO for the Serial DAta line
 * @param scl The GPIO for the Serial CLock line
 * @param pullup Either \c GPIO_PULLUP_DISABLE if there are external pullup resistors on SDA and SCL or \c
 * GPIO_PULLUP_ENABLE if internal pull-ups should be used
 * @return ESP_OK if the accelerometer initialized, or a non-zero value if it did not
 */
esp_err_t initAccelerometer(gpio_num_t sda, gpio_num_t scl, gpio_pullup_t pullup)
{
    // Default to the swadge sitting still, face-up on a table somewhere on earth
    _accelX = 0;
    _accelY = 0;
    _accelZ = ONE_G;

    LSM6DSL.fqQuat[0] = 0.0;
    LSM6DSL.fqQuat[1] = 0.0;
    LSM6DSL.fqQuat[2] = 0.0;
    LSM6DSL.fqQuat[3] = 1.0;

    accelInit = true;
    return ESP_OK;
}

/**
 * @brief Deinitialize the accelerometer (do nothing)
 *
 * @return esp_err_t
 */
esp_err_t deInitAccelerometer(void)
{
    accelInit = false;
    return ESP_OK;
}

/**
 * @brief Read and return the 16-bit step counter
 *
 * Note that this can be configured with ::QMA7981_REG_STEP_CONF_0 through ::QMA7981_REG_STEP_CONF_3
 *
 * @param data The step counter value is written here
 * @return ESP_OK if the step count was read, or a non-zero value if it was not
 */
// esp_err_t accelGetStep(uint16_t* data)
// {
//     if (accelInit)
//     {
//         // TODO emulate step better
//         *data = 0;
//         return ESP_OK;
//     }
//     else
//     {
//         return ESP_ERR_INVALID_STATE;
//     }
// }

/**
 * @brief Read the current acceleration vector from the accelerometer and return
 * the vector through arguments. If the read fails, the last known values are
 * returned instead.
 *
 * @param x The X component of the acceleration vector is written here
 * @param y The Y component of the acceleration vector is written here
 * @param z The Z component of the acceleration vector is written here
 * @return ESP_OK if the acceleration was read, or a non-zero value if it was not
 */
esp_err_t accelGetAccelVecRaw(int16_t* x, int16_t* y, int16_t* z)
{
    if (accelInit)
    {
        *x = _accelX;
        *y = _accelY;
        *z = _accelZ;

        return ESP_OK;
    }
    else
    {
        return ESP_ERR_INVALID_STATE;
    }
}

esp_err_t accelGetOrientVec(int16_t* x, int16_t* y, int16_t* z)
{
    return accelGetAccelVecRaw(x, y, z);
}

esp_err_t accelGetQuaternion(float* q)
{
    // TODO: implement proper emulation of this function.
    return ESP_OK;
}

esp_err_t accelGetSteeringAngleDegrees(int16_t * xcomp, int16_t * ycomp)
{
    // TODO: implement proper emulation of this function.
    return ESP_OK;
}

/**
 * @brief Sets the raw accelerometer reading to be returned by the emulator
 *
 * @param x The x axis value, from -512 to 512, inclusive
 * @param y The y axis value, from -512 to 512, inclusive
 * @param z The z axis value, from -512 to 512, inclusive
 */
void emulatorSetAccelerometer(int16_t x, int16_t y, int16_t z)
{
    _accelX = CLAMP(x, ACCEL_MIN, ACCEL_MAX);
    _accelY = CLAMP(y, ACCEL_MIN, ACCEL_MAX);
    _accelZ = CLAMP(z, ACCEL_MIN, ACCEL_MAX);
}

/**
 * @brief Returns the raw accelerometer readings to be returned by the emulator via out-params
 *
 * @param x A pointer whose value will be updated to the x accelerometer reading
 * @param y A pointer whose value will be updated to the x accelerometer reading
 * @param z A pointer whose value will be updated to the x accelerometer reading
 */
void emulatorGetAccelerometer(int16_t* x, int16_t* y, int16_t* z)
{
    *x = _accelX;
    *y = _accelY;
    *z = _accelZ;
}

/**
 * @brief Returns the bounds of the accelerometer readings via out-params
 *
 * @param min A pointer whose value will be updated to the minimum accelerometer reading
 * @param max A pointer whose value will be updated to the maximum accelerometer reading
 */
void emulatorGetAccelerometerRange(int16_t* min, int16_t* max)
{
    *min = ACCEL_MIN;
    *max = ACCEL_MAX;
}

/**
 * @brief Sets the raw accelerometer reading to the vector defined by the given magnitude and rotations.
 *
 * @param value     The magnitude of the acceleration vector, from -512 to 512, inclusive
 * @param yaw       The counterclockwise rotation about the +Z axis, in degrees
 * @param pitch     The counterclockwise rotation about the +X axis, in degrees
 * @param roll      The counterclockwise rotation about the +Y axis, in degrees [UNUSED]
 */
void emulatorSetAccelerometerRotation(int16_t value, uint16_t yaw, uint16_t pitch, uint16_t roll)
{
    _accelX = CLAMP(value * getSin1024(yaw % 360) / 1024 * getCos1024(pitch % 360) / 1024, ACCEL_MIN, ACCEL_MAX);
    _accelY = CLAMP(value * getSin1024(yaw % 360) / 1024 * getSin1024(pitch % 360) / 1024, ACCEL_MIN, ACCEL_MAX);
    _accelZ = CLAMP(value * getCos1024(yaw % 360) / 1024, ACCEL_MIN, ACCEL_MAX);

    float eulerAngles[3] = {
        _accelX * 3.1415925635 / 180,
        _accelY * 3.1415925635 / 180,
        _accelZ * 3.1415925635 / 180,
    };

    mathEulerToQuat(LSM6DSL.fqQuat, eulerAngles);
}

esp_err_t accelIntegrate()
{
    // Do nothing (is stub function)
    return ESP_OK;
}

// stub
void accelSetRegistersAndReset()
{
}

// stub
esp_err_t accelPerformCal()
{
    return ESP_OK;
}

// stub
float accelGetStdDevInCal()
{
    return 0.0f;
}
