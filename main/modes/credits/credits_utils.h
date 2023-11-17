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
    font_t* font;
    int64_t tElapsedUs;
    int8_t scrollMod;
    int16_t yOffset;
    song_t song;

    const char* const* names;
    const paletteColor_t* colors;
    int32_t numElements;
} credits_t;

//==============================================================================
// Function Declarations
//==============================================================================

void initCredits(credits_t* credits, font_t* font, const char* const* names, const paletteColor_t* colors,
                 int32_t numElements);
void deinitCredits(credits_t* credits);
void drawCredits(credits_t* credits, uint32_t elapsedUs);
bool creditsButtonCb(credits_t* credits, buttonEvt_t* evt);
