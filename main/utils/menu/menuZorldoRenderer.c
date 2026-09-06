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
#include "menuZorldoRenderer.h"

#include "esp_log.h"
#include "esp_random.h"
#include "fill.h"
#include "shapes.h"

//==============================================================================
// Defines
//==============================================================================

#define ITEMS_PER_PAGE      5   // The number of items show per menu page
#define ITEM_MARGIN         1   // Vertical spacing between items
#define ITEM_HEIGHT         31  // Height of an item. Text will be centered vertically in this space.
#define Y_SECTION_MARGIN    6   // Where to start drawing the header
#define Y_ITEM_START        55  // Where to start drawing items
#define MAX_ITEM_TEXT_WIDTH 191 // Maximum width of item text

#define ARROW_PERIOD_US 1000000

#define DEFAULT_BODY_HEIGHT 66

#define CLOUD_COUNT                4
#define CLOUD_LIGHT_COLOR_1        c555
#define CLOUD_LIGHT_COLOR_2        c444
#define CLOUD_DARK_COLOR_1         c222
#define CLOUD_DARK_COLOR_2         c111
#define CLOUD_MIN_X                (-500)
#define CLOUD_MIN_Y_VISIBLE        5 // Minimum amount of a cloud peeking from offscreen vertically
#define CLOUD_MIN_SPEED_PX_PER_SEC 4
#define CLOUD_MAX_SPEED_PX_PER_SEC 25

#define BATTERY_LVL_FULL          872 // The battery read value that indicates "full"
#define BATTERY_LVL_DEAD          452 // The battery read value that indicates "RIP"
#define BATTERY_TOP_INSET         3
#define BATTERY_SIDE_BOTTOM_INSET 2

//==============================================================================
// Variables
//==============================================================================

// TODO: Figure out LEDs once we have prototype boards
static const uint8_t ledConveyorOrder[] = {
    1, 0, 2, 3, 5, 4, 6, 7, 8,
};

//==============================================================================
// Function Prototypes
//==============================================================================

static void initCloud(menuZorldoCloud_t* cloud, bool offscreenOnly);
static void drawMenuText(menuZorldoRenderer_t* renderer, const char* text, int16_t x, int16_t y, bool isSelected,
                         bool leftArrow, bool rightArrow, bool doubleArrows);
static void setLedsFromBg(menuZorldoRenderer_t* renderer);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initialize a and return a menu renderer.
 *
 * @param titleFont The font used to draw the title, preferably PIX_ROMANA_FONT. If this is NULL it will be
 * allocated by the renderer in SPIRAM.
 * @param menuFont The font used to draw this menu, preferably MINISHCAP_FONT. If this is NULL it will be
 * allocated by the renderer in SPIRAM.
 * @return A pointer to the menu renderer. This memory is allocated and must be freed with deinitMenuZorldoRenderer()
 * when done
 */
