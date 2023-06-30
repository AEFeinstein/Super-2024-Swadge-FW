//==============================================================================
// Includes
//==============================================================================

#include "menuRenderer.h"
#include "hdw-tft.h"
#include "shapes.h"
#include "fill.h"

//==============================================================================
// Defines
//==============================================================================

#define CORNER_THICKNESS    2
#define CORNER_LENGTH       7
#define FILL_OFFSET         4
#define TEXT_OFFSET         6
#define ROW_SPACING         3
#define TOP_LINE_SPACING    3
#define TOP_LINE_THICKNESS  1
#define ITEMS_PER_PAGE      5
#define PAGE_ARROW_X_OFFSET 60
#define PAGE_ARROW_Y_OFFSET 5
#define Y_SECTION_MARGIN    20

//==============================================================================
// Function Prototypes
//==============================================================================

static void drawMenuText(menuRender_t* renderer, const char* text, int16_t x, int16_t y, bool isSelected,
                         bool leftArrow, bool rightArrow, bool doubleArrows);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief TODO doc
 *
 * @param menuFont
 * @return menuRender_t*
 */
menuRender_t* initMenuRenderer(font_t* menuFont)
{
    menuRender_t* renderer = calloc(1, sizeof(menuRender_t));
    renderer->font         = menuFont;
    loadWsg("mnuArrow.wsg", &renderer->arrow, false);
    loadWsg("mnuArrowS.wsg", &renderer->arrowS, false);
    return renderer;
}

/**
 * @brief TODO doc
 *
 * @param renderer
 */
void deinitMenuRenderer(menuRender_t* renderer)
{
    freeWsg(&renderer->arrow);
    freeWsg(&renderer->arrowS);
    free(renderer);
}

/**
 * @brief TODO doc
 *
 * @param renderer
 * @param text
 * @param x
 * @param y
 * @param isSelected
 * @param leftArrow
 * @param rightArrow
 * @param doubleArrows
 */
static void drawMenuText(menuRender_t* renderer, const char* text, int16_t x, int16_t y, bool isSelected,
                         bool leftArrow, bool rightArrow, bool doubleArrows)
{
    // Pick colors based on selection
    paletteColor_t cornerColor  = c411;
    paletteColor_t textColor    = c511;
    paletteColor_t topLineColor = c211;
    if (isSelected)
    {
        cornerColor  = c532;
        textColor    = c554;
        topLineColor = c422;
    }

    // Helper dimensions
    int16_t tWidth  = textWidth(renderer->font, text);
    int16_t tHeight = renderer->font->height;

    // Upper left corner
    fillDisplayArea(x, y, //
                    x + CORNER_LENGTH, y + CORNER_THICKNESS, cornerColor);
    fillDisplayArea(x, y, //
                    x + CORNER_THICKNESS, y + CORNER_LENGTH, cornerColor);

    // Upper right corner
    fillDisplayArea(x + tWidth + (2 * TEXT_OFFSET) - CORNER_LENGTH, y, //
                    x + (2 * TEXT_OFFSET) + tWidth, y + CORNER_THICKNESS, cornerColor);
    fillDisplayArea(x + tWidth + (2 * TEXT_OFFSET) - CORNER_THICKNESS, y, //
                    x + (2 * TEXT_OFFSET) + tWidth, y + CORNER_LENGTH, cornerColor);

    // Lower left corner
    fillDisplayArea(x, y + tHeight + (2 * TEXT_OFFSET) - CORNER_THICKNESS, //
                    x + CORNER_LENGTH, y + tHeight + (2 * TEXT_OFFSET), cornerColor);
    fillDisplayArea(x, y + tHeight + (2 * TEXT_OFFSET) - CORNER_LENGTH, //
                    x + CORNER_THICKNESS, y + tHeight + (2 * TEXT_OFFSET), cornerColor);

    // Lower right corner
    fillDisplayArea(x + tWidth + (2 * TEXT_OFFSET) - CORNER_LENGTH,
                    y + tHeight + (2 * TEXT_OFFSET) - CORNER_THICKNESS, //
                    x + (2 * TEXT_OFFSET) + tWidth, y + tHeight + (2 * TEXT_OFFSET), cornerColor);
    fillDisplayArea(x + tWidth + (2 * TEXT_OFFSET) - CORNER_THICKNESS,
                    y + tHeight + (2 * TEXT_OFFSET) - CORNER_LENGTH, //
                    x + (2 * TEXT_OFFSET) + tWidth, y + tHeight + (2 * TEXT_OFFSET), cornerColor);

    // Top line
    fillDisplayArea(x + CORNER_LENGTH + TOP_LINE_SPACING, y, //
                    x + tWidth + (2 * TEXT_OFFSET) - CORNER_LENGTH - TOP_LINE_SPACING, y + TOP_LINE_THICKNESS,
                    topLineColor);

    // Bottom line
    fillDisplayArea(x + CORNER_LENGTH + TOP_LINE_SPACING, y + tHeight + (2 * TEXT_OFFSET) - TOP_LINE_THICKNESS, //
                    x + tWidth + (2 * TEXT_OFFSET) - CORNER_LENGTH - TOP_LINE_SPACING, y + tHeight + (2 * TEXT_OFFSET),
                    topLineColor);

    // Fill the background for selected items
    if (isSelected)
    {
        fillDisplayArea(x + FILL_OFFSET, y + FILL_OFFSET, x + TEXT_OFFSET + tWidth + (TEXT_OFFSET - FILL_OFFSET),
                        y + TEXT_OFFSET + tHeight + (TEXT_OFFSET - FILL_OFFSET), c411);
    }

    // Draw the text
    drawText(renderer->font, textColor, text, x + TEXT_OFFSET, y + TEXT_OFFSET);

    // Draw the left arrow, if applicable
    if (leftArrow)
    {
        wsg_t* arrow = &renderer->arrow;
        if (isSelected)
        {
            arrow = &renderer->arrowS;
        }
        int16_t arrowX = x + CORNER_THICKNESS - arrow->w;
        int16_t arrowY = y + TEXT_OFFSET + (tHeight / 2) - (arrow->h / 2);
        drawWsg(arrow, arrowX, arrowY, true, false, 0);

        // Draw another arrow if requested
        if (doubleArrows)
        {
            arrowX -= arrow->w;
            drawWsg(arrow, arrowX, arrowY, true, false, 0);
        }
    }

    // Draw the right arrow, if applicable
    if (rightArrow)
    {
        wsg_t* arrow = &renderer->arrow;
        if (isSelected)
        {
            arrow = &renderer->arrowS;
        }
        int16_t arrowX = x + (TEXT_OFFSET * 2) + tWidth - CORNER_THICKNESS;
        int16_t arrowY = y + TEXT_OFFSET + (tHeight / 2) - (arrow->h / 2);
        drawWsg(arrow, arrowX, arrowY, false, false, 0);

        // Draw another arrow if requested
        if (doubleArrows)
        {
            arrowX += arrow->w;
            drawWsg(arrow, arrowX, arrowY, false, false, 0);
        }
    }
}

