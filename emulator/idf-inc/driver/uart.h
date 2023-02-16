/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "hal/uart_types.h"

// Valid UART port number
#define UART_NUM_0             (0) /*!< UART port 0 */
#define UART_NUM_1             (1) /*!< UART port 1 */
#if SOC_UART_NUM > 2
#define UART_NUM_2             (2) /*!< UART port 2 */
#endif
#define UART_NUM_MAX           (SOC_UART_NUM) /*!< UART port max */