menuZorldoRenderer_t* initMenuZorldoRenderer(font_t* titleFont, font_t* menuFont)
{
    menuZorldoRenderer_t* renderer = heap_caps_calloc(1, sizeof(menuZorldoRenderer_t), MALLOC_CAP_SPIRAM);

    // Set text color
    renderer->textFillColor    = c555;
    renderer->textOutlineColor = c003;

    loadWsg(ZORLDO_MENU_CORNER_WSG, &renderer->corner, true);
    loadWsg(ZORLDO_MENU_EWI_WSG, &renderer->ewi, true);
    loadWsg(ZORLDO_MENU_BATTERY_WSG, &renderer->battery, true);
    loadWsg(ZORLDO_MENU_TOMI_WSG, &renderer->tomi, true);

    loadWsg(ZORLDO_MENU_ARROW_PAGE_UP_WSG, &renderer->arrowPageUp, true);
    loadWsg(ZORLDO_MENU_ARROW_RIGHT_WSG, &renderer->arrowRight, true);
    loadWsg(ZORLDO_MENU_ARROW_SUBMENU_WSG, &renderer->arrowSubmenu, true);

    loadWsg(ZORLDO_MENU_CLOUD_1_WSG, &renderer->clouds[0].wsg, true);
    loadWsg(ZORLDO_MENU_CLOUD_2_WSG, &renderer->clouds[1].wsg, true);
    loadWsg(ZORLDO_MENU_CLOUD_3_WSG, &renderer->clouds[2].wsg, true);
    loadWsg(ZORLDO_MENU_CLOUD_4_WSG, &renderer->clouds[3].wsg, true);

    // Save or allocate title font
    if (NULL == titleFont)
    {
        renderer->titleFont = heap_caps_calloc(1, sizeof(font_t), MALLOC_CAP_SPIRAM);
        loadFont(PIX_ROMANA_FONT, renderer->titleFont, true);
        renderer->titleFontAllocated = true;
    }
    else
    {
        renderer->titleFont          = titleFont;
        renderer->titleFontAllocated = false;
    }

    // Save or allocate menu font
    if (NULL == menuFont)
    {
        renderer->menuFont = heap_caps_calloc(1, sizeof(font_t), MALLOC_CAP_SPIRAM);
        loadFont(MINISHCAP_FONT, renderer->menuFont, true);
        renderer->menuFontAllocated = true;
    }
    else
    {
        renderer->menuFont          = menuFont;
        renderer->menuFontAllocated = false;
    }

    for (uint8_t i = 0; i < CLOUD_COUNT; i++)
    {
        initCloud(&renderer->clouds[i], false);
    }

    // Initialize LEDs
    setLedsFromBg(renderer);
    setLeds(renderer->leds, CONFIG_NUM_LEDS);

    // LEDs on by default
    renderer->ledsOn = true;

    // Draw the body by default
    renderer->drawBody   = true;
    renderer->bodyHeight = DEFAULT_BODY_HEIGHT;

    return renderer;
}

/**
 * @brief Deinitialize a menu renderer and free associated memory. This will not free the font passed into
 * initMenuZorldoRenderer()
 *
 * @param renderer The renderer to deinitialize. It must not be used after deinitialization.
 */
void deinitMenuZorldoRenderer(menuZorldoRenderer_t* renderer)
{
    // Free fonts if allocated
    if (renderer->titleFontAllocated)
    {
        freeFont(renderer->titleFont);
        heap_caps_free(renderer->titleFont);
    }
    if (renderer->menuFontAllocated)
    {
        freeFont(renderer->menuFont);
        heap_caps_free(renderer->menuFont);
    }

    freeWsg(&renderer->corner);
    freeWsg(&renderer->ewi);
    freeWsg(&renderer->battery);
    freeWsg(&renderer->tomi);

    freeWsg(&renderer->arrowPageUp);
    freeWsg(&renderer->arrowRight);
    freeWsg(&renderer->arrowSubmenu);

    for (uint8_t i = 0; i < CLOUD_COUNT; i++)
    {
        freeWsg(&renderer->clouds[i].wsg);
    }

    heap_caps_free(renderer);
}

/**
 * @brief Set whether or not to draw the menu background
 *
 * @param renderer The renderer.
 * @param drawBody true to draw the body background, false to skip it
 */
void setDrawMenuZorldoBody(menuZorldoRenderer_t* renderer, bool drawBody)
{
    renderer->drawBody = drawBody;
}

/**
 * @brief Set the height of the body between top and bottom decorated parts
 * If the given height is negative, it will be set to the default height
 *
 * @param renderer The renderer to adjust the body height for
 * @param height The new height. If the given value is negative, the default height will be set.
 */
void setMenuZorldoBodyHeight(menuZorldoRenderer_t* renderer, int16_t height)
{
    if (height < 0)
    {
        renderer->bodyHeight = DEFAULT_BODY_HEIGHT;
    }
    else
    {
        renderer->bodyHeight = height;
    }
}

/**
 * @brief (Re)initializes a cloud to float across the screen, randomizing the position and movement.
 *
 * @param cloud Where servers live
 * @param offscreenOnly Should clouds be allowed to pop onto the screen. Used for initial setup
 */
