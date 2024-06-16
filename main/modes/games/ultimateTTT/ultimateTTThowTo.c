//==============================================================================
// Includes
//==============================================================================

#include "ultimateTTThowTo.h"

//==============================================================================
// Defines
//==============================================================================

#define TEXT_MARGIN 18

#define ARROW_BLINK_PERIOD 1000000

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
    // If the button was pressed
    if (evt->down)
    {
        switch (evt->button)
        {
            case PB_B:
            case PB_LEFT:
            {
                // These buttons scroll back
                if (ttt->pageIdx > 0)
                {
                    ttt->pageIdx--;
                }
                else if (ttt->tutorialRead)
                {
                    // Return to main menu if going back from page 0, only if the rules have been read
                    ttt->ui = TUI_MENU;
                }
                break;
            }
            case PB_A:
            case PB_RIGHT:
            {
                // These buttons scroll forward
                if (NULL != ttt->pageStarts[ttt->pageIdx + 1])
                {
                    ttt->pageIdx++;
                }
                else
                {
                    // Mark the tutorial as completed
                    if (false == ttt->tutorialRead)
                    {
                        ttt->tutorialRead = true;
                        writeNvs32(tttTutorialKey, ttt->tutorialRead);
                    }
                    // If a piece hasn't been selected
                    if (-1 == ttt->activePieceIdx)
                    {
                        // Select the piece after reading the tutorial
                        ttt->ui = TUI_PIECE_SELECT;
                    }
                    else
                    {
                        // Return to main menu if going forward from the last page
                        ttt->ui = TUI_MENU;
                    }
                }
                break;
            }
        }
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
    drawMenuMania(ttt->bgMenu, ttt->menuRenderer, 0);
    led_t leds[CONFIG_NUM_LEDS] = {0};
    setLeds(leds, CONFIG_NUM_LEDS);

    // Start with this text
    if (0 == ttt->pageIdx)
    {
        ttt->pageStarts[ttt->pageIdx] = howToText;
    }

    // Draw the text here
    int16_t xOff = TEXT_MARGIN;
    int16_t yOff = 50 + TEXT_MARGIN;
    // Draw the text and save the next page
    ttt->pageStarts[ttt->pageIdx + 1] = drawTextWordWrap(&ttt->font_rodin, c000, ttt->pageStarts[ttt->pageIdx], &xOff,
                                                         &yOff, TFT_WIDTH - TEXT_MARGIN, TFT_HEIGHT - TEXT_MARGIN);

    // Blink the arrows
    ttt->arrowBlinkTimer += elapsedUs;
    while (ttt->arrowBlinkTimer >= ARROW_BLINK_PERIOD)
    {
        ttt->arrowBlinkTimer -= ARROW_BLINK_PERIOD;
    }

    if (ttt->arrowBlinkTimer < (ARROW_BLINK_PERIOD / 2))
    {
        // Draw arrows to indicate this can be scrolled
        if (0 != ttt->pageIdx)
        {
            // Draw left arrow if not on the first page
            drawText(&ttt->font_rodin, c000, "<", 0, (TFT_HEIGHT - ttt->font_rodin.height) / 2);
        }

        if (NULL != ttt->pageStarts[ttt->pageIdx + 1])
        {
            // Draw right arrow if not on the last page
            drawText(&ttt->font_rodin, c000, ">", TFT_WIDTH - textWidth(&ttt->font_rodin, ">"),
                     (TFT_HEIGHT - ttt->font_rodin.height) / 2);
        }
    }
}
