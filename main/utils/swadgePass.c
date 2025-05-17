#include <esp_log.h>
#include <hdw-nvs.h>
#include "swadgePass.h"
#include "modeIncludeList.h"

#define MAC_ADDR_LEN 6
#define MAC_STR_LEN  ((2 * MAC_ADDR_LEN) + 1)

#define SWADGE_PASS_PREAMBLE 0x5350 // 'SP' in ASCII
static const char NS_SP[] = "SP";

/**
 * @brief TODO
 *
 * @param macAddr
 * @param outStr
 * @param outStrLen
 */
static inline void macToStr(const uint8_t* macAddr, char* outStr, size_t outStrLen)
{
    snprintf(outStr, outStrLen, "%02X%02X%02X%02X%02X%02X", macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4],
             macAddr[5]);
}

/**
 * @brief TODO
 *
 * @param packet
 */
void sendSwadgePass(swadgePassPacket_t* packet)
{
    espNowSend((void*)packet, sizeof(swadgePassPacket_t));
}

/**
 * @brief TODO
 *
 * @param esp_now_info
 * @param data
 * @param len
 * @param rssi
 */
void receiveSwadgePass(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len,
                       int8_t rssi __attribute__((unused)))
{
    // Validate length
    if (len == sizeof(swadgePassPacket_t))
    {
        // Validate preamble
        const swadgePassPacket_t* packet = (const swadgePassPacket_t*)data;
        if (SWADGE_PASS_PREAMBLE == packet->preamble)
        {
            // Convert the incoming MAC to a key for NVS
            char macStr[MAC_STR_LEN];
            macToStr(esp_now_info->src_addr, macStr, sizeof(macStr));
            // Write the received data to NVS
            writeNamespaceNvsBlob(NS_SP, macStr, data, len);
        }
    }
}

/**
 * @brief TODO
 *
 */
void getSwadgePassKeys(list_t* keyList)
{
    // Get Keys
    getNvsKeys(NS_SP, keyList);
}

/**
 * @brief Fill a SwadgePass packet with data from all modes before transmission
 *
 * @param packet The packet to fill
 */
void fillSwadgePassPacket(swadgePassPacket_t* packet)
{
    // Set the preamble
    packet->preamble = SWADGE_PASS_PREAMBLE;

    // Ask each mode to fill in the rest
    int modeCount = modeListGetCount();
    for (int idx = 0; idx < modeCount; idx++)
    {
        if (allSwadgeModes[idx]->fnAddToSwadgePassPacket)
        {
            allSwadgeModes[idx]->fnAddToSwadgePassPacket(packet);
        }
    }
}
