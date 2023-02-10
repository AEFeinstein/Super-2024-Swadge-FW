/*
 * espNowUtils.h
 *
 *  Created on: Oct 29, 2018
 *      Author: adam
 */

#ifndef USER_ESPNOWUTILS_H_
#define USER_ESPNOWUTILS_H_

//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include <stdbool.h>

#include <esp_now.h>
#include <hal/gpio_types.h>
#include <driver/uart.h>

//==============================================================================
// Types
//==============================================================================

typedef enum __attribute__((packed))
{
    NO_WIFI,           ///< WiFi is not used at all
    ESP_NOW,           ///< ESP-NOW packets are delievered to Swadge modes from the main loop
    ESP_NOW_IMMEDIATE, ///< ESP-NOW packets are delievered to Swadge modes from the interrupt
} wifiMode_t;

//==============================================================================
// Prototypes
//==============================================================================

/**
 * @brief A function typedef for a callback called when an ESP-NOW packet is received
 * @param mac_addr The MAC address of the sender of the received packet
 * @param data The received packet
 * @param len The length of the received packet
 * @param rssi The signal strength of the received packet
 */
typedef void (*hostEspNowRecvCb_t)(const uint8_t* mac_addr, const char* data, uint8_t len, int8_t rssi);
/**
 * @brief A function typedef for a callback called when an ESP-NOW packet transmission finishes
 * @param mac_addr The MAC address which was transmitted to
 * @param status The transmission status, either ESP_NOW_SEND_SUCCESS or ESP_NOW_SEND_FAIL
 */
typedef void (*hostEspNowSendCb_t)(const uint8_t* mac_addr, esp_now_send_status_t status);

void initEspNow(hostEspNowRecvCb_t recvCb, hostEspNowSendCb_t sendCb, gpio_num_t rx, gpio_num_t tx, uart_port_t uart,
                wifiMode_t mode);
void espNowDeinit(void);

void espNowUseWireless(void);
void espNowUseSerial(bool crossoverPins);

void espNowSend(const char* data, uint8_t len);
void checkEspNowRxQueue(void);

#endif /* USER_ESPNOWUTILS_H_ */
