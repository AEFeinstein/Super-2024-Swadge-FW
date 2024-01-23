//==============================================================================
// Includes
//==============================================================================
#include "dialogBox.h"
#include "font.h"
#include "hdw-tft.h"
#include "macros.h"
#include "shapes.h"
#include "palette.h"
#include "fill.h"

#include <stdint.h>

//==============================================================================
// Defines
//==============================================================================
///< The space between the dialog box and the edge of the screen
#define DIALOG_MARGIN 5
///< The space between the edge of the dialog box and the content inside
#define DIALOG_PADDING 5
///< The space around the
#define DIALOG_ICON_MARGIN 5

///< The space between the edge of the button and the text or image inside
#define OPTION_PADDING 2
///< The space between each option
#define OPTION_MARGIN 5
///< The space between an option's icon and text
#define OPTION_ICON_MARGIN 3

///< The color of the dialog title text
#define COL_TITLE c000
///< The color of the dialog detail text
#define COL_DETAIL c000

///< The color of the dialog's background
#define COL_DIALOG_BG c555
///< The color of the dialog border outline
#define COL_DIALOG_BORDER c000

///< The background color of non-selected options
#define COL_OPTION_BG c333
///< The background color of the selected option
#define COL_OPTION_BG_SEL c455
///< The color of the dialog's border outline
#define COL_OPTION_BORDER c444

//==============================================================================
// Structs
//==============================================================================
/// @brief Holds information about how and where to draw the dialog box and its main components
typedef struct
{
    /// @brief The X coordinate of left side of the dialog
    uint16_t x;
    /// @brief The Y coordinate of the top edge of the dialog
    uint16_t y;
    /// @brief The total width of the dialog box
    uint16_t w;
    /// @brief The total height of the dialog box
    uint16_t h;

    /// @brief The X coordinate to draw the dialog title text at
    uint16_t titleX;
    /// @brief The Y coordinate to draw the dialog title text at
    uint16_t titleY;

    /// @brief The Y coordinate for the horizontal rule between the title and detail
    uint16_t ruleY;

    /// @brief The X coordinate to draw the dialog detail text at
    uint16_t detailX;
    /// @brief The Y coordinate to draw the dialog detail text at
    uint16_t detailY;

    /// @brief The X coordinate to draw the dialog icon at
    uint16_t iconX;
    /// @brief The Y coordinate to draw the dialog icon at
    uint16_t iconY;
} dialogDrawInfo_t;

/// @brief Holds information about how and where to draw a dialog box option
typedef struct
{
    /// @brief The X coordinate of the option's left edge
    uint16_t x;
    /// @brief The Y coordinate of the option's top edge
    uint16_t y;
    /// @brief The total width of the option button
    uint16_t w;
    /// @brief The total height of the option button
    uint16_t h;
    /// @brief The row of buttons this option is in, starting at 0 for the top row
    uint8_t row;

    /// @brief Whether this option button is disabled
    bool disabled;
    /// @brief Whether this option button is currently selected
    bool selected;
    /// @brief Whether this option button is currently being pressed
    bool pressed;

    const dialogBoxOption_t* option;
} optionDrawInfo_t;

//==============================================================================
// Static function declarations
//==============================================================================
static void layoutDialogBox(const dialogBox_t* dialogBox, const font_t* titleFont, const font_t* detailFont, uint16_t x,
                            uint16_t y, uint16_t w, uint16_t h, uint16_t r, dialogDrawInfo_t* dialogInfo,
                            optionDrawInfo_t* options);

//==============================================================================
// Function definitions
//==============================================================================
/**
 * @brief Allocate and return a new dialogBox_t with the given settings.
 *
 * The result must be deallocated with deinitDialogBox().
 *
 * @param title The title text of the dialog box
 * @param detail The body text of the dialog box
 * @param icon   The icon to display in the dialog box, if not NULL
 * @param cbFn   The callback function to be called when an option is selected
 * @return dialogBox_t* The newly allocated dialog box
 */
