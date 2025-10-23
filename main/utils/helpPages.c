//==============================================================================
// Includes
//==============================================================================

#include <inttypes.h>
#include <esp_heap_caps.h>
#include "hdw-tft.h"
#include "helpPages.h"

//==============================================================================
// Defines
//==============================================================================

#define TEXT_MARGIN   4
#define TEXT_MARGIN_L (22 + TEXT_MARGIN)
#define TEXT_MARGIN_R (24 + TEXT_MARGIN)
#define TEXT_MARGIN_U (49 + TEXT_MARGIN)

#define ARROW_BLINK_PERIOD 1000000

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initialize a help screen
 *
 * @param bgMenu A menu to draw behind the help screen. It should not have any entries!
 * @param menuRenderer A renderer to draw the menu
 * @param pages The pages to be displayed on the help screen
 * @param numPages The total number of pages
 * @return An initialized helpPageVars_t*
 */
helpPageVars_t* initHelpScreen(menu_t* bgMenu, menuMegaRenderer_t* menuRenderer, const helpPage_t* pages,
                               int32_t numPages)
{
    helpPageVars_t* help = heap_caps_calloc(1, sizeof(helpPageVars_t), MALLOC_CAP_SPIRAM);
    help->bgMenu         = bgMenu;
    help->menuRenderer   = menuRenderer;
    help->pages          = pages;
    help->numPages       = numPages;
    return help;
}

/**
 * @brief Deinitialize a help screen
 *
 * @param help The help screen to deinitialize
 */
void deinitHelpScreen(helpPageVars_t* help)
{
    heap_caps_free(help);
}

/**
 * @brief Draw the help screen
 *
 * @param help The help screen state
 * @param elapsedUs The time elapsed since the last function call
 */
void drawHelp(helpPageVars_t* help, int32_t elapsedUs)
{
    // Set the title
    help->bgMenu->title = help->pages[help->helpIdx].title;

    // Draw background, without animation
    drawMenuMega(help->bgMenu, help->menuRenderer, 0);

    font_t* f = help->menuRenderer->menuFont;

    // Draw text
    paletteColor_t textColor   = c555;
    paletteColor_t shadowColor = c000;
    int16_t xOff               = TEXT_MARGIN_L + 1;
    int16_t yOff               = TEXT_MARGIN_U + 1;
    drawTextWordWrap(f, shadowColor, help->pages[help->helpIdx].text, &xOff, &yOff, TFT_WIDTH - TEXT_MARGIN_R + 1,
                     TFT_HEIGHT + 1);
    xOff = TEXT_MARGIN_L;
    yOff = TEXT_MARGIN_U;
    drawTextWordWrap(f, textColor, help->pages[help->helpIdx].text, &xOff, &yOff, TFT_WIDTH - TEXT_MARGIN_R,
                     TFT_HEIGHT);

    // Draw page numbers
    char pageText[32];
    snprintf(pageText, sizeof(pageText) - 1, "%" PRId32 "/%" PRId32 "", 1 + help->helpIdx, help->numPages);

    int16_t tWidth = textWidth(f, pageText);
    drawText(f, shadowColor, pageText, TFT_WIDTH - 50 - tWidth + 1, TFT_HEIGHT - f->height + 4);
    drawText(f, textColor, pageText, TFT_WIDTH - 50 - tWidth, TFT_HEIGHT - f->height + 3);

    // Blink the arrows
    help->arrowBlinkTimer += elapsedUs;
    while (help->arrowBlinkTimer >= ARROW_BLINK_PERIOD)
    {
        help->arrowBlinkTimer -= ARROW_BLINK_PERIOD;
    }

    if (help->arrowBlinkTimer < (ARROW_BLINK_PERIOD / 2))
    {
        // Draw arrows to indicate this can be scrolled
        if (0 != help->helpIdx)
        {
            // Draw left arrow if not on the first page
            drawText(f, shadowColor, "<", 1, 1 + (TFT_HEIGHT - f->height) / 2);
            drawText(f, textColor, "<", 0, (TFT_HEIGHT - f->height) / 2);
        }

        if ((help->numPages - 1) != help->helpIdx)
        {
            // Draw right arrow if not on the last page
            drawText(f, shadowColor, ">", 1 + TFT_WIDTH - textWidth(f, ">"), 1 + (TFT_HEIGHT - f->height) / 2);
            drawText(f, textColor, ">", TFT_WIDTH - textWidth(f, ">"), (TFT_HEIGHT - f->height) / 2);
        }
    }
}

/**
 * @brief Handle a button event on the help screen
 *
 * @param help The help screen state
 * @param evt The button event to handle
 * @return true to exit the help screen, false to remain on it
 */
bool buttonHelp(helpPageVars_t* help, buttonEvt_t* evt)
{
    if (evt->down)
    {
        switch (evt->button)
        {
            case PB_B:
            case PB_LEFT:
            {
                if (help->helpIdx > 0)
                {
                    help->helpIdx--;
                }
                else
                {
                    return true;
                }
                break;
            }
            case PB_A:
            case PB_RIGHT:
            {
                if (help->helpIdx < help->numPages - 1)
                {
                    help->helpIdx++;
                }
                else
                {
                    return true;
                }
                break;
            }
            case PB_SELECT:
            case PB_START:
            {
                // Exit the help
                return true;
            }
            case PB_UP:
            case PB_DOWN:
            default:
            {
                // Do nothing
                break;
            }
        }
    }
    return false;
}
