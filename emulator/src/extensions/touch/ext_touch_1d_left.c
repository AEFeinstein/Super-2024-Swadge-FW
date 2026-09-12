//==============================================================================
// Imports
//==============================================================================

#include <stddef.h>
#include "ext_touch_1d_left.h"

//==============================================================================
// Function Prototypes
//==============================================================================

static bool touch_1D_left_Init(emuArgs_t* emuArgs);
static int32_t touch_1D_left_Key(uint32_t key, bool down, modKey_t modifiers);
static bool touch_1D_left_MouseMove(int32_t x, int32_t y, mouseBit_t buttonMask);
static bool touch_1D_left_MouseButton(int32_t x, int32_t y, mouseButton_t button, bool down);
static void touch_1D_left_Render(uint32_t winW, uint32_t winH, const emuPane_t* pane, uint8_t numPanes);

//==============================================================================
// Variables
//==============================================================================

emuExtension_t touchEmu1DLeftExtension = {
    .name            = "touch_1D_left",
    .fnInitCb        = touch_1D_left_Init,
    .fnDeinitCb      = NULL,
    .fnPreFrameCb    = NULL,
    .fnPostFrameCb   = NULL,
    .fnKeyCb         = touch_1D_left_Key,
    .fnMouseMoveCb   = touch_1D_left_MouseMove,
    .fnMouseButtonCb = touch_1D_left_MouseButton,
    .fnRenderCb      = touch_1D_left_Render,
};

static emuTouch_t emuTouch1DLeft = {0};

//==============================================================================
// Functions
//==============================================================================

static bool touch_1D_left_Init(emuArgs_t* emuArgs)
{
    static const char keys[] = {'1', '2', '3', '4', '5'};
    return touchLinearInit(&emuTouch1DLeft, &touchEmu1DLeftExtension, emuArgs, true, keys,
                           sizeof(keys) / sizeof(keys[0]));
}

static int32_t touch_1D_left_Key(uint32_t key, bool down, modKey_t modifiers)
{
    return touchLinearKey(&emuTouch1DLeft, key, down, modifiers);
}

static bool touch_1D_left_MouseMove(int32_t x, int32_t y, mouseBit_t buttonMask)
{
    return touchLinearMouseMove(&emuTouch1DLeft, x, y, buttonMask);
}

static bool touch_1D_left_MouseButton(int32_t x, int32_t y, mouseButton_t button, bool down)
{
    return touchLinearMouseButton(&emuTouch1DLeft, x, y, button, down);
}

static void touch_1D_left_Render(uint32_t winW, uint32_t winH, const emuPane_t* pane, uint8_t numPanes)
{
    return touchLinearRender(&emuTouch1DLeft, winW, winH, pane, numPanes);
}
