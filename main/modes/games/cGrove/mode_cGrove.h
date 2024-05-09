#ifndef _CGROVE_MODE_H_
#define _CGROVE_MODE_H_

// Includes
//==============================================================================
#include "swadge2024.h"
#include "cGrove_Types.h"
#include "cGrove_Online.h"

// Function Prototypes
//==============================================================================
static void cGroveMainLoop(int64_t);
static void cGroveExitMode(void);
static void cGroveEnterMode(void);
static void cGroveInitMenu(void);
static void cGroveMenuCB(const char*, bool, uint32_t);
static void cGroveBackgroundDrawCallback(int16_t, int16_t, int16_t, int16_t, int16_t, int16_t);\

// Make Swadgemode available
extern swadgeMode_t cGroveMode;

#endif