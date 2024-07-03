/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * @brief UART port number, can be UART_NUM_0 ~ (UART_NUM_MAX -1).
 */
typedef int uart_port_t;

/**
 * @brief UART mode selection
 */
typedef enum
{
    UART_MODE_UART                   = 0x00, /*!< mode: regular UART mode*/
    UART_MODE_RS485_HALF_DUPLEX      = 0x01, /*!< mode: half duplex RS485 UART mode control by RTS pin */
    UART_MODE_IRDA                   = 0x02, /*!< mode: IRDA  UART mode*/
    UART_MODE_RS485_COLLISION_DETECT = 0x03, /*!< mode: RS485 collision detection UART mode (used for test purposes)*/
    UART_MODE_RS485_APP_CTRL         = 0x04, /*!< mode: application control RS485 UART mode (used for test purposes)*/
} uart_mode_t;
