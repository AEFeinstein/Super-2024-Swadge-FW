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

// C
#include <stdio.h>
#include <inttypes.h>

// ESP
#include <esp_log.h>
#include "esp_random.h"

// Swadge
#include "fs_wsg.h"
#include "hdw-nvs.h"
#include "hdw-tft.h"
#include "wsgPalette.h"
#include "macros.h"

//==============================================================================
// Defines
//==============================================================================

#define EYE_Y_CENTERPOINT 32 ///< Eye y coordinate

//==============================================================================
// Consts
//==============================================================================

static const char* const nvsStr[] = {"swadgesona", "swadgesona-"};

/* static const cnfsFileIdx_t earSprites[] = {
    KID_0_WSG,
    KID_1_WSG,
    KID_0_WSG,
    KID_1_WSG,
    KID_0_WSG,
    KID_1_WSG,
};
static const cnfsFileIdx_t hairSprites[] = {
    KID_0_WSG,
    KID_1_WSG,
    KID_0_WSG,
    KID_1_WSG,
    KID_0_WSG,
};
static const cnfsFileIdx_t eyebrowSprites[] = {
    EYES_WSG,
    KID_1_WSG,
}; */
static const cnfsFileIdx_t eyeSprites[] = {
    EYES_WSG,
    KID_1_WSG,
};
/* static const cnfsFileIdx_t noseSprites[] = {
    KID_0_WSG,
    KID_1_WSG,
};
static const cnfsFileIdx_t mouthSprites[] = {
    KID_0_WSG,
    KID_1_WSG,
};
static const cnfsFileIdx_t bodyMarkingSprites[] = {
    KID_0_WSG,
    KID_1_WSG,
    KID_0_WSG,
    KID_1_WSG,
    KID_0_WSG,
    KID_1_WSG,
    KID_0_WSG,
    KID_1_WSG,
    KID_0_WSG,
    KID_1_WSG,
    KID_0_WSG,
    KID_1_WSG,
};
static const cnfsFileIdx_t clothesSprites[] = {
    KID_0_WSG,
    KID_1_WSG,
}; */

//==============================================================================
// Function declarations
//==============================================================================

/**
 * @brief Grabs the relevant colors based on the provided index for the skin
 *
 * @param palette Palette to swap
 * @param idx Index to switch colors on
 */
static void _getSkinPaletteFromIdx(wsgPalette_t* palette, int idx);

/**
 * @brief Grabs the relevant colors based on the provided index for the hair
 *
 * @param palette Palette to swap
 * @param idx Index to switch colors on
 */
static void _getHairPaletteFromIdx(wsgPalette_t* palette, int idx);

/**
 * @brief Grabs the relevant colors based on the provided index for the eyes
 *
 * @param palette
 * @param idx
 */
static void _getEyesPaletteFromIdx(wsgPalette_t* palette, int idx);

static void _getClothesPaletteFromIdx(wsgPalette_t* palette, int idx);

static void _initWsgs(wsg_t* imageLocation, const cnfsFileIdx_t* images, int imageArraySize, int enumCount);

static void _freeWsgs(wsg_t* imageLocation, int imageArraySize);

//==============================================================================
// Functions
//==============================================================================

// Data
void saveSwadgesona(swadgesona_t* s, int idx)
{
    // Generate NVS key
    char nvsTag[NVS_KEY_NAME_MAX_SIZE];
    snprintf(nvsTag, NVS_KEY_NAME_MAX_SIZE - 1, "%s%" PRIu8, nvsStr[1], idx);
    if (!writeNamespaceNvsBlob(nvsStr[0], nvsTag, s, sizeof(s)))
    {
        ESP_LOGE("SONA", "Swadgesona failed to save");
    }
}

