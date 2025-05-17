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
#include "swadge2024.h"

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
    "suitable", "eminent",  "shiny",    "modal",    "playful",  "fun",      "flexible", "wiggly",
    "live",     "rhythmic", "amusing",  "fearless", "hot",      "debonair", "quirky",   "pleasant",
    "sick",     "lovely",   "sassy",    "logical",  "smart",    "speedy",   "silent",   "correct",
    "quick",    "elastic",  "accurate", "outgoing", "ordinary", "plain",    "loud",     "zany",
    "festive",  "loutish",  "noxious",  "sly",      "known",    "sudden",   "sharp",    "glorious",
};
static const char* const adjList2[] = {
    "lazy",    "weepy",    "creative", "faded",    "shaggy",  "truthful", "dazzling", "measly", "absurd", "healthy",
    "mammoth", "generous", "educated", "scraggly", "upset",   "cheeky",   "abnormal", "gentle", "useful", "ruddy",
    "gentle",  "smoggy",   "lumpy",    "odd",      "helpful", "round",    "succinct", "weird",  "rabid",  "steady",
    "untidy",  "bright",   "macho",    "nutty",    "hulking", "tuned",    "exciting", "wry",    "meek",   "hungry",
};
static const char* const nounList[] = {
    "lizard",   "hedgehog", "vocalist", "smurf",   "noob",     "bass",    "boss",     "badger", "drums",   "keyboard",
    "pitch",    "octave",   "aardvark", "warthog", "cat",      "gamepad", "lemon",    "dog",    "harmony", "ghost",
    "deer",     "guitar",   "gecko",    "seahawk", "musician", "osprey",  "hamster",  "string", "weasel",  "gamedev",
    "woodwind", "drummer",  "rocker",   "wolf",    "snake",    "driver",  "seahorse", "artist", "foxbat",  "eagle",
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

typedef struct __attribute__((packed))
{
    uint8_t baseMac[6];
    uint8_t adj1 : 6; // MAC bits 18-23
    uint8_t adj2 : 6; // MAC bits 12-17
    uint8_t noun : 6; // MAC bits 6-11
    uint8_t code : 6; // MAC bits 0-5
} macBits_t;

//==============================================================================
// Function Declarations
//==============================================================================

static void _getMacAddress(void);

static void _getWordFromList(int listIdx, int idx, char* buffer, int buffLen);

static uint8_t _mutateIdx(uint8_t arrSize, uint8_t idx, uint8_t mac);

// Drawing functions

static void _drawWordCenteredOnX(int x, int y, const char* str, paletteColor_t col);

//==============================================================================
// Variables
//==============================================================================

static macBits_t macBits;
static uint8_t arrayLens[3];
static font_t fnt;

//==============================================================================
// Functions
//==============================================================================

void initUsernameSystem()
{
    _getMacAddress();
    arrayLens[0] = ARRAY_SIZE(adjList1);
    arrayLens[1] = ARRAY_SIZE(adjList2);
    arrayLens[2] = ARRAY_SIZE(nounList);
    loadFont(IBM_VGA_8_FONT, &fnt, true);
}

void generateMACUsername(nameData_t* nd)
{
    nd->idxs[ADJ1] = macBits.adj1 % arrayLens[ADJ1]; // Functionally equivalent to _mutateIdx() w/ idx = 0
    nd->idxs[ADJ2] = macBits.adj2 % arrayLens[ADJ2];
    nd->idxs[NOUN] = macBits.noun % arrayLens[NOUN];
    nd->randCode   = macBits.baseMac[5];
    snprintf(nd->nameBuffer, USERNAME_MAX_LEN - 1, "%s-%s-%s-%" PRId16, adjList1[nd->idxs[ADJ1]],
             adjList2[nd->idxs[ADJ2]], nounList[nd->idxs[NOUN]], nd->randCode);
}

void generateRandUsername(nameData_t* nd, bool user)
{
    if (user)
    {
        nd->idxs[ADJ1] = _mutateIdx(arrayLens[ADJ1], esp_random(), macBits.adj1);
        nd->idxs[ADJ2] = _mutateIdx(ARRAY_SIZE(adjList2), esp_random(), macBits.adj2);
        nd->idxs[NOUN] = _mutateIdx(ARRAY_SIZE(nounList), esp_random(), macBits.noun);
        nd->randCode   = macBits.baseMac[5];
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
        nd->idxs[ADJ1] = _mutateIdx(arrayLens[ADJ1], nd->idxs[ADJ1], macBits.adj1);
        nd->idxs[ADJ2] = _mutateIdx(ARRAY_SIZE(adjList2), nd->idxs[ADJ2], macBits.adj2);
        nd->idxs[NOUN] = _mutateIdx(ARRAY_SIZE(nounList), nd->idxs[NOUN], macBits.noun);
        nd->randCode   = macBits.baseMac[5];
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
                // Not set by user, overflow takes care of value
                nd->randCode--;
            }
            else
            {
                if (user)
                {
                    nd->idxs[nd->arrayIdx] = _mutateIdx(arrayLens[nd->arrayIdx], nd->idxs[nd->arrayIdx],
                                                        macBits.baseMac[nd->arrayIdx + 2]);
                }
                else
                {
                    nd->idxs[nd->arrayIdx] = (nd->idxs[nd->arrayIdx]) % arrayLens[nd->arrayIdx];
                }
            }
        }
        else if (evt->button & PB_DOWN)
        {
            if (nd->arrayIdx == RAND_NUM)
            {
                // Not set by user, overflow takes care of value
                nd->randCode++;
            }
            else
            {
                if (user)
                {
                    nd->idxs[nd->arrayIdx] = _mutateIdx(arrayLens[nd->arrayIdx], nd->idxs[nd->arrayIdx] + 1,
                                                        macBits.baseMac[nd->arrayIdx + 2]);
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

void drawUsernamePicker(nameData_t* nd, bool user)
{
    // Clear display
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);

    // Draw current selection
    _drawWordCenteredOnX(50, 0, adjList1[nd->idxs[ADJ1]], c544);
    _drawWordCenteredOnX(120, 0, adjList2[nd->idxs[ADJ2]], c454);
    _drawWordCenteredOnX(190, 0, nounList[nd->idxs[NOUN]], c445);
    char buffer[32];
    snprintf(buffer, sizeof(buffer) - 1, "%" PRIu8, nd->randCode);
    _drawWordCenteredOnX(240, 0, buffer, user ? c444 : c555);

    // draw tack marks

    // Draw other options above and below

    // draw selection box

    // Draw currently set name
    drawText(&fnt, c444, nd->nameBuffer, 16, TFT_HEIGHT - 48);
}

//==============================================================================
// Static Functions
//==============================================================================

static void _getMacAddress()
{
    esp_err_t ret = esp_wifi_get_mac(WIFI_IF_STA, macBits.baseMac);
    if (ret != ESP_OK)
    {
        ESP_LOGE("USRN", "Failed to read MAC address");
        for (int idx = 0; idx < 6; idx++)
        {
            // Produces an obvious, statistically unlikely result
            macBits.baseMac[idx] = 0;
        }
    }
    ESP_LOGI("USRN", "MAC:%d:%d:%d:%d:%d:%d", macBits.baseMac[0], macBits.baseMac[1], macBits.baseMac[2],
             macBits.baseMac[3], macBits.baseMac[4], macBits.baseMac[5]);

    // load bits into packed data
    // TODO: Add bit shuffling to increase entropy
    macBits.adj1 = (macBits.baseMac[3] & 0xFC) >> 2; // 6 bits, 23-18
    macBits.adj2 = ((macBits.baseMac[3] & 0x03) << 4) | ((macBits.baseMac[4] & 0xF0) >> 2);
    macBits.noun = ((macBits.baseMac[4] & 0x0F) << 4) | ((macBits.baseMac[5] & 0xC0) >> 2);
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

// Drawing functions

static void _drawWordCenteredOnX(int xCenter, int yOffset, const char* str, paletteColor_t col)
{
    int xCoord = xCenter - (textWidth(&fnt, str) >> 1);
    int yCoord = ((TFT_HEIGHT - fnt.height) >> 1) + yOffset;
    drawText(&fnt, col, str, xCoord, yCoord);
}