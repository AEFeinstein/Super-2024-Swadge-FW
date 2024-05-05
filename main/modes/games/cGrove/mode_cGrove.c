/**
 * @file mode_cGrove.c
 * @author Jeremy Stintzcum (Jeremy.Stintzcum@gmail.com)
 * @brief A small Chao garden inspiried program
 * @version 0.1
 * @date 2024-05-04
 * 
 * @copyright Copyright (c) 2024
 * 
 */

// Includes
//==============================================================================
 #include "mode_cGrove.h"

// Defines
//==============================================================================

// Enums
//==============================================================================

// Structs
//==============================================================================
typedef struct
{

} cGrove_t;

// Function Prototypes
//==============================================================================
static void cGroveMainLoop(int64_t elapsedUs);
static void cGroveExitMode(void);
static void cGroveEnterMode(void);
static void cGroveBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);
static void cGroveEspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi);
static void cGroveEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status);

// Strings
//==============================================================================
static const char cGroveTitle[] = "Chowa Grove";

// Variables
//==============================================================================
 swadgeMode_t cGroveMode = {
    .modeName                 = cGroveTitle,
    .wifiMode                 = ESP_NOW,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = cGroveEnterMode,
    .fnExitMode               = cGroveExitMode,
    .fnMainLoop               = cGroveMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = cGroveBackgroundDrawCallback,
    .fnEspNowRecvCb           = cGroveEspNowRecvCb,
    .fnEspNowSendCb           = cGroveEspNowSendCb,
    .fnAdvancedUSB            = NULL,
    .fnDacCb                  = NULL,
};

cGrove_t* grove = NULL;

// Functions
//==============================================================================

static void cGroveEnterMode(void)
{
    grove = calloc(1, sizeof(cGrove_t));
}

static void cGroveExitMode(void)
{
    free(grove);
}

static void cGroveMainLoop(int64_t elapsedUs) 
{

}

static void cGroveBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{

}

static void cGroveEspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi)
{

}

static void cGroveEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status)
{

}