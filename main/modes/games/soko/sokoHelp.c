//==============================================================================
// Includes
//==============================================================================

#include "hdw-tft.h"
#include "sokoHelp.h"

//==============================================================================
// Defines
//==============================================================================

#define TEXT_MARGIN_L 18
#define TEXT_MARGIN_R 13

#define ARROW_BLINK_PERIOD 1000000

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    const char* title;
    const char* text;
} sokoHelpPage_t;

//==============================================================================
// Const data
//==============================================================================

static const char controlTitle[] = "Controls";
static const char boxTitle[]     = "Boxes";

static const sokoHelpPage_t helpPages[] = {
    {
        .title = sokoModeName,
        .text  = "You are an Eye. Your job is to paint the floors in twenty two rooms. Don't ask questions.",
    },
    {
        .title = controlTitle,
        .text  = "Use the D-Pad to walk around the main hallway, enter rooms, and paint the floors.",
    },
    {
        .title = controlTitle,
        .text  = "When painting a floor, you can't step on wet paint. \nNo returning to a spot you've already been",
    },
    {
        .title = controlTitle,
        .text  = "If you make a small mistake, you can press A to undo one step before anyone notices.",
    },
    {
        .title = controlTitle,
        .text
        = "If you make a big mistake, you can press B to reset the entire room. Hunter will probably notice though.",
    },
    {
        .title = controlTitle,
        .text = "If you're frustrated and need a break from a room, you can press Start to return to the main hallway.",
        //progress on the last room you entered is saved when you leave.
    },
    {
        .title = boxTitle,
        .text  = "Hunter, frustratingly, left orange boxes in some rooms. Push them around to make sure the entire floor "
                 "is painted.",
    },
    {
        .title = boxTitle,
        .text  = "Purple boxes are like orange boxes, but sticky and gross. In the event that one is stuck to "
                 "you, try scraping it off.",
    },
    {
        .title = boxTitle,
        .text  = "Red & purple boxes are highly toxic to touch. You can't push them directly, but maybe you can with "
                 "something else. Safety first.",
    },
    {
        .title = sokoModeName,
        .text  = "Good luck and get painting! \n\n'I hope you enjoy my puzzles'\n - Hunter",
    },
};

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Draw the help menu
 *
 * @param soko The entire soko state
 * @param elapsedUs The time elapsed since the last function call
 */
void drawSokoHelp(soko_abs_t* soko, int32_t elapsedUs)
{
    // Set the title
    soko->bgMenu->title = helpPages[soko->helpIdx].title;

    // Draw background, without animation
    drawMenuMania(soko->bgMenu, soko->menuManiaRenderer, 0);

    // Draw text
    paletteColor_t textColor = c000;
    int16_t xOff             = TEXT_MARGIN_L;
    int16_t yOff             = MANIA_TITLE_HEIGHT + 8;
    drawTextWordWrap(&soko->font_rodin, textColor, helpPages[soko->helpIdx].text, &xOff, &yOff,
                     TFT_WIDTH - TEXT_MARGIN_R, TFT_HEIGHT);
    xOff = TEXT_MARGIN_L;
    yOff = MANIA_TITLE_HEIGHT + 8;

    // Draw page numbers
    char pageText[32];
    snprintf(pageText, sizeof(pageText) - 1, "%" PRId32 "/%" PRId32 "", 1 + soko->helpIdx,
             (int32_t)ARRAY_SIZE(helpPages));

    int16_t tWidth = textWidth(&soko->font_rodin, pageText);
    drawText(&soko->font_rodin, textColor, pageText, TFT_WIDTH - 30 - tWidth, TFT_HEIGHT - soko->font_rodin.height + 2);

    // Blink the arrows
    soko->arrowBlinkTimer += elapsedUs;
    while (soko->arrowBlinkTimer >= ARROW_BLINK_PERIOD)
    {
        soko->arrowBlinkTimer -= ARROW_BLINK_PERIOD;
    }

    if (soko->arrowBlinkTimer < (ARROW_BLINK_PERIOD / 2))
    {
        // Draw arrows to indicate this can be scrolled
        if (0 != soko->helpIdx)
        {
            // Draw left arrow if not on the first page
            drawText(&soko->font_rodin, textColor, "<", 0, (TFT_HEIGHT - soko->font_rodin.height) / 2);
        }

        if ((ARRAY_SIZE(helpPages) - 1) != soko->helpIdx)
        {
            // Draw right arrow if not on the last page
            drawText(&soko->font_rodin, textColor, ">", TFT_WIDTH - textWidth(&soko->font_rodin, ">"),
                     (TFT_HEIGHT - soko->font_rodin.height) / 2);
        }
    }
}

/**
 * @brief Handle a button event on the help screen
 *
 * @param soko The entire soko state
 * @param evt The button event to handle
 */
void buttonSokoHelp(soko_abs_t* soko, buttonEvt_t* evt)
{
    if (evt->down)
    {
        switch (evt->button)
        {
            case PB_LEFT:
            {
                if (soko->helpIdx > 0)
                {
                    soko->helpIdx--;
                }
                else
                {
                    soko->screen = SOKO_MENU;
                    setManiaLedsOn(soko->menuManiaRenderer, true);
                    setManiaDrawRings(soko->menuManiaRenderer, true);
                }
                break;
            }
            case PB_RIGHT:
            {
                if (soko->helpIdx < ARRAY_SIZE(helpPages) - 1)
                {
                    soko->helpIdx++;
                }
                else
                {
                    soko->screen = SOKO_MENU;
                    setManiaLedsOn(soko->menuManiaRenderer, true);
                    setManiaDrawRings(soko->menuManiaRenderer, true);
                }
                break;
            }
            default:
            {
                break;
            }
        }
    }
}