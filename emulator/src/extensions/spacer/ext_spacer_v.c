#include <stddef.h>
#include "ext_touch_linear.h"
#include "ext_spacer_v.h"

bool vSpacerInit(emuArgs_t* emuArgs);

emuExtension_t vSpacerExtension = {
    .name            = "vSpacer",
    .fnInitCb        = vSpacerInit,
    .fnDeinitCb      = NULL,
    .fnPreFrameCb    = NULL,
    .fnPostFrameCb   = NULL,
    .fnKeyCb         = NULL,
    .fnMouseMoveCb   = NULL,
    .fnMouseButtonCb = NULL,
    .fnRenderCb      = NULL,
};

bool vSpacerInit(emuArgs_t* emuArgs)
{
    if (emuArgs->emulateTouch)
    {
        requestPane(&vSpacerExtension, PANE_LEFT, TOUCH_PANE_MIN_SIZE, TOUCH_PANE_MIN_SIZE);
    }
    return true;
}