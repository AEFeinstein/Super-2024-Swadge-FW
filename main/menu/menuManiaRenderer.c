//==============================================================================
// Includes
//==============================================================================

#include <esp_random.h>
#include <esp_heap_caps.h>
#include "hdw-battmon.h"
#include "menuManiaRenderer.h"
#include "menu_utils.h"
#include "hdw-tft.h"
#include "shapes.h"
#include "fill.h"
#include "color_utils.h"
#include "hdw-nvs.h"
#include "vector2d.h"
#include "swadge2024.h"
#include "color_utils.h"

//==============================================================================
// Defines
//==============================================================================

#define ITEMS_PER_PAGE 5

#define PARALLELOGRAM_X_OFFSET 13
#define PARALLELOGRAM_HEIGHT   25
#define PARALLELOGRAM_WIDTH    229
#define ROW_MARGIN             8
#define DROP_SHADOW_OFFSET     (ROW_MARGIN / 2)

#define ARROW_MARGIN 4
#define ARROW_WIDTH  2

#define UP_ARROW_HEIGHT 10
#define UP_ARROW_MARGIN 2

#define TITLE_BG_COLOR     c115
#define TITLE_TEXT_COLOR   c542
#define TEXT_OUTLINE_COLOR c000
#define BG_COLOR           c540
#define OUTER_RING_COLOR   c243
#define INNER_RING_COLOR   c531
#define ROW_COLOR          c000
#define ROW_TEXT_COLOR     c555
// #define ROW_TEXT_SELECTED_COLOR c533

#define ORBIT_RING_RADIUS_1   26
#define ORBIT_RING_RADIUS_2   18
#define RING_STROKE_THICKNESS 8
#define MIN_RING_RADIUS       64
#define MAX_RING_RADIUS       114

//==============================================================================
// Const Variables
//==============================================================================

/// @brief Colors to cycle through for the selected drop shadow
static const paletteColor_t selectedShadowColors[] = {
    c500, c511, c522, c533, c544, c555, c544, c533, c522, c511,
};

/// @brief Offsets to cycle through to bounce an item when selected
static const int16_t selectedBounceOffsets[] = {
    0, -1, -2, -3, -4, -5, -6, -7, -6, -5, -4, -3, -2, -1,
};

//==============================================================================
// Function Prototypes
//==============================================================================

static void drawMenuText(menuManiaRenderer_t* renderer, const char* text, int16_t x, int16_t y, bool isSelected,
                         bool leftArrow, bool rightArrow, bool doubleArrows);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initialize a and return a menu renderer.
 *
 * @param titleFont The font used to draw the title, preferably "righteous_150.font". If this is NULL it will be
 * allocated by the renderer in SPIRAM.
 * @param titleFontOutline The outline font used to draw the title. If this is NULL it will be allocated by the renderer
 * in SPIRAM.
 * @param menuFont The font used to draw this menu, preferably "rodin_eb.font". If this is NULL it will be allocated by
 * the renderer in SPIRAM.
 * @return A pointer to the menu renderer. This memory is allocated and must be freed with deinitMenuManiaRenderer()
 * when done
 */
menuManiaRenderer_t* initMenuManiaRenderer(font_t* titleFont, font_t* titleFontOutline, font_t* menuFont)
{
    menuManiaRenderer_t* renderer = calloc(1, sizeof(menuManiaRenderer_t));

    // Save or allocate title font
    if (NULL == titleFont)
    {
        renderer->titleFont = heap_caps_calloc(1, sizeof(font_t), MALLOC_CAP_SPIRAM);
        loadFont("righteous_150.font", renderer->titleFont, true);
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
        makeOutlineFont(renderer->titleFont, renderer->titleFontOutline, false);
        renderer->titleFontOutlineAllocated = true;
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
        loadFont("rodin_eb.font", renderer->menuFont, true);
        renderer->menuFontAllocated = true;
    }
    else
    {
        renderer->menuFont          = menuFont;
        renderer->menuFontAllocated = false;
    }

    // Load battery images
    loadWsg("batt1.wsg", &renderer->batt[0], false);
    loadWsg("batt2.wsg", &renderer->batt[1], false);
    loadWsg("batt3.wsg", &renderer->batt[2], false);
    loadWsg("batt4.wsg", &renderer->batt[3], false);

    // Initialize LEDs
    setLeds(renderer->leds, CONFIG_NUM_LEDS);

    // Initialize Rings
    const paletteColor_t ringColors[] = {
        INNER_RING_COLOR,
        OUTER_RING_COLOR,
    };
    int32_t ringMinSpeed = 15000;
    int32_t ringMaxSpeed = 20000;
    int32_t ringDir      = 1; // Flips each ring
    for (int16_t i = 0; i < ARRAY_SIZE(renderer->rings); i++)
    {
        maniaRing_t* ring      = &renderer->rings[i];
        ring->diameterAngle    = i * (360 / ARRAY_SIZE(renderer->rings));
        ring->diameterTimer    = 0;
        ring->orbitAngle       = i * (360 / ARRAY_SIZE(renderer->rings));
        ring->orbitTimer       = 0;
        ring->orbitUsPerDegree = ringMinSpeed + i * (ringMaxSpeed - ringMinSpeed);
        ring->orbitDirection   = ringDir;
        ringDir                = (ringDir == 1) ? -1 : 1;
        ring->color            = ringColors[i];
    }

    // LEDs on by default
    renderer->ledsOn = true;

    return renderer;
}

