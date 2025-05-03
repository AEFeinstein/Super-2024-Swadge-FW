/**
 * @file nameList.c
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief Provides index-based usernames that will avoid the issues inherent in free text entry.
 * @date 2025-05-02
 *
 * @copyright Copyright (c) 2025
 *
 */

//=============================================================================
// Includes
//==============================================================================

#include "nameList.h"
#include "macros.h"
#include <esp_wifi.h>
#include <esp_log.h>

//==============================================================================
// Consts
//==============================================================================

// TODO: Fill out lists with non-AI generated text
static const char* const nameList1[] = {"Large", "Small", "Obsequent", "Malignant"};
static const char* const nameList2[] = {"Poisonous", "Surreptitious", "Crazed", "Loltastic"};
static const char* const nameList3[] = {"Marmoset", "Kalina", "Spooder", "Shikikan"};

//==============================================================================
// Function Declarations
//==============================================================================

void generateMACName(nameStruct_t* ns, char* buffer, int buffLen)
{
    uint8_t baseMac[6];
    esp_err_t ret = esp_wifi_get_mac(WIFI_IF_STA, baseMac);
    if (ret == ESP_OK)
    {
        // NOTE: There's a large assumption that the vendor code is in the first bytes
        // May need to re-eval if this generates the same names over and over/some names more often than others.
        ns->nameIdxs[0] = baseMac[2] % ARRAY_SIZE(nameList1);
        ns->nameIdxs[1] = baseMac[3] % ARRAY_SIZE(nameList2);
        ns->nameIdxs[2] = baseMac[4] % ARRAY_SIZE(nameList3);
        ns->randCode    = baseMac[5];
    }
    else
    {
        ESP_LOGE("NGEN", "Failed to read MAC address");
        // Should be obvious something went wrong
        ns->nameIdxs[0] = 0;
        ns->nameIdxs[1] = 0;
        ns->nameIdxs[2] = 0;
        ns->randCode    = 0;
    }
    snprintf(buffer, buffLen - 1, "%s-%s-%s-%d", nameList1[ns->nameIdxs[0]], nameList2[ns->nameIdxs[1]],
             nameList3[ns->nameIdxs[2]], ns->randCode);
}

void getTextFromList(int listIdx, int idx, char* buffer, int buffLen)
{
    switch (listIdx)
    {
        case 0:
        {
            snprintf(buffer, buffLen - 1, "%s", nameList1[idx]);
            break;
        }
        case 1:
        {
            snprintf(buffer, buffLen - 1, "%s", nameList2[idx]);
            break;
        }
        case 2:
        {
            snprintf(buffer, buffLen - 1, "%s", nameList3[idx]);
            break;
        }
        default:
        {
            snprintf(buffer, buffLen - 1, "%s", "Bad request");
        }
    }
}