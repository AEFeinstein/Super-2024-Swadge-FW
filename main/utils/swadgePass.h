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

//==============================================================================
// Includes
//==============================================================================

#include "swadge2024.h"

//==============================================================================
// Structs
//==============================================================================

/**
 * @brief A SwadgePass packet which is transmitted over ESP NOW
 */
typedef struct swadgePassPacket
{
    uint16_t preamble; ///< Sixteen bits that specifically begin a SwadgePass packet
} swadgePassPacket_t;

/**
 * @brief SwadgePass data that is saved to NVS.
 */
typedef struct
{
    uint32_t usedModeMask;     ///< A bitmask indicating if a mode has used this data
    swadgePassPacket_t packet; ///< The received SwadgePass packet
} swadgePassNvs_t;

/**
 * @brief SwadgePass data that is received from another Swadge
 */
typedef struct
{
    char key[NVS_KEY_NAME_MAX_SIZE]; ///< A string representation of the other Swadge's MAC address
    swadgePassNvs_t data;            ///< The SwadgePass data stored in this Swadge's NVS
} swadgePassData_t;

//==============================================================================
// Forward declaration
//==============================================================================

struct swadgeMode;

//==============================================================================
// Function Declarations
//==============================================================================

void fillSwadgePassPacket(swadgePassPacket_t* packet);
void sendSwadgePass(swadgePassPacket_t* packet);
void receiveSwadgePass(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi);

void getSwadgePasses(list_t* swadgePasses, const struct swadgeMode* mode, bool getUsed);
void freeSwadgePasses(list_t* swadgePasses);

bool isPacketUsedByMode(swadgePassData_t* data, const struct swadgeMode* mode);
void setPacketUsedByMode(swadgePassData_t* data, const struct swadgeMode* mode, bool isUsed);