dialogBox_t* initDialogBox(const char* title, const char* detail, const wsg_t* icon, dialogBoxCbFn_t cbFn)
{
    dialogBox_t* dialogBox = calloc(1, sizeof(dialogBox_t));

    dialogBox->title  = title;
    dialogBox->detail = detail;
    dialogBox->icon   = icon;
    dialogBox->cbFn   = cbFn;

    return dialogBox;
}

/**
 * @brief Deallocate all memory associated with the given dialog box
 *
 * @param dialogBox The dialog box to deinitialize
 */
void deinitDialogBox(dialogBox_t* dialogBox)
{
    dialogBoxReset(dialogBox);
    free(dialogBox);
}

/**
 * @brief Add an option button to a dialog box
 *
 * @param dialogBox The dialog box to add the button to
 * @param label     The text of the option button
 * @param icon      The icon for this button, or NULL for no icon
 * @param hints     Any number of ::dialogOptionHint_t, combined with bitwise OR for multiple
 */
void dialogBoxAddOption(dialogBox_t* dialogBox, const char* label, const wsg_t* icon, dialogOptionHint_t hints)
{
    dialogBoxOption_t* option = malloc(sizeof(dialogBoxOption_t));
    option->label             = label;
    option->icon              = icon;
    option->hints             = hints;

    push(&dialogBox->options, option);

    if (NULL == dialogBox->selectedOption || OPTHINT_DEFAULT == (hints & OPTHINT_DEFAULT))
    {
        // Mark the option as selected if it's the first option, or hinted as the default
        dialogBox->selectedOption = dialogBox->options.last;
    }
}

/**
 * @brief Clear all options from the given dialog box
 *
 * @param dialogBox The dialog box to reset
 */
void dialogBoxReset(dialogBox_t* dialogBox)
{
    dialogBox->selectedOption = NULL;

    dialogBoxOption_t* item = NULL;
    while (NULL != (item = pop(&dialogBox->options)))
    {
        free(item);
    }
}

/**
 * @brief Calculates the location of the dialog box and all its child elements, returning them via \c dialogInfo and \c
 * options respectively.
 *
 * @param dialogBox The dialog box to arrange
 * @param titleFont The font to use for the dialog box title text
 * @param detailFont The font to use for the dialog box detail text
 * @param x The location of the left edge of the dialog box, or \c DIALOG_CENTER to center it horizontally
 * @param y The location of the top edge of the dialog box, or \c DIALOG_CENTER to center it vertically
 * @param w The width of the dialog box. \c DIALOG_AUTO may be used to automatically calculate the width, and
 *          optionally include a maximum width by bitwise-OR-ing it with the width.
 * @param h The height of the dialog box. \c DIALOG_AUTO may be used to automatically calculate the height
 * @param r The corner-radius of the dialog box.
 * @param[out] dialogInfo
 * @param[out] options
 */
