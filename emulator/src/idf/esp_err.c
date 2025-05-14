#include <esp_err.h>

const char* esp_err_to_name(esp_err_t code)
{
    switch (code)
    {
        case ESP_OK:
        {
            return "ESP_OK";
        }
        case ESP_FAIL:
        {
            return "ESP_FAIL";
        }
        case ESP_ERR_NO_MEM:
        {
            return "ESP_ERR_NO_MEM";
        }
        case ESP_ERR_INVALID_ARG:
        {
            return "ESP_ERR_INVALID_ARG";
        }
        case ESP_ERR_INVALID_STATE:
        {
            return "ESP_ERR_INVALID_STATE";
        }
        case ESP_ERR_INVALID_SIZE:
        {
            return "ESP_ERR_INVALID_SIZE";
        }
        case ESP_ERR_NOT_FOUND:
        {
            return "ESP_ERR_NOT_FOUND";
        }
        case ESP_ERR_NOT_SUPPORTED:
        {
            return "ESP_ERR_NOT_SUPPORTED";
        }
        case ESP_ERR_TIMEOUT:
        {
            return "ESP_ERR_TIMEOUT";
        }
        case ESP_ERR_INVALID_RESPONSE:
        {
            return "ESP_ERR_INVALID_RESPONSE";
        }
        case ESP_ERR_INVALID_CRC:
        {
            return "ESP_ERR_INVALID_CRC";
        }
        case ESP_ERR_INVALID_VERSION:
        {
            return "ESP_ERR_INVALID_VERSION";
        }
        case ESP_ERR_INVALID_MAC:
        {
            return "ESP_ERR_INVALID_MAC";
        }
        case ESP_ERR_NOT_FINISHED:
        {
            return "ESP_ERR_NOT_FINISHED";
        }
        case ESP_ERR_WIFI_BASE:
        {
            return "ESP_ERR_WIFI_BASE";
        }
        case ESP_ERR_MESH_BASE:
        {
            return "ESP_ERR_MESH_BASE";
        }
        case ESP_ERR_FLASH_BASE:
        {
            return "ESP_ERR_FLASH_BASE";
        }
        case ESP_ERR_HW_CRYPTO_BASE:
        {
            return "ESP_ERR_HW_CRYPTO_BASE";
        }
        case ESP_ERR_MEMPROT_BASE:
        {
            return "ESP_ERR_MEMPROT_BASE";
        }
    }
    return "ERR_UNKNOWN";
}