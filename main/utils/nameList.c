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

// Swadge
#include "nameList.h"
#include "macros.h"

// C
#include <string.h>

// ESP
#include <esp_wifi.h>
#include <esp_log.h>
#include <esp_random.h>

//=============================================================================
// Defines
//==============================================================================

#define USER_LIST_SHIFT 1

//==============================================================================
// Consts
//==============================================================================

static const char* const adjList1[] = {
    "outgoing", "fearless",     "debonair", "righteous", "psychological", "sparkling", "symptomatic", "quick",
    "zany",     "entertaining", "hot",      "noiseless", "dysfunctional", "glorious",  "scientific",  "accurate",
    "noxious",  "adaptable",    "sassy",    "elastic",   "known",         "loutish",   "suitable",    "rambunctious",
    "amusing",  "absurd",       "rhythmic", "modal",     "syncopated",    "sharp",     "auxilary",    "deafening",
    "plain",    "sudden",       "ordinary", "eminent",   "orchestral",    "speedy",    "lovely",      "zany",
};
static const char* const adjList2[] = {
    "wry",       "bright",      "educated",     "hulking",  "mammoth",     "lumpy",       "overwrought", "rabid",
    "faded",     "therapeutic", "rambunctious", "odd",      "cooperative", "psychotic",   "skinny",      "ruddy",
    "untidy",    "mysterious",  "gentle",       "shaggy",   "exciting",    "magnificent", "uppity",      "succinct",
    "voracious", "meek",        "smoggy",       "abnormal", "useful",      "macho",       "measly",      "truthful",
    "healthy",   "dazzling",    "nutty",        "round",    "magnanimous", "suspended",   "dissonant",   "consonant",
};
static const char* const nounList[] = {
    "woodwind", "driver",   "harmony",  "percussion", "octave",    "cat",      "dog",      "weasel",
    "deer",     "seahorse", "warthog",  "hamster",    "chameleon", "snake",    "lizard",   "pitch",
    "hedgehog", "ghost",    "string",   "noob",       "smurf",     "boss",     "aardvark", "lemon",
    "eagle",    "foxbat",   "seahawk",  "osprey",     "badger",    "drums",    "bass",     "wolf",
    "guitar",   "keyboard", "vocalist", "developer",  "rocker",    "musician", "artist",   "controller",
};

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    ADJ1,
    ADJ2,
    NOUN,
    RAND_NUM,
} listArrayIdx_t;

//==============================================================================
// Function Declarations
//==============================================================================

static void _getMacAddress(void);

static void _getWordFromList(int listIdx, int idx, char* buffer, int buffLen);

static uint8_t _mutateIdx(uint8_t arrSize, uint8_t idx, uint8_t mac);

//==============================================================================
// Variables
//==============================================================================

static uint8_t baseMac[6];
static uint8_t arrayLens[3];

//==============================================================================
// Functions
//==============================================================================

void initUsernameSystem()
{
    _getMacAddress();
    arrayLens[0] = ARRAY_SIZE(adjList1);
    arrayLens[1] = ARRAY_SIZE(adjList2);
    arrayLens[2] = ARRAY_SIZE(nounList);
}

void generateMACUsername(nameData_t* nd)
{
    nd->idxs[ADJ1] = baseMac[2 + ADJ1] % arrayLens[ADJ1]; // Functionally equivalent to _mutateIdx() w/ idx = 0
    nd->idxs[ADJ2] = baseMac[2 + ADJ2] % arrayLens[ADJ2];
    nd->idxs[NOUN] = baseMac[2 + NOUN] % arrayLens[NOUN];
    nd->randCode   = baseMac[5];
    snprintf(nd->nameBuffer, USERNAME_MAX_LEN - 1, "%s-%s-%s-%" PRId16, adjList1[nd->idxs[ADJ1]],
             adjList2[nd->idxs[ADJ2]], nounList[nd->idxs[NOUN]], nd->randCode);
}

void generateRandUsername(nameData_t* nd, bool user)
{
    if (user)
    {
        nd->idxs[ADJ1] = _mutateIdx(arrayLens[ADJ1], esp_random(), baseMac[2]);
        nd->idxs[ADJ2] = _mutateIdx(ARRAY_SIZE(adjList2), esp_random(), baseMac[3]);
        nd->idxs[NOUN] = _mutateIdx(ARRAY_SIZE(nounList), esp_random(), baseMac[4]);
        nd->randCode   = baseMac[5];
    }
    else
    {
        nd->idxs[ADJ1] = esp_random() % arrayLens[ADJ1];
        nd->idxs[ADJ2] = esp_random() % ARRAY_SIZE(adjList2);
        nd->idxs[NOUN] = esp_random() % ARRAY_SIZE(nounList);
        nd->randCode   = esp_random() % 256;
    }
    snprintf(nd->nameBuffer, USERNAME_MAX_LEN - 1, "%s-%s-%s-%" PRId16, adjList1[nd->idxs[ADJ1]],
             adjList2[nd->idxs[ADJ2]], nounList[nd->idxs[NOUN]], nd->randCode);
}

