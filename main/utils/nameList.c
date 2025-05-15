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
#include <esp_random.h>

//==============================================================================
// Consts
//==============================================================================

static const char* const nameList1[] = {
    "outgoing", "fearless",     "debonair", "righteous", "psychological", "sparkling", "symptomatic", "quick",
    "zany",     "entertaining", "hot",      "noiseless", "dysfunctional", "glorious",  "scientific",  "accurate",
    "noxious",  "adaptable",    "sassy",    "elastic",   "known",         "loutish",   "suitable",    "rambunctious",
    "amusing",  "absurd",       "rhythmic", "modal",     "syncopated",    "sharp",     "auxilary",    "deafening",
    "plain",    "sudden",       "ordinary", "eminent",   "orchestral",    "speedy",    "lovely",      "zany",
};
static const char* const nameList2[] = {
    "wry",       "bright",      "educated",     "hulking",  "mammoth",     "lumpy",       "overwrought", "rabid",
    "faded",     "therapeutic", "rambunctious", "odd",      "cooperative", "psychotic",   "skinny",      "ruddy",
    "untidy",    "mysterious",  "gentle",       "shaggy",   "exciting",    "magnificent", "uppity",      "succinct",
    "voracious", "meek",        "smoggy",       "abnormal", "useful",      "macho",       "measly",      "truthful",
    "healthy",   "dazzling",    "nutty",        "round",    "magnanimous", "suspended",   "dissonant",   "consonant",
};
static const char* const nameList3[] = {
    "woodwind", "percussion", "harmony",  "percussion", "octave",    "cat",      "dog",      "weasel",
    "deer",     "seahorse",   "warthog",  "hamster",    "chameleon", "snake",    "lizard",   "pitch",
    "hedgehog", "ghost",      "string",   "noob",       "smurf",     "boss",     "aardvark", "lemon",
    "eagle",    "foxbat",     "seahawk",  "osprey",     "badger",    "drums",    "bass",     "wolf",
    "guitar",   "keyboard",   "vocalist", "developer",  "rocker",    "musician", "artist",   "controller",
};

//==============================================================================
// Function Declarations
//==============================================================================

void generateMACName(nameStruct_t* ns, char* outBuffer, int buffLen)
{
    uint8_t baseMac[6];
    esp_err_t ret = esp_wifi_get_mac(WIFI_IF_STA, baseMac);
    if (ret == ESP_OK)
    {
        // NOTE: There's a large assumption that the vendor code is in the first bytes
        // May need to re-eval if this generates the same names over and over/some names more often than others.
        ns->nameIdxs[LIST1] = baseMac[2] % ARRAY_SIZE(nameList1);
        ns->nameIdxs[LIST2] = baseMac[3] % ARRAY_SIZE(nameList2);
        ns->nameIdxs[LIST3] = baseMac[4] % ARRAY_SIZE(nameList3);
        ns->randCode        = baseMac[5];
    }
    else
    {
        ESP_LOGE("NGEN", "Failed to read MAC address");
        // Should be obvious something went wrong
        ns->nameIdxs[LIST1] = 0;
        ns->nameIdxs[LIST2] = 0;
        ns->nameIdxs[LIST3] = 0;
        ns->randCode        = 0;
    }
    snprintf(outBuffer, buffLen - 1, "%s-%s-%s-%" PRId16, nameList1[ns->nameIdxs[LIST1]],
             nameList2[ns->nameIdxs[LIST2]], nameList3[ns->nameIdxs[LIST3]], ns->randCode);
}

void generateRandName(nameStruct_t* ns, char* outBuffer, int buffLen)
{
    ns->nameIdxs[LIST1] = esp_random() % ARRAY_SIZE(nameList1);
    ns->nameIdxs[LIST2] = esp_random() % ARRAY_SIZE(nameList2);
    ns->nameIdxs[LIST3] = esp_random() % ARRAY_SIZE(nameList3);
    ns->randCode        = esp_random() % 256;
    snprintf(outBuffer, buffLen - 1, "%s-%s-%s-%" PRId16, nameList1[ns->nameIdxs[LIST1]],
             nameList2[ns->nameIdxs[LIST2]], nameList3[ns->nameIdxs[LIST3]], ns->randCode);
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

void getFullName(nameStruct_t name, char* outBuffer, int buffLen)
{
    char buff1[32], buff2[32], buff3[32];
    getTextFromList(LIST1, name.nameIdxs[LIST1], buff1, 32);
    getTextFromList(LIST2, name.nameIdxs[LIST2], buff2, 32);
    getTextFromList(LIST3, name.nameIdxs[LIST3], buff3, 32);
    snprintf(outBuffer, buffLen - 1, "%s-%s-%s-%" PRId16, buff1, buff2, buff3, name.randCode);
}