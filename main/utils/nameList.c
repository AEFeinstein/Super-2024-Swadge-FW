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

#define USER_LIST_SHIFT 2

// Screen arrangement
#define ADJ1_X      47
#define WORDSPACING 72
#define ADJ2_X      (ADJ1_X + WORDSPACING)
#define NOUN_X      (ADJ2_X + WORDSPACING)
#define WORD_OFFSET 20

// Color
// All colors go down index every 43.
// For instance, c454 - 43 = c343
#define COLOR_OFFSET 43

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

//==============================================================================
// Function Declarations
//==============================================================================

static void _getMacAddress(void);

static void _getWordFromList(int listIdx, int idx, char* buffer, int buffLen);

static uint8_t _mutateIdx(uint8_t arrSize, uint8_t idx, uint8_t seed);

// Drawing functions

static void _drawWordCenteredOnX(int x, int y, const char* str, paletteColor_t col);

static void _drawFadingWords(nameData_t* nd);

//==============================================================================
// Variables
//==============================================================================

static uint8_t baseMac[6];
static int listLen[3];
static uint8_t mutatorSeeds[3];
static font_t fnt;

//==============================================================================
// Functions
//==============================================================================

void initUsernameSystem()
{
    _getMacAddress();
    listLen[0] = ARRAY_SIZE(adjList1);
    listLen[1] = ARRAY_SIZE(adjList2);
    listLen[2] = ARRAY_SIZE(nounList);
    loadFont(IBM_VGA_8_FONT, &fnt, true);
}

void generateMACUsername(nameData_t* nd)
{
    nd->idxs[ADJ1] = mutatorSeeds[0] % listLen[ADJ1]; // Functionally equivalent to _mutateIdx() w/ idx = 0
    nd->idxs[ADJ2] = mutatorSeeds[1] % listLen[ADJ2];
    nd->idxs[NOUN] = mutatorSeeds[2] % listLen[NOUN];
    nd->randCode   = baseMac[5];
    snprintf(nd->nameBuffer, USERNAME_MAX_LEN - 1, "%s-%s-%s-%" PRId16, adjList1[nd->idxs[ADJ1]],
             adjList2[nd->idxs[ADJ2]], nounList[nd->idxs[NOUN]], nd->randCode);
}

void generateRandUsername(nameData_t* nd)
{
    if (nd->user)
    {
        nd->idxs[ADJ1] = _mutateIdx(listLen[ADJ1], esp_random(), mutatorSeeds[ADJ1]);
        nd->idxs[ADJ2] = _mutateIdx(listLen[ADJ2], esp_random(), mutatorSeeds[ADJ2]);
        nd->idxs[NOUN] = _mutateIdx(listLen[NOUN], esp_random(), mutatorSeeds[NOUN]);
        nd->randCode   = baseMac[5];
    }
    else
    {
        nd->idxs[ADJ1] = esp_random() % listLen[ADJ1];
        nd->idxs[ADJ2] = esp_random() % listLen[ADJ2];
        nd->idxs[NOUN] = esp_random() % listLen[NOUN];
        nd->randCode   = esp_random() % 256;
    }
    snprintf(nd->nameBuffer, USERNAME_MAX_LEN - 1, "%s-%s-%s-%" PRId16, adjList1[nd->idxs[ADJ1]],
             adjList2[nd->idxs[ADJ2]], nounList[nd->idxs[NOUN]], nd->randCode);
}

void setUsernameFromND(nameData_t* nd)
{
    if (nd->user)
    {
        nd->idxs[ADJ1] = _mutateIdx(listLen[ADJ1], nd->idxs[ADJ1], mutatorSeeds[ADJ1]);
        nd->idxs[ADJ2] = _mutateIdx(listLen[ADJ2], nd->idxs[ADJ2], mutatorSeeds[ADJ2]);
        nd->idxs[NOUN] = _mutateIdx(listLen[NOUN], nd->idxs[NOUN], mutatorSeeds[NOUN]);
        nd->randCode   = baseMac[5];
    }

    char buff1[MAX_ADJ1_LEN], buff2[MAX_ADJ2_LEN], buff3[MAX_NOUN_LEN];
    _getWordFromList(ADJ1, nd->idxs[ADJ1], buff1, MAX_ADJ1_LEN);
    _getWordFromList(ADJ2, nd->idxs[ADJ2], buff2, MAX_ADJ2_LEN);
    _getWordFromList(NOUN, nd->idxs[NOUN], buff3, MAX_NOUN_LEN);
    snprintf(nd->nameBuffer, USERNAME_MAX_LEN - 1, "%s-%s-%s-%" PRId16, buff1, buff2, buff3, nd->randCode);
}

