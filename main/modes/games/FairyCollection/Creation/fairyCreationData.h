#pragma once

//==============================================================================
// Define
//==============================================================================

#include "fairyCollectionData.h"

//==============================================================================
// Struct
//==============================================================================

typedef struct
{
    int optionSelection;
    int options[FC_OPT_COUNT];
    fairy_t* fairy;
    profileCard_t* card;
    bool displayFairy;
} fcCreationData_t;