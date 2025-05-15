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
#include <hdw-esp-now.h>

void sendSwadgePass(void);
void receiveSwadgePass(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi);
void listSwadgePass(void);
