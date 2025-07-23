//==============================================================================
// Includes
//==============================================================================

#include <string.h>
#include "hdw-battmon.h"
#include "hdw-tft.h"
#include "macros.h"
#include "trigonometry.h"
#include "menu_utils.h"
#include "color_utils.h"
#include "menuMegaRenderer.h"

//==============================================================================
// Defines
//==============================================================================

#define ITEMS_PER_PAGE      5   // The number of items show per menu page
#define ITEM_MARGIN         1   // Vertical spacing between items
#define Y_SECTION_MARGIN    13  // Where to start drawing the header
#define Y_ITEM_START        55  // Where to start drawing items
#define MAX_ITEM_TEXT_WIDTH 191 // Maximum width of item text

#define ARROW_PERIOD_US 1000000

//==============================================================================
// Variables
//==============================================================================

static const paletteColor_t defaultBgColors[] = {
    c500, c410, c320, c230, c140, c050, c041, c032, c023, c014, c005,
};

//==============================================================================
// Function Prototypes
//==============================================================================

static void drawMenuText(menuMegaRenderer_t* renderer, const char* text, int16_t x, int16_t y, bool isSelected,
                         bool leftArrow, bool rightArrow, bool doubleArrows);
static void setLedsFromBg(menuMegaRenderer_t* renderer);

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

    // Set text color
    renderer->textFillColor    = c555;
    renderer->textOutlineColor = c000;

    // Set the background
    renderer->bgColors    = defaultBgColors;
    renderer->numBgColors = ARRAY_SIZE(defaultBgColors);

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
        loadFont(OXANIUM_FONT, renderer->titleFont, true);
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
        loadFont(PULSE_AUX_FONT, renderer->menuFont, true);
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
    setLedsFromBg(renderer);
    setLeds(renderer->leds, CONFIG_NUM_LEDS);

    // LEDs on by default
    renderer->ledsOn = true;

    // Reset the palette
    wsgPaletteReset(&renderer->palette);

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
 * @brief TODO doc
 *
 * @param renderer
 * @param textFill
 * @param textOutline
 * @param c1
 * @param c2
 * @param c3
 * @param c4
 * @param c5
 * @param c6
 * @param c7
 * @param c8
 * @param bgColors
 * @param numBgColors
 */
