#pragma once

//==============================================================================
// Includes
//==============================================================================

#include "fairyCollectionData.h"

#include "nameList.h"
#include "wsg.h"

//==============================================================================
// Function Definitions
//==============================================================================

void fcGenerateFairyImage(fairy_t* fairy, wsg_t* wsg, bool drawBottle);

void fcDrawFairyCard(profileCard_t* card, fairy_t* fairy, nameData_t* nd, font_t* font);