static void layoutDialogBox(const dialogBox_t* dialogBox, const font_t* titleFont, const font_t* detailFont, uint16_t x,
                            uint16_t y, uint16_t w, uint16_t h, uint16_t r, dialogDrawInfo_t* dialogInfo,
                            optionDrawInfo_t* options)
{
    bool autoW   = (w & DIALOG_AUTO) == DIALOG_AUTO;
    bool autoH   = (h & DIALOG_AUTO) == DIALOG_AUTO;
    bool centerX = (x & DIALOG_CENTER) == DIALOG_CENTER;
    bool centerY = (y & DIALOG_CENTER) == DIALOG_CENTER;

    uint16_t maxW = TFT_WIDTH - DIALOG_MARGIN * 2;
    uint16_t maxH = TFT_HEIGHT - DIALOG_MARGIN * 2;

    // Remove the flags from the real values
    x &= ~DIALOG_CENTER;
    y &= ~DIALOG_CENTER;
    w &= ~DIALOG_AUTO;
    h &= ~DIALOG_AUTO;

    // If the width/height are given, use them as the max instead of the default
    if (w != 0)
    {
        maxW = w;
    }

    if (h != 0)
    {
        maxH = h;
    }

    uint16_t curX = DIALOG_PADDING;
    uint16_t curY = DIALOG_PADDING;

    // Measure text

    uint16_t titleH
        = textWordWrapHeight(titleFont, dialogBox->title, maxW - DIALOG_PADDING * 2, maxH - DIALOG_PADDING * 2);

    dialogInfo->titleX = curX;
    dialogInfo->titleY = curY;

    curY += titleH + 1;

    if (autoW)
    {
        // TODO maybe treat single-line texts differently here too?
        w = MIN(maxW, MAX(OPTION_MARGIN + textWidth(titleFont, dialogBox->title), w));
    }

    curY += 3;
    dialogInfo->ruleY = curY;

    curY += 3;

    uint16_t textOffset = 0;
    if (dialogBox->icon)
    {
        // If there's an icon, we draw it on the left side, and the text next to it.
        // If the text geoes past the icon, measure it... more...
        dialogInfo->iconX = curX;
        dialogInfo->iconY = curY;

        textOffset = dialogBox->icon->w + DIALOG_ICON_MARGIN;

        curX += textOffset;
    }
    else
    {
        dialogInfo->iconX = 0;
        dialogInfo->iconY = 0;
    }

    uint16_t detailH    = textWordWrapHeight(detailFont, dialogBox->detail, maxW - DIALOG_PADDING * 2 - textOffset,
                                             maxH - DIALOG_PADDING - curY);
    dialogInfo->detailX = curX;
    dialogInfo->detailY = curY;
    curY += detailH + 1;

    if (autoW)
    {
        w = MIN(maxW, MAX(OPTION_MARGIN + textOffset + textWidth(detailFont, dialogBox->detail), w));
    }

    curX = DIALOG_PADDING;
    curY += OPTION_MARGIN;

    uint8_t row                = 0;
    optionDrawInfo_t* drawInfo = options;
    for (node_t* node = dialogBox->options.first; NULL != node; node = node->next, ++drawInfo)
    {
        dialogBoxOption_t* option = node->val;
        drawInfo->option          = option;
        drawInfo->selected        = (node == dialogBox->selectedOption);
        drawInfo->pressed
            = drawInfo->selected
              && (dialogBox->holdA || (dialogBox->holdB && (OPTHINT_CANCEL == (option->hints & OPTHINT_CANCEL))));
        drawInfo->disabled = OPTHINT_DISABLED == (option->hints & OPTHINT_DISABLED);

        // go through and measure the things
        // if width is auto, we try to add the item to the width
        // if it doesn't fit, we add it to the height and reset width

        uint16_t itemW = OPTION_PADDING * 2 + 2;
        uint16_t itemH = OPTION_PADDING * 2 + detailFont->height + 1 + 2;

        if (option->icon)
        {
            itemW += option->icon->w + OPTION_ICON_MARGIN;

            // Account for the icon height, if it's greater than the font
            if (option->icon->h > titleFont->height + 1)
            {
                itemH += (option->icon->h - titleFont->height - 1);
            }
        }

        itemW += textWidth(detailFont, option->label);

        // Now we're done calculating the item width, arrange it in the dialog

        if (curY + itemH > maxH - DIALOG_PADDING)
        {
            /// Can't draw any more buttons, no room! Oh well
            break;
        }

        if (autoH)
        {
            h = MAX(h, curY + itemH + OPTION_MARGIN);
        }

        if (curX + itemW <= maxW - DIALOG_PADDING)
        {
            drawInfo->x   = curX;
            drawInfo->y   = curY;
            drawInfo->h   = itemH;
            drawInfo->w   = itemW;
            drawInfo->row = row;

            // The option fits on this line, advance X
            curX += itemW + OPTION_MARGIN * 2;

            if (autoW)
            {
                w = MAX(w, curX);
            }

            if (curX > maxW - DIALOG_PADDING)
            {
                ++row;
                curX = DIALOG_PADDING;
                curY += itemH + OPTION_MARGIN * 2;
            }
        }
        else
        {
            // The option doesn't fit, go to the next line
            ++row;
            curX = DIALOG_PADDING;
            curY += itemH + OPTION_MARGIN * 2;

            drawInfo->x   = curX;
            drawInfo->y   = curY;
            drawInfo->row = row;
        }
    }

    if (autoH)
    {
        h = MAX(h, curY);
    }

    //// Determine the final position and size of the dialog box, if not already decided
    if (centerX)
    {
        x = DIALOG_MARGIN + (TFT_WIDTH - DIALOG_MARGIN * 2 - w) / 2;
    }

    if (centerY)
    {
        y = DIALOG_MARGIN + (TFT_HEIGHT - DIALOG_MARGIN * 2 - h) / 2;
    }

    dialogInfo->x = x;
    dialogInfo->y = y;
    dialogInfo->w = w;
    dialogInfo->h = h;

    if (titleH <= titleFont->height + 1)
    {
        dialogInfo->titleX += (w - DIALOG_PADDING - textWidth(titleFont, dialogBox->title)) / 2;
    }

    // Adjust everything for the final X and Y
    dialogInfo->titleX += x;
    dialogInfo->titleY += y;
    dialogInfo->ruleY += y;
    dialogInfo->detailX += x;
    dialogInfo->detailY += y;
    dialogInfo->iconX += x;
    dialogInfo->iconY += y;

    for (optionDrawInfo_t* optionInfo = options; optionInfo < (options + dialogBox->options.length); optionInfo++)
    {
        optionInfo->x += x;
        optionInfo->y += y;
    }
}

