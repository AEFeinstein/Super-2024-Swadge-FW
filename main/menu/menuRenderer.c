#include "menuRenderer.h"

#include "hdw-tft.h"
#include "shapes.h"
#include "fill.h"

static void drawMenuText(menuRender_t* renderer, const char* text, int16_t x, int16_t y, bool isSelected,
                         bool leftArrow, bool rightArrow);

/**
 * Render a menu to the TFT. This should be called each main loop so that menu
 * animations are smooth. It will draw over the entire display, but may be drawn
 * over afterwards.
 *
 * @param menu The menu to render
 * @param font The font to render the menu with
 */
void drawMenu(menu_t* menu, font_t* font)
{
    // Clear everything first
    clearPxTft();

#define ITEMS_PER_PAGE 5
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

    int16_t x = 20;
    int16_t y = 20;

    // Draw an underlined title
    drawText(font, c555, menu->title, x, y);
    y += (font->height + 2);
    drawLine(x, y, x + textWidth(font, menu->title), y, c555, 0);
    y += 3;

    // Draw page indicators
    if (menu->items->length > ITEMS_PER_PAGE)
    {
        drawText(font, c555, "~UP~", x, y);
    }
    y += (font->height + 2);

    // Draw a page-worth of items
    for (uint8_t itemIdx = 0; itemIdx < ITEMS_PER_PAGE; itemIdx++)
    {
        if (NULL != pageStart)
        {
            menuItem_t* item = (menuItem_t*)pageStart->val;
            // Draw an indicator if the current item is selected
            if (menu->currentItem->val == item)
            {
                drawText(font, c555, "X", 0, y);
            }

            // Draw the label(s)
            if (item->minSetting != item->maxSetting)
            {
                char label[64] = {0};
                snprintf(label, sizeof(label) - 1, "< %s: %d >", item->label, item->currentSetting);
                drawText(font, c555, label, x, y);
            }
            else if (item->label)
            {
                if (item->subMenu)
                {
                    char label[64] = {0};
                    snprintf(label, sizeof(label) - 1, "%s >>", item->label);
                    drawText(font, c555, label, x, y);
                }
                else
                {
                    drawText(font, c555, item->label, x, y);
                }
            }
            else if (item->options)
            {
                char label[64] = {0};
                snprintf(label, sizeof(label) - 1, "< %s >", item->options[item->currentOpt]);
                drawText(font, c555, label, x, y);
            }

            // Move to the next item
            pageStart = pageStart->next;
        }

        // Move to the next row
        y += (font->height + 2);
    }

    // Draw page indicators
    if (menu->items->length > ITEMS_PER_PAGE)
    {
        drawText(font, c555, "~DOWN~", x, y);
    }
    y += (font->height + 2);
}

// Defines for dimensions
#define CORNER_THICKNESS   2
#define CORNER_LENGTH      7
#define FILL_OFFSET        4
#define TEXT_OFFSET        8
#define ROW_SPACING        2
#define TOP_LINE_SPACING   3
#define TOP_LINE_THICKNESS 1

/**
 * @brief TODO
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
 * @param font
 * @param text
 * @param x
 * @param y
 * @param isSelected
 * @param leftArrow
 * @param rightArrow
 */
static void drawMenuText(menuRender_t* renderer, const char* text, int16_t x, int16_t y, bool isSelected,
                         bool leftArrow, bool rightArrow)
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
        fillDisplayArea(x + FILL_OFFSET, y + FILL_OFFSET, x + tWidth + FILL_OFFSET + TEXT_OFFSET,
                        y + tHeight + FILL_OFFSET + TEXT_OFFSET, c411);
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
        drawWsg(arrow, x, y + TEXT_OFFSET + (tHeight / 2) - (arrow->h / 2), true, false, 0);
    }

    // Draw the right arrow, if applicable
    if (rightArrow)
    {
        wsg_t* arrow = &renderer->arrow;
        if (isSelected)
        {
            arrow = &renderer->arrowS;
        }
        drawWsg(arrow, x + (TEXT_OFFSET * 2) + tWidth - arrow->w, y + TEXT_OFFSET + (tHeight / 2) - (arrow->h / 2),
                false, false, 0);
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

#define ITEMS_PER_PAGE 5

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

    int16_t x = 20;
    int16_t y = 12;

    // Draw a title
    drawText(renderer->font, c542, menu->title, x, y);
    y += (renderer->font->height + 2);

    if (menu->items->length > ITEMS_PER_PAGE)
    {
        // TODO Draw UP page indicator
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
                drawMenuText(renderer, label, x, y, isSelected, true, true);
            }
            else if (item->label)
            {
                // Draw text with arrow if there is a submenu or this is the back button
                drawMenuText(renderer, item->label, x, y, isSelected, (mnuBackStr == item->label),
                             (NULL != item->subMenu));
            }
            else if (item->options)
            {
                // Draw text with arrows
                drawMenuText(renderer, item->options[item->currentOpt], x, y, isSelected, true, true);
            }

            // Move to the next item
            pageStart = pageStart->next;
        }

        // Move to the next row
        y += (renderer->font->height + (TEXT_OFFSET * 2) + ROW_SPACING);
    }

    if (menu->items->length > ITEMS_PER_PAGE)
    {
        // TODO Draw DOWN page indicator
    }
}
