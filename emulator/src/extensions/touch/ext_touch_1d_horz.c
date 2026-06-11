//==============================================================================
// Imports
//==============================================================================

#include <stddef.h>
#include "ext_touch_1d_horz.h"

//==============================================================================
// Function Prototypes
//==============================================================================

static bool touch_1D_horz_Init(emuArgs_t* emuArgs);
static int32_t touch_1D_horz_Key(uint32_t key, bool down, modKey_t modifiers);
static bool touch_1D_horz_MouseMove(int32_t x, int32_t y, mouseBit_t buttonMask);
static bool touch_1D_horz_MouseButton(int32_t x, int32_t y, mouseButton_t button, bool down);
static void touch_1D_horz_Render(uint32_t winW, uint32_t winH, const emuPane_t* pane, uint8_t numPanes);

//==============================================================================
// Variables
//==============================================================================

emuExtension_t touchEmu1DHorzExtension = {
    .name            = "touch_1D_horz",
    .fnInitCb        = touch_1D_horz_Init,
    .fnDeinitCb      = NULL,
    .fnPreFrameCb    = NULL,
    .fnPostFrameCb   = NULL,
    .fnKeyCb         = touch_1D_horz_Key,
    .fnMouseMoveCb   = touch_1D_horz_MouseMove,
    .fnMouseButtonCb = touch_1D_horz_MouseButton,
    .fnRenderCb      = touch_1D_horz_Render,
};

static emuTouch_t emuTouch1DHorz = {0};

//==============================================================================
// Functions
//==============================================================================

static bool touch_1D_horz_Init(emuArgs_t* emuArgs)
{
    static const char keys[] = {'1', '2', '3', '4', '5'};
    return touchLinearInit(&emuTouch1DHorz, &touchEmu1DHorzExtension, emuArgs, true, keys,
                           sizeof(keys) / sizeof(keys[0]));
}

static int32_t touch_1D_horz_Key(uint32_t key, bool down, modKey_t modifiers)
{
    return touchLinearKey(&emuTouch1DHorz, key, down, modifiers);
}

static bool touch_1D_horz_MouseMove(int32_t x, int32_t y, mouseBit_t buttonMask)
{
    return touchLinearMouseMove(&emuTouch1DHorz, x, y, buttonMask);
}

static bool touch_1D_horz_MouseButton(int32_t x, int32_t y, mouseButton_t button, bool down)
{
    return touchLinearMouseButton(&emuTouch1DHorz, x, y, button, down);
}

static void touch_1D_horz_Render(uint32_t winW, uint32_t winH, const emuPane_t* pane, uint8_t numPanes)
{
    return touchLinearRender(&emuTouch1DHorz, winW, winH, pane, numPanes);
}