/**
 * @brief Draw the dialog box
 *
 * @param dialogBox The dialog box to draw
 * @param titleFont The font to use for drawing the dialog box title text
 * @param detailFont The font to use for drawing the dialog box detail text
 * @param x The X coordinate of the left side of the dialog box, or \c DIALOG_CENTER to automatically center it
 * @param y The Y coordinate of the top of the dialog box, or \c DIALOG_CENTER to automatically center it
 * @param w The width of the dialog box, or if combined with \c DIALOG_AUTO, the maximum width of the dialog box
 * @param h The height of the dialog box, or if combined with \c DIALOG_AUTO, the maximum height of the dialog box
 * @param r The corner-radius of the dialog box
 */
void drawDialogBox(const dialogBox_t* dialogBox, const font_t* titleFont, const font_t* detailFont, uint16_t x,
                   uint16_t y, uint16_t w, uint16_t h, uint16_t r)
{
    dialogDrawInfo_t dialogInfo;
    optionDrawInfo_t optionInfos[dialogBox->options.length];

    layoutDialogBox(dialogBox, titleFont, detailFont, x, y, w, h, r, &dialogInfo, optionInfos);

    x = dialogInfo.x;
    y = dialogInfo.y;
    w = dialogInfo.w;
    h = dialogInfo.h;

    //// DRAW PHASE

    // Fill the background

    // Top line, excluding round corners
    fillDisplayArea(x + r, y, x + w - r, y + r, COL_DIALOG_BG);

    // Middle chunk, including sides
    fillDisplayArea(x, y + r, x + w, y + h - r, COL_DIALOG_BG);

    // Bottom line, excluding round corners
    fillDisplayArea(x + r, y + h - r, x + w - r, y + h, COL_DIALOG_BG);

    // Round corners
    drawCircleFilled(x + r, y + r, r - 1, COL_DIALOG_BG);         // Top-left
    drawCircleFilled(x + w - r, y + r, r - 1, COL_DIALOG_BG);     // Top-right
    drawCircleFilled(x + r, y + h - r, r - 1, COL_DIALOG_BG);     // Bottom-left
    drawCircleFilled(x + w - r, y + h - r, r - 1, COL_DIALOG_BG); // Bottom-right

    // Borders
    drawLineFast(x + r, y, x + w - r, y, COL_DIALOG_BORDER);         // Top
    drawLineFast(x, y + r, x, y + h - r, COL_DIALOG_BORDER);         // Left
    drawLineFast(x + w, y + r, x + w, y + h - r, COL_DIALOG_BORDER); // Right
    drawLineFast(x + r, y + h, x + w - r, y + h, COL_DIALOG_BORDER); // Bottom

    // Round borders
    // Sectors:
    // BR, BL, TL(?), TR(?)
    drawCircleQuadrants(x + r, y + r, r, false, false, true, false, COL_DIALOG_BORDER);         // Top-left
    drawCircleQuadrants(x + w - r, y + r, r, false, false, false, true, COL_DIALOG_BORDER);     // Top-right
    drawCircleQuadrants(x + r, y + h - r, r, false, true, false, false, COL_DIALOG_BORDER);     // Bottom-left
    drawCircleQuadrants(x + w - r, y + h - r, r, true, false, false, false, COL_DIALOG_BORDER); // Bottom-right

    //// Draw Text
    int16_t textX = dialogInfo.titleX;
    int16_t textY = dialogInfo.titleY;

    // Draw title
    drawTextWordWrap(titleFont, COL_TITLE, dialogBox->title, &textX, &textY, x + w - DIALOG_PADDING,
                     y + h - DIALOG_PADDING);

    // Draw a line under the title
    drawLineFast(x + DIALOG_PADDING, dialogInfo.ruleY, x + w - DIALOG_PADDING, dialogInfo.ruleY, COL_DIALOG_BORDER);

    textX = dialogInfo.detailX;
    textY = dialogInfo.detailY;

    if (dialogBox->icon)
    {
        drawWsgSimple(dialogBox->icon, dialogInfo.iconX, dialogInfo.iconY);
    }

    // Draw the detail text
    drawTextWordWrap(detailFont, COL_DETAIL, dialogBox->detail, &textX, &textY, x + w - DIALOG_PADDING,
                     y + h - DIALOG_PADDING);

    // Loop over buttons and draw them
    for (optionDrawInfo_t* drawInfo = optionInfos; drawInfo < (optionInfos + dialogBox->options.length); ++drawInfo)
    {
        const dialogBoxOption_t* option = drawInfo->option;

        // Button background
        if (drawInfo->disabled)
        {
            shadeDisplayArea(drawInfo->x, drawInfo->y, drawInfo->x + drawInfo->w, drawInfo->y + drawInfo->h, 3, c444);
        }
        else
        {
            fillDisplayArea(drawInfo->x, drawInfo->y, drawInfo->x + drawInfo->w, drawInfo->y + drawInfo->h,
                            drawInfo->selected ? (drawInfo->pressed ? COL_DETAIL : COL_OPTION_BG_SEL) : COL_OPTION_BG);
        }

        paletteColor_t borderCol = drawInfo->selected ? (drawInfo->pressed ? c555 : c333) : COL_OPTION_BORDER;

        // Button border - don't draw the corner pixels
        // Top
        drawLineFast(drawInfo->x, drawInfo->y - 1, drawInfo->x + drawInfo->w, drawInfo->y - 1, borderCol);
        // Left
        drawLineFast(drawInfo->x - 1, drawInfo->y, drawInfo->x - 1, drawInfo->y + drawInfo->h, borderCol);
        // Right
        drawLineFast(drawInfo->x + drawInfo->w + 1, drawInfo->y, drawInfo->x + drawInfo->w + 1,
                     drawInfo->y + drawInfo->h, borderCol);
        // Bottom
        drawLineFast(drawInfo->x, drawInfo->y + drawInfo->h + 1, drawInfo->x + drawInfo->w,
                     drawInfo->y + drawInfo->h + 1, borderCol);

        // Draw the actual label text
        drawText(detailFont, drawInfo->disabled ? c333 : (drawInfo->pressed ? COL_OPTION_BG : COL_DETAIL),
                 option->label, drawInfo->x + OPTION_PADDING, drawInfo->y + OPTION_PADDING);
    }
}

