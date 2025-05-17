/*! \file swadgePass.h
 *
 * \section swadgePass_design Design Philosophy
 *
 * TODO
 *
 * \section swadgePass_usage Usage
 *
 * TODO
 *
 * \section swadgePass_example Example
 *
 * \code{.c}
 * TODO
 * \endcode
 */

#pragma once

#include <stdint.h>

#include "hdw-esp-now.h"
#include "linked_list.h"

typedef struct
{
    uint16_t preamble;
} swadgePassPacket_t;

void fillSwadgePassPacket(swadgePassPacket_t* packet);
void sendSwadgePass(swadgePassPacket_t* packet);
void receiveSwadgePass(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi);

void getSwadgePassKeys(list_t* keyList);
