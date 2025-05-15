#include <esp_log.h>
#include <hdw-nvs.h>
#include "swadgePass.h"

#define MAC_ADDR_LEN 6
#define MAC_STR_LEN  ((2 * MAC_ADDR_LEN) + 1)

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
 */
void sendSwadgePass(void)
{
    const char bcastData[16] = "BROADCAST";
    espNowSend(bcastData, sizeof(bcastData));
}

/**
 * @brief TODO
 *
 * @param esp_now_info
 * @param data
 * @param len
 * @param rssi
 */
void receiveSwadgePass(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi)
{
    char macStr[MAC_STR_LEN];
    macToStr(esp_now_info->src_addr, macStr, sizeof(macStr));
    writeNamespaceNvsBlob(NS_SP, macStr, data, len);
}

/**
 * @brief TODO
 *
 */
void listSwadgePass(void)
{
    // Get Keys
    list_t list = {0};
    getNvsKeys(NS_SP, &list);

    // Print keys
    node_t* keyNode = list.first;
    while (keyNode)
    {
        ESP_LOGI("KEY", "%s", (char*)keyNode->val);
        keyNode = keyNode->next;
    }

    // Free keys
    void* val;
    while ((val = pop(&list)))
    {
        heap_caps_free(val);
    }
}
