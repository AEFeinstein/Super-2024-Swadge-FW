//==============================================================================
// Includes
//==============================================================================

#include <esp_log.h>
#include <hdw-nvs.h>
#include "swadgePass.h"
#include "modeIncludeList.h"
#include "nameList.h"

//==============================================================================
// Defines
//==============================================================================

#define MAC_ADDR_LEN 6
#define MAC_STR_LEN  ((2 * MAC_ADDR_LEN) + 1)

#define SWADGE_PASS_PREAMBLE 0x5350 // 'SP' in ASCII
#define SWADGE_PASS_VERSION  0      // Version 0 for 2026

#define MAX_NUM_SWADGE_PASSES 100

//==============================================================================
// Const Variables
//==============================================================================

static const char NS_SP[] = "SP";

//==============================================================================
// Variables
//==============================================================================

static list_t rxSwadgePasses = {0};
static bool swadgePassRxInit = false;

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

    // Automatically fill in username
    nameData_t copied = *getSystemUsername(); // Would inline, but macro says no
    packet->username  = GET_PACKED_USERNAME(copied);

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
 * @brief Initialize the SwadgePass receiver. This reads SwadgePass data from NVS to SPIRAM so that each reception
 * doesn't require a bunch of NVS reads
 */
void initSwadgePassReceiver(void)
{
    if (false == swadgePassRxInit)
    {
        getSwadgePasses(&rxSwadgePasses, NULL, true);
        swadgePassRxInit = true;
    }
}

/**
 * @brief Deinitialize the SwadgePass receiver. This frees memory
 */
void deinitSwadgePassReceiver(void)
{
    if (true == swadgePassRxInit)
    {
        swadgePassRxInit = false;
        freeSwadgePasses(&rxSwadgePasses);
    }
}

/**
 * @brief Receive an ESP NOW packet and save it if it is a SwadgePass packet
 *
 * This limits the number of received SwadgePasses to ::MAX_NUM_SWADGE_PASSES. If a SwadgePass is received while at
 * capacity, the most used SwadgePass will be deleted first, as determined by the most number of bits set in
 * swadgePassNvs_t.usedModeMask.
 *
 * If a SwadgePass is received from a Swadge for which there already is data, the old data will be overwritten if it's
 * different and the swadgePassNvs_t.usedModeMask will always be cleared.
 *
 * @param esp_now_info Metadata for the packet, including src and dst MAC addresses
 * @param data The received data
 * @param len The length of the received data
 * @param rssi unused
 */
void receiveSwadgePass(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len,
                       int8_t rssi __attribute__((unused)))
{
    // Make sure it's initialized first
    if (false == swadgePassRxInit)
    {
        // Not initialized, so ignore this packet
        return;
    }

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

            // Variables to find the most used data
            int maxBitsUsed      = 0;
            node_t* mostUsedNode = rxSwadgePasses.first;

            // Iterate through the known SwadgePass data
            node_t* spNode = rxSwadgePasses.first;
            while (spNode)
            {
                // Convenience pointer
                swadgePassData_t* spd = (swadgePassData_t*)spNode->val;

                // If the received key matches a key in the local list
                if (0 == strcmp(spd->key, macStr))
                {
                    // SwadgePass data already exists, so check if it's different or if it's been used
                    if (0 != memcmp(&spd->data.packet, data, sizeof(swadgePassPacket_t)) || //
                        0 != spd->data.usedModeMask)
                    {
                        // Packet is different, copy into local list
                        memcpy(&spd->data.packet, data, sizeof(swadgePassPacket_t));

                        // Clear the used mode mask too
                        spd->data.usedModeMask = 0;

                        // Write to NVS. This will overwrite the old entry
                        writeNamespaceNvsBlob(NS_SP, spd->key, &spd->data, sizeof(swadgePassNvs_t));
                    }

                    // Return here because the data is already in the local list and NVS
                    return;
                }
                else
                {
                    // Count the number of bits set in this data's usedModeMask
                    int bitsUsed = __builtin_popcount(spd->data.usedModeMask);

                    // If this data has more bits set (i.e. more used)
                    if (bitsUsed > maxBitsUsed)
                    {
                        // Save it for potential removal later
                        maxBitsUsed  = bitsUsed;
                        mostUsedNode = spNode;
                    }
                }

                // Iterate to the next node
                spNode = spNode->next;
            }

            // Made it this far, which means the data isn't in the local list or NVS.
            // Check if the local list is at capacity first
            if (MAX_NUM_SWADGE_PASSES == rxSwadgePasses.length)
            {
                // If the local list is at capacity, but no data has been used yet,
                // don't delete, don't add.
                if (0 == maxBitsUsed)
                {
                    // The user must use their data before collecting new ones, so return here
                    return;
                }

                // Remove from the local list
                swadgePassData_t* removedVal = removeEntry(&rxSwadgePasses, mostUsedNode);

                // Remove from NVS
                eraseNamespaceNvsKey(NS_SP, removedVal->key);

                // Free from memory
                heap_caps_free(removedVal);
            }

            // By here, we know that the SwadgePass data isn't in the local list, and there's room for it.
            // add it to NVS and the local list

            // Allocate for storage for the local list
            swadgePassData_t* newSpd = heap_caps_calloc(1, sizeof(swadgePassData_t), MALLOC_CAP_SPIRAM);
            memcpy(newSpd->key, macStr, sizeof(macStr));
            newSpd->data.usedModeMask = 0;
            memcpy(&newSpd->data.packet, data, sizeof(swadgePassPacket_t));

            // Push into the local local list
            push(&rxSwadgePasses, newSpd);

            // Write the received data to NVS
            writeNamespaceNvsBlob(NS_SP, newSpd->key, (void*)&newSpd->data, sizeof(swadgePassNvs_t));
        }
    }
}

/**
 * @brief Fill a list with SwadgePass data. The list should be empty before calling this function.
 *
 * @param swadgePasses A list to fill with type ::swadgePassData_t
 * @param mode The Swadge Mode getting SwadgePass data (may be NULL)
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
            if (getUsed || (mode && false == isPacketUsedByMode(spd, mode)))
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
 * @param swadgePasses A list of SwadgePasses to free. The list should contain ::swadgePassData_t
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