//==============================================================================
// Includes
//==============================================================================

#include "hdw-accel.h"
#include "hdw-accel_emu.h"
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
 * @brief Initialize the accelerometer
 *
 * @param _i2c_port The i2c port to use for the accelerometer
 * @param range The range to measure, between ::QMA_RANGE_2G and ::QMA_RANGE_32G
 * @param bandwidth The bandwidth to measure at, between ::QMA_BANDWIDTH_128_HZ and ::QMA_BANDWIDTH_1024_HZ
 * @return ESP_OK if the accelerometer initialized, or a non-zero value if it did not
 */
esp_err_t initAccelerometer(i2c_port_t _i2c_port, gpio_num_t sda, gpio_num_t scl, gpio_pullup_t pullup, uint32_t clkHz)
{
    // Default to the swadge sitting still, face-up on a table somewhere on earth
    _accelX = 0;
    _accelY = 0;
    _accelZ = ONE_G;

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
esp_err_t accelGetStep(uint16_t* data)
{
    if (accelInit)
    {
        // TODO emulate step better
        *data = 0;
        return ESP_OK;
    }
    else
    {
        return ESP_ERR_INVALID_STATE;
    }
}

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
}

esp_err_t accelIntegrate()
{
    // Do nothing (is stub function)
    return ESP_OK;
}

/**
 * @brief Rotate a 3D vector by a quaternion
 *
 * @param pout Pointer to the float[3] output of the rotation
 * @param q Pointer to the wzyz quaternion (float[4]) of the rotation.
 * @param p Pointer to the float[3] of the vector to rotates.
 */
void mathRotateVectorByQuaternion(float* pout, const float* q, const float* p)
{
    // return v + 2.0 * cross(q.xyz, cross(q.xyz, v) + q.w * v);
    float iqo[3];
    mathCrossProduct(iqo, q + 1 /*.xyz*/, p);
    iqo[0] += q[0] * p[0];
    iqo[1] += q[0] * p[1];
    iqo[2] += q[0] * p[2];
    float ret[3];
    mathCrossProduct(ret, q + 1 /*.xyz*/, iqo);
    pout[0] = ret[0] * 2.0 + p[0];
    pout[1] = ret[1] * 2.0 + p[1];
    pout[2] = ret[2] * 2.0 + p[2];
}

/**
 * @brief Perform a 3D cross product
 *
 * @param p Pointer to the float[3] output of the cross product (p = a x b)
 * @param a Pointer to the float[3] of the cross product a vector.
 * @param a Pointer to the float[3] of the cross product b vector.
 */
void mathCrossProduct(float* p, const float* a, const float* b)
{
    float tx = a[1] * b[2] - a[2] * b[1];
    float ty = a[2] * b[0] - a[0] * b[2];
    p[2]     = a[0] * b[1] - a[1] * b[0];
    p[1]     = ty;
    p[0]     = tx;
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
