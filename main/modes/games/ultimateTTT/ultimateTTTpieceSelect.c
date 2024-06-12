//==============================================================================
// Includes
//==============================================================================

#include "ultimateTTTpieceSelect.h"

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief TODO
 *
 */
void tttInputPieceSelect(ultimateTTT_t* ttt, buttonEvt_t* evt)
{
    if (evt->down && PB_B == evt->button)
    {
        ttt->ui = TUI_MENU;
    }
}

/**
 * @brief TODO
 *
 * @param ttt
 * @param elapsedUs
 */
void tttDrawPieceSelect(ultimateTTT_t* ttt, int64_t elapsedUs)
{
    clearPxTft();

    drawMenuMania(ttt->bgMenu, ttt->menuRenderer, elapsedUs);
    drawText(&ttt->font_rodin, c000, "Only defaults.", 40, 40);
}
