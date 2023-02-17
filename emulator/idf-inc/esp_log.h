/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ESP_LOG_H__
#define __ESP_LOG_H__

#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <inttypes.h>

#ifndef LOG_LOCAL_LEVEL
#ifndef BOOTLOADER_BUILD
#define LOG_LOCAL_LEVEL  CONFIG_LOG_MAXIMUM_LEVEL
#else
#define LOG_LOCAL_LEVEL  CONFIG_BOOTLOADER_LOG_LEVEL
#endif
#endif

/**
 * @brief Log level
 *
 */
typedef enum {
    ESP_LOG_NONE,       /*!< No log output */
    ESP_LOG_ERROR,      /*!< Critical errors, software module can not recover on its own */
    ESP_LOG_WARN,       /*!< Error conditions from which recovery measures have been taken */
    ESP_LOG_INFO,       /*!< Information messages which describe normal flow of events */
    ESP_LOG_DEBUG,      /*!< Extra information which is not necessary for normal use (values, pointers, sizes, etc). */
    ESP_LOG_VERBOSE     /*!< Bigger chunks of debugging information, or frequent messages which can potentially flood the output. */
} esp_log_level_t;

#define ESP_LOGE( tag, format, ... ) esp_log_write(ESP_LOG_ERROR,   tag, format, ##__VA_ARGS__)
#define ESP_LOGW( tag, format, ... ) esp_log_write(ESP_LOG_WARN,    tag, format, ##__VA_ARGS__)
#define ESP_LOGI( tag, format, ... ) esp_log_write(ESP_LOG_INFO,    tag, format, ##__VA_ARGS__)
#define ESP_LOGD( tag, format, ... ) esp_log_write(ESP_LOG_DEBUG,   tag, format, ##__VA_ARGS__)
#define ESP_LOGV( tag, format, ... ) esp_log_write(ESP_LOG_VERBOSE, tag, format, ##__VA_ARGS__)

/**
 * @brief Write message into the log
 *
 * This function is not intended to be used directly. Instead, use one of
 * ESP_LOGE, ESP_LOGW, ESP_LOGI, ESP_LOGD, ESP_LOGV macros.
 *
 * This function or these macros should not be used from an interrupt.
 */
void esp_log_write(esp_log_level_t level, const char* tag, const char* format, ...)  __attribute__ ((format (printf, 3, 4)));

#endif /* __ESP_LOG_H__ */
