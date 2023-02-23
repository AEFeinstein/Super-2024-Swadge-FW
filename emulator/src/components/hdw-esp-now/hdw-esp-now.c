//==============================================================================
// Includes
//==============================================================================

#include "hdw-esp-now.h"
#include "emu_main.h"

//==============================================================================
// Functions
//==============================================================================

/**
 * Initialize ESP-NOW and attach callback functions.
 * This uses wifi by default, but espNowUseSerial() may be called later to
 * communicate over the given UART instead
 *
 * @param recvCb A callback to call when data is sent
 * @param sendCb A callback to call when data is received
 * @param rx The receive pin when using serial communication instead of wifi. Use GPIO_NUM_NC for no GPIO
 * @param tx The transmit pin when using serial communication instead of wifi. Use GPIO_NUM_NC for no GPIO
 * @param uart The UART to use for serial communication. Use UART_NUM_MAX for no UART
 * @param wifiMode The WiFi mode. If ESP_NOW_IMMEDIATE, then recvCb is called directly from the interrupt. If ESP_NOW,
 * then recvCb is called from checkEspNowRxQueue()
 */
esp_err_t initEspNow(hostEspNowRecvCb_t recvCb, hostEspNowSendCb_t sendCb, gpio_num_t rx, gpio_num_t tx,
                     uart_port_t uart, wifiMode_t wifiMode)
{
    WARN_UNIMPLEMENTED();
    return ESP_OK;
}

/**
 * Start wifi and use it for communication
 */
esp_err_t espNowUseWireless(void)
{
    WARN_UNIMPLEMENTED();
    return ESP_OK;
}

/**
 * Start the UART and use it for communication
 *
 * @param crossoverPins true to crossover the rx and tx pins, false to use them
 *                      as normal.
 */
void espNowUseSerial(bool crossoverPins)
{
    WARN_UNIMPLEMENTED();
}

/**
 * Check the ESP NOW receive queue. If there are any received packets, send
 * them to hostEspNowRecvCb()
 */
void checkEspNowRxQueue(void)
{
    WARN_UNIMPLEMENTED();
}

/**
 * This is a wrapper for esp_now_send(). It also sets the wifi power with
 * wifi_set_user_fixed_rate()
 *
 * @param data The data to broadcast using ESP NOW
 * @param len  The length of the data to broadcast
 */
void espNowSend(const char* data, uint8_t len)
{
    WARN_UNIMPLEMENTED();
}

/**
 * This function is called to de-initialize ESP-NOW
 */
void deinitEspNow(void)
{
    WARN_UNIMPLEMENTED();
}
