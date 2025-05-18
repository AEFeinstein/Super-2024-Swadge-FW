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
#include "wsgPalette.h"
#include "fs_wsg.h"
#include "hdw-tft.h"

//==============================================================================
// Defines
//==============================================================================
#define NVS_KEY_LEN 15

//==============================================================================
// Consts
//==============================================================================

static const char* const nvsStr[] = {"swadgesona", "swadgesona-"};

//==============================================================================
// Struct
//==============================================================================
typedef struct 
{
    swadgesona_t* sona;
    wsg_t* imgs;
} swadgesonaWrapper_t;

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
        case BLUE_SKIN:
        { 
            palette->newColors[c100] = c100; //dark color
            palette->newColors[c422] = c135; //mid color
            palette->newColors[c544] = c255; //base color
            break;
        }
        
        case GREEN_SKIN:
        { 
            palette->newColors[c100] = c100; //dark color
            palette->newColors[c422] = c133; //mid color
            palette->newColors[c544] = c243; //base color
            break;
        }
        case PURPLE_SKIN:
        { 
            palette->newColors[c100] = c100; //dark color
            palette->newColors[c422] = c223; //mid color
            palette->newColors[c544] = c334; //base color
            break;
        }
        case ONE_SKIN:
        { 
            palette->newColors[c100] = c100; //dark color
            palette->newColors[c422] = c432; //mid color
            palette->newColors[c544] = c543; //base color
            break;
        }
        case ZERO_SKIN:
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

void grabEyesPaletteFromIdx(wsgPalette_t* palette, int idx)
{
    switch (idx)
    {
        case PURPLE_EYES:
        { 
            palette->newColors[c130] = c345;//HIGHLIGHT
            palette->newColors[c010] = c224;//BASE
            break;
        }
         case BLUE_EYES:
        { 
            palette->newColors[c130] = c255;//HIGHLIGHT
            palette->newColors[c010] = c005;//BASE
            break;
        }
        case BLACK_EYES:
        { 
            palette->newColors[c130] = c444;//HIGHLIGHT
            palette->newColors[c010] = c000;//BASE
            break;
        }
        case RED_EYES:
        { 
            palette->newColors[c130] = c533;//HIGHLIGHT
            palette->newColors[c010] = c300;//BASE
            break;
        }
        case BROWN_EYES:
        { 
            palette->newColors[c130] = c432;//HIGHLIGHT
            palette->newColors[c010] = c210;//BASE
            break;
        }
        default:
        case GREEN_EYES:
        {
            wsgPaletteReset(palette);
            break;
        }
    }
}


void drawSwadgesona(swadgesona_t s, int x, int y)
{
    // TODO: Draw swadgesonas to the screen
    swadgesonaWrapper_t* sw = heap_caps_calloc(sizeof(swadgesonaWrapper_t),1,MALLOC_CAP_8BIT);
    sw->imgs = heap_caps_calloc(sizeof(wsg_t),2,MALLOC_CAP_8BIT);
    wsgPalette_t pal;
    wsgPaletteReset(&pal);
    sw->sona=&s;
    grabSkinPaletteFromIdx(&pal,sw->sona->skin);
    loadWsg(BODY_WSG,&sw->imgs[0],true);
    loadWsg(EYES_WSG,&sw->imgs[1],true);
    int xpos=(TFT_WIDTH-sw->imgs[0].w*4)/2;
    int ypos=(TFT_HEIGHT-sw->imgs[0].h*4)/2;
    drawWsgPaletteSimpleScaled(&sw->imgs[0],xpos,ypos,&pal,4,4);
    wsgPaletteReset(&pal);
    drawWsgPaletteSimpleScaled(&sw->imgs[1],xpos,ypos,&pal,4,4);
    freeWsg(&sw->imgs[0]);
    freeWsg(&sw->imgs[1]);
    heap_caps_free(sw->imgs);
    heap_caps_free(sw);
}