//==============================================================================
// Includes
//==============================================================================

#include "ultimateTTThowTo.h"

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief TODO
 *
 * @param ttt The entire game state
 * @param evt
 */
void tttInputHowTo(ultimateTTT_t* ttt, buttonEvt_t* evt)
{
    if (evt->down && PB_B == evt->button)
    {
        ttt->ui = TUI_MENU;
    }
}

/**
 * @brief TODO
 *
 * @param ttt The entire game state
 */
void tttDrawHowTo(ultimateTTT_t* ttt)
{
    clearPxTft();
    drawText(&ttt->font_rodin, c555, "Get good.", 40, 40);
}
