#pragma once

#include <stdint.h>
#include <esp_err.h>

typedef enum
{
    ESP_IF_WIFI_STA = 0, /**< Station interface */
    ESP_IF_WIFI_AP,      /**< Soft-AP interface */
    ESP_IF_WIFI_NAN,     /**< NAN interface */
    ESP_IF_ETH,          /**< Ethernet interface */
    ESP_IF_MAX
} esp_interface_t;

typedef enum
{
    WIFI_MODE_NULL = 0, /**< null mode */
    WIFI_MODE_STA,      /**< WiFi station mode */
    WIFI_MODE_AP,       /**< WiFi soft-AP mode */
    WIFI_MODE_APSTA,    /**< WiFi station + soft-AP mode */
    WIFI_MODE_NAN,      /**< WiFi NAN mode */
    WIFI_MODE_MAX
} wifi_mode_t;

typedef enum
{
    WIFI_IF_STA = ESP_IF_WIFI_STA,
    WIFI_IF_AP  = ESP_IF_WIFI_AP,
#if defined(CONFIG_IDF_TARGET_ESP32) || defined(CONFIG_IDF_TARGET_ESP32S2)
    WIFI_IF_NAN = ESP_IF_WIFI_NAN,
#endif
    WIFI_IF_MAX
} wifi_interface_t;

esp_err_t esp_wifi_get_mac(wifi_interface_t ifx, uint8_t mac[6]);
