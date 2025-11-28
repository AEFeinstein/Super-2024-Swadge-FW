//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include <stdio.h>
#include <esp_heap_caps.h>
#include <string.h>
#include "cutscene.h"
#include "esp_random.h"
#include "fs_wsg.h"


//==============================================================================
// Function Prototypes
//==============================================================================
static int randomInt(int lowerBoundInclusive, int upperBoundInclusive);
static uint8_t getRandomVariationFromStyle(cutsceneStyle_t* style);
static uint8_t getRandomVariationFromStyleIdx(cutscene_t* cutscene, uint8_t styleIdx);
static cutsceneStyle_t* getCurrentStyle(cutscene_t* cutscene);

cutscene_t* initCutscene(cutsceneCb cbFunc)
{
    cutscene_t* cutscene = (cutscene_t*)heap_caps_calloc(1, sizeof(cutscene_t), MALLOC_CAP_SPIRAM);
    cutscene->cbFunc = cbFunc;
    cutscene->lines = heap_caps_calloc(1, sizeof(list_t), MALLOC_CAP_SPIRAM);
    cutscene->styles = heap_caps_calloc(1, sizeof(list_t), MALLOC_CAP_SPIRAM);
    cutscene->sprite = heap_caps_calloc(1, sizeof(wsg_t), MALLOC_CAP_SPIRAM);
    cutscene->yOffset = 240;
    return cutscene;
}

void addCutsceneStyle(cutscene_t* cutscene, paletteColor_t textColor, cnfsFileIdx_t spriteIdx, uint8_t numSpriteVariations)
{
    cutsceneStyle_t* style = (cutsceneStyle_t*)heap_caps_calloc(1, sizeof(cutsceneLine_t), MALLOC_CAP_SPIRAM);
    style->textColor = textColor;
    style->spriteIdx = spriteIdx;
    style->numSpriteVariations = numSpriteVariations;

    // push to tail
    push(cutscene->styles, (void*)style);
}

void addCutsceneLine(cutscene_t* cutscene, char* title, char* body, uint8_t styleIdx)
{
    cutsceneLine_t* line = (cutsceneLine_t*)heap_caps_calloc(1, sizeof(cutsceneLine_t), MALLOC_CAP_SPIRAM);
    line->title = (char*)heap_caps_calloc(strlen(title) + 1, sizeof(char), MALLOC_CAP_SPIRAM);
    strcpy(line->title, title);
    line->body = (char*)heap_caps_calloc(strlen(body) + 1, sizeof(char), MALLOC_CAP_SPIRAM);
    strcpy(line->body, body);
    line->styleIdx = styleIdx;
    line->spriteVariation = getRandomVariationFromStyleIdx(cutscene, styleIdx);

    // push to tail
    push(cutscene->lines, (void*)line);
}

static int randomInt(int lowerBoundInclusive, int upperBoundInclusive)
{
    return esp_random() % (upperBoundInclusive - lowerBoundInclusive + 1) + lowerBoundInclusive;
}

static uint8_t getRandomVariationFromStyle(cutsceneStyle_t* style)
{
    return randomInt(0,style->numSpriteVariations - 1);
}

static uint8_t getRandomVariationFromStyleIdx(cutscene_t* cutscene, uint8_t styleIdx)
{
    return getRandomVariationFromStyle((cutsceneStyle_t*)getAtIndex(cutscene->styles, styleIdx));
}

static cutsceneStyle_t* getCurrentStyle(cutscene_t* cutscene)
{
    return getAtIndex(cutscene->styles, ((cutsceneLine_t*)cutscene->lines->first->val)->styleIdx);
}

void updateCutscene(cutscene_t* cutscene)
{
    if(cutscene->yOffset > 0)
    {
        cutscene->yOffset-=8;
    }
    cutscene->blinkTimer++;
    if(cutscene->sprite->w == 0)
    {
        cutsceneStyle_t* style = getCurrentStyle(cutscene);
        loadWsg(style->spriteIdx + ((cutsceneLine_t*)cutscene->lines->first->val)->spriteVariation, cutscene->sprite, true);
    }
}

void drawCutscene(cutscene_t* cutscene, font_t* font)
{
    cutsceneLine_t* line = (cutsceneLine_t*)cutscene->lines->first->val;

    drawWsgSimple(cutscene->sprite, 0, cutscene->yOffset);

    // if (dData->blinkTimer > 0)
    // {
    //     drawWsgSimple(&dData->spriteNext, 254 + (textColor == c525 ? 4 : 0), -dData->offsetY + 186);
    // }
    if(cutscene->yOffset == 0)
    {
        cutsceneStyle_t* style = getAtIndex(cutscene->styles, line->styleIdx);
        drawText(font, style->textColor, line->title, 13, 152);

        int16_t xOff = 13;
        int16_t yOff = 177;
        drawTextWordWrap(font, style->textColor, line->body, &xOff, &yOff, 253, 230);
    }
}

