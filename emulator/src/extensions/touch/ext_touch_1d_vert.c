//==============================================================================
// Imports
//==============================================================================

#include <stddef.h>
#include "ext_touch_1d_vert.h"

//==============================================================================
// Function Prototypes
//==============================================================================

static bool touch_1D_vert_Init(emuArgs_t* emuArgs);
static int32_t touch_1D_vert_Key(uint32_t key, bool down, modKey_t modifiers);
static bool touch_1D_vert_MouseMove(int32_t x, int32_t y, mouseBit_t buttonMask);
static bool touch_1D_vert_MouseButton(int32_t x, int32_t y, mouseButton_t button, bool down);
static void touch_1D_vert_Render(uint32_t winW, uint32_t winH, const emuPane_t* pane, uint8_t numPanes);

//==============================================================================
// Variables
//==============================================================================

emuExtension_t touchEmu1DVertExtension = {
    .name            = "touch_1D_vert",
    .fnInitCb        = touch_1D_vert_Init,
    .fnDeinitCb      = NULL,
    .fnPreFrameCb    = NULL,
    .fnPostFrameCb   = NULL,
    .fnKeyCb         = touch_1D_vert_Key,
    .fnMouseMoveCb   = touch_1D_vert_MouseMove,
    .fnMouseButtonCb = touch_1D_vert_MouseButton,
    .fnRenderCb      = touch_1D_vert_Render,
};

static emuTouch_t emuTouch1DVert = {0};

//==============================================================================
// Functions
//==============================================================================

static bool touch_1D_vert_Init(emuArgs_t* emuArgs)
{
    static const char keys[] = {'6', '7', '8', '9', '0'};
    return touchLinearInit(&emuTouch1DVert, &touchEmu1DVertExtension, emuArgs, false, keys,
                           sizeof(keys) / sizeof(keys[0]));
}

static int32_t touch_1D_vert_Key(uint32_t key, bool down, modKey_t modifiers)
{
    return touchLinearKey(&emuTouch1DVert, key, down, modifiers);
}

static bool touch_1D_vert_MouseMove(int32_t x, int32_t y, mouseBit_t buttonMask)
{
    return touchLinearMouseMove(&emuTouch1DVert, x, y, buttonMask);
}

static bool touch_1D_vert_MouseButton(int32_t x, int32_t y, mouseButton_t button, bool down)
{
    return touchLinearMouseButton(&emuTouch1DVert, x, y, button, down);
}

static void touch_1D_vert_Render(uint32_t winW, uint32_t winH, const emuPane_t* pane, uint8_t numPanes)
{
    return touchLinearRender(&emuTouch1DVert, winW, winH, pane, numPanes);
}
