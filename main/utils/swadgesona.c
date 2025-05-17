/**
 * @file swadgesona.c
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief The data structures and Helper functions for utilizing Swadgesonas
 * @date 2025-05-02
 *
 * @copyright Copyright (c) 2025
 *
 */

//==============================================================================
// Includes
//==============================================================================
#include "swadgesona.h"
#include "esp_random.h"
#include "hdw-nvs.h"
#include <stdio.h>
#include <inttypes.h>
#include <esp_log.h>

//==============================================================================
// Defines
//==============================================================================
#define NVS_KEY_LEN 15

//==============================================================================
// Consts
//==============================================================================

static const char* const nvsStr[] = {"swadgesona", "swadgesona-"};

//==============================================================================
// Functions
//==============================================================================

// Data
void saveSwadgesona(swadgesona_t* s, int idx)
{
    // Generate NVS key
    char nvsTag[NVS_KEY_LEN];
    snprintf(nvsTag, NVS_KEY_LEN - 1, "%s%" PRIu8, nvsStr[1], idx);
    if (!writeNamespaceNvsBlob(nvsStr[0], nvsTag, s, sizeof(s)))
    {
        ESP_LOGI("SONA", "Swadgesona failed to save");
    }
}

void loadSwadgesona(swadgesona_t* s, int idx)
{
    char nvsTag[NVS_KEY_LEN];
    size_t len = 0;
    snprintf(nvsTag, NVS_KEY_LEN - 1, "%s%" PRIu8, nvsStr[1], idx);
    readNamespaceNvsBlob(nvsStr[0], nvsTag, s, &len);
    if (!readNamespaceNvsBlob(nvsStr[0], nvsTag, s, &len))
    {
        ESP_LOGI("SONA", "Swadgesona failed to Load/does not exist");
        generateRandomSwadgesona(s);
    }
}

void generateRandomSwadgesona(swadgesona_t* s)
{
    s->bodyMarks    = esp_random() % BODY_MARKS_COUNT;
    s->clothes      = esp_random() % CLOTHES_OPTION_COUNT;
    s->clothesColor = esp_random() % cTransparent;
    s->earShape     = esp_random() % EAR_COUNT;
    s->eyebrows     = esp_random() % EYEBROW_COUNT;
    s->eyeColor     = esp_random() % cTransparent;
    s->eyeShape     = esp_random() % EYE_SHAPE_COUNT;
    s->hairColor    = esp_random() % HAIR_COLOR_COUNT;
    s->hairStyle    = esp_random() % HAIR_COUNT;
    s->mouthShape   = esp_random() % MOUTH_COUNT;
    s->noseShape    = esp_random() % NOSE_COUNT;
    s->skin         = esp_random() % SKIN_COLOR_COUNT;
    //s->name; // TODO: Generate based on name lists + random number
}

// Drawing
void grabSkinPaletteFromIdx(wsgPalette_t* palette, int idx)
{
    switch (idx)
    {
        case BLONDE:
        { // FIXME: Get real colors
            palette->newColors[c111] = c345;
            palette->newColors[c222] = c123;
            palette->newColors[c333] = c323;
            break;
        }
        default:
        {
            wsgPaletteReset(palette);
            break;
        }
    }
}

void grabHairPaletteFromIdx(wsgPalette_t* palette, int idx)
{
    switch (idx)
    {
        case BLONDE:
        { // FIXME: Get real colors
            palette->newColors[c111] = c345;
            palette->newColors[c222] = c123;
            palette->newColors[c333] = c323;
            break;
        }
        default:
        {
            wsgPaletteReset(palette);
            break;
        }
    }
}

void drawSwadgesona(swadgesona_t s, int x, int y)
{
    // TODO: Draw swadgesonas to the screen
}