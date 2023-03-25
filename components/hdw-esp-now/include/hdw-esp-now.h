/*! \file hdw-esp-now.h
 * \date Oct 29, 2018
 * \author adam
 *
 * \section esp-now_design Design Philosophy
 *
 * <a
 * href="https://docs.espressif.com/projects/esp-idf/en/v5.0.1/esp32s2/api-reference/network/esp_now.html">ESP-NOW</a>
 * is a kind of connection-less Wi-Fi communication protocol that is defined by Espressif. This component manages ESP-NOW
 * so that you don't have to. It provides a simple wrapper to broadcast a packet, espNowSend(), and passes all received
 * packets through a callback given to initEspNow().
 *
 * Swadges do not use any ESP-NOW security and do not pair with each other using ESP-NOW.
 * All transmissions are broadcasts and all Swadges will receive all other transmissions.
 * Two Swadges may 'pair' with each other by including the recipient's MAC address in a transmitted packet.
 * This pairing can be done using p2pConnection.c.
 *
 * This component can also facilitate communication between to Swadges using a <a
 * href="https://docs.espressif.com/projects/esp-idf/en/v5.0.1/esp32s2/api-reference/peripherals/uart.html">Universal
 * Asynchronous Receiver/Transmitter (UART)</a> wired connection. The wired UART is significantly faster and
 * significantly more reliable than the wireless one, but it does require a physical wire.
 *
 * Swadges have USB-C ports and the wired connection reconfigures the D+ and D- pins within the USB cable to be serial
 * RX and TX instead. This means that while the serial connection is active, USB is non-functional. It also means that
 * one Swadge has to treat D+ as RX and D- as TX, while the other has to do the opposite and treat D+ as TX and D- as
 * RX. If a Swadge mode uses wired connections, it's UI must have an options to assume either role (commonly Swadge A
 * and Swadge B).
 *
 * \section esp-now_usage Usage
 *
 * You don't need to call initEspNow(), checkEspNowRxQueue(), or deinitEspNow() . The system does at the appropriate
 * time.
 *
 * espNowUseWireless() and espNowUseSerial() may be used to switch between wireless connections and wired connections,
 * respectively. espNowUseSerial() is also used to configure the RX and TX pins for UART communication.
 *
 * To send a packet, call espNowSend().
 * When the packet transmission finishes, the ::hostEspNowSendCb_t callback passed to initEspNow() is called with a
 * status of either ESP_NOW_SEND_SUCCESS or ESP_NOW_SEND_FAIL.
 *
 * When a packet is received, the ::hostEspNowRecvCb_t callback passed to initEspNow() is called with the received
 * packet.
 *
 * \section esp-now_example Example
 *
 * \code{.c}
 * static void swadgeModeEspNowRecvCb(const uint8_t* mac_addr, const char* data, uint8_t len, int8_t rssi)
 * {
 *     printf("Received %d bytes\n", len);
 * }
 *
 * static void swadgeModeEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status)
 * {
 *     printf("Sent packet: %d\n", status);
 * }
 *
 * ...
 *
 * {
 *     initEspNow(&swadgeModeEspNowRecvCb, &swadgeModeEspNowSendCb,
 *         GPIO_NUM_19, GPIO_NUM_20, UART_NUM_1, ESP_NOW);
 *
 *     while(1)
 *     {
 *         // Check for received packets
 *         checkEspNowRxQueue();
 *
 *         // Send a test packet
 *         char testPacket[] = "TEST";
 *         espNowSend(testPacket, ARRAY_SIZE(testPacket));
 *     }
 * }
 * \endcode
 */

#ifndef USER_ESP_NOW_UTILS_H_
#define USER_ESP_NOW_UTILS_H_

//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include <stdbool.h>

#include <esp_now.h>
#include <esp_err.h>
#include <hal/gpio_types.h>
#include <driver/uart.h>

//==============================================================================
// Types
//==============================================================================

typedef enum __attribute__((packed))
{
    NO_WIFI,           ///< WiFi is not used at all
    ESP_NOW,           ///< ESP-NOW packets are delivered to Swadge modes from the main loop
    ESP_NOW_IMMEDIATE, ///< ESP-NOW packets are delivered to Swadge modes from the interrupt
} wifiMode_t;

//==============================================================================
// Prototypes
//==============================================================================

/**
 * @brief A function typedef for a callback called when an ESP-NOW packet is received
 * @param esp_now_info Information about the transmission, including The MAC addresses
 * @param data The received packet
 * @param len The length of the received packet
 * @param rssi The signal strength of the received packet
 */
typedef void (*hostEspNowRecvCb_t)(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len,
                                   int8_t rssi);
/**
 * @brief A function typedef for a callback called when an ESP-NOW packet transmission finishes
 * @param mac_addr The MAC address which was transmitted to
 * @param status The transmission status, either ESP_NOW_SEND_SUCCESS or ESP_NOW_SEND_FAIL
 */
typedef void (*hostEspNowSendCb_t)(const uint8_t* mac_addr, esp_now_send_status_t status);

esp_err_t initEspNow(hostEspNowRecvCb_t recvCb, hostEspNowSendCb_t sendCb, gpio_num_t rx, gpio_num_t tx,
                     uart_port_t uart, wifiMode_t wifiMode);
void deinitEspNow(void);

esp_err_t espNowUseWireless(void);
void espNowUseSerial(bool crossoverPins);

void espNowSend(const char* data, uint8_t len);
void checkEspNowRxQueue(void);

#endif /* USER_ESP_NOW_UTILS_H_ */