/**
 * @brief TODO doc
 *
 * @param menu
 * @param renderer
 */
void drawMenuThemed(menu_t* menu, menuRender_t* renderer)
{
    // Clear everything first
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c100);

    // Find the start of the 'page'
    node_t* pageStart = menu->items->first;
    uint8_t pageIdx   = 0;

    node_t* curNode = menu->items->first;
    while (NULL != curNode)
    {
        if (curNode->val == menu->currentItem->val)
        {
            // Found it, stop!
            break;
        }
        else
        {
            curNode = curNode->next;
            pageIdx++;
            if (ITEMS_PER_PAGE <= pageIdx && NULL != curNode)
            {
                pageIdx   = 0;
                pageStart = curNode;
            }
        }
    }

    // Where to start drawing
    int16_t x = 20;
    int16_t y = Y_SECTION_MARGIN;

    // Draw a title
    drawText(renderer->font, c542, menu->title, x, y);
    y += renderer->font->height + Y_SECTION_MARGIN;

    if (menu->items->length > ITEMS_PER_PAGE)
    {
        // Draw UP page indicator
        int16_t arrowX = PAGE_ARROW_X_OFFSET;
        int16_t arrowY = y - renderer->arrow.h - PAGE_ARROW_Y_OFFSET;
        drawWsg(&renderer->arrow, arrowX, arrowY, false, false, 270);
    }

    // Draw a page-worth of items
    for (uint8_t itemIdx = 0; itemIdx < ITEMS_PER_PAGE; itemIdx++)
    {
        if (NULL != pageStart)
        {
            menuItem_t* item = (menuItem_t*)pageStart->val;
            bool isSelected  = (menu->currentItem->val == item);

            // Draw the label(s)
            if (item->minSetting != item->maxSetting)
            {
                // Create key and value label, then draw it
                char label[64] = {0};
                snprintf(label, sizeof(label) - 1, "%s: %d", item->label, item->currentSetting);
                drawMenuText(renderer, label, x, y, isSelected, item->currentSetting != item->minSetting,
                             item->currentSetting != item->maxSetting, false);
            }
            else if (item->label)
            {
                // Draw text with arrow if there is a submenu or this is the back button
                drawMenuText(renderer, item->label, x, y, isSelected, (mnuBackStr == item->label),
                             (NULL != item->subMenu), true);
            }
            else if (item->options)
            {
                // Draw text with arrows
                drawMenuText(renderer, item->options[item->currentOpt], x, y, isSelected, true, true, false);
            }

            // Move to the next item
            pageStart = pageStart->next;
        }

        // Move to the next row
        y += (renderer->font->height + (TEXT_OFFSET * 2) + ROW_SPACING);
    }

    y -= ROW_SPACING;
    if (menu->items->length > ITEMS_PER_PAGE)
    {
        // TODO Draw DOWN page indicator
        int16_t arrowX = PAGE_ARROW_X_OFFSET;
        int16_t arrowY = y + PAGE_ARROW_Y_OFFSET;
        drawWsg(&renderer->arrow, arrowX, arrowY, false, false, 90);
    }
}
