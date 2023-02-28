/*! \file hdw-accel.h
 *
 * \section accel_design Design Philosophy
 *
 * The accelerometer used is a QMA7981.
 * The datasheet can be found here: <a href="https://datasheet.lcsc.com/lcsc/2004281102_QST-QMA7981_C457290.pdf">QMA7981
 * Datasheet</a>.
 *
 * The accelerometer component does not automatically poll the accelerometer.
 * All it does is set up and configure the accelerometer, then it is up to the Swadge Mode to query for acceleration as
 * appropriate.
 *
 * This component requires the I2C bus to be initialized, so it does that as well.
 * If other I2C peripherals are added in the future, common I2C bus initialization should be moved to a more common
 * location.
 *
 * \section accel_usage Usage
 *
 * You don't need to call initAccelerometer() or deInitAccelerometer(). The system does this the appropriate time.
 *
 * You do need to call accelGetAccelVec() to get the current acceleration vector.
 * If you want to poll this from your Swadge Mode's main function, you may.
 *
 * You may call accelSetRange() if you want to adjust the measurement range.
 *
 * accelGetStep() exists, but it has not been tested, so use it with caution.
 * You may need to configure parameters related to step counting.
 *
 * \section accel_example Example
 *
 * \code{.c}
 * // Declare variables to receive acceleration
 * int16_t a_x, a_y, a_z;
 *
 * // Get the current acceleration
 * if(ESP_OK == accelGetAccelVec(&a_x, &a_y, &a_z))
 * {
 *     // Print data to debug logs
 *     printf("x: %"PRId16", y: %"PRId16", z:%"PRId16, a_x, a_y, a_z);
 * }
 * \endcode
 */

#ifndef _HDW_ACCEL_H_
#define _HDW_ACCEL_H_

#include <stdint.h>

#include <driver/i2c.h>
#include <hal/gpio_types.h>
#include <esp_err.h>

typedef enum
{
    QMA_RANGE_2G  = 0b0001, ///< Two G's of measurement range
    QMA_RANGE_4G  = 0b0010, ///< Four G's of measurement range
    QMA_RANGE_8G  = 0b0100, ///< Eight G's of measurement range
    QMA_RANGE_16G = 0b1000, ///< Sixteen G's of measurement range
    QMA_RANGE_32G = 0b1111, ///< Thirty-two G's of measurement range
} qma_range_t;

typedef enum
{
    QMA_BANDWIDTH_128_HZ  = 0b111, ///< 128Hz bandwidth
    QMA_BANDWIDTH_256_HZ  = 0b110, ///< 256Hz bandwidth
    QMA_BANDWIDTH_1024_HZ = 0b101, ///< 1024Hz bandwidth
} qma_bandwidth_t;

esp_err_t initAccelerometer(i2c_port_t _i2c_port, gpio_num_t sda, gpio_num_t scl, gpio_pullup_t pullup, uint32_t clkHz,
                            qma_range_t range, qma_bandwidth_t bandwidth);
esp_err_t deInitAccelerometer(void);
esp_err_t accelSetRange(qma_range_t range);
esp_err_t accelGetAccelVec(int16_t* x, int16_t* y, int16_t* z);
esp_err_t accelGetStep(uint16_t* data);

#endif
