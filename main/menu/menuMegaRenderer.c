//==============================================================================
// Includes
//==============================================================================

#include <string.h>
#include "hdw-battmon.h"
#include "menu_utils.h"
#include "menuMegaRenderer.h"

//==============================================================================
// Defines
//==============================================================================

#define ITEMS_PER_PAGE 5

#define Y_SECTION_MARGIN 16

#define ITEM_START 55

#define ROW_MARGIN 1

//==============================================================================
// Function Prototypes
//==============================================================================

static void drawMenuText(menuMegaRenderer_t* renderer, const char* text, int16_t x, int16_t y, bool isSelected,
                         bool leftArrow, bool rightArrow, bool doubleArrows);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initialize a and return a menu renderer.
 *
 * @param titleFont The font used to draw the title, preferably RIGHTEOUS_150_FONT. If this is NULL it will be
 * allocated by the renderer in SPIRAM.
 * @param titleFontOutline The outline font used to draw the title. If this is NULL it will be allocated by the renderer
 * in SPIRAM.
 * @param menuFont The font used to draw this menu, preferably RODIN_EB_FONT. If this is NULL it will be allocated by
 * the renderer in SPIRAM.
 * @return A pointer to the menu renderer. This memory is allocated and must be freed with deinitMenuMegaRenderer()
 * when done
 */
menuMegaRenderer_t* initMenuMegaRenderer(font_t* titleFont, font_t* titleFontOutline, font_t* menuFont)
{
    menuMegaRenderer_t* renderer = heap_caps_calloc(1, sizeof(menuMegaRenderer_t), MALLOC_CAP_SPIRAM);

    loadWsg(MMM_BACK_WSG, &renderer->back, true);
    loadWsg(MMM_BG_WSG, &renderer->bg, true);
    loadWsg(MMM_BODY_WSG, &renderer->body, true);
    loadWsg(MMM_DOWN_WSG, &renderer->down, true);
    loadWsg(MMM_ITEM_WSG, &renderer->item, true);
    loadWsg(MMM_ITEM_SEL_WSG, &renderer->item_sel, true);
    loadWsg(MMM_NEXT_WSG, &renderer->next, true);
    loadWsg(MMM_PREV_WSG, &renderer->prev, true);
    loadWsg(MMM_SUBMENU_WSG, &renderer->submenu, true);
    loadWsg(MMM_UP_WSG, &renderer->up, true);

    // Save or allocate title font
    if (NULL == titleFont)
    {
        renderer->titleFont = heap_caps_calloc(1, sizeof(font_t), MALLOC_CAP_SPIRAM);
        loadFont(RIGHTEOUS_150_FONT, renderer->titleFont, true);
        renderer->titleFontAllocated = true;
    }
    else
    {
        renderer->titleFont          = titleFont;
        renderer->titleFontAllocated = false;
    }

    // Save or allocate title font outline
    if (NULL == titleFontOutline)
    {
        renderer->titleFontOutline = heap_caps_calloc(1, sizeof(font_t), MALLOC_CAP_SPIRAM);
        makeOutlineFont(renderer->titleFont, renderer->titleFontOutline, true);
        renderer->titleFontOutlineAllocated = true;
    }
    else
    {
        renderer->titleFontOutline          = titleFontOutline;
        renderer->titleFontOutlineAllocated = false;
    }

    // Save or allocate menu font
    if (NULL == menuFont)
    {
        renderer->menuFont = heap_caps_calloc(1, sizeof(font_t), MALLOC_CAP_SPIRAM);
        loadFont(RODIN_EB_FONT, renderer->menuFont, true);
        renderer->menuFontAllocated = true;
    }
    else
    {
        renderer->menuFont          = menuFont;
        renderer->menuFontAllocated = false;
    }

    // Load battery images
    loadWsg(BATT_1_WSG, &renderer->batt[0], true);
    loadWsg(BATT_2_WSG, &renderer->batt[1], true);
    loadWsg(BATT_3_WSG, &renderer->batt[2], true);
    loadWsg(BATT_4_WSG, &renderer->batt[3], true);

    // Initialize LEDs
    setLeds(renderer->leds, CONFIG_NUM_LEDS);

    // LEDs on by default
    renderer->ledsOn = true;

    return renderer;
}

/**
 * @brief Deinitialize a menu renderer and free associated memory. This will not free the font passed into
 * initMenuMegaRenderer()
 *
 * @param renderer The renderer to deinitialize. It must not be used after deinitialization.
 */
void deinitMenuMegaRenderer(menuMegaRenderer_t* renderer)
{
    freeWsg(&renderer->batt[0]);
    freeWsg(&renderer->batt[1]);
    freeWsg(&renderer->batt[2]);
    freeWsg(&renderer->batt[3]);

    // Free fonts if allocated
    if (renderer->titleFontAllocated)
    {
        freeFont(renderer->titleFont);
        heap_caps_free(renderer->titleFont);
    }
    if (renderer->titleFontOutlineAllocated)
    {
        freeFont(renderer->titleFontOutline);
        heap_caps_free(renderer->titleFontOutline);
    }
    if (renderer->menuFontAllocated)
    {
        freeFont(renderer->menuFont);
        heap_caps_free(renderer->menuFont);
    }

    freeWsg(&renderer->back);
    freeWsg(&renderer->bg);
    freeWsg(&renderer->body);
    freeWsg(&renderer->down);
    freeWsg(&renderer->item);
    freeWsg(&renderer->item_sel);
    freeWsg(&renderer->next);
    freeWsg(&renderer->prev);
    freeWsg(&renderer->submenu);
    freeWsg(&renderer->up);

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
static void drawMenuText(menuMegaRenderer_t* renderer, const char* text, int16_t x, int16_t y, bool isSelected,
                         bool leftArrow, bool rightArrow, bool doubleArrows)
{
    // Draw background for the menu item
    if (isSelected)
    {
        drawWsgSimple(&renderer->item_sel, x, y);
    }
    else
    {
        drawWsgSimple(&renderer->item, x, y);
    }

    // Draw the text
    if (isSelected && textWidth(renderer->menuFont, text) > renderer->item_sel.w)
    {
        drawTextMarquee(renderer->menuFont, c555, text, x, y, x + renderer->item_sel.w,
                        &renderer->selectedMarqueeTimer);
    }
    else
    {
        drawTextEllipsize(renderer->menuFont, c555, text, x, y, renderer->item.w, false);
    }

    // Draw the left arrow, if applicable
    if (leftArrow)
    {
        const wsg_t* arrow = &renderer->prev;
        if (doubleArrows)
        {
            arrow = &renderer->back;
        }
        int16_t drawY = y + (renderer->item.h - arrow->h) / 2 + 1;
        drawWsgSimple(arrow, x - 10, drawY);
    }

    // Draw the right arrow, if applicable
    if (rightArrow)
    {
        const wsg_t* arrow = &renderer->next;
        if (doubleArrows)
        {
            arrow = &renderer->submenu;
        }
        int16_t drawY = y + (renderer->item.h - arrow->h) / 2 + 1;
        drawWsgSimple(arrow, x + renderer->item.w - arrow->w - 15, drawY);
    }
}

/**
 * @brief Draw a themed menu to the display and control the LEDs
 *
 * @param menu The menu to draw
 * @param renderer The renderer to draw with
 * @param elapsedUs The time elapsed since this function was last called, for LED animation
 */
void drawMenuMega(menu_t* menu, menuMegaRenderer_t* renderer, int64_t elapsedUs)
{
    // Only poll the battery if requested
    if (menu->showBattery)
    {
        // Read battery every 10s
        menu->batteryReadTimer -= elapsedUs;
        if (0 >= menu->batteryReadTimer)
        {
            menu->batteryReadTimer += 10000000;
            menu->batteryLevel = readBattmon();
        }
    }

    // Run a timer to blink up and down page arrows
    if (renderer->pageArrowTimer <= 0)
    {
        renderer->pageArrowTimer += 1000000;
    }
    else
    {
        renderer->pageArrowTimer -= elapsedUs;
    }

    if (renderer->ledsOn)
    {
        // Set LEDs
        setLeds(renderer->leds, CONFIG_NUM_LEDS);
    }

    renderer->selectedMarqueeTimer += elapsedUs;

    // Clear the background
    drawWsgTile(&renderer->bg, 0, 0);

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
    int16_t y = Y_SECTION_MARGIN;

    // Draw a title
    // Draw the menu text
    drawText(renderer->titleFont, c555, menu->title, 20, y);
    // Outline the menu text
    drawText(renderer->titleFontOutline, c000, menu->title, 20, y);

    // Move to drawing the rows
    // y += renderer->titleFont->height + Y_SECTION_MARGIN;
    y = ITEM_START;

    drawWsgSimple(&renderer->body, 0, 0);

    if (menu->items->length > ITEMS_PER_PAGE && renderer->pageArrowTimer > 500000)
    {
        // Draw UP page indicator
        drawWsgSimple(&renderer->up, 222, 38);
    }

    // Draw a page-worth of items
    for (uint8_t itemIdx = 0; itemIdx < ITEMS_PER_PAGE; itemIdx++)
    {
        if (NULL != pageStart)
        {
            menuItem_t* item = (menuItem_t*)pageStart->val;
            bool isSelected  = (menu->currentItem->val == item);

            // // If there's a new selected item
            // if (isSelected && renderer->selectedItem != item)
            // {
            //     // Save it
            //     renderer->selectedItem = item;
            //     // Bounce the selected item
            //     renderer->selectedBounceIdx    = 1;
            //     renderer->selectedMarqueeTimer = 0;
            // }
            // else if (isSelected)
            // {
            //     // If the selected option has changed
            //     if (menuItemHasOptions(item) || menuItemIsSetting(item))
            //     {
            //         int32_t value = (item->options) ? item->currentOpt : item->currentSetting;
            //         if (value != renderer->selectedValue)
            //         {
            //             renderer->selectedMarqueeTimer = 0;
            //             renderer->selectedValue        = value;
            //         }
            //     }
            // }

            char buffer[64]   = {0};
            const char* label = getMenuItemLabelText(buffer, sizeof(buffer), item);

            bool leftArrow    = menuItemHasPrev(item) || menuItemIsBack(item);
            bool rightArrow   = menuItemHasNext(item) || menuItemHasSubMenu(item);
            bool doubleArrows = menuItemIsBack(item) || menuItemHasSubMenu(item);

            drawMenuText(renderer, label, 22, y, isSelected, leftArrow, rightArrow, doubleArrows);

            // Move to the next item
            pageStart = pageStart->next;
        }

        // Move to the next row
        y += renderer->item.h + ROW_MARGIN;
    }

    if (menu->items->length > ITEMS_PER_PAGE && renderer->pageArrowTimer > 500000)
    {
        // Draw DOWN page indicator
        drawWsgSimple(&renderer->down, 222, 221);
    }

    // Only draw the battery if requested
    if (menu->showBattery)
    {
        // Draw the battery indicator depending on the last read value
        wsg_t* toDraw = NULL;
        // 872 is full
        if (menu->batteryLevel == 0 || menu->batteryLevel > 741)
        {
            toDraw = &renderer->batt[3];
        }
        else if (menu->batteryLevel > 695)
        {
            toDraw = &renderer->batt[2];
        }
        else if (menu->batteryLevel > 652)
        {
            toDraw = &renderer->batt[1];
        }
        else // 452 is dead
        {
            toDraw = &renderer->batt[0];
        }

        drawWsg(toDraw, 224, 11, false, false, 0);
    }
}

/**
 * @brief Set the renderer's LEDs to be on or off
 *
 * @param renderer The renderer to set
 * @param ledsOn true to animate the LEDs, false to keep them off
 */
void setMegaLedsOn(menuMegaRenderer_t* renderer, bool ledsOn)
{
    renderer->ledsOn = ledsOn;
    if (false == ledsOn)
    {
        memset(renderer->leds, 0, sizeof(renderer->leds));
        setLeds(renderer->leds, CONFIG_NUM_LEDS);
    }
}
