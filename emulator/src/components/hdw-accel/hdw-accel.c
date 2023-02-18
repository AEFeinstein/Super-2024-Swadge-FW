//==============================================================================
// Includes
//==============================================================================

#include "hdw-accel.h"
#include "emu_main.h"

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
    WARN_UNIMPLEMENTED();
    return ESP_OK;
}

/**
 * @brief Deinitialize the accelerometer (do nothing)
 *
 * @return esp_err_t
 */
esp_err_t deInitAccelerometer(void)
{
    WARN_UNIMPLEMENTED();
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
    WARN_UNIMPLEMENTED();
    *data = 0;
    return ESP_OK;
}

/**
 * @brief Set the accelerometer's measurement range
 *
 * @param range The range to measure, from ::QMA_RANGE_2G to ::QMA_RANGE_32G
 * @return ESP_OK if the range was set, or a non-zero value if it was not
 */
esp_err_t accelSetRange(qma_range_t range)
{
    WARN_UNIMPLEMENTED();
    return ESP_OK;
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
    WARN_UNIMPLEMENTED();
    *x = 0;
    *y = 0;
    *z = 0;
    return ESP_OK;
}
