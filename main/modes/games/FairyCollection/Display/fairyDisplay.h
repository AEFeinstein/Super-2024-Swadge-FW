#pragma once

//==============================================================================
// Includes
//==============================================================================

#include "fairyCollectionData.h"

//==============================================================================
// Struct
//==============================================================================

typedef struct
{
    savedProfile_t spFairies[FC_MAX_NUM_FAIRIES];
    int fairyIndex;
    bool drawingFairy;
} fcspField_t;

//==============================================================================
// Function definitions
//==============================================================================

bool fcRunSPField(fcspField_t* fcspf);

void fcDrawSPField(fcspField_t* fcspf,  font_t* font, int64_t elapsedUs);