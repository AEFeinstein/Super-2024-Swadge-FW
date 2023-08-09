//==============================================================================
// Includes
//==============================================================================
#include "emu_ext.h"
#include "macros.h"

// Extension Includes

//==============================================================================
// Variables
//==============================================================================

// ADD ALL CALLBACKS HERE IN ORDER TO REGISTER THEM
static const emuCallback_t* registeredCallbacks[] = {
};

//==============================================================================
// Functions
//==============================================================================

const emuCallback_t** getEmuCallbacks(int* count)
{
    *count = ARRAY_SIZE(registeredCallbacks);

    return registeredCallbacks;
}