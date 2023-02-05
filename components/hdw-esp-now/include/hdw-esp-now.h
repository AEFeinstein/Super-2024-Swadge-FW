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
    NO_WIFI,
    ESP_NOW,           // Calls back on main thread.
    ESP_NOW_IMMEDIATE, // Calls back from interrupt.
} wifiMode_t;

//==============================================================================
// Prototypes
//==============================================================================

typedef void (*hostEspNowRecvCb_t)(const uint8_t* mac_addr, const char* data, uint8_t len, int8_t rssi);
typedef void (*hostEspNowSendCb_t)(const uint8_t* mac_addr, esp_now_send_status_t status);

void espNowInit(hostEspNowRecvCb_t recvCb, hostEspNowSendCb_t sendCb, gpio_num_t rx, gpio_num_t tx, uart_port_t uart,
                wifiMode_t mode);
void espNowDeinit(void);

void espNowUseWireless(void);
void espNowUseSerial(bool crossoverPins);

void espNowSend(const char* data, uint8_t len);
void checkEspNowRxQueue(void);

#endif /* USER_ESPNOWUTILS_H_ */