void recolorMenuMegaRenderer(menuMegaRenderer_t* renderer, paletteColor_t textFill, paletteColor_t textOutline,
                             paletteColor_t c1, paletteColor_t c2, paletteColor_t c3, paletteColor_t c4,
                             paletteColor_t c5, paletteColor_t c6, paletteColor_t c7, paletteColor_t c8,
                             const paletteColor_t* bgColors, int32_t numBgColors)
{
    renderer->textFillColor    = textFill;
    renderer->textOutlineColor = textOutline;

    wsgPaletteSet(&renderer->palette, c001, c1); // Darkest blue
    wsgPaletteSet(&renderer->palette, c012, c2); // Very dark blue
    wsgPaletteSet(&renderer->palette, c023, c3); // Dark blue
    wsgPaletteSet(&renderer->palette, c113, c4); // Dark moderate blue
    wsgPaletteSet(&renderer->palette, c124, c5); // Dark strong blue
    wsgPaletteSet(&renderer->palette, c034, c6); // Light strong blue
    wsgPaletteSet(&renderer->palette, c045, c7); // Pure cyan
    wsgPaletteSet(&renderer->palette, c455, c8); // Very pale cyan

    if (bgColors)
    {
        renderer->bgColors    = bgColors;
        renderer->numBgColors = numBgColors;
    }
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
        drawWsgPaletteSimple(&renderer->item_sel, x, y, &renderer->palette);
    }
    else
    {
        drawWsgPaletteSimple(&renderer->item, x, y, &renderer->palette);
    }

    int16_t textX = x + 13;
    int16_t textY = y + (renderer->item.h - renderer->menuFont->height) / 2;

    // Draw the text
    if (isSelected && textWidth(renderer->menuFont, text) > MAX_ITEM_TEXT_WIDTH)
    {
        // Drop shadow
        drawTextMarquee(renderer->menuFont, renderer->textOutlineColor, text, textX + 1, textY + 1,
                        textX + MAX_ITEM_TEXT_WIDTH - 5, &renderer->selectedMarqueeTimer);
        // Text
        drawTextMarquee(renderer->menuFont, renderer->textFillColor, text, textX, textY,
                        textX + MAX_ITEM_TEXT_WIDTH - 5, &renderer->selectedMarqueeTimer);
    }
    else
    {
        // Drop shadow
        drawTextEllipsize(renderer->menuFont, renderer->textOutlineColor, text, textX + 1, textY + 1,
                          MAX_ITEM_TEXT_WIDTH, false);
        // Text
        drawTextEllipsize(renderer->menuFont, renderer->textFillColor, text, textX, textY, MAX_ITEM_TEXT_WIDTH, false);
    }

    // Draw the left arrow, if applicable
    if (leftArrow)
    {
        const wsg_t* arrow = &renderer->prev;
        int16_t drawX      = x + 1;
        if (doubleArrows)
        {
            arrow = &renderer->back;
            drawX = x - 10;
        }
        int16_t drawY = y + (renderer->item.h - arrow->h) / 2 + 1;
        drawWsgPaletteSimple(arrow, drawX, drawY, &renderer->palette);
    }

    // Draw the right arrow, if applicable
    if (rightArrow)
    {
        const wsg_t* arrow = &renderer->next;
        int16_t drawX      = x + renderer->item.w - arrow->w - 18;
        if (doubleArrows)
        {
            arrow = &renderer->submenu;
            drawX = x + renderer->item.w - arrow->w - 14;
        }
        int16_t drawY = y + (renderer->item.h - arrow->h) / 2 + 1;
        drawWsgPaletteSimple(arrow, drawX, drawY, &renderer->palette);
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
    // Set LEDs
    if (renderer->ledsOn)
    {
        // Set LEDs
        setLedsFromBg(renderer);
        setLeds(renderer->leds, CONFIG_NUM_LEDS);
    }

    // Set text spacing to two pixels
    setGlobalCharSpacing(2);

    // Only poll the battery if requested
    if (menu->showBattery)
    {
        // Read battery every 10s
        RUN_TIMER_EVERY(menu->batteryReadTimer, 10000000, elapsedUs, { menu->batteryLevel = readBattmon(); });
    }

    // Run a timer to blink up and down page arrows
    RUN_TIMER_EVERY(renderer->pageArrowTimer, ARROW_PERIOD_US, elapsedUs, {});

    // Run a timer to adjust background color
    // A cycle completes in 180 degrees, or three seconds
    RUN_TIMER_EVERY(renderer->bgColorTimer, 3000000 / 180, elapsedUs, {
        renderer->bgColorDeg++;
        if (180 == renderer->bgColorDeg)
        {
            renderer->bgColorDeg = 0;
        }
        renderer->bgColorIdx = ((getSin1024(renderer->bgColorDeg) * (renderer->numBgColors - 1)) + 512) / 1024;
    });

    // Run a timer to scroll text
    if (menu->currentItem != renderer->currentItem)
    {
        // Reset the timer when the item changes
        renderer->currentItem          = menu->currentItem;
        renderer->selectedMarqueeTimer = 0;
    }
    else
    {
        renderer->selectedMarqueeTimer += elapsedUs;
    }

    // Clear the background
    paletteColor_t* fb = getPxTftFramebuffer();
    memset(fb, renderer->bgColors[renderer->bgColorIdx], sizeof(paletteColor_t) * TFT_HEIGHT * TFT_WIDTH);
    drawWsgPaletteSimple(&renderer->bg, 0, 0, &renderer->palette);

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
    y = Y_ITEM_START;

    drawWsgPaletteSimple(&renderer->body, 0, 0, &renderer->palette);

    if (menu->items->length > ITEMS_PER_PAGE && renderer->pageArrowTimer > ARROW_PERIOD_US / 2)
    {
        // Draw UP page indicator
        drawWsgPaletteSimple(&renderer->up, 222, 38, &renderer->palette);
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
        y += renderer->item.h + ITEM_MARGIN;
    }

    if (menu->items->length > ITEMS_PER_PAGE && renderer->pageArrowTimer > ARROW_PERIOD_US / 2)
    {
        // Draw DOWN page indicator
        drawWsgPaletteSimple(&renderer->down, 222, 221, &renderer->palette);
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
    setGlobalCharSpacing(1);
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
        setLedsFromBg(renderer);
        setLeds(renderer->leds, CONFIG_NUM_LEDS);
    }
}

/**
 * @brief TODO doc
 *
 * @param renderer
 */
static void setLedsFromBg(menuMegaRenderer_t* renderer)
{
    // Extract LED color from bg color
    int32_t rgb = paletteToRGB(renderer->bgColors[renderer->bgColorIdx]);
    led_t led   = {
          .r = (rgb >> 16) & 0xFF,
          .g = (rgb >> 8) & 0xFF,
          .b = (rgb >> 0) & 0xFF,
    };

    // Set all LEDs
    for (int32_t idx = 0; idx < CONFIG_NUM_LEDS; idx++)
    {
        renderer->leds[idx] = led;
    }
}