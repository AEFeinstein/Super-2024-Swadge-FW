//==============================================================================
// Imports
//==============================================================================

#include <stddef.h>
#include "ext_touch_1d_right.h"

//==============================================================================
// Function Prototypes
//==============================================================================

static bool touch_1D_right_Init(emuArgs_t* emuArgs);
static int32_t touch_1D_right_Key(uint32_t key, bool down, modKey_t modifiers);
static bool touch_1D_right_MouseMove(int32_t x, int32_t y, mouseBit_t buttonMask);
static bool touch_1D_right_MouseButton(int32_t x, int32_t y, mouseButton_t button, bool down);
static void touch_1D_right_Render(uint32_t winW, uint32_t winH, const emuPane_t* pane, uint8_t numPanes);

//==============================================================================
// Variables
//==============================================================================

emuExtension_t touchEmu1DRightExtension = {
    .name            = "touch_1D_right",
    .fnInitCb        = touch_1D_right_Init,
    .fnDeinitCb      = NULL,
    .fnPreFrameCb    = NULL,
    .fnPostFrameCb   = NULL,
    .fnKeyCb         = touch_1D_right_Key,
    .fnMouseMoveCb   = touch_1D_right_MouseMove,
    .fnMouseButtonCb = touch_1D_right_MouseButton,
    .fnRenderCb      = touch_1D_right_Render,
};

static emuTouch_t emuTouch1DRight = {0};

//==============================================================================
// Functions
//==============================================================================

static bool touch_1D_right_Init(emuArgs_t* emuArgs)
{
    static const char keys[] = {'6', '7', '8', '9', '0'};
    return touchLinearInit(&emuTouch1DRight, &touchEmu1DRightExtension, emuArgs, false, keys,
                           sizeof(keys) / sizeof(keys[0]));
}

static int32_t touch_1D_right_Key(uint32_t key, bool down, modKey_t modifiers)
{
    return touchLinearKey(&emuTouch1DRight, key, down, modifiers);
}

static bool touch_1D_right_MouseMove(int32_t x, int32_t y, mouseBit_t buttonMask)
{
    return touchLinearMouseMove(&emuTouch1DRight, x, y, buttonMask);
}

static bool touch_1D_right_MouseButton(int32_t x, int32_t y, mouseButton_t button, bool down)
{
    return touchLinearMouseButton(&emuTouch1DRight, x, y, button, down);
}

static void touch_1D_right_Render(uint32_t winW, uint32_t winH, const emuPane_t* pane, uint8_t numPanes)
{
    return touchLinearRender(&emuTouch1DRight, winW, winH, pane, numPanes);
}
