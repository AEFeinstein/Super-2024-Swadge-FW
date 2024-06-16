//==============================================================================
// Includes
//==============================================================================

#include "ultimateTTThowTo.h"

//==============================================================================
// Variables
//==============================================================================

static const char howToText[]
    = "Ultimate TTT is a big game of tic-tac-toe made up of nine small games of tic-tac-toe.\n"
      "The goal is to win three games of tic-tac-toe in a row.\n"
      "Players will take turns placing markers in the small games of tic-tac-toe.\n"
      "The starting player may place a marker anywhere.\n"
      "The square a marker is placed in determines the next small game a player must play in.\n"
      "For instance, if a marker is placed in the top-left square of a small game, then the next marker must be placed "
      "in the top-left small game.\n"
      "When a small game is won, markers may not be placed there anymore.\n"
      "If a marker cannot be placed in a small game, then it may be placed anywhere instead.";

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Handle a button input when How To is being shown
 *
 * @param ttt The entire game state
 * @param evt The button event
 */
void tttInputHowTo(ultimateTTT_t* ttt, buttonEvt_t* evt)
{
    // TODO handle text scrolling
    if (evt->down && PB_B == evt->button)
    {
        ttt->ui = TUI_MENU;
    }
}

/**
 * @brief Draw the How To UI
 *
 * @param ttt The entire game state
 * @param elapsedUs The time elapsed since this was last called
 */
void tttDrawHowTo(ultimateTTT_t* ttt, int64_t elapsedUs)
{
    // Draw the background
    drawMenuMania(ttt->bgMenu, ttt->menuRenderer, elapsedUs);
    // Draw the text
    int16_t xOff = 0;
    int16_t yOff = 68;
    drawTextWordWrap(&ttt->font_rodin, c000, howToText, &xOff, &yOff, TFT_WIDTH, TFT_HEIGHT);
    // TODO handle text scrolling
}