void setUsernameFromND(nameData_t* nd, bool user)
{
    if (user)
    {
        nd->idxs[ADJ1] = _mutateIdx(arrayLens[ADJ1], nd->idxs[ADJ1], baseMac[2]);
        nd->idxs[ADJ2] = _mutateIdx(ARRAY_SIZE(adjList2), nd->idxs[ADJ2], baseMac[3]);
        nd->idxs[NOUN] = _mutateIdx(ARRAY_SIZE(nounList), nd->idxs[NOUN], baseMac[4]);
        nd->randCode   = baseMac[5];
    }

    char buff1[MAX_ADJ1_LEN], buff2[MAX_ADJ2_LEN], buff3[MAX_NOUN_LEN];
    _getWordFromList(ADJ1, nd->idxs[ADJ1], buff1, MAX_ADJ1_LEN);
    _getWordFromList(ADJ2, nd->idxs[ADJ2], buff2, MAX_ADJ2_LEN);
    _getWordFromList(NOUN, nd->idxs[NOUN], buff3, MAX_NOUN_LEN);
    snprintf(nd->nameBuffer, USERNAME_MAX_LEN - 1, "%s-%s-%s-%" PRId16, buff1, buff2, buff3, nd->randCode);
}

void setUsernameFromIdxs(nameData_t* nd, int idx1, int idx2, int idx3, int randomCode, bool user)
{
    // setUsernameFromND() will convert these if user == true
    nd->idxs[ADJ1] = idx1;
    nd->idxs[ADJ2] = idx2;
    nd->idxs[NOUN] = idx3;
    nd->randCode   = randomCode;
    setUsernameFromND(nd, user);
}

bool handleUsernamePickerInput(buttonEvt_t* evt, nameData_t* nd, bool user)
{
    if (evt->down)
    {
        if (evt->button & PB_LEFT)
        {
            nd->arrayIdx--;
            if (nd->arrayIdx < ADJ1)
            {
                if (user) // Can't change last number
                {
                    nd->arrayIdx = NOUN;
                }
                else
                {
                    nd->arrayIdx = RAND_NUM;
                }
            }
        }
        else if (evt->button & PB_RIGHT)
        {
            nd->arrayIdx++;
            if (user)
            {
                if (nd->arrayIdx >= RAND_NUM) // Can't change last number
                {
                    nd->arrayIdx = ADJ1;
                }
            }
            else
            {
                if (nd->arrayIdx > RAND_NUM)
                {
                    nd->arrayIdx = ADJ1;
                }
            }
        }
        else if (evt->button & PB_UP)
        {
            if (nd->arrayIdx == RAND_NUM)
            {
                // Not set by user, overflow takes car of value
                nd->idxs[nd->arrayIdx]--;
            }
            else
            {
                if (user)
                {
                    nd->idxs[nd->arrayIdx]
                        = _mutateIdx(arrayLens[nd->arrayIdx], nd->idxs[nd->arrayIdx] - 1, baseMac[nd->arrayIdx + 2]);
                }
                else
                {
                    nd->idxs[nd->arrayIdx] = (nd->idxs[nd->arrayIdx] - 1) % arrayLens[nd->arrayIdx];
                }
            }
        }
        else if (evt->button & PB_DOWN)
        {
            if (nd->arrayIdx == RAND_NUM)
            {
                // Not set by user, overflow takes car of value
                nd->idxs[nd->arrayIdx]++;
            }
            else
            {
                if (user)
                {
                    nd->idxs[nd->arrayIdx]
                        = _mutateIdx(arrayLens[nd->arrayIdx], nd->idxs[nd->arrayIdx] + 1, baseMac[nd->arrayIdx + 2]);
                }
                else
                {
                    nd->idxs[nd->arrayIdx] = (nd->idxs[nd->arrayIdx] + 1) % arrayLens[nd->arrayIdx];
                }
            }
        }
        else if (evt->button & PB_A)
        {
            setUsernameFromND(nd, user);
            return true;
        }
    }
    return false;
}

void drawUsernamePicker(bool user)
{
}

//==============================================================================
// Static Functions
//==============================================================================

static void _getMacAddress()
{
    esp_err_t ret = esp_wifi_get_mac(WIFI_IF_STA, baseMac);
    if (ret != ESP_OK)
    {
        ESP_LOGE("USRN", "Failed to read MAC address");
        for (int idx = 0; idx < 6; idx++)
        {
            // Produces an obvious, statistically unlikely result
            baseMac[idx] = 0;
        }
    }
    ESP_LOGI("USRN", "MAC:%d:%d:%d:%d:%d:%d", baseMac[0], baseMac[1], baseMac[2], baseMac[3], baseMac[4], baseMac[5]);
}

static void _getWordFromList(int listIdx, int idx, char* buffer, int buffLen)
{
    switch (listIdx)
    {
        case ADJ1:
        {
            snprintf(buffer, buffLen - 1, "%s", adjList1[idx]);
            break;
        }
        case ADJ2:
        {
            snprintf(buffer, buffLen - 1, "%s", adjList2[idx]);
            break;
        }
        case NOUN:
        {
            snprintf(buffer, buffLen - 1, "%s", nounList[idx]);
            break;
        }
        default:
        {
            snprintf(buffer, buffLen - 1, "%s", "Bad request");
        }
    }
}

static uint8_t _mutateIdx(uint8_t arrSize, uint8_t idx, uint8_t mac)
{
    return (mac + (idx % (arrSize >> USER_LIST_SHIFT))) % arrSize;
}