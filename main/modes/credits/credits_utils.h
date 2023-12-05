#pragma once

//==============================================================================
// Includes
//==============================================================================

#include "hdw-btn.h"
#include "font.h"

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    const char* name;
    const paletteColor_t color;
} creditsEntry_t;

typedef struct
{
    font_t* font;
    int64_t tElapsedUs;
    int8_t scrollMod;
    int16_t yOffset;
    song_t song;

    const creditsEntry_t* entries;
    int32_t numEntries;
} credits_t;

//==============================================================================
// Function Declarations
//==============================================================================

void initCredits(credits_t* credits, font_t* font, const creditsEntry_t* entries, int32_t numEntries);
void deinitCredits(credits_t* credits);
void drawCredits(credits_t* credits, uint32_t elapsedUs);
bool creditsButtonCb(credits_t* credits, buttonEvt_t* evt);
