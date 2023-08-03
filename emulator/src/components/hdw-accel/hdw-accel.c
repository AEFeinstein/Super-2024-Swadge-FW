//==============================================================================
// Includes
//==============================================================================

#include "hdw-accel.h"
#include "esp_random.h"
#include "emu_main.h"

#define ONE_G 242
///< The range of randomness to add to or subtract from the actual value at each reading
#define ACCEL_JITTER 3

static bool accelInit = false;
static int16_t _accelX = 0;
static int16_t _accelY = 0;
static int16_t _accelZ = 0;

/**
 * @brief Initialize the accelerometer
 *
 * @param _i2c_port The i2c port to use for the accelerometer
 * @param range The range to measure, between ::QMA_RANGE_2G and ::QMA_RANGE_32G
 * @param bandwidth The bandwidth to measure at, between ::QMA_BANDWIDTH_128_HZ and ::QMA_BANDWIDTH_1024_HZ
 * @return ESP_OK if the accelerometer initialized, or a non-zero value if it did not
 */
esp_err_t initAccelerometer(i2c_port_t _i2c_port, gpio_num_t sda, gpio_num_t scl, gpio_pullup_t pullup, uint32_t clkHz,
                            qma_range_t range, qma_bandwidth_t bandwidth)
{
    // divide up one G evenly between the axes, randomly
    // the math doesn't quite math but honestly it's good enough
    int16_t start = ONE_G;

    _accelX = (esp_random() % start);
    start -= _accelX;

    _accelY = (esp_random() % start);
    start -= _accelY;

    _accelZ = start;

    _accelX *= (esp_random() % 2) ? 1 : -1;
    _accelY *= (esp_random() % 2) ? 1 : -1;
    _accelZ *= (esp_random() % 2) ? 1 : -1;

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
 * @brief Set the accelerometer's measurement range
 *
 * @param range The range to measure, from ::QMA_RANGE_2G to ::QMA_RANGE_32G
 * @return ESP_OK if the range was set, or a non-zero value if it was not
 */
esp_err_t accelSetRange(qma_range_t range)
{
    if (accelInit)
    {
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
esp_err_t accelGetAccelVec(int16_t* x, int16_t* y, int16_t* z)
{

    if (accelInit)
    {
        *x = _accelX + (esp_random() % (ACCEL_JITTER * 2 + 1) - ACCEL_JITTER - 1);
        *y = _accelY + (esp_random() % (ACCEL_JITTER * 2 + 1) - ACCEL_JITTER - 1);
        *z = _accelZ + (esp_random() % (ACCEL_JITTER * 2 + 1) - ACCEL_JITTER - 1);

        // randomly, approximately every 3 readings
        if (!(esp_random() % 3))
        {
            // change the value, in a random direction
            // this will make sure the sum is about 1G
            switch ((esp_random() % 6))
            {
                #define SWAP(a,b) if (a > -ONE_G && b < ONE_G) { a--; b++; }

                // X -> Y
                case 0:
                SWAP(_accelX, _accelY)
                break;

                // X -> Z
                case 3:
                SWAP(_accelX, _accelZ)
                break;

                // Y -> X
                case 1:
                SWAP(_accelY, _accelX)
                break;

                // Y -> Z
                case 4:
                SWAP(_accelY, _accelZ)
                break;

                // Z -> X
                case 2:
                SWAP(_accelZ, _accelX)
                break;

                // Z -> Y
                case 5:
                SWAP(_accelZ, _accelY)
                break;

                #undef SWAP
            }
        }
        return ESP_OK;
    }
    else
    {
        return ESP_ERR_INVALID_STATE;
    }
}
