//==============================================================================
// Includes
//==============================================================================

#include <esp_log.h>
#include <hdw-nvs.h>
#include "swadgePass.h"
#include "modeIncludeList.h"

//==============================================================================
// Defines
//==============================================================================

#define MAC_ADDR_LEN 6
#define MAC_STR_LEN  ((2 * MAC_ADDR_LEN) + 1)

#define SWADGE_PASS_PREAMBLE 0x5350 // 'SP' in ASCII
#define SWADGE_PASS_VERSION  0      // Version 0 for 2026

//==============================================================================
// Const Variables
//==============================================================================

static const char NS_SP[] = "SP";

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Fill a SwadgePass packet with data from all modes before transmission
 *
 * @param packet The packet to fill
 */
void fillSwadgePassPacket(swadgePassPacket_t* packet)
{
    // Set the preamble
    packet->preamble = SWADGE_PASS_PREAMBLE;
    packet->version  = SWADGE_PASS_VERSION;

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

/**
 * @brief Transmit a SwadgePass packet
 *
 * @param packet The packet to transmit
 */
inline void sendSwadgePass(swadgePassPacket_t* packet)
{
    espNowSend((void*)packet, sizeof(swadgePassPacket_t));
}

/**
 * @brief Convert a six byte MAC address to a string so it can be used as an NVS key
 *
 * @param macAddr The MAC addres to convert
 * @param outStr The string to print into
 * @param outStrLen The length of the string to print into
 */
static inline void macToStr(const uint8_t* macAddr, char* outStr, size_t outStrLen)
{
    snprintf(outStr, outStrLen, "%02X%02X%02X%02X%02X%02X", macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4],
             macAddr[5]);
}

/**
 * @brief Receive an ESP NOW packet and save it if it is a SwadgePass packet
 *
 * @param esp_now_info Metadata for the packet, including src and dst MAC addresses
 * @param data The received data
 * @param len The length of the received data
 * @param rssi unused
 */
void receiveSwadgePass(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len,
                       int8_t rssi __attribute__((unused)))
{
    // Validate length
    if (len == sizeof(swadgePassPacket_t))
    {
        // Validate preamble and version
        const swadgePassPacket_t* packet = (const swadgePassPacket_t*)data;
        if (SWADGE_PASS_PREAMBLE == packet->preamble && SWADGE_PASS_VERSION == packet->version)
        {
            // Convert the incoming MAC to a key for NVS
            char macStr[MAC_STR_LEN];
            macToStr(esp_now_info->src_addr, macStr, sizeof(macStr));

            // Create the NVS data, which is the packet and a mask of if it was used by modes
            swadgePassNvs_t packetNvs;
            memcpy(&packetNvs.packet, data, sizeof(swadgePassPacket_t));

            // Clear the used mode mask because this is a new packet
            packetNvs.usedModeMask = 0;

            // Write the received data to NVS
            writeNamespaceNvsBlob(NS_SP, macStr, (void*)&packetNvs, sizeof(swadgePassNvs_t));
        }
    }
}

/**
 * @brief Fill a list with SwadgePass data. The list should be empty before calling this function.
 *
 * @param swadgePasses A list to fill with type \ref swadgePassData_t
 * @param mode The Swadge Mode getting SwadgePass data
 * @param getUsed true to return all SwadgePass data, false to return only unused SwadgePass data
 */
void getSwadgePasses(list_t* swadgePasses, const struct swadgeMode* mode, bool getUsed)
{
    // Get the NVS keys
    list_t keyList = {0};
    getNvsKeys(NS_SP, &keyList);

    // For each key
    node_t* keyNode = keyList.first;
    while (keyNode)
    {
        // Get the key
        const char* key = (const char*)keyNode->val;

        // Read the data from NVS
        size_t outLen         = sizeof(swadgePassNvs_t);
        swadgePassData_t* spd = heap_caps_calloc(1, sizeof(swadgePassData_t), MALLOC_CAP_SPIRAM);
        readNamespaceNvsBlob(NS_SP, key, &spd->data, &outLen);

        // Validate the length
        if (sizeof(swadgePassNvs_t) == outLen)
        {
            // Add to the list if either all data is requested or it hasn't been used yet
            if (getUsed || !isPacketUsedByMode(spd, mode))
            {
                // Add key to the data
                memcpy(spd->key, key, strlen(key));
                push(swadgePasses, spd);
            }
        }

        // If the data wasn't added to the list
        if ((NULL == swadgePasses->last) || (spd != swadgePasses->last->val))
        {
            // Free it
            heap_caps_free(spd);
        }

        // Iterate keys
        keyNode = keyNode->next;
    }

    // Free keys
    while (keyList.first)
    {
        heap_caps_free(pop(&keyList));
    }
}

/**
 * @brief Free SwadgePass data loaded with getSwadgePasses()
 *
 * @param swadgePasses A list of SwadgePasses to free. The list should contain \ref swadgePassData_t
 */
void freeSwadgePasses(list_t* swadgePasses)
{
    while (swadgePasses->last)
    {
        heap_caps_free(pop(swadgePasses));
    }
}

/**
 * @brief Return if a given mode has used this SwadgePass data yet
 *
 * @param data The SwadgePass data
 * @param mode The mode using the SwadgePass data
 * @return true if this data has been used by the given mode yet, false if it has not
 */
bool isPacketUsedByMode(swadgePassData_t* data, const struct swadgeMode* mode)
{
    int32_t modeIdx = getModeIdx(mode);
    return (1 << modeIdx) & data->data.usedModeMask;
}

/**
 * @brief Set if a given mode has used this SwadgePass data yet.
 *
 * This will write changes to NVS.
 *
 * @param data The SwadgePass data
 * @param mode The mode using the SwadgePass data
 * @param isUsed true if the data should be set as used, false if it should be set as unused
 */
void setPacketUsedByMode(swadgePassData_t* data, const struct swadgeMode* mode, bool isUsed)
{
    // If there is a change
    if (isPacketUsedByMode(data, mode) != isUsed)
    {
        // Make the change
        int32_t modeBit = (1 << getModeIdx(mode));
        if (isUsed)
        {
            data->data.usedModeMask |= modeBit;
        }
        else
        {
            data->data.usedModeMask &= ~modeBit;
        }

        // Write the change to NVS
        writeNamespaceNvsBlob(NS_SP, data->key, (void*)&data->data, sizeof(swadgePassNvs_t));
    }
}