void loadSwadgesona(swadgesona_t* s, int idx)
{
    char nvsTag[NVS_KEY_NAME_MAX_SIZE];
    size_t len = 0;
    snprintf(nvsTag, NVS_KEY_NAME_MAX_SIZE - 1, "%s%" PRIu8, nvsStr[1], idx);
    readNamespaceNvsBlob(nvsStr[0], nvsTag, s, &len);
    if (!readNamespaceNvsBlob(nvsStr[0], nvsTag, s, &len))
    {
        ESP_LOGE("SONA", "Swadgesona failed to Load/does not exist");
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
    s->hairStyle    = esp_random() % HAIR_STYLE_COUNT;
    s->mouthShape   = esp_random() % MOUTH_COUNT;
    s->noseShape    = esp_random() % NOSE_COUNT;
    s->skin         = esp_random() % SKIN_COLOR_COUNT;

    // Generate name
    nameData_t nd = {.user = false};
    generateRandUsername(&nd);
    s->packedName = GET_PACKED_USERNAME(nd);
}

// Drawing

void initSwadgesonaDraw(swadgesonaData_t* ssd)
{
    // Initialize WSGs
    /* _initWsgs(ssd->ears, earSprites, ARRAY_SIZE(earSprites), EAR_COUNT);
    _initWsgs(ssd->hairstyles, hairSprites, ARRAY_SIZE(hairSprites), HAIR_STYLE_COUNT);
    _initWsgs(ssd->eyebrows, eyebrowSprites, ARRAY_SIZE(eyebrowSprites), EYEBROW_COUNT);
    _initWsgs(ssd->eyes, eyeSprites, ARRAY_SIZE(eyeSprites), EYE_SHAPE_COUNT);
    _initWsgs(ssd->noses, noseSprites, ARRAY_SIZE(noseSprites), NOSE_COUNT);
    _initWsgs(ssd->mouths, mouthSprites, ARRAY_SIZE(mouthSprites), MOUTH_COUNT);
    _initWsgs(ssd->bodymarks, bodyMarkingSprites, ARRAY_SIZE(bodyMarkingSprites), BODY_MARKS_COUNT);
    _initWsgs(ssd->clothes, clothesSprites, ARRAY_SIZE(clothesSprites), CLOTHES_OPTION_COUNT); */
    ssd->eyes = (wsg_t*)heap_caps_calloc(sizeof(wsg_t), ARRAY_SIZE(eyeSprites), MALLOC_CAP_8BIT);
    for (int idx = 0; idx < ARRAY_SIZE(eyeSprites); idx++)
    {
        loadWsg(eyeSprites[idx], &ssd->eyes[idx], true);
    }
    loadWsg(BODY_WSG, &ssd->body, true);
    ssd->sw = (swadgesonaWrapper_t*)heap_caps_calloc(sizeof(swadgesonaWrapper_t), 1, MALLOC_CAP_8BIT);

    // Initialize Palettes
    wsgPaletteReset(&ssd->sw->skinPalette);
    wsgPaletteReset(&ssd->sw->hairPalette);
    wsgPaletteReset(&ssd->sw->eyePalette);
    wsgPaletteReset(&ssd->sw->clothesPalette);
}

void freeSwadgesonaDraw(swadgesonaData_t* ssd)
{
    /* _freeWsgs(ssd->ears, ARRAY_SIZE(earSprites));
    _freeWsgs(ssd->hairstyles, ARRAY_SIZE(hairSprites));
    _freeWsgs(ssd->eyebrows, ARRAY_SIZE(eyebrowSprites));
    _freeWsgs(ssd->eyes, ARRAY_SIZE(eyeSprites));
    _freeWsgs(ssd->noses, ARRAY_SIZE(noseSprites));
    _freeWsgs(ssd->mouths, ARRAY_SIZE(mouthSprites));
    _freeWsgs(ssd->bodymarks, ARRAY_SIZE(bodyMarkingSprites));
    _freeWsgs(ssd->clothes, ARRAY_SIZE(clothesSprites)); */
    for (int idx = 0; idx < ARRAY_SIZE(eyeSprites); idx++)
    {
        freeWsg(&ssd->eyes[idx]);
    }
    heap_caps_free(&ssd->eyes);
    freeWsg(&ssd->body);
    heap_caps_free(ssd->sw);
}

void drawSwadgesona(swadgesonaData_t* ssd, swadgesona_t* s, int x, int y, int scale)
{
    ssd->sw->sona = s;

    // Initialize palettes and name
    _getSkinPaletteFromIdx(&ssd->sw->skinPalette, ssd->sw->sona->skin);
    _getHairPaletteFromIdx(&ssd->sw->hairPalette, ssd->sw->sona->hairColor);
    _getEyesPaletteFromIdx(&ssd->sw->eyePalette, ssd->sw->sona->eyeColor);
    _getClothesPaletteFromIdx(&ssd->sw->clothesPalette, ssd->sw->sona->clothesColor);
    setUsernameFrom32(&ssd->sw->name, s->packedName);

    int centerlineX = x + (ssd->body.w >> 1);

    // Draw items in order
    // Ears

    // Body
    drawWsgPaletteSimpleScaled(&ssd->body, x, y, &ssd->sw->skinPalette, scale, scale);

    // Clothes

    // Eyes
    // x: centerlineX - ((ssd->eyes[s->eyeShape].w * scale) >> 1)
    // y: + (EYE_Y_CENTERPOINT * scale) + ((ssd->eyes[s->eyeShape].h * scale) >> 1)
    drawWsgPaletteSimpleScaled(&ssd->eyes[s->eyeShape], x, y, &ssd->sw->eyePalette, scale, scale);

    // Mouth
    drawWsgPaletteSimpleScaled(&ssd->mouths[s->mouthShape], x, y, &ssd->sw->eyePalette, scale, scale);//check palettes
    // Nose
    drawWsgPaletteSimpleScaled(&ssd->noses[s->noseShape], x, y, &ssd->sw->eyePalette, scale, scale);
    // Eyebrows
    drawWsgPaletteSimpleScaled(&ssd->eyebrows, x, y, &ssd->sw->eyePalette, scale, scale);
    // Hair
    drawWsgPaletteSimpleScaled(&ssd->hairstyles, x, y, &ssd->sw->hairPalette, scale, scale);//hairpalette
    // Body marks
    drawWsgPaletteSimpleScaled(&ssd->eyes[s->eyeShape], x, y, &ssd->sw->eyePalette, scale, scale);
}

//==============================================================================
// Static Functions
//==============================================================================

static void _getSkinPaletteFromIdx(wsgPalette_t* palette, int idx)
{
    switch (idx)
    {
        case SKIN_ZERO:
        default:
        {
            palette->newColors[c422] = c422; // mid color
            palette->newColors[c544] = c544; // base color
            break;
        }
        case SKIN_ONE:
        {
            palette->newColors[c422] = c423; //mid color
            palette->newColors[c544] = c545; // base color
            break;
        }
        case SKIN_TWO:
        {
            palette->newColors[c422] = c432; // mid color
            palette->newColors[c544] = c543; // base color
            break;
        }
        case SKIN_THREE:
        {
            palette->newColors[c422] = c321; // mid color
            palette->newColors[c544] = c432; // base color
            break;
        }
        case SKIN_FOUR:
        {
            palette->newColors[c422] = c211; // mid color
            palette->newColors[c544] = c321; // base color
            break;
        }
        case SKIN_FIVE:
        {
            palette->newColors[c422] = c200; // mid color
            palette->newColors[c544] = c210; // base color
            break;
        }
        case SKIN_BLUE:
        {
            palette->newColors[c422] = c135; // mid color
            palette->newColors[c544] = c255; // base color
            break;
        }
        case SKIN_GRAY: 
        {
            palette->newColors[c422] = c333; // mid color
            palette->newColors[c544] = c444; // base color
            break;
        }
        case SKIN_GREEN:
        {
            palette->newColors[c422] = c133; // mid color
            palette->newColors[c544] = c243; // base color
            break;
        }
        case SKIN_PURPLE:
        {
            palette->newColors[c422] = c223; // mid color
            palette->newColors[c544] = c334; // base color
            break;
        }
        case SKIN_RED: 
        {
            palette->newColors[c422] = c411; // mid color
            palette->newColors[c544] = c422; // base color
            break;
        }
        case SKIN_PINK: 
        {
            palette->newColors[c422] = c525; //mid color
            palette->newColors[c544] = c545; // base color
            break;
        }
    }
}

static void _getHairPaletteFromIdx(wsgPalette_t* palette, int idx)
{
    switch (idx)
    {
        case HAIR_GRAY:
        { 
            palette->newColors[c333] = c333;   //highlight
            palette->newColors[c222] = c222;   //midtone
            palette->newColors[c111] = c111;   //low light
            break;
        } 
        case HAIR_BLONDE:
        { 
            palette->newColors[c333] = c542;   //highlight
            palette->newColors[c222] = c432;   //midtone
            palette->newColors[c111] = c321;   //low light
            break;
        } 
        case HAIR_COPPER:
        { 
            palette->newColors[c333] = c531;   //highlight
            palette->newColors[c222] = c421;   //midtone
            palette->newColors[c111] = c411;   //low light
            break;
        } 
        case HAIR_ORANGE:
        { 
            palette->newColors[c333] = c520;   //highlight
            palette->newColors[c222] = c400;   //midtone
            palette->newColors[c111] = c300;   //low light
            break;
        } 
        case HAIR_RED:
        { 
            palette->newColors[c333] = c300;   //highlight
            palette->newColors[c222] = c200;   //midtone
            palette->newColors[c111] = c100;   //low light
            break;
        } 
        case HAIR_BROWN:
        { 
            palette->newColors[c333] = c210;   //highlight
            palette->newColors[c222] = c100;   //midtone
            palette->newColors[c111] = c000;   //low light
            break;
        } 
        case HAIR_BLACK:
        { 
            palette->newColors[c333] = c000;   //highlight
            palette->newColors[c222] = c012;   //midtone
            palette->newColors[c111] = c001;   //low light
            break;
        } 
        case HAIR_WHITE:
        { 
            palette->newColors[c333] = c555;   //highlight
            palette->newColors[c222] = c444;   //midtone
            palette->newColors[c111] = c222;   //low light
            break;
        } 
        case HAIR_LPINK:
        { 
            palette->newColors[c333] = c535;   //highlight
            palette->newColors[c222] = c424;   //midtone
            palette->newColors[c111] = c302;   //low light
            break;
        } 
        case HAIR_DPINK:
        { 
            palette->newColors[c333] = c413;   //highlight
            palette->newColors[c222] = c302;   //midtone
            palette->newColors[c111] = c201;   //low light
            break;
        } 
        case HAIR_LAVENDER:
        { 
            palette->newColors[c333] = c435;   //highlight
            palette->newColors[c222] = c324;   //midtone
            palette->newColors[c111] = c314;   //low light
            break;
        } 
        case HAIR_PURPLE:
        { 
            palette->newColors[c333] = c203;   //highlight
            palette->newColors[c222] = c102;   //midtone
            palette->newColors[c111] = c101;   //low light
            break;
        } 
        case HAIR_LBLUE:
        { 
            palette->newColors[c333] = c044;   //high light
            palette->newColors[c222] = c033;   //midtone
            palette->newColors[c111] = c012;   //low light
            break;
        }
        case HAIR_DBLUE:
        { 
            palette->newColors[c333] = c013;   //highlight
            palette->newColors[c222] = c002;   //midtone
            palette->newColors[c111] = c001;   //low light
            break;
        }
        case HAIR_LGREEN:
        { 
            palette->newColors[c333] = c353;   //highlight
            palette->newColors[c222] = c032;   //midtone
            palette->newColors[c111] = c021;   //low light
            break;
        }
        case HAIR_DGREEN:
        { 
            palette->newColors[c333] = c131;   //highlight
            palette->newColors[c222] = c121;   //midtone
            palette->newColors[c111] = c010;   //low light
            break;
        }
        default:
        {
            wsgPaletteReset(palette);
            break;
        }
    }
}

static void _getEyesPaletteFromIdx(wsgPalette_t* palette, int idx)
{
    switch (idx)
    {
        case EYES_BLACK:
        {
            palette->newColors[c130] = c444; // HIGHLIGHT
            palette->newColors[c010] = c000; // BASE
            break;
        }
        case EYES_BLUE:
        {
            palette->newColors[c130] = c255; // HIGHLIGHT
            palette->newColors[c010] = c005; // BASE
            break;
        }
        case EYES_BROWN:
        {
            palette->newColors[c130] = c432; // HIGHLIGHT
            palette->newColors[c010] = c210; // BASE
            break;
        }
        case EYES_GRAY: 
        {
            palette->newColors[c130] = c444; // HIGHLIGHT
            palette->newColors[c010] = c222; // BASE
            break;
        }
        default:
        case EYES_GREEN:
        {
            palette->newColors[c130] = c353; // HIGHLIGHT
            palette->newColors[c010] = c121; // BASE
            break;
        }
        case EYES_PINK: 
        {
            palette->newColors[c130] = c525; // HIGHLIGHT
            palette->newColors[c010] = c403; // BASE
            break;
        }
        case EYES_PURPLE:
        {
            palette->newColors[c130] = c345; // HIGHLIGHT
            palette->newColors[c010] = c224; // BASE
            break;
        }
        case EYES_RED:
        {
            palette->newColors[c130] = c533; // HIGHLIGHT
            palette->newColors[c010] = c300; // BASE
            break;
        }
        case EYES_YELLOW: 
        {
            palette->newColors[c130] = c553; // HIGHLIGHT
            palette->newColors[c010] = c541; // BASE
            break;
        }
    }
}

static void _getClothesPaletteFromIdx(wsgPalette_t* palette, int idx)
{
    switch (idx)
    {
        default:
        {
            // TODO: Add colors
            break;
        }
    }
}

static void _initWsgs(wsg_t* imageLocation, const cnfsFileIdx_t* images, int imageArraySize, int enumCount)
{
    // FIXME: Memory clears itself so I've done something wrong
    assert(imageArraySize == enumCount);
    imageLocation = (wsg_t*)heap_caps_calloc(sizeof(wsg_t), imageArraySize, MALLOC_CAP_8BIT);
    for (int idx = 0; idx < imageArraySize; idx++)
    {
        loadWsg(images[idx], &imageLocation[idx], true);
    }
}

static void _freeWsgs(wsg_t* imageLocation, int imageArraySize)
{
    for (int idx = 0; idx < imageArraySize; idx++)
    {
        freeWsg(&imageLocation[idx]);
    }
    heap_caps_free(imageLocation);
}