/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

typedef enum
{
    GPIO_PULLUP_DISABLE = 0x0, /*!< Disable GPIO pull-up resistor */
    GPIO_PULLUP_ENABLE  = 0x1, /*!< Enable GPIO pull-up resistor */
} gpio_pullup_t;
