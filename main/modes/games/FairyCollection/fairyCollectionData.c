//==============================================================================
// Includes
//==============================================================================

#include "fairyCollectionData.h"

//==============================================================================
// Consts
//==============================================================================

const int fairyMaxList[FC_NUM_OPTIONS] = {
    16, // Years
    FC_AT_COUNT,  FC_CARD_COUNT, FC_NONE,       FC_SHAPE_COUNT, FC_CHARM_COUNT, FC_FILL_COUNT,
    FC_PED_COUNT, FC_COL_COUNT,  FC_WING_COUNT, FC_BALL_COUNT,  FC_AURA_COUNT,
};

const char* const nvsStrs[] = {
    "fairy-col", "user-fairy", "user-card",
    "spp-saved", "spp-nextIdx",
};