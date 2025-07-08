//==============================================================================
// Includes
//==============================================================================

#include "swadge2024.h"
#include "menuSimpleRenderer.h"
#include "menu_utils.h"

//==============================================================================
// Defines
//==============================================================================

#define MENU_MARGIN 16
#define FONT_MARGIN 8

//==============================================================================
// Function Prototypes
//==============================================================================

static void drawMenuText(menuSimpleRenderer_t* renderer, const char* text, int16_t x, int16_t y, bool isSelected,
                         bool leftArrow, bool rightArrow, bool doubleArrows);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initialize a and return a menu renderer.
 *
 * @param font  The font used to draw this menu
 * @param border The color for the background border
 * @param bg The color for the background
 * @param text The color for the text
 * @param rows The number of rows to display
 * @return A pointer to the menu renderer. This memory is allocated and must be freed with deinitMenuSimpleRenderer()
 */
menuSimpleRenderer_t* initMenuSimpleRenderer(font_t* font, paletteColor_t border, paletteColor_t bg,
                                             paletteColor_t text, int32_t rows)
{
    menuSimpleRenderer_t* renderer = heap_caps_calloc(1, sizeof(menuSimpleRenderer_t), MALLOC_CAP_SPIRAM);

    // Save or allocate font
    if (font)
    {
        renderer->font = font;
    }
    else
    {
        renderer->font = getSysFont();
    }

    renderer->numRows = rows;

    // Colors
    renderer->bgColor      = bg;
    renderer->borderColor  = border;
    renderer->rowTextColor = text;

    return renderer;
}

/**
 * @brief Deinitialize a menu renderer and free associated memory. This will not free the font passed into
 * initMenuSimpleRenderer()
 *
 * @param renderer The renderer to deinitialize. It must not be used after deinitialization.
 */
void deinitMenuSimpleRenderer(menuSimpleRenderer_t* renderer)
{
    heap_caps_free(renderer);
}

/**
 * @brief Draw a single line of themed menu text
 *
 * @param renderer The renderer to draw with
 * @param text The text to draw
 * @param x The X coordinate to draw the text at
 * @param y The Y coordinate to draw the text at
 * @param isSelected true if the text is selected, false if it is not
 * @param leftArrow true to draw an arrow to the left, used when a menu item has options or a submenu
 * @param rightArrow true to draw an arrow to the right, used when a menu item has options or a super-menu
 * @param doubleArrows true to draw double arrows instead of single arrows, used when entering or leaving submenus
 */
static void drawMenuText(menuSimpleRenderer_t* renderer, const char* text, int16_t x, int16_t y, bool isSelected,
                         bool leftArrow, bool rightArrow, bool doubleArrows)
{
    // Pick colors based on selection
    if (isSelected)
    {
        drawText(renderer->font, renderer->rowTextColor, "*", x, y);
    }

    // Draw the text
    drawText(renderer->font, renderer->rowTextColor, text, x + 10, y);

    // Draw the left arrow, if applicable
    if (leftArrow)
    {
        // Draw another arrow if requested
        if (doubleArrows)
        {
            // TODO DRAW DOUBLE LEFT
        }
        {
            // TODO DRAW LEFT
        }
    }

    // Draw the right arrow, if applicable
    if (rightArrow)
    {
        // Draw another arrow if requested
        if (doubleArrows)
        {
            // TODO DRAW DOUBLE RIGHT
        }
        else
        {
            // TODO DRAW RIGHT
        }
    }
}

/**
 * @brief Draw a themed menu to the display and control the LEDs
 *
 * @param menu The menu to draw
 * @param renderer The renderer to draw with
 */
void drawMenuSimple(menu_t* menu, menuSimpleRenderer_t* renderer)
{
    int32_t menuHeight = (renderer->numRows * (renderer->font->height + FONT_MARGIN)) + FONT_MARGIN;

    // Clear the background
    fillDisplayArea(MENU_MARGIN,                           //
                    TFT_HEIGHT - menuHeight - MENU_MARGIN, //
                    TFT_WIDTH - MENU_MARGIN,               //
                    TFT_HEIGHT - MENU_MARGIN,              //
                    renderer->bgColor);
    drawRect(MENU_MARGIN,                           //
             TFT_HEIGHT - menuHeight - MENU_MARGIN, //
             TFT_WIDTH - MENU_MARGIN,               //
             TFT_HEIGHT - MENU_MARGIN,              //
             renderer->borderColor);

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
            if (renderer->numRows <= pageIdx && NULL != curNode)
            {
                pageIdx   = 0;
                pageStart = curNode;
            }
        }
    }

    // Where to start drawing
    int16_t yOff = TFT_HEIGHT - menuHeight - FONT_MARGIN;

    if (menu->items->length > renderer->numRows)
    {
        // TODO Draw UP page
    }

    // Draw a page-worth of items
    for (uint8_t itemIdx = 0; itemIdx < renderer->numRows; itemIdx++)
    {
        if (NULL != pageStart)
        {
            menuItem_t* item = (menuItem_t*)pageStart->val;
            bool isSelected  = (menu->currentItem->val == item);

            char buffer[64]   = {0};
            const char* label = getMenuItemLabelText(buffer, sizeof(buffer), item);

            bool leftArrow    = menuItemHasPrev(item) || menuItemIsBack(item);
            bool rightArrow   = menuItemHasNext(item) || menuItemHasSubMenu(item);
            bool doubleArrows = menuItemIsBack(item) || menuItemHasSubMenu(item);

            drawMenuText(renderer, label, 2 * MENU_MARGIN, yOff, isSelected, leftArrow, rightArrow, doubleArrows);

            // Move to the next item
            pageStart = pageStart->next;
        }

        // Move to the next row
        yOff += (renderer->font->height + FONT_MARGIN);
    }

    if (menu->items->length > renderer->numRows)
    {
        // TODO Draw DOWN page
    }
}
