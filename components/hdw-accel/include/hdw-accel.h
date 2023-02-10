#ifndef _HDW_ACCEL_H_
#define _HDW_ACCEL_H_

#include <stdint.h>

#include <driver/i2c.h>
#include <hal/gpio_types.h>
#include <esp_err.h>

typedef enum
{
    QMA_RANGE_2G  = 0b0001,
    QMA_RANGE_4G  = 0b0010,
    QMA_RANGE_8G  = 0b0100,
    QMA_RANGE_16G = 0b1000,
    QMA_RANGE_32G = 0b1111,
} qma_range_t;

typedef enum
{
    QMA_BANDWIDTH_128_HZ  = 0b111,
    QMA_BANDWIDTH_256_HZ  = 0b110,
    QMA_BANDWIDTH_1024_HZ = 0b101,
} qma_bandwidth_t;

esp_err_t qma7981_init(i2c_port_t _i2c_port, gpio_num_t sda, gpio_num_t scl, gpio_pullup_t pullup, uint32_t clkHz,
                       qma_range_t range, qma_bandwidth_t bandwidth);
esp_err_t qma7981_set_range(qma_range_t range);
esp_err_t qma7981_get_accel(int16_t* x, int16_t* y, int16_t* z);
esp_err_t qma7981_get_step(uint16_t* data);

#endif
