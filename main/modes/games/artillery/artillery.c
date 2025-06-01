#include "artillery.h"

void artilleryEnterMode(void);
void artilleryExitMode(void);
void artilleryMainLoop(int64_t elapsedUs);
void artilleryBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);
void artilleryEspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi);
void artilleryEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status);

swadgeMode_t artilleryMode = {
    .modeName                 = "Artillery",
    .wifiMode                 = ESP_NOW,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = artilleryEnterMode,
    .fnExitMode               = artilleryExitMode,
    .fnMainLoop               = artilleryMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = artilleryBackgroundDrawCallback,
    .fnEspNowRecvCb           = artilleryEspNowRecvCb,
    .fnEspNowSendCb           = artilleryEspNowSendCb,
    .fnAdvancedUSB            = NULL,
    .fnDacCb                  = NULL,
    .fnAddToSwadgePassPacket  = NULL,
    .trophyData               = NULL,
};

/**
 * @brief This function is called when this mode is started. It should initialize variables and start the mode.
 */
void artilleryEnterMode(void)
{
}

/**
 * @brief This function is called when the mode is exited. It should free any allocated memory.
 */
void artilleryExitMode(void)
{
}

/**
 * @brief This function is called from the main loop. It's pretty quick, but the timing may be inconsistent.
 *
 * @param elapsedUs The time elapsed since the last time this function was called. Use this value to determine when
 * it's time to do things
 */
void artilleryMainLoop(int64_t elapsedUs)
{
}

/**
 * @brief This function is called when the display driver wishes to update a section of the display.
 *
 * @param x the x coordinate that should be updated
 * @param y the x coordinate that should be updated
 * @param w the width of the rectangle to be updated
 * @param h the height of the rectangle to be updated
 * @param up update number
 * @param upNum update number denominator
 */
void artilleryBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
}

/**
 * @brief This function is called whenever an ESP-NOW packet is received.
 *
 * @param esp_now_info Information about the transmission, including The MAC addresses
 * @param data     A pointer to the data received
 * @param len      The length of the data received
 * @param rssi     The RSSI for this packet, from 1 (weak) to ~90 (touching)
 */
void artilleryEspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi)
{
}

/**
 * @brief This function is called whenever an ESP-NOW packet is sent. It is just a status callback whether or not
 * the packet was actually sent. This will be called after calling espNowSend().
 *
 * @param mac_addr The MAC address which the data was sent to
 * @param status   The status of the transmission
 */
void artilleryEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status)
{
}