void initCloud(menuZorldoCloud_t* cloud, bool offscreenOnly)
{
    // Start clouds in the screen area or way to the left and let them float in
    int16_t maxX = offscreenOnly ? -cloud->wsg.w : TFT_WIDTH;
    cloud->x     = (int32_t)esp_random() % (maxX - CLOUD_MIN_X) + CLOUD_MIN_X;
    /// At least CLOUD_MIN_Y_VISIBLE pixels peeking from the top or bottom
    int16_t minY = CLOUD_MIN_Y_VISIBLE - cloud->wsg.h;
    cloud->y     = esp_random() % (TFT_HEIGHT - CLOUD_MIN_Y_VISIBLE - minY) + minY;

    cloud->speed
        = (esp_random() % (CLOUD_MAX_SPEED_PX_PER_SEC - CLOUD_MIN_SPEED_PX_PER_SEC) + CLOUD_MIN_SPEED_PX_PER_SEC)
          / 1000000.f;
    cloud->mirrored = esp_random() % 2 == 0;
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
static void drawMenuText(menuZorldoRenderer_t* renderer, const char* text, int16_t x, int16_t y, bool isSelected,
                         bool leftArrow, bool rightArrow, bool doubleArrows)
{
    int16_t textX = x + 25;
    int16_t textY = y + (ITEM_HEIGHT - renderer->menuFont->height) / 2;

    // Draw border for the menu item
    if (isSelected)
    {
        int16_t boxX      = textX - 8;
        int16_t boxY      = textY - 4;
        int16_t boxHeight = renderer->menuFont->height + 8;
        drawRoundedRect(boxX, boxY, TFT_WIDTH - boxX, boxY + boxHeight, 1, cTransparent, renderer->textOutlineColor);
        drawRoundedRect(boxX - 1, boxY - 1, TFT_WIDTH - boxX + 1, boxY + boxHeight + 1, 1, cTransparent,
                        renderer->textFillColor);
        drawRoundedRect(boxX - 2, boxY - 2, TFT_WIDTH - boxX + 2, boxY + boxHeight + 2, 1, cTransparent,
                        renderer->textOutlineColor);

        renderer->tomiYTarget = y + renderer->menuFont->height / 2 - 2;
        if (renderer->tomiYPos == 0)
        {
            renderer->tomiYPos = renderer->tomiYTarget;
        }
    }

    // Draw the text
    if (isSelected && textWidth(renderer->menuFont, text) > MAX_ITEM_TEXT_WIDTH)
    {
        // Drop shadow
        drawTextMarquee(renderer->menuFont, renderer->textOutlineColor, text, textX + 1, textY + 0,
                        textX + MAX_ITEM_TEXT_WIDTH - 5, &renderer->shadowMarqueeTimer);
        drawTextMarquee(renderer->menuFont, renderer->textOutlineColor, text, textX - 1, textY + 0,
                        textX + MAX_ITEM_TEXT_WIDTH - 5, &renderer->shadowMarqueeTimer);
        drawTextMarquee(renderer->menuFont, renderer->textOutlineColor, text, textX + 0, textY + 1,
                        textX + MAX_ITEM_TEXT_WIDTH - 5, &renderer->shadowMarqueeTimer);
        drawTextMarquee(renderer->menuFont, renderer->textOutlineColor, text, textX + 0, textY - 1,
                        textX + MAX_ITEM_TEXT_WIDTH - 5, &renderer->shadowMarqueeTimer);
        // Text
        drawTextMarquee(renderer->menuFont, renderer->textFillColor, text, textX, textY,
                        textX + MAX_ITEM_TEXT_WIDTH - 5, &renderer->selectedMarqueeTimer);
    }
    else
    {
        // Drop shadow
        drawTextEllipsize(renderer->menuFont, renderer->textOutlineColor, text, textX + 1, textY + 0,
                          MAX_ITEM_TEXT_WIDTH, false);
        drawTextEllipsize(renderer->menuFont, renderer->textOutlineColor, text, textX - 1, textY + 0,
                          MAX_ITEM_TEXT_WIDTH, false);
        drawTextEllipsize(renderer->menuFont, renderer->textOutlineColor, text, textX + 0, textY + 1,
                          MAX_ITEM_TEXT_WIDTH, false);
        drawTextEllipsize(renderer->menuFont, renderer->textOutlineColor, text, textX + 0, textY - 1,
                          MAX_ITEM_TEXT_WIDTH, false);
        // Text
        drawTextEllipsize(renderer->menuFont, renderer->textFillColor, text, textX, textY, MAX_ITEM_TEXT_WIDTH, false);
    }

    // Draw the left arrow, if applicable
    if (leftArrow)
    {
        const wsg_t* arrow = &renderer->arrowRight;
        int16_t drawX      = 24;
        if (doubleArrows)
        {
            arrow = &renderer->arrowSubmenu;
            drawX = 23;
        }
        int16_t drawY = y + (ITEM_HEIGHT - arrow->h) / 2 + 1;
        drawWsg(arrow, drawX, drawY, true, false, 0);
    }

    // Draw the right arrow, if applicable
    if (rightArrow)
    {
        const wsg_t* arrow = &renderer->arrowRight;
        int16_t drawX      = TFT_WIDTH - arrow->w - 24;
        if (doubleArrows)
        {
            arrow = &renderer->arrowSubmenu;
            drawX = TFT_WIDTH - arrow->w - 23;
        }
        int16_t drawY = y + (ITEM_HEIGHT - arrow->h) / 2 + 1;
        drawWsgSimple(arrow, drawX, drawY);
    }
}

/**
 * @brief Draw the menu body background
 *
 * @param topLeftX The X coordinate of the top left corner of the body
 * @param topLeftY The Y coordinate of the top left corner of the body
 * @param renderer The renderer to draw a body with
 * @param elapsedUs The time elapsed since this function was last called, for cloud animation
 */
void drawMenuZorldoBody(uint16_t topLeftX, uint16_t topLeftY, menuZorldoRenderer_t* renderer, int64_t elapsedUs)
{
    uint16_t bottomRightX = TFT_WIDTH - topLeftX;
    uint16_t bottomRightY = TFT_HEIGHT - 14;

    // Sky "translucent" background and clouds
    drawRectFilled(topLeftX + 6, topLeftY + 6, bottomRightX - 6, bottomRightY - 6, c011);
    for (int i = 0; i < CLOUD_COUNT; i++)
    {
        menuZorldoCloud_t* cloud = &renderer->clouds[i];

        cloud->x += cloud->speed * elapsedUs;
        if (cloud->x >= TFT_WIDTH)
        {
            initCloud(cloud, true);
        }

        if (cloud->x >= -cloud->wsg.w)
        {
            for (uint16_t x = 0; x < cloud->wsg.w; x++)
            {
                for (uint16_t y = 0; y < cloud->wsg.h; y++)
                {
                    paletteColor_t* pixel = &cloud->wsg.px[y * cloud->wsg.w + x];
                    uint16_t localX       = x;
                    if (cloud->mirrored)
                    {
                        localX = cloud->wsg.w - x - 1;
                    }
                    if (localX + (int16_t)cloud->x < topLeftX + 6 || y + cloud->y < topLeftY + 6
                        || localX + (int16_t)cloud->x >= bottomRightX - 6 || y + cloud->y >= bottomRightY - 6)
                    {
                        if (*pixel == CLOUD_DARK_COLOR_1)
                        {
                            *pixel = CLOUD_LIGHT_COLOR_1;
                        }
                        else if (*pixel == CLOUD_DARK_COLOR_2)
                        {
                            *pixel = CLOUD_LIGHT_COLOR_2;
                        }
                    }
                    else
                    {
                        if (*pixel == CLOUD_LIGHT_COLOR_1)
                        {
                            *pixel = CLOUD_DARK_COLOR_1;
                        }
                        else if (*pixel == CLOUD_LIGHT_COLOR_2)
                        {
                            *pixel = CLOUD_DARK_COLOR_2;
                        }
                    }
                }
            }
            drawWsg(&cloud->wsg, cloud->x, cloud->y, cloud->mirrored, false, 0);
        }
    }

    // Rounded rect so the outline doesn't stick out past the corner images
    drawRoundedRect(topLeftX, topLeftY, bottomRightX, bottomRightY, 5, cTransparent, c210);
    drawRect(topLeftX + 1, topLeftY + 1, bottomRightX - 1, bottomRightY - 1, c430);
    drawRect(topLeftX + 2, topLeftY + 2, bottomRightX - 2, bottomRightY - 2, c430);
    drawRect(topLeftX + 3, topLeftY + 3, bottomRightX - 3, bottomRightY - 3, c430);
    drawRect(topLeftX + 4, topLeftY + 4, bottomRightX - 4, bottomRightY - 4, c542);
    drawRect(topLeftX + 5, topLeftY + 5, bottomRightX - 5, bottomRightY - 5, c210);

    drawWsg(&renderer->corner, topLeftX - 3, topLeftY - 1, true, false, 0);
    drawWsg(&renderer->corner, bottomRightX - 10, topLeftY - 1, false, false, 0);
    drawWsg(&renderer->corner, topLeftX - 3, bottomRightY - renderer->corner.h + 1, true, true, 0);
    drawWsg(&renderer->corner, bottomRightX - 10, bottomRightY - renderer->corner.h + 1, false, true, 0);
}

/**
 * @brief Draw a themed menu to the display and control the LEDs
 *
 * @param menu The menu to draw
 * @param renderer The renderer to draw with
 * @param elapsedUs The time elapsed since this function was last called, for LED animation
 */
void drawMenuZorldo(menu_t* menu, menuZorldoRenderer_t* renderer, int64_t elapsedUs)
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

    // Run a timer to scroll text
    if (menu->currentItem != renderer->currentItem)
    {
        // Reset the timer when the item changes
        renderer->currentItem          = menu->currentItem;
        renderer->selectedMarqueeTimer = 0;
        renderer->shadowMarqueeTimer   = 0;
    }
    else
    {
        renderer->selectedMarqueeTimer += elapsedUs;
        renderer->shadowMarqueeTimer += elapsedUs;
    }

    // Clear the background
    paletteColor_t* fb = getPxTftFramebuffer();
    memset(fb, c344, sizeof(paletteColor_t) * TFT_HEIGHT * TFT_WIDTH);

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

        curNode = curNode->next;
        pageIdx++;
        if (ITEMS_PER_PAGE <= pageIdx && NULL != curNode)
        {
            pageIdx   = 0;
            pageStart = curNode;
        }
    }

    if (renderer->drawBody)
    {
        drawMenuZorldoBody(12, 42, renderer, elapsedUs);
    }

    // Where to start drawing
    int16_t y = Y_SECTION_MARGIN;

    // Draw the title
    int16_t ewiY = y + (renderer->titleFont->height / 2 - renderer->ewi.h / 2) - 3;
    drawWsgSimple(&renderer->ewi, 11, ewiY);
    drawTextShadow(renderer->titleFont, c555, c111, menu->title, 15, y);

    // Move to drawing the rows
    y = Y_ITEM_START;

    if (menu->items->length > ITEMS_PER_PAGE && renderer->pageArrowTimer > ARROW_PERIOD_US / 2)
    {
        // Draw UP page indicator
        drawWsgSimple(&renderer->arrowPageUp, 211, 39);
    }

    // Draw a page-worth of items
    for (uint8_t itemIdx = 0; itemIdx < ITEMS_PER_PAGE; itemIdx++)
    {
        if (NULL != pageStart)
        {
            menuItem_t* item = pageStart->val;
            bool isSelected  = menu->currentItem->val == item;

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
        y += ITEM_HEIGHT + ITEM_MARGIN;
    }

    if (menu->items->length > ITEMS_PER_PAGE && renderer->pageArrowTimer > ARROW_PERIOD_US / 2)
    {
        // Draw DOWN page indicator
        drawWsg(&renderer->arrowPageUp, 211, 218, false, true, 0);
    }

    if (renderer->tomiYPos != 0)
    {
        // Move Tomi towards the selected item
        float distance = ABS(renderer->tomiYPos - renderer->tomiYTarget);
        if (distance >= 0.5)
        {
            float delta = MIN(distance, elapsedUs / (700000.0 / distance));
            if (renderer->tomiYPos > renderer->tomiYTarget)
            {
                delta = -delta;
            }
            renderer->tomiYPos += delta;
        }
        else
        {
            renderer->tomiYPos = renderer->tomiYTarget;
        }
        // Animate Tomi in a figure-8
        renderer->tomiXDegrees += elapsedUs / 14000.0;
        renderer->tomiYDegrees += elapsedUs / 7000.0;
        if (renderer->tomiXDegrees >= 360)
        {
            renderer->tomiXDegrees -= 360;
        }
        if (renderer->tomiYDegrees >= 360)
        {
            renderer->tomiYDegrees -= 360;
        }
        float tomiX = getSin1024(renderer->tomiXDegrees) / 50.0;
        float tomiY = getSin1024(renderer->tomiYDegrees) / 300.0;
        drawWsgSimple(&renderer->tomi, 175 + tomiX, renderer->tomiYPos + tomiY);
    }

    if (menu->showBattery)
    {
        //  Draw the battery indicator depending on the last read value
        uint8_t batteryPct;
        if (menu->batteryLevel <= 0)
        {
            batteryPct = 100;
        }
        else
        {
            batteryPct = (menu->batteryLevel - BATTERY_LVL_DEAD) * 100 / (BATTERY_LVL_FULL - BATTERY_LVL_DEAD);
        }

        paletteColor_t meterColor;
        if (batteryPct < 30)
        {
            meterColor = c400;
        }
        else
        {
            meterColor = c131;
        }

        int16_t batteryX = TFT_WIDTH - renderer->battery.w - 12;
        int16_t batteryY = ewiY + (renderer->ewi.h / 2 - renderer->battery.h / 2);
        drawWsgSimple(&renderer->battery, batteryX, batteryY);

        int16_t batteryInnerHeight = renderer->battery.h - BATTERY_TOP_INSET - BATTERY_SIDE_BOTTOM_INSET;
        int16_t filledHeight       = MAX(1, batteryInnerHeight * batteryPct / 100);
        drawRectFilled(batteryX + BATTERY_SIDE_BOTTOM_INSET,
                       batteryY + BATTERY_TOP_INSET + (batteryInnerHeight - filledHeight),
                       batteryX + renderer->battery.w - BATTERY_SIDE_BOTTOM_INSET,
                       batteryY + renderer->battery.h - BATTERY_SIDE_BOTTOM_INSET, meterColor);
    }

    setGlobalCharSpacing(1);
}

/**
 * @brief Set the renderer's LEDs to be on or off
 *
 * @param renderer The renderer to set
 * @param ledsOn true to animate the LEDs, false to keep them off
 */
void setZorldoLedsOn(menuZorldoRenderer_t* renderer, bool ledsOn)
{
    renderer->ledsOn = ledsOn;
    if (false == ledsOn)
    {
        setLedsFromBg(renderer);
        setLeds(renderer->leds, CONFIG_NUM_LEDS);
    }
}

/**
 * @brief Set the LEDs to be the same as the current background color
 *
 * @param renderer The renderer to set LEDs in
 */
static void setLedsFromBg(menuZorldoRenderer_t* renderer)
{
    // Set all LEDs
    for (int32_t idx = 0; idx < CONFIG_NUM_LEDS; idx++)
    {
        // Extract LED color from bg color
        int32_t rgb = paletteToRGB(c344);
        led_t led   = {
            .r = (rgb >> 16) & 0xFF,
            .g = (rgb >> 8) & 0xFF,
            .b = (rgb >> 0) & 0xFF,
        };
        renderer->leds[ledConveyorOrder[idx]] = led;
    }
}