/**
 * @brief Handle button presses for the dialog box
 *
 * @param dialogBox The dialog box to update
 * @param evt       The button event to be handled by the dialog box
 */
void dialogBoxButton(dialogBox_t* dialogBox, const buttonEvt_t* evt)
{
    switch (evt->button)
    {
        case PB_UP:
        {
            if (evt->down)
            {
                dialogBox->holdB = false;
                // Pick first item that's not disabled
                for (node_t* node = dialogBox->options.first; NULL != node; node = node->next)
                {
                    dialogBoxOption_t* option = node->val;
                    if (0 == (option->hints & OPTHINT_DISABLED))
                    {
                        dialogBox->selectedOption = node;
                        break;
                    }
                }
            }
            break;
        }
        case PB_DOWN:
        {
            if (evt->down)
            {
                dialogBox->holdB = false;
                // Pick last item that's not disabled
                for (node_t* node = dialogBox->options.last; NULL != node; node = node->prev)
                {
                    dialogBoxOption_t* option = node->val;
                    if (0 == (option->hints & OPTHINT_DISABLED))
                    {
                        dialogBox->selectedOption = node;
                        break;
                    }
                }
            }
            break;
        }

        case PB_LEFT:
        {
            if (evt->down)
            {
                dialogBox->holdB = false;
                if (!dialogBox->selectedOption)
                {
                    dialogBox->selectedOption = dialogBox->options.first;
                    break;
                }

                // Pick previous item that's not disabled, wrapping around
                for (node_t* node = dialogBox->selectedOption->prev; node && node != dialogBox->selectedOption;
                     node         = (node == dialogBox->options.first) ? dialogBox->options.last : node->prev)
                {
                    dialogBoxOption_t* option = node->val;
                    if (0 == (option->hints & OPTHINT_DISABLED))
                    {
                        dialogBox->selectedOption = node;
                        break;
                    }
                }
            }
            break;
        }

        case PB_RIGHT:
        {
            if (evt->down)
            {
                dialogBox->holdB = false;
                if (!dialogBox->selectedOption)
                {
                    dialogBox->selectedOption = dialogBox->options.last;
                    break;
                }

                // Pick next item that's not disabled, wrapping around until we get to the start
                for (node_t* node = dialogBox->selectedOption->next; node && node != dialogBox->selectedOption;
                     node         = (node == dialogBox->options.last) ? dialogBox->options.first : node->next)
                {
                    dialogBoxOption_t* option = node->val;
                    if (0 == (option->hints & OPTHINT_DISABLED))
                    {
                        dialogBox->selectedOption = node;
                        break;
                    }
                }
            }
            break;
        }

        case PB_A:
        {
            dialogBox->holdB = false;
            if (dialogBox->selectedOption)
            {
                if (evt->down)
                {
                    dialogBox->holdA = true;
                }
                else
                {
                    if (dialogBox->holdA)
                    {
                        dialogBox->holdA = false;
                        if (dialogBox->cbFn)
                        {
                            dialogBox->cbFn(((dialogBoxOption_t*)dialogBox->selectedOption->val)->label);
                            return;
                        }
                    }
                }
            }
            break;
        }

        case PB_B:
        {
            if (dialogBox->holdA)
            {
                // Cancel a held A-press if B is then pressed
                // But don't treat it as a B press, just return after the release
                // and do nothing on the press to cancel normal B behavior
                if (!evt->down)
                {
                    dialogBox->holdA = false;
                }
                return;
            }

            if (dialogBox->selectedOption)
            {
                dialogBoxOption_t* curOption = dialogBox->selectedOption->val;

                if (OPTHINT_CANCEL == (curOption->hints & OPTHINT_CANCEL))
                {
                    // This is the cancel option
                    if (evt->down)
                    {
                        // Just highlight the box until they release the button
                        dialogBox->holdB = true;
                    }
                    else
                    {
                        if (dialogBox->holdB)
                        {
                            dialogBox->holdB = false;
                            if (dialogBox->cbFn)
                            {
                                dialogBox->cbFn(curOption->label);
                                return;
                            }
                        }
                        break;
                    }
                }
                else
                {
                    if (evt->down)
                    {
                        dialogBox->holdB = true;
                    }
                    else if (dialogBox->holdB)
                    {
                        dialogBox->holdB = false;
                        // When B is released, go to the cancel option
                        // Pick item with "CANCEL" option
                        for (node_t* node = dialogBox->options.first; NULL != node; node = node->next)
                        {
                            dialogBoxOption_t* option = node->val;
                            if (OPTHINT_CANCEL == (option->hints & OPTHINT_CANCEL))
                            {
                                dialogBox->selectedOption = node;
                                break;
                            }
                        }
                    }
                }
            }
            break;
        }

        case PB_SELECT:
        case PB_START:
            break;
    }
}
