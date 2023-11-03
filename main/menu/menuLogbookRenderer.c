//==============================================================================
// Includes
//==============================================================================

#include <esp_random.h>
#include "hdw-battmon.h"
#include "menuLogbookRenderer.h"
#include "menu_utils.h"
#include "hdw-tft.h"
#include "shapes.h"
#include "fill.h"
#include "color_utils.h"

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

#define MENU_LED_BRIGHTNESS_MIN     128
#define MENU_LED_BRIGHTNESS_RANGE   128
#define MENU_LED_TIME_STEP_US_MIN   8192
#define MENU_LED_TIME_STEP_US_RANGE 16384

//==============================================================================
// Function Prototypes
//==============================================================================

static void drawMenuText(menuLogbookRenderer_t* renderer, const char* text, int16_t x, int16_t y, bool isSelected,
                         bool leftArrow, bool rightArrow, bool doubleArrows);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initialize a and return a menu renderer.
 *
 * @param menuFont The font used to draw this menu, preferably "logbook.font"
 * @return A pointer to the menu renderer. This memory is allocated and must be freed with deinitMenuLogbookRenderer()
 * when done
 */
menuLogbookRenderer_t* initMenuLogbookRenderer(font_t* menuFont)
{
    menuLogbookRenderer_t* renderer = calloc(1, sizeof(menuLogbookRenderer_t));
    renderer->font                  = menuFont;
    loadWsg("mnuArrow.wsg", &renderer->arrow, false);
    loadWsg("mnuArrowS.wsg", &renderer->arrowS, false);

    // Load battery images
    loadWsg("batt1.wsg", &renderer->batt[0], false);
    loadWsg("batt2.wsg", &renderer->batt[1], false);
    loadWsg("batt3.wsg", &renderer->batt[2], false);
    loadWsg("batt4.wsg", &renderer->batt[3], false);

    // Initialize LEDs
    for (uint16_t idx = 0; idx < CONFIG_NUM_LEDS; idx++)
    {
        renderer->ledTimers[idx].maxBrightness = MENU_LED_BRIGHTNESS_MIN + (esp_random() % MENU_LED_BRIGHTNESS_RANGE);
        renderer->ledTimers[idx].periodUs = MENU_LED_TIME_STEP_US_MIN + (esp_random() % MENU_LED_TIME_STEP_US_RANGE);
    }
    setLeds(renderer->leds, CONFIG_NUM_LEDS);

    return renderer;
}

/**
 * @brief Deinitialize a menu renderer and free associated memory. This will not free the font passed into
 * initMenuLogbookRenderer()
 *
 * @param renderer The renderer to deinitialize. It must not be used after deinitialization.
 */
void deinitMenuLogbookRenderer(menuLogbookRenderer_t* renderer)
{
    freeWsg(&renderer->arrow);
    freeWsg(&renderer->arrowS);
    freeWsg(&renderer->batt[0]);
    freeWsg(&renderer->batt[1]);
    freeWsg(&renderer->batt[2]);
    freeWsg(&renderer->batt[3]);
    free(renderer);
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
static void drawMenuText(menuLogbookRenderer_t* renderer, const char* text, int16_t x, int16_t y, bool isSelected,
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
 * @brief Draw a themed menu to the display and control the LEDs
 *
 * @param menu The menu to draw
 * @param renderer The renderer to draw with
 * @param elapsedUs The time elapsed since this function was last called, for LED animation
 */
void drawMenuLogbook(menu_t* menu, menuLogbookRenderer_t* renderer, int64_t elapsedUs)
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

    // For each LED
    for (uint16_t idx = 0; idx < CONFIG_NUM_LEDS; idx++)
    {
        // Get a convenient reference to that LED's timers
        menuLed_t* mLed = &renderer->ledTimers[idx];
        // Increment the timer
        mLed->timerUs += elapsedUs;
        // Check if the timer expired
        while (mLed->timerUs >= mLed->periodUs)
        {
            // Decrement the timer
            mLed->timerUs -= mLed->periodUs;
            // If the LED is lighting
            if (mLed->isLighting)
            {
                // Make it brighter
                renderer->ledTimers[idx].brightness++;
                // Check if it hit peak brightness
                if (mLed->maxBrightness == renderer->ledTimers[idx].brightness)
                {
                    mLed->isLighting = false;
                }
            }
            else
            {
                // Make it dimmer
                renderer->ledTimers[idx].brightness--;
                // Check if the LED is off
                if (0 == renderer->ledTimers[idx].brightness)
                {
                    mLed->isLighting = true;
                    // Pick new random speed and brightness
                    mLed->maxBrightness = MENU_LED_BRIGHTNESS_MIN + (esp_random() % MENU_LED_BRIGHTNESS_RANGE);
                    mLed->periodUs      = MENU_LED_TIME_STEP_US_MIN + (esp_random() % MENU_LED_TIME_STEP_US_RANGE);
                }
            }
            renderer->leds[idx].r = gamma_correction_table[renderer->ledTimers[idx].brightness];
        }
    }
    // Light the LEDs
    setLeds(renderer->leds, CONFIG_NUM_LEDS);

    // Clear the TFT
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
    int16_t x = 16;
    int16_t y = Y_SECTION_MARGIN;

    // Draw a title
    drawText(renderer->font, c542, menu->title, x, y);
    y += renderer->font->height + Y_SECTION_MARGIN;

    // Shift the text a little after drawing the title
    x = 10;

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

            char buffer[64]   = {0};
            const char* label = getMenuItemLabelText(buffer, sizeof(buffer), item);

            bool leftArrow    = menuItemHasPrev(item) || menuItemIsBack(item);
            bool rightArrow   = menuItemHasNext(item) || menuItemHasSubMenu(item);
            bool doubleArrows = menuItemIsBack(item) || menuItemHasSubMenu(item);

            drawMenuText(renderer, label, x, y, isSelected, leftArrow, rightArrow, doubleArrows);

            // Move to the next item
            pageStart = pageStart->next;
        }

        // Move to the next row
        y += (renderer->font->height + (TEXT_OFFSET * 2) + ROW_SPACING);
    }

    y -= ROW_SPACING;
    if (menu->items->length > ITEMS_PER_PAGE)
    {
        // Draw DOWN page indicator
        int16_t arrowX = PAGE_ARROW_X_OFFSET;
        int16_t arrowY = y + PAGE_ARROW_Y_OFFSET;
        drawWsg(&renderer->arrow, arrowX, arrowY, false, false, 90);
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

        drawWsg(toDraw, 212, 3, false, false, 0);
    }
}
