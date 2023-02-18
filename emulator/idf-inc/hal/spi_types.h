/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * @brief Enum with the three SPI peripherals that are software-accessible in it
 */
typedef enum
{
    // SPI1 can be used as GPSPI only on ESP32
    SPI1_HOST = 0, ///< SPI1
    SPI2_HOST = 1, ///< SPI2
    SPI3_HOST = 2, ///< SPI3
    SPI_HOST_MAX,  ///< invalid host value
} spi_host_device_t;
