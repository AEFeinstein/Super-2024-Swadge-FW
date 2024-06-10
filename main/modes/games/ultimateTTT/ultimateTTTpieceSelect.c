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
 */
void tttDrawPieceSelect(ultimateTTT_t* ttt)
{
    clearPxTft();
    drawText(&ttt->font_rodin, c555, "Only defaults.", 40, 40);
}
