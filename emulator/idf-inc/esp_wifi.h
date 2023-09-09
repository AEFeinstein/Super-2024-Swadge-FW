#pragma once

#include <stdint.h>
#include <esp_err.h>

#define ESP_ERR_WIFI_NOT_INIT    (ESP_ERR_WIFI_BASE + 1) /*!< WiFi driver was not installed by esp_wifi_init */
#define ESP_ERR_WIFI_NOT_STARTED (ESP_ERR_WIFI_BASE + 2) /*!< WiFi driver was not started by esp_wifi_start */
#define ESP_ERR_WIFI_NOT_STOPPED (ESP_ERR_WIFI_BASE + 3) /*!< WiFi driver was not stopped by esp_wifi_stop */
#define ESP_ERR_WIFI_IF          (ESP_ERR_WIFI_BASE + 4) /*!< WiFi interface error */
#define ESP_ERR_WIFI_MODE        (ESP_ERR_WIFI_BASE + 5) /*!< WiFi mode error */
#define ESP_ERR_WIFI_STATE       (ESP_ERR_WIFI_BASE + 6) /*!< WiFi internal state error */
#define ESP_ERR_WIFI_CONN        (ESP_ERR_WIFI_BASE + 7) /*!< WiFi internal control block of station or soft-AP error */
#define ESP_ERR_WIFI_NVS         (ESP_ERR_WIFI_BASE + 8) /*!< WiFi internal NVS module error */
#define ESP_ERR_WIFI_MAC         (ESP_ERR_WIFI_BASE + 9) /*!< MAC address is invalid */
#define ESP_ERR_WIFI_SSID        (ESP_ERR_WIFI_BASE + 10) /*!< SSID is invalid */
#define ESP_ERR_WIFI_PASSWORD    (ESP_ERR_WIFI_BASE + 11) /*!< Password is invalid */
#define ESP_ERR_WIFI_TIMEOUT     (ESP_ERR_WIFI_BASE + 12) /*!< Timeout error */
#define ESP_ERR_WIFI_WAKE_FAIL   (ESP_ERR_WIFI_BASE + 13) /*!< WiFi is in sleep state(RF closed) and wakeup fail */
#define ESP_ERR_WIFI_WOULD_BLOCK (ESP_ERR_WIFI_BASE + 14) /*!< The caller would block */
#define ESP_ERR_WIFI_NOT_CONNECT (ESP_ERR_WIFI_BASE + 15) /*!< Station still in disconnect status */

#define ESP_ERR_WIFI_POST        (ESP_ERR_WIFI_BASE + 18) /*!< Failed to post the event to WiFi task */
#define ESP_ERR_WIFI_INIT_STATE  (ESP_ERR_WIFI_BASE + 19) /*!< Invalid WiFi state when init/deinit is called */
#define ESP_ERR_WIFI_STOP_STATE  (ESP_ERR_WIFI_BASE + 20) /*!< Returned when WiFi is stopping */
#define ESP_ERR_WIFI_NOT_ASSOC   (ESP_ERR_WIFI_BASE + 21) /*!< The WiFi connection is not associated */
#define ESP_ERR_WIFI_TX_DISALLOW (ESP_ERR_WIFI_BASE + 22) /*!< The WiFi TX is disallowed */

#define ESP_ERR_WIFI_TWT_FULL (ESP_ERR_WIFI_BASE + 23) /*!< no available flow id */
#define ESP_ERR_WIFI_TWT_SETUP_TIMEOUT \
    (ESP_ERR_WIFI_BASE                 \
     + 24) /*!< Timeout of receiving twt setup response frame, timeout times can be set during twt setup */

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