/**
 * @brief Deinitialize a menu renderer and free associated memory. This will not free the font passed into
 * initMenuManiaRenderer()
 *
 * @param renderer The renderer to deinitialize. It must not be used after deinitialization.
 */
void deinitMenuManiaRenderer(menuManiaRenderer_t* renderer)
{
    freeWsg(&renderer->batt[0]);
    freeWsg(&renderer->batt[1]);
    freeWsg(&renderer->batt[2]);
    freeWsg(&renderer->batt[3]);

    // Free fonts if allocated
    if (renderer->titleFontAllocated)
    {
        freeFont(renderer->titleFont);
        free(renderer->titleFont);
    }
    if (renderer->titleFontOutlineAllocated)
    {
        freeFont(renderer->titleFontOutline);
        free(renderer->titleFontOutline);
    }
    if (renderer->menuFontAllocated)
    {
        freeFont(renderer->menuFont);
        free(renderer->menuFont);
    }

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
static void drawMenuText(menuManiaRenderer_t* renderer, const char* text, int16_t x, int16_t y, bool isSelected,
                         bool leftArrow, bool rightArrow, bool doubleArrows)
{
    // Pick colors based on selection
    paletteColor_t textColor = ROW_TEXT_COLOR;
    if (isSelected)
    {
        // Draw drop shadow for selected item
        for (int rows = 0; rows < PARALLELOGRAM_HEIGHT; rows++)
        {
            drawLineFast(x + PARALLELOGRAM_HEIGHT - rows - 1 + DROP_SHADOW_OFFSET,                       //
                         y + rows + DROP_SHADOW_OFFSET,                                                  //
                         x + PARALLELOGRAM_HEIGHT - rows - 1 + PARALLELOGRAM_WIDTH + DROP_SHADOW_OFFSET, //
                         y + rows + DROP_SHADOW_OFFSET,                                                  //
                         selectedShadowColors[renderer->selectedShadowIdx]);
        }

        // Bounce the item
        y += selectedBounceOffsets[renderer->selectedBounceIdx];
    }

    // Draw background for the menu item
    for (int rows = 0; rows < PARALLELOGRAM_HEIGHT; rows++)
    {
        drawLineFast(x + PARALLELOGRAM_HEIGHT - rows - 1,                       //
                     y + rows,                                                  //
                     x + PARALLELOGRAM_HEIGHT - rows - 1 + PARALLELOGRAM_WIDTH, //
                     y + rows,                                                  //
                     ROW_COLOR);
    }

    // Draw the text
    if (isSelected && textWidth(renderer->menuFont, text) > (PARALLELOGRAM_WIDTH - PARALLELOGRAM_HEIGHT - 10))
    {
        drawTextMarquee(renderer->menuFont, textColor, text, x + PARALLELOGRAM_HEIGHT + 10, y + 2,
                        x + PARALLELOGRAM_WIDTH - 5, &renderer->selectedMarqueeTimer);
    }
    else
    {
        drawTextEllipsize(renderer->menuFont, textColor, text, x + PARALLELOGRAM_HEIGHT + 10, y + 2,
                          PARALLELOGRAM_WIDTH - PARALLELOGRAM_HEIGHT - 10, false);
    }

    // Draw the left arrow, if applicable
    if (leftArrow)
    {
        // Draw Arrow
        int16_t arrowX     = x + (PARALLELOGRAM_HEIGHT / 2) + 4;
        int16_t lineHeight = (PARALLELOGRAM_HEIGHT - (ARROW_MARGIN * 2)) / 2;

        // Arrows are stacked to add thickness
        int16_t numArrows = ARROW_WIDTH;

        // Draw another arrow if requested
        if (doubleArrows)
        {
            numArrows += ARROW_WIDTH;
        }

        for (int16_t aw = 0; aw < numArrows; aw++)
        {
            drawLineFast(arrowX + lineHeight, y + ARROW_MARGIN, //
                         arrowX, y + ARROW_MARGIN + lineHeight, //
                         textColor);
            drawLineFast(arrowX, y + ARROW_MARGIN + lineHeight,                    //
                         arrowX + lineHeight, y + ARROW_MARGIN + (lineHeight * 2), //
                         textColor);
            arrowX++;

            // Check for extra spacing between double arrows
            if (ARROW_WIDTH - 1 == aw)
            {
                arrowX += numArrows;
            }
        }
    }

    // Draw the right arrow, if applicable
    if (rightArrow)
    {
        // Draw Arrow
        int16_t arrowX     = PARALLELOGRAM_WIDTH + x;
        int16_t lineHeight = (PARALLELOGRAM_HEIGHT - (ARROW_MARGIN * 2)) / 2;

        // Arrows are stacked to add thickness
        int16_t numArrows = ARROW_WIDTH;

        // Draw another arrow if requested
        if (doubleArrows)
        {
            numArrows += ARROW_WIDTH;
        }

        for (int16_t aw = 0; aw < numArrows; aw++)
        {
            drawLineFast(arrowX, y + ARROW_MARGIN,                           //
                         arrowX + lineHeight, y + ARROW_MARGIN + lineHeight, //
                         textColor);
            drawLineFast(arrowX + lineHeight, y + ARROW_MARGIN + lineHeight, //
                         arrowX, y + ARROW_MARGIN + (lineHeight * 2),        //
                         textColor);
            arrowX--;

            // Check for extra spacing between double arrows
            if (ARROW_WIDTH - 1 == aw)
            {
                arrowX -= numArrows;
            }
        }
    }
}

/**
 * @brief Draw a background ring on the menu
 *
 * @param radius The radius of the ring
 * @param angle The angle of the ring for orbiting circles
 * @param color The color of the ring
 */
static void drawManiaRing(int16_t radius, int16_t angle, paletteColor_t color)
{
    // Draw the ring
    drawCircleOutline(TFT_WIDTH / 2, TFT_HEIGHT / 2, radius, RING_STROKE_THICKNESS, color);

    // Draw the the smaller ring on the orbit (two filled circles)
    vec_t circlePos = {
        .x = 0,
        .y = -radius + (RING_STROKE_THICKNESS / 2),
    };
    circlePos = rotateVec2d(circlePos, angle);
    drawCircleFilled((TFT_WIDTH / 2) + circlePos.x, (TFT_HEIGHT / 2) + circlePos.y, ORBIT_RING_RADIUS_1, color);
    drawCircleFilled((TFT_WIDTH / 2) + circlePos.x, (TFT_HEIGHT / 2) + circlePos.y,
                     ORBIT_RING_RADIUS_1 - RING_STROKE_THICKNESS, BG_COLOR);

    // Draw an opposite filled circle
    circlePos.x = -circlePos.x;
    circlePos.y = -circlePos.y;
    drawCircleFilled((TFT_WIDTH / 2) + circlePos.x, (TFT_HEIGHT / 2) + circlePos.y, ORBIT_RING_RADIUS_2, color);
}

/**
 * @brief Draw a themed menu to the display and control the LEDs
 *
 * @param menu The menu to draw
 * @param renderer The renderer to draw with
 * @param elapsedUs The time elapsed since this function was last called, for LED animation
 */
void drawMenuMania(menu_t* menu, menuManiaRenderer_t* renderer, int64_t elapsedUs)
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

    if (renderer->ledsOn)
    {
        // Run timer for LED excitation
        renderer->ledExciteTimer += elapsedUs;
        while (renderer->ledExciteTimer >= 40000 * 8)
        {
            renderer->ledExciteTimer -= 40000 * 8;
            uint32_t ledColor                      = paletteToRGB(BG_COLOR);
            renderer->leds[renderer->currentLed].r = ((ledColor >> 16) & 0xFF) / 2;
            renderer->leds[renderer->currentLed].g = ((ledColor >> 8) & 0xFF) / 2;
            renderer->leds[renderer->currentLed].b = ((ledColor >> 0) & 0xFF) / 2;
            renderer->currentLed                   = (renderer->currentLed + 1) % CONFIG_NUM_LEDS;
        }

        // Run timer for LED decay
        renderer->ledDecayTimer += elapsedUs;
        while (renderer->ledDecayTimer >= 16667)
        {
            renderer->ledDecayTimer -= 16667;
            for (int16_t i = 0; i < CONFIG_NUM_LEDS; i++)
            {
                if (renderer->leds[i].r)
                {
                    renderer->leds[i].r--;
                }
                if (renderer->leds[i].g)
                {
                    renderer->leds[i].g--;
                }
                if (renderer->leds[i].b)
                {
                    renderer->leds[i].b--;
                }
            }
        }

        // Set LEDs
        setLeds(renderer->leds, CONFIG_NUM_LEDS);
    }

    // For each ring
    for (int16_t i = 0; i < ARRAY_SIZE(renderer->rings); i++)
    {
        maniaRing_t* ring = &renderer->rings[i];

        // Run timer for orbit
        ring->orbitTimer += elapsedUs;
        while (ring->orbitTimer >= ring->orbitUsPerDegree)
        {
            ring->orbitTimer -= ring->orbitUsPerDegree;
            ring->orbitAngle += ring->orbitDirection;
        }

        // Run timer for ring size
        ring->diameterTimer += elapsedUs;
        while (ring->diameterTimer >= 22500)
        {
            ring->diameterTimer -= 22500;
            ring->diameterAngle++;
            if (ring->diameterAngle == 360)
            {
                ring->diameterAngle = 0;
            }
        }
    }

    // Run timer to cycle colors under the selected item
    renderer->selectedShadowTimer += elapsedUs;
    while (renderer->selectedShadowTimer > (100000))
    {
        renderer->selectedShadowTimer -= (100000);
        renderer->selectedShadowIdx = (renderer->selectedShadowIdx + 1) % ARRAY_SIZE(selectedShadowColors);
    }

    // Run a timer to bounce the selected item, when transitioned to
    if (0 != renderer->selectedBounceIdx)
    {
        renderer->selectedBounceTimer += elapsedUs;
        while (renderer->selectedBounceTimer > (16667))
        {
            renderer->selectedBounceTimer -= (16667);
            renderer->selectedBounceIdx = (renderer->selectedBounceIdx + 1) % ARRAY_SIZE(selectedBounceOffsets);
            if (0 == renderer->selectedBounceIdx)
            {
                break;
            }
        }
    }
    else
    {
        renderer->selectedBounceTimer = 0;
    }

    renderer->selectedMarqueeTimer += elapsedUs;

    // Clear the background
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, BG_COLOR);

    // Draw the rings
    for (int16_t i = 0; i < ARRAY_SIZE(renderer->rings); i++)
    {
        maniaRing_t* ring  = &renderer->rings[i];
        int16_t ringRadius = (MIN_RING_RADIUS + MAX_RING_RADIUS) / 2
                             + (((MAX_RING_RADIUS - MIN_RING_RADIUS) * getSin1024(ring->diameterAngle)) / 1024);
        drawManiaRing(ringRadius, ring->orbitAngle, ring->color);
    }

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

    int16_t tWidth = textWidth(renderer->titleFont, menu->title);

    // Draw blue hexagon behind the title
    int16_t titleBgX0 = (TFT_WIDTH - tWidth) / 2 - 6;
    int16_t titleBgX1 = (TFT_WIDTH + tWidth) / 2 + 6;
    int16_t titleBgY0 = y;
    int16_t titleBgY1 = y + TITLE_BG_HEIGHT;
    fillDisplayArea(titleBgX0, titleBgY0, titleBgX1, titleBgY1, TITLE_BG_COLOR);
    drawTriangleOutlined(titleBgX0, titleBgY0, titleBgX0, titleBgY1, titleBgX0 - (TITLE_BG_HEIGHT / 2),
                         (titleBgY0 + titleBgY1) / 2, TITLE_BG_COLOR, TITLE_BG_COLOR);
    drawTriangleOutlined(titleBgX1, titleBgY0, titleBgX1, titleBgY1, titleBgX1 + (TITLE_BG_HEIGHT / 2),
                         (titleBgY0 + titleBgY1) / 2, TITLE_BG_COLOR, TITLE_BG_COLOR);

    // Draw a title
    y += (TITLE_BG_HEIGHT - renderer->titleFont->height) / 2;
    // Draw the menu text
    drawText(renderer->titleFont, TITLE_TEXT_COLOR, menu->title, (TFT_WIDTH - tWidth) / 2, y);
    // Outline the menu text
    drawText(renderer->titleFontOutline, TEXT_OUTLINE_COLOR, menu->title, (TFT_WIDTH - tWidth) / 2, y);

    // Move to drawing the rows
    y = titleBgY1 + Y_SECTION_MARGIN;

    if (menu->items->length > ITEMS_PER_PAGE)
    {
        // Draw UP page indicator
        y -= (UP_ARROW_HEIGHT);
        for (int t = 0; t < UP_ARROW_HEIGHT - UP_ARROW_MARGIN; t++)
        {
            drawLineFast(PARALLELOGRAM_X_OFFSET + PARALLELOGRAM_HEIGHT - t + (UP_ARROW_HEIGHT * 2 - 1) / 2, y + t,
                         PARALLELOGRAM_X_OFFSET + PARALLELOGRAM_HEIGHT + t + (UP_ARROW_HEIGHT * 2 - 1) / 2, y + t,
                         ROW_COLOR);
        }
        y += (UP_ARROW_HEIGHT);
    }

    // Draw a page-worth of items
    for (uint8_t itemIdx = 0; itemIdx < ITEMS_PER_PAGE; itemIdx++)
    {
        if (NULL != pageStart)
        {
            menuItem_t* item = (menuItem_t*)pageStart->val;
            bool isSelected  = (menu->currentItem->val == item);

            // If there's a new selected item
            if (isSelected && renderer->selectedItem != item)
            {
                // Save it
                renderer->selectedItem = item;
                // Bounce the selected item
                renderer->selectedBounceIdx    = 1;
                renderer->selectedMarqueeTimer = 0;
            }
            else if (isSelected)
            {
                // If the selected option has changed
                if (menuItemHasOptions(item) || menuItemIsSetting(item))
                {
                    int32_t value = (item->options) ? item->currentOpt : item->currentSetting;
                    if (value != renderer->selectedValue)
                    {
                        renderer->selectedMarqueeTimer = 0;
                        renderer->selectedValue        = value;
                    }
                }
            }

            char buffer[64]   = {0};
            const char* label = getMenuItemLabelText(buffer, sizeof(buffer), item);

            bool leftArrow    = menuItemHasPrev(item) || menuItemIsBack(item);
            bool rightArrow   = menuItemHasNext(item) || menuItemHasSubMenu(item);
            bool doubleArrows = menuItemIsBack(item) || menuItemHasSubMenu(item);

            drawMenuText(renderer, label, PARALLELOGRAM_X_OFFSET, y, isSelected, leftArrow, rightArrow, doubleArrows);

            // Move to the next item
            pageStart = pageStart->next;
        }

        // Move to the next row
        y += PARALLELOGRAM_HEIGHT + ROW_MARGIN;
    }

    if (menu->items->length > ITEMS_PER_PAGE)
    {
        y += UP_ARROW_MARGIN;
        // Draw DOWN page indicator
        for (int16_t t = UP_ARROW_HEIGHT - UP_ARROW_MARGIN; t >= 0; t--)
        {
            drawLineFast(PARALLELOGRAM_X_OFFSET + PARALLELOGRAM_WIDTH - t - (UP_ARROW_HEIGHT * 2) / 2, y - t,
                         PARALLELOGRAM_X_OFFSET + PARALLELOGRAM_WIDTH + t - (UP_ARROW_HEIGHT * 2) / 2, y - t,
                         ROW_COLOR);
        }
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
void setManiaLedsOn(menuManiaRenderer_t* renderer, bool ledsOn)
{
    renderer->ledsOn = ledsOn;
    if (false == ledsOn)
    {
        memset(renderer->leds, 0, sizeof(renderer->leds));
        setLeds(renderer->leds, CONFIG_NUM_LEDS);
    }
}