void setUsernameFromIdxs(nameData_t* nd, int idx1, int idx2, int idx3, int randomCode)
{
    // setUsernameFromND() will convert these if user == true
    nd->idxs[ADJ1] = idx1;
    nd->idxs[ADJ2] = idx2;
    nd->idxs[NOUN] = idx3;
    nd->randCode   = randomCode;
    setUsernameFromND(nd);
}

bool handleUsernamePickerInput(buttonEvt_t* evt, nameData_t* nd)
{
    if (evt->down)
    {
        if (evt->button & PB_LEFT)
        {
            nd->arrayIdx--;
            if (nd->arrayIdx < ADJ1)
            {
                if (nd->user) // Can't change last number
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
            if (nd->user)
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
        else if (evt->button & PB_UP) // FIXME: Allow holding down after a pause
        {
            if (nd->arrayIdx == RAND_NUM)
            {
                nd->randCode--;
            }
            else
            {
                nd->idxs[nd->arrayIdx]--;
                if (nd->user)
                {
                    if (nd->idxs[nd->arrayIdx] < _mutateIdx(listLen[nd->arrayIdx], 0, mutatorSeeds[nd->arrayIdx]))
                    {
                        nd->idxs[nd->arrayIdx]
                            = _mutateIdx(listLen[nd->arrayIdx], listLen[nd->arrayIdx] - 1, mutatorSeeds[nd->arrayIdx]);
                    }
                }
                else
                {
                    if (nd->idxs[nd->arrayIdx] < 0)
                    {
                        nd->idxs[nd->arrayIdx] = listLen[nd->arrayIdx] - 1;
                    }
                }
            }
        }
        else if (evt->button & PB_DOWN)
        {
            if (nd->arrayIdx == RAND_NUM)
            {
                nd->randCode++;
            }
            else
            {
                nd->idxs[nd->arrayIdx]++;
                if (nd->user)
                {
                    if (nd->idxs[nd->arrayIdx]
                        > _mutateIdx(listLen[nd->arrayIdx], listLen[nd->arrayIdx] - 1, mutatorSeeds[nd->arrayIdx]))
                    {
                        nd->idxs[nd->arrayIdx] = _mutateIdx(listLen[nd->arrayIdx], 0, mutatorSeeds[nd->arrayIdx]);
                    }
                }
                else
                {
                    if (nd->idxs[nd->arrayIdx] >= listLen[nd->arrayIdx])
                    {
                        nd->idxs[nd->arrayIdx] = 0;
                    }
                }
            }
        }
        else if (evt->button & PB_A)
        {
            setUsernameFromND(nd);
            return true;
        }
    }
    return false;
}

void drawUsernamePicker(nameData_t* nd)
{
    // Clear display
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);

    // Draw current selection
    _drawWordCenteredOnX(ADJ1_X, 0, adjList1[nd->idxs[ADJ1]], c544);
    _drawWordCenteredOnX(ADJ2_X, 0, adjList2[nd->idxs[ADJ2]], c454);
    _drawWordCenteredOnX(NOUN_X, 0, nounList[nd->idxs[NOUN]], c445);
    char buffer[32];
    snprintf(buffer, sizeof(buffer) - 1, "%" PRIu8, nd->randCode);
    _drawWordCenteredOnX(245, 0, buffer, nd->user ? c444 : c555);

    // draw tack marks
    _drawWordCenteredOnX(83, 0, "-", c555); // Numbers determined experimentally
    _drawWordCenteredOnX(155, 0, "-", c555);
    _drawWordCenteredOnX(227, 0, "-", c555);

    // Draw other options above and below
    _drawFadingWords(nd);

    // draw selection line
    switch (nd->arrayIdx)
    {
        case ADJ1:
        {
            drawLine(ADJ1_X - 35, (TFT_HEIGHT + fnt.height + 4) >> 1, ADJ1_X + 35, (TFT_HEIGHT + fnt.height + 4) >> 1,
                     c550, 0);
            break;
        }
        case ADJ2:
        {
            drawLine(ADJ2_X - 35, (TFT_HEIGHT + fnt.height + 4) >> 1, ADJ2_X + 35, (TFT_HEIGHT + fnt.height + 4) >> 1,
                     c550, 0);
            break;
        }
        case NOUN:
        {
            drawLine(NOUN_X - 35, (TFT_HEIGHT + fnt.height + 4) >> 1, NOUN_X + 35, (TFT_HEIGHT + fnt.height + 4) >> 1,
                     c550, 0);
            break;
        }
        case RAND_NUM:
        {
            drawLine(235, (TFT_HEIGHT + fnt.height + 4) >> 1, 255, (TFT_HEIGHT + fnt.height + 4) >> 1, c550, 0);
            break;
        }
        default:
        {
            break;
        }
    }

    // Draw currently set name
    drawText(&fnt, c444, "Current:", 32, TFT_HEIGHT - 32);
    drawText(&fnt, c555, nd->nameBuffer, 32, TFT_HEIGHT - 16);
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

    // Generate seeds
    mutatorSeeds[NOUN] = baseMac[4] ^ baseMac[5];
    mutatorSeeds[ADJ2] = baseMac[3] ^ baseMac[5];
    mutatorSeeds[ADJ1] = baseMac[3] ^ mutatorSeeds[NOUN];
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

static uint8_t _mutateIdx(uint8_t arrSize, uint8_t idx, uint8_t seed)
{
    return (seed + (idx % (arrSize >> USER_LIST_SHIFT))) % arrSize;
}

// Drawing functions

static void _drawWordCenteredOnX(int xCenter, int yOffset, const char* str, paletteColor_t col)
{
    int xCoord = xCenter - (textWidth(&fnt, str) >> 1);
    int yCoord = ((TFT_HEIGHT - fnt.height) >> 1) + yOffset;
    drawText(&fnt, col, str, xCoord, yCoord);
}

static void _drawFadingWords(nameData_t* nd)
{
    for (int listId = 0; listId < 3; listId++)
    {
        const char* const* curr;
        paletteColor_t color;
        switch (listId)
        {
            case ADJ1:
            {
                curr  = adjList1;
                color = c544;
                break;
            }
            case ADJ2:
            {
                curr  = adjList2;
                color = c454;
                break;
            }
            case NOUN:
            {
                curr  = nounList;
                color = c445;
                break;
            }
            default:
            {
                break;
            }
        }
        for (int offset = 1; offset < 4; offset++)
        {
            if (nd->user)
            {
                _drawWordCenteredOnX(
                    ADJ1_X + listId * WORDSPACING, -WORD_OFFSET * offset,
                    curr[_mutateIdx(listLen[listId], nd->idxs[listId] - offset + listLen[listId], mutatorSeeds[listId])],
                    c544 - offset * COLOR_OFFSET);
                _drawWordCenteredOnX(ADJ1_X + listId * WORDSPACING, WORD_OFFSET * offset,
                                     curr[_mutateIdx(listLen[listId], nd->idxs[listId] + offset, mutatorSeeds[listId])],
                                     c544 - offset * COLOR_OFFSET);
            }
            else
            {
                _drawWordCenteredOnX(ADJ1_X + listId * WORDSPACING, -WORD_OFFSET * offset,
                                     curr[(nd->idxs[listId] - offset + listLen[listId]) % listLen[listId]],
                                     color - offset * COLOR_OFFSET);
                _drawWordCenteredOnX(ADJ1_X + listId * WORDSPACING, WORD_OFFSET * offset,
                                     curr[(nd->idxs[listId] + offset) % listLen[listId]],
                                     color - offset * COLOR_OFFSET);
            }
        }
    }
}