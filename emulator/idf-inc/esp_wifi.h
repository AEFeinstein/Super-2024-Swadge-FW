#pragma once

#include <stdint.h>
#include <esp_err.h>

typedef enum
{
    ESP_IF_WIFI_STA = 0, /**< ESP32 station interface */
    ESP_IF_WIFI_AP,      /**< ESP32 soft-AP interface */
    ESP_IF_ETH,          /**< ESP32 ethernet interface */
    ESP_IF_MAX
} esp_interface_t;

typedef enum
{
    WIFI_MODE_NULL = 0, /**< null mode */
    WIFI_MODE_STA,      /**< WiFi station mode */
    WIFI_MODE_AP,       /**< WiFi soft-AP mode */
    WIFI_MODE_APSTA,    /**< WiFi station + soft-AP mode */
    WIFI_MODE_MAX
} wifi_mode_t;

typedef enum
{
    WIFI_IF_STA = ESP_IF_WIFI_STA,
    WIFI_IF_AP  = ESP_IF_WIFI_AP,
} wifi_interface_t;

esp_err_t esp_wifi_get_mac(wifi_interface_t ifx, uint8_t mac[6]);
