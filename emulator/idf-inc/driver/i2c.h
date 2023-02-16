/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _DRIVER_I2C_H_
#define _DRIVER_I2C_H_

#include "hal/i2c_types.h"

#define I2C_NUM_MAX            (SOC_I2C_NUM) /*!< I2C port max */
#define I2C_NUM_0              (0) /*!< I2C port 0 */
#if SOC_I2C_NUM >= 2
#define I2C_NUM_1              (1) /*!< I2C port 1 */
#endif

#endif /*_DRIVER_I2C_H_*/
