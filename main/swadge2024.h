#ifndef _SWADGE_MODE_H_
#define _SWADGE_MODE_H_

// Standard C includes
#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Hardware interfaces
#include "hdw-accel.h"
#include "hdw-btn.h"
#include "hdw-bzr.h"
#include "hdw-esp-now.h"
#include "hdw-led.h"
#include "hdw-mic.h"
#include "hdw-nvs.h"
#include "hdw-spiffs.h"
#include "hdw-temperature.h"
#include "hdw-tft.h"
#include "hdw-usb.h"

// Drawing interfaces
#include "palette.h"
#include "color_utils.h"
#include "font.h"
#include "wsg.h"
#include "bresenham.h"
#include "cndraw.h"
#include "fill.h"

// Asset loaders
#include "spiffs_wsg.h"
#include "spiffs_font.h"
#include "spiffs_txt.h"
#include "spiffs_json.h"
#include "spiffs_sng.h"

// Connection interface
#include "p2pConnection.h"

// General utilities
#include "linked_list.h"
#include "macros.h"
#include "trigonometry.h"

/**
 * A struct of all the function pointers necessary for a swadge mode. If a mode
 * does not need a particular function, say it doesn't do audio handling, it
 * is safe to set the pointer to NULL. It just won't be called.
 */
typedef struct
{
    /**
     * This swadge mode's name, used in menus.
     * This is not a function pointer.
     */
    const char* modeName;

    /**
     * This is a setting, not a function pointer. Set it to one of these
     * values to have the system configure the swadge's WiFi
     *
     * NO_WIFI - Don't use WiFi at all. This saves power.
     * ESP_NOW - Send and receive packets to and from all swadges in range
     */
    wifiMode_t wifiMode;

    /**
     * If this is false, then the default TinyUSB driver will be installed (HID gamepad)
     * If this is true, then the swadge mode can do whatever it wants with USB
     */
    bool overrideUsb;

    /**
     * If this is false, the accelerometer will not be initialized and accelGetAccelVec() will not function
     * If this is true, then the swadge will be initialized
     */
    bool usesAccelerometer;

    /**
     * If this is false, the thermometer will not be initialized and readTemperatureSensor() will not function
     * If this is true, then the swadge will be initialized
     */
    bool usesThermometer;

    /**
     * This function is called when this mode is started. It should initialize
     * variables and start the mode.
     */
    void (*fnEnterMode)(void);

    /**
     * This function is called when the mode is exited. It should free any allocated memory.
     */
    void (*fnExitMode)(void);

    /**
     * This function is called from the main loop. It's pretty quick, but the
     * timing may be inconsistent.
     *
     * @param elapsedUs The time elapsed since the last time this function was called. Use this value to determine when
     * it's time to do things
     */
    void (*fnMainLoop)(int64_t elapsedUs);

    /**
     * This function is called whenever audio samples are read from the
     * microphone (ADC) and are ready for processing. Samples are read at 8KHz
     * This cannot be used at the same time as fnBatteryCallback
     *
     * @param samples A pointer to 12 bit audio samples
     * @param sampleCnt The number of samples read
     */
    void (*fnAudioCallback)(uint16_t* samples, uint32_t sampleCnt);

    /**
     * This function is called when the display driver wishes to update a
     * section of the display.
     *
     * @param disp The display to draw to
     * @param x the x coordiante that should be updated
     * @param y the x coordiante that should be updated
     * @param w the width of the rectangle to be updated
     * @param h the height of the rectangle to be updated
     * @param up update number
     * @param numUp update number denominator
     */
    void (*fnBackgroundDrawCallback)(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);

    /**
     * This function is called whenever an ESP-NOW packet is received.
     *
 * @param esp_now_info Information about the transmission, including The MAC addresses
     * @param data     A pointer to the data received
     * @param len      The length of the data received
     * @param rssi     The RSSI for this packet, from 1 (weak) to ~90 (touching)
     */
    void (*fnEspNowRecvCb)(const esp_now_recv_info_t * esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi);

    /**
     * This function is called whenever an ESP-NOW packet is sent.
     * It is just a status callback whether or not the packet was actually sent.
     * This will be called after calling espNowSend()
     *
     * @param mac_addr The MAC address which the data was sent to
     * @param status   The status of the transmission
     */
    void (*fnEspNowSendCb)(const uint8_t* mac_addr, esp_now_send_status_t status);

    /**
     * Advanced USB Functionality, for hooking existing advanced_usb interface.
     * if \c isGet == 1, that is a "get" or an "IN" endpoint, which means Swadge -> Host
     * if \c isGet == 0, that is a "set" or an "OUT" endpoint, where the Host sends the Swadge data.
     *
     * @param buffer Pointer to full command
     * @param length Total length of the buffer (command ID incldued)
     * @param isGet 0 if this is a \c SET_REPORT, 1 if this is a \c GET_REPORT
     * @return The number of bytes returned to the host
     */
    int16_t (*fnAdvancedUSB)(uint8_t* buffer, uint16_t length, uint8_t isGet);
} swadgeMode_t;

void deinitSystem(void);

#endif
