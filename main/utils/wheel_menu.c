//==============================================================================
// Includes
//==============================================================================

#include "wheel_menu.h"

#include "hdw-tft.h"
#include "shapes.h"
#include "fill.h"
#include "trigonometry.h"
#include "menu.h"
#include "wsg.h"
#include "esp_log.h"
#include "macros.h"
#include "geometry.h"

#include <esp_heap_caps.h>

#include <inttypes.h>
#include <string.h>

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    const char* label;
    const wsg_t* icon;
    uint8_t position;
    wheelScrollDir_t scroll;
    paletteColor_t selectedBg;
    paletteColor_t unselectedBg;
    const char* textIcon;
    bool customSize;
    int16_t width;
    int16_t height;
    wheelMenuShapeFlags_t shapeFlags;
} wheelItemInfo_t;

typedef struct
{
    int8_t drawOrder;
    uint16_t x;
    uint16_t y;
    wheelMenuShapeFlags_t shape;
    int16_t w;
    int16_t h;
    int16_t r;
    bool selected;
    const wsg_t* wsg;
    paletteColor_t bgColor;
    const char* text;
} wheelDrawInfo_t;

//==============================================================================
// Function Prototypes
//==============================================================================

static wheelItemInfo_t* findInfo(wheelMenuRenderer_t* renderer, const char* label);
static wheelItemInfo_t* findOrAddInfo(wheelMenuRenderer_t* renderer, const char* label);
static int cmpDrawInfo(const void* a, const void* b);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initializes and returns a new wheel menu with the default settings
 *
 * @param font The font to draw menu item labels with
 * @param anchorAngle The angle where the center of the first item will be anchored
 * @param textBox If non-NULL, defines the bounding box where the current item label will be written
 * @return A newly allocated wheel menu renderer
 */
wheelMenuRenderer_t* initWheelMenu(const font_t* font, uint16_t anchorAngle, const rectangle_t* textBox)
{
    wheelMenuRenderer_t* renderer = heap_caps_calloc(1, sizeof(wheelMenuRenderer_t), MALLOC_CAP_SPIRAM);

    renderer->font        = font;
    renderer->anchorAngle = anchorAngle;
    renderer->textBox     = textBox;

    renderer->textColor    = c000;
    renderer->borderColor  = c000;
    renderer->unselBgColor = c333;
    renderer->selBgColor   = c555;

    renderer->spokeR = TFT_HEIGHT / 4;
    renderer->unselR = TFT_HEIGHT / 10;
    renderer->selR   = TFT_HEIGHT / 7;

    renderer->x = TFT_WIDTH / 2;
    renderer->y = TFT_HEIGHT / 2;

    renderer->active = false;

    return renderer;
}

/**
 * @brief Deinitialize and free all memory associated with the wheel menu renderer
 *
 * @param renderer The whele menu renderer to deinitialize
 */
void deinitWheelMenu(wheelMenuRenderer_t* renderer)
{
    wheelItemInfo_t* info = NULL;
    while ((info = pop(&renderer->itemInfos)))
    {
        free(info);
    }

    free(renderer);
}

/**
 * @brief Set the text color for the selected item
 *
 * @param renderer The wheel menu renderer
 * @param textColor The text color for the selected item
 */
void wheelMenuSetColor(wheelMenuRenderer_t* renderer, paletteColor_t textColor)
{
    renderer->textColor = textColor;
}

/**
 * @brief Sets the icon and ring position for the item with the given label
 *
 * @param renderer The wheel menu renderer
 * @param label The label of the item to set the icon and position for
 * @param icon  The icon to use when drawing the menu item
 * @param position The order of the item in the ring, centered around the anchor angle
 * @param scrollDir Which axis this item should be scrollable in
 */
void wheelMenuSetItemInfo(wheelMenuRenderer_t* renderer, const char* label, const wsg_t* icon, uint8_t position,
                          wheelScrollDir_t scrollDir)
{
    wheelItemInfo_t* info = findOrAddInfo(renderer, label);

    info->icon     = icon;
    info->position = position;
    info->scroll   = scrollDir;

    if (label == mnuBackStr)
    {
        renderer->customBack = true;
    }
}

/**
 * @brief Set the icon for an item with the given label
 *
 * @param renderer The wheel menu renderer
 * @param label The label of the item to set the icon and position for
 * @param icon  The icon to use when drawing the menu item
 */
void wheelMenuSetItemIcon(wheelMenuRenderer_t* renderer, const char* label, const wsg_t* icon)
{
    findOrAddInfo(renderer, label)->icon = icon;
}

/**
 * @brief Set both the selected and unselected background colors for the item with the given laebl
 *
 * @param renderer The wheel menu renderer
 * @param label The label of the item to set the color of
 * @param selectedBg The background color for the item when selected
 * @param unselectedBg The background color for the item when not selected
 */
void wheelMenuSetItemColor(wheelMenuRenderer_t* renderer, const char* label, paletteColor_t selectedBg,
                           paletteColor_t unselectedBg)
{
    wheelItemInfo_t* info = findOrAddInfo(renderer, label);

    info->unselectedBg = unselectedBg;
    info->selectedBg   = selectedBg;
}

/**
 * @brief Set a short text string to use a very short text tring as an item's icon, instead of an image
 *
 * @param renderer The wheel menu renderer
 * @param label The label to set the icon for
 * @param textIcon The short text string to draw as the icon text
 */
void wheelMenuSetItemTextIcon(wheelMenuRenderer_t* renderer, const char* label, const char* textIcon)
{
    wheelItemInfo_t* info = findOrAddInfo(renderer, label);

    info->textIcon = textIcon;
}

/**
 * @brief Set specific dimensions for a menu item
 *
 * @param renderer The whee lmenu renderer
 * @param label The label of the item to configure dimensions for
 * @param w The width of the item, or a negative number for "fit to content"
 * @param h The height of the item, or a negative number for "fit to content"
 * @param shapeFlags Extra flags for configuring the shape of the item
 */
void wheelMenuSetItemSize(wheelMenuRenderer_t* renderer, const char* label, int16_t w, int16_t h,
                          wheelMenuShapeFlags_t shapeFlags)
{
    wheelItemInfo_t* info = findOrAddInfo(renderer, label);

    info->width      = w;
    info->height     = h;
    info->shapeFlags = shapeFlags;
}

/**
 * @brief Draw the wheel menu onto the screen
 *
 * @param menu The current top-level menu to render
 * @param renderer The wheel menu rendrer
 * @param elapsedUs The number of microseconds elapsed since the last frame was drawn
 */
void drawWheelMenu(menu_t* menu, wheelMenuRenderer_t* renderer, int64_t elapsedUs)
{
    // Draw background circle for the unselected area
    // drawCircleFilled(renderer->x, renderer->y, renderer->unselR, renderer->unselBgColor);

    // Add one for the convenience of looping
    uint16_t spokeR = renderer->spokeR;

    // If we haven't customized the back option to be around the ring, then we use the center
    // So, remove one from the ring items to compensate
    bool missingItem  = (menu->parentMenu
                        || (!menu->parentMenu && menu->items->last && menu->items->last->val
                            && ((menuItem_t*)menu->items->last->val)->label == mnuBackStr));
    uint8_t ringItems = menu->items->length - (missingItem ? 1 : 0);

    uint16_t unselR = renderer->unselR;
    uint16_t selR   = renderer->selR;
    int16_t inR     = renderer->unselR / 3 - 6;
    int16_t outR    = -renderer->unselR / 3 + 6;
    bool alternate  = false;

    wheelItemInfo_t* backInfo = NULL;

    menuItem_t* curItem      = (menu->currentItem) ? menu->currentItem->val : NULL;
    wheelItemInfo_t* curInfo = curItem ? findInfo(renderer, curItem->label ? curItem->label : *curItem->options) : NULL;
    uint8_t curPosition      = curInfo ? curInfo->position : UINT8_MAX;

    if (renderer->zoomed && curItem && menuItemHasOptions(curItem))
    {
        ringItems = curItem->numOptions;
    }
    uint16_t anchorAngle = (360 + renderer->anchorAngle - (360 / ringItems / 2)) % 360;

#define wheelAspectRatio ((ringItems >= 10) ? (TFT_WIDTH + 60) : TFT_WIDTH) / TFT_HEIGHT
    if (ringItems >= 10)
    {
        inR += 15;
        outR += 15;
    }

    // Just figure out what to draw where, then draw them all
    wheelDrawInfo_t drawInfos[ringItems + 1];
    uint8_t curDraw = 0;

    if (renderer->zoomed)
    {
        wheelScrollDir_t zoomSetting = curInfo ? (curInfo->scroll & MASK_ZOOM) : 0;
        if (curItem == NULL)
        {
            ESP_LOGW("Wheel", "Zoomed but there's no selected item?");
            renderer->zoomed = false;
        }
        else if ((menuItemHasOptions(curItem) || (zoomSetting == ZOOM_SUBMENU)) && (zoomSetting != ZOOM_GAUGE))
        {
            ringItems = curItem->numOptions;
            for (int i = 0; i < curItem->numOptions; i++)
            {
                wheelItemInfo_t* optInfo = findInfo(renderer, curItem->options[i]);
                uint8_t optPos           = (optInfo && optInfo->position != UINT8_MAX) ? optInfo->position : i;

                if (curItem->currentOpt == i && !renderer->zoomBackSelected)
                {
                    drawInfos[curDraw].drawOrder = -INT8_MAX;
                    drawInfos[curDraw].selected  = true;
                    drawInfos[curDraw].bgColor   = optInfo ? optInfo->selectedBg : renderer->selBgColor;
                }
                else
                {
                    // We want to draw the selected one last, and the farthest away one first
                    // So, order the items by how close they are to the selected one
                    // -((ringItems / 2) - MIN(abs(info->position - curPosition), ringItems - abs(info->position -
                    // curPosition)));
                    drawInfos[curDraw].drawOrder
                        = -((ringItems / 2)
                            - MIN(abs(optPos - curItem->currentOpt), ringItems - abs(optPos - curItem->currentOpt)));
                    drawInfos[curDraw].selected = false;
                    drawInfos[curDraw].bgColor  = optInfo ? optInfo->unselectedBg : renderer->unselBgColor;
                }

                // Calculate where this sector starts
                uint16_t centerAngle = (anchorAngle + optPos * 360 / ringItems + (180 / ringItems)) % 360;

                if (ringItems > 8)
                {
                    alternate = true;
                }
                uint16_t itemSpokeR = alternate ? (((optPos % 2) == 1) ? (spokeR + inR) : (spokeR + outR)) : spokeR;

                drawInfos[curDraw].x     = renderer->x + getCos1024(centerAngle) * itemSpokeR * wheelAspectRatio / 1024;
                drawInfos[curDraw].y     = renderer->y - getSin1024(centerAngle) * itemSpokeR / 1024;
                drawInfos[curDraw].wsg   = optInfo ? optInfo->icon : NULL;
                drawInfos[curDraw].text  = optInfo ? optInfo->textIcon : NULL;
                drawInfos[curDraw].shape = optInfo ? optInfo->shapeFlags : WM_SHAPE_DEFAULT;
                drawInfos[curDraw].w     = optInfo ? optInfo->width : 0;
                drawInfos[curDraw].h     = optInfo ? optInfo->height : 0;
                drawInfos[curDraw].r     = (optInfo && !optInfo->shapeFlags) ? optInfo->width : 0;
                curDraw++;
            }

            // Make the info for the back / confirm icon
            drawInfos[curDraw].drawOrder = (!curItem || menuItemIsBack(curItem)) ? INT8_MAX : -INT8_MIN;
            drawInfos[curDraw].x         = renderer->x;
            drawInfos[curDraw].y         = renderer->y;
            drawInfos[curDraw].selected  = renderer->zoomBackSelected;
            // TODO: Confirm icon?
            drawInfos[curDraw].wsg     = NULL;
            drawInfos[curDraw].bgColor = renderer->zoomBackSelected ? renderer->selBgColor : renderer->unselBgColor;
            drawInfos[curDraw].text    = (backInfo && backInfo->textIcon) ? backInfo->textIcon : NULL;
            drawInfos[curDraw].shape   = backInfo ? backInfo->shapeFlags : WM_SHAPE_DEFAULT;
            drawInfos[curDraw].w       = backInfo ? backInfo->width : 0;
            drawInfos[curDraw].h       = backInfo ? backInfo->height : 0;
            drawInfos[curDraw].r       = (backInfo && !backInfo->shapeFlags) ? backInfo->width : 0;
            curDraw++;
        }
        else if (menuItemIsSetting(curItem) || (zoomSetting == ZOOM_GAUGE))
        {
            drawCircleFilled(renderer->x, renderer->y, renderer->spokeR, renderer->unselBgColor);
            drawCircle(renderer->x, renderer->y, renderer->spokeR, renderer->borderColor);
            int tickStart = renderer->spokeR - 4;
            int tickEnd   = renderer->spokeR;
            int textR     = renderer->spokeR + 5;
            uint16_t curAngle
                = ((curItem->currentSetting - curItem->minSetting) * 180) / (curItem->maxSetting - curItem->minSetting)
                  + 180;
            char tickLabel[16];
            bool hasOptions  = menuItemHasOptions(curItem);
            int nextOptIndex = 0;

            uint16_t lastTickAngle  = 360;
            rectangle_t lastTextBox = {0};

            // Draw tick marks
            for (int i = curItem->minSetting; i <= curItem->maxSetting; i++)
            {
                // Offset
                uint16_t tickAngle
                    = ((i - curItem->minSetting) * -180) / (curItem->maxSetting - curItem->minSetting) + 180;
                ESP_LOGD("Wheel", "tick angle: %" PRIu16, tickAngle);

                // Start tick
                uint16_t x0 = renderer->x + getCos1024(tickAngle) * tickStart / 1024;
                uint16_t y0 = renderer->y - getSin1024(tickAngle) * tickStart / 1024;

                // End tick
                uint16_t x1 = renderer->x + getCos1024(tickAngle) * tickEnd / 1024;
                uint16_t y1 = renderer->y - getSin1024(tickAngle) * tickEnd / 1024;

                uint16_t textX = renderer->x + getCos1024(tickAngle) * textR / 1024;
                uint16_t textY = renderer->y - getSin1024(tickAngle) * textR / 1024;

                snprintf(tickLabel, sizeof(tickLabel), "%" PRIu8, i);
                uint16_t textLen = textWidth(renderer->font, tickLabel) + 1;

                textX += (getCos1024(tickAngle) - 1024) * textLen / 2048;
                textY -= (getSin1024(tickAngle) + 1024) * renderer->font->height / 2048;

                ESP_LOGD("Wheel", "tick loc: (%" PRIu16 ", %" PRIu16 ") ->  (%" PRIu16 ", %" PRIu16 ")", x0, y0, x1,
                         y1);

                drawLine(x0, y0, x1, y1, renderer->borderColor, 0);

                if (hasOptions ? (nextOptIndex < curItem->numOptions && i == curItem->settingVals[nextOptIndex])
                               : (tickAngle > 5 + lastTickAngle || tickAngle < lastTickAngle - 5))
                {
                    rectangle_t textBox = {
                        .pos = {
                            .x = textX,
                            .y = textY,
                        },
                        .width = textWidth(renderer->font, hasOptions ? curItem->options[nextOptIndex] : tickLabel),
                        .height = renderer->font->height + 1,
                    };

                    if (!rectRectIntersection(textBox, lastTextBox, NULL))
                    {
                        drawText(renderer->font, renderer->textColor,
                                 hasOptions ? curItem->options[nextOptIndex] : tickLabel, textX, textY);
                        lastTickAngle = tickAngle;
                        lastTextBox   = textBox;
                    }

                    if (hasOptions)
                    {
                        nextOptIndex++;
                    }
                }
            }

            drawLine(renderer->x, renderer->y, renderer->x + getCos1024(curAngle) * (tickStart - 2) / 1024,
                     renderer->y + getSin1024(curAngle) * (tickStart - 2) / 1024, c500, 0);
        }
        else if (zoomSetting == ZOOM_CONVEYOR)
        {
            ESP_LOGI("Wheel", "TODO: ZOOM_CONVEYOR");
        }
    }
    else
    {
        if (ringItems > 8)
        {
            alternate = true;

            /*if (ringItems > 12)
            {
                unselR -= 2;
                selR -= 5;
                alternate = true;
            }*/
        }

        node_t* node = menu->items->last;
        while (node != NULL)
        {
            menuItem_t* item = node->val;
            // Find menu item by either its label or its first option label
            wheelItemInfo_t* info = findInfo(renderer, item->label ? item->label : *item->options);

            if (info != NULL)
            {
                if (menuItemIsBack(item))
                {
                    // This is the center "back" circle, so don't draw it as a circle
                    // But, do save the info for later
                    backInfo = info;
                }
                else
                {
                    // Calculate where this sector starts
                    uint16_t centerAngle = (anchorAngle + info->position * 360 / ringItems + (180 / ringItems)) % 360;

                    bool isCur = (renderer->touched && menu->currentItem == node);
                    uint16_t itemSpokeR
                        = alternate ? (((info->position % 2) == 1) ? spokeR + inR : spokeR + outR) : spokeR;

                    drawInfos[curDraw].drawOrder
                        = -((ringItems / 2)
                            - MIN(abs(info->position - curPosition), ringItems - abs(info->position - curPosition)));
                    drawInfos[curDraw].x = renderer->x + getCos1024(centerAngle) * itemSpokeR * wheelAspectRatio / 1024;
                    drawInfos[curDraw].y = renderer->y - getSin1024(centerAngle) * itemSpokeR / 1024;
                    drawInfos[curDraw].selected = isCur;
                    drawInfos[curDraw].wsg      = info->icon;
                    drawInfos[curDraw].bgColor  = isCur ? info->selectedBg : info->unselectedBg;
                    drawInfos[curDraw].text     = (info && info->textIcon) ? info->textIcon : NULL;
                    drawInfos[curDraw].w        = info ? info->width : 0;
                    drawInfos[curDraw].h        = info ? info->height : 0;
                    drawInfos[curDraw].shape    = info ? info->shapeFlags : WM_SHAPE_DEFAULT;
                    drawInfos[curDraw].r        = (info && !info->shapeFlags) ? info->width : 0;
                    curDraw++;

                    ESP_LOGD("Wheel", "drawOrder is %" PRIu8 " for %s", drawInfos[curDraw].drawOrder, item->label);
                }
            }
            node = node->prev;
        }

        bool curBackOrNull = (!curItem || menuItemIsBack(curItem));

        // Back info found
        if (backInfo)
        {
            drawInfos[curDraw].drawOrder = curBackOrNull ? INT8_MAX : 0;
            drawInfos[curDraw].x         = renderer->x;
            drawInfos[curDraw].y         = renderer->y;
            drawInfos[curDraw].selected  = curBackOrNull;
            drawInfos[curDraw].wsg       = backInfo->icon;
            drawInfos[curDraw].bgColor   = curBackOrNull ? backInfo->selectedBg : backInfo->unselectedBg;
            drawInfos[curDraw].text      = (backInfo && backInfo->textIcon) ? backInfo->textIcon : NULL;
            drawInfos[curDraw].w         = backInfo->width ? backInfo->width : 0;
            drawInfos[curDraw].h         = backInfo->height ? backInfo->height : 0;
            drawInfos[curDraw].shape     = backInfo ? backInfo->shapeFlags : WM_SHAPE_DEFAULT;
            drawInfos[curDraw].r         = 0;
            curDraw++;
        }
        else
        {
            drawInfos[curDraw].drawOrder = curBackOrNull ? INT8_MAX : 0;
            drawInfos[curDraw].x         = renderer->x;
            drawInfos[curDraw].y         = renderer->y;
            drawInfos[curDraw].selected  = curBackOrNull;
            drawInfos[curDraw].wsg       = NULL;
            drawInfos[curDraw].bgColor   = (curBackOrNull) ? renderer->selBgColor : renderer->unselBgColor;
            drawInfos[curDraw].text      = NULL; //(curInfo && curInfo->textIcon) ? curInfo->textIcon : NULL;
            drawInfos[curDraw].shape     = WM_SHAPE_DEFAULT;
            drawInfos[curDraw].r         = 0;
            curDraw++;
        }
    }

    // Sort by the draw order
    if (0 < curDraw)
    {
        qsort(drawInfos, curDraw, sizeof(wheelDrawInfo_t), cmpDrawInfo);
    }

    for (int i = 0; i < curDraw; i++)
    {
        wheelDrawInfo_t* info = &drawInfos[i];
        int r                 = info->selected ? selR : unselR;
        int w                 = r;
        int h                 = r;
        bool hasWsg           = (NULL != info->wsg);
        bool hasText          = (NULL != info->text);

#define PAD_FILL 5

        int minW = (hasWsg ? info->wsg->w : (hasText ? textWidth(renderer->font, info->text) + PAD_FILL : r));
        int minH = (hasWsg ? info->wsg->h : (hasText ? renderer->font->height + 1 + PAD_FILL : r));

        if (info->selected)
        {
            r = selR;
        }
        else
        {
            if (info->r < 0)
            {
                // Fit radius to content
                if (hasWsg)
                {
                    r = MAX(MAX(minW, minH) * 3 / 4, unselR);
                }
                else
                {
                    r = MAX(MAX(minW, minH) / 2, unselR);
                }
            }
            else if (info->r > 0)
            {
                r = info->r;
            }
            else
            {
                r = unselR;
            }
        }

        if (info->w < 0)
        {
            w = minW;
        }
        else if (info->w > 0)
        {
            w = info->w;
        }
        if (info->selected)
        {
            w *= 2;
        }

        if (info->h < 0)
        {
            h = minH;
        }
        else if (info->h > 0)
        {
            h = info->h;
        }
        if (info->selected)
        {
            h = h * 3 / 2;
        }

        // Draw shape FILL only
        switch (info->shape)
        {
            case WM_SHAPE_SQUARE:
                fillDisplayArea(info->x - w / 2, info->y - h / 2, w + 1, h + 1, info->bgColor);
                break;

            case WM_SHAPE_ROUNDED_RECT:
                drawRoundedRect(info->x - w / 2, info->y - h / 2 - 1, info->x + w / 2, info->y + h / 2, 6,
                                info->bgColor, cTransparent);
                break;

            case WM_SHAPE_DEFAULT:
            default:
                drawCircleFilled(info->x, info->y, r, info->bgColor);
                break;
        }

        if (info->wsg)
        {
            drawWsgSimple(info->wsg, info->x - info->wsg->w / 2, info->y - info->wsg->h / 2);
        }
        if (info->text)
        {
            uint16_t textW = textWidth(renderer->font, info->text);
            drawText(renderer->font, renderer->textColor, info->text, info->x - textW / 2,
                     info->y - (renderer->font->height - 1) / 2);
        }

        switch (info->shape)
        {
            case WM_SHAPE_SQUARE:
                drawRect(info->x - w / 2, info->y - h / 2, info->x + w / 2 + 1, info->y + h / 2 + 1,
                         renderer->borderColor);
                break;

            case WM_SHAPE_ROUNDED_RECT:
                drawRoundedRect(info->x - w / 2, info->y - h / 2 - 1, info->x + w / 2, info->y + h / 2, 6, cTransparent,
                                renderer->borderColor);
                break;

            case WM_SHAPE_DEFAULT:
            default:
                drawCircle(info->x, info->y, r, renderer->borderColor);
                break;
        }
    }

    if (renderer->textBox)
    {
        char buffer[128]  = {0};
        const char* label = menu->title;

        if (renderer->touched)
        {
            label = menu->currentItem ? getMenuItemLabelText(buffer, sizeof(buffer) - 1, menu->currentItem->val)
                                      : "Close Menu";
        }
        else if (renderer->zoomed)
        {
            if (renderer->zoomBackSelected)
            {
                label = mnuBackStr;
            }
            else
            {
                label = menu->currentItem ? getMenuItemLabelText(buffer, sizeof(buffer) - 1, menu->currentItem->val)
                                          : mnuBackStr;
            }
        }

        if (label != NULL)
        {
            renderer->timer += elapsedUs;

            uint16_t textW = textWidth(renderer->font, label);

            if (textW > renderer->textBox->width)
            {
                drawTextMarquee(renderer->font, renderer->textColor, label, renderer->textBox->pos.x,
                                renderer->textBox->pos.y + (renderer->textBox->height - renderer->font->height - 1) / 2,
                                renderer->textBox->width, &renderer->timer);
            }
            else
            {
                drawText(renderer->font, renderer->textColor, label,
                         renderer->textBox->pos.x + (renderer->textBox->width - textW) / 2,
                         renderer->textBox->pos.y + (renderer->textBox->height - renderer->font->height - 1) / 2);
            }
        }
    }
}

/**
 * @brief Register a touchpad touch event with the wheel menu
 *
 * @param menu The menu touched
 * @param renderer The wheel menu renderer
 * @param angle The touchpad touch angle
 * @param radius The touchpad touch radius
 * @return menu_t* The current top-level menu, menuButton() would return
 */
menu_t* wheelMenuTouch(menu_t* menu, wheelMenuRenderer_t* renderer, uint16_t angle, uint16_t radius)
{
    renderer->touched = true;
    renderer->active  = true;

    if (radius <= 512)
    {
        if (renderer->zoomed)
        {
            renderer->timer            = 0;
            renderer->zoomBackSelected = true;
            return menu;
        }
        else if (!renderer->customBack && menu->parentMenu)
        {
            // Only navigate if we're not already on the "Back" item.
            if (!menu->currentItem || ((menuItem_t*)menu->currentItem->val)->label != mnuBackStr)
            {
                renderer->timer = 0;
                return menuNavigateToItem(menu, mnuBackStr);
            }

            // Just return the menu, the callback was already called
            return menu;
        }
        else
        {
            if (menu->currentItem)
            {
                renderer->timer   = 0;
                menu->currentItem = NULL;
                menu->cbFunc(NULL, false, 0);
            }
            return menu;
        }
    }

    renderer->zoomBackSelected = false;

    bool missingItem = (menu->parentMenu
                        || (!menu->parentMenu && menu->items->last && menu->items->last->val
                            && ((menuItem_t*)menu->items->last->val)->label == mnuBackStr));

    if (renderer->zoomed && menu->currentItem)
    {
        menuItem_t* cur = menu->currentItem->val;

        uint8_t min, max;
        if (menuItemHasOptions(cur))
        {
            min = 0;
            max = cur->numOptions - 1;
        }
        else
        {
            min = cur->minSetting;
            max = cur->maxSetting;
        }

        wheelItemInfo_t* info = findInfo(renderer, cur->label);
        if (menuItemHasOptions(cur) && (!info || (ZOOM_GAUGE != (info->scroll & MASK_ZOOM))))
        {
            uint8_t ringItems = cur->numOptions;
            // For actual options values, with distinct items
            uint16_t anchorAngle = (360 + renderer->anchorAngle - (360 / ringItems / 2)) % 360;
            if ((angle % 360) < anchorAngle)
            {
                // just add 360 to it -- so if e.g. angle is 15 and startAngle is 20, angle
                // would then be 375, and (375 - 20) = 355, so the offset is still < 360
                angle = (angle % 360) + 360;
            }

            uint8_t selection = (angle - anchorAngle) * ringItems / 360;

            // Find corresponding selection
            if (selection < ringItems)
            {
                // "selection" refers to the elemnt, in order of its position around the ring
                // the items could be rearranged from the actual option order, so find out the real one
                for (int optIndex = 0; optIndex < ringItems; optIndex++)
                {
                    wheelItemInfo_t* optInfo = findInfo(renderer, cur->options[optIndex]);
                    // If there's some info, use the position it specifies, othewise the index
                    uint8_t pos = (optInfo && optInfo->position != UINT8_MAX) ? optInfo->position : optIndex;

                    // The position matches the selection index
                    if (pos == selection)
                    {
                        if ((cur->currentOpt != optIndex || renderer->zoomValue != optIndex))
                        {
                            renderer->zoomValue = optIndex;
                            renderer->timer     = 0;
                            return menuSetCurrentOption(menu, optIndex);
                        }
                        break;
                    }
                }
            }
        }
        else
        {
            // For setting values, with a gauge style input

            // If the angle is in the top half, just clamp it to wherever's closer
            uint16_t zoomAngle = (angle > 270) ? 0 : (angle > 180 ? 180 : angle);
            uint8_t selection  = (180 - zoomAngle) * (max - min) / 180 + min;
            ESP_LOGD("Wheel", "Selection is %" PRIu8 ", from %" PRIu16, selection, zoomAngle);

            if (cur->currentOpt != selection)
            {
                renderer->zoomValue = selection;

                if (menuItemHasOptions(cur))
                {
                    renderer->timer = 0;
                }
                return menuSetCurrentOption(menu, selection);
            }
        }
    }
    else
    {
        // Compensate for the "Back" item unless it's included in the ring
        uint8_t ringItems    = menu->items->length - (missingItem ? 1 : 0);
        uint16_t anchorAngle = (360 + renderer->anchorAngle - (360 / ringItems / 2)) % 360;

        // Check if the angle is actually before the starting angle
        if ((angle % 360) < anchorAngle)
        {
            // just add 360 to it -- so if e.g. angle is 15 and startAngle is 20, angle
            // would then be 375, and (375 - 20) = 355, so the offset is still < 360
            angle = (angle % 360) + 360;
        }

        // Calculate the offset
        uint8_t index = (angle - anchorAngle) * ringItems / 360;

        if (index < ringItems)
        {
            node_t* node = menu->items->last;

            // Find the item configured for that offset
            while (node != NULL)
            {
                menuItem_t* menuItem  = node->val;
                wheelItemInfo_t* info = findInfo(renderer, menuItem->label ? menuItem->label : menuItem->options[0]);
                if (info && (info->position == index))
                {
                    // Only navigate to it if it's not the current item already
                    if (node != menu->currentItem)
                    {
                        renderer->timer = 0;
                        return menuNavigateToItem(menu, info->label);
                    }

                    return menu;
                }

                node = node->prev;
            }
        }
    }

    return menu;
}

/**
 * @brief Handle a button press, used for when a setting or option menu item can be scrolled.
 *
 * The direction of the scroll depends on the wheelScrollDir_t value passed to wheelMenuSetItemInfo()
 *
 * @param menu The menu to accept the button
 * @param renderer The wheel menu renderer
 * @param evt The button event to be handled
 * @return menu_t* The new top-level menu after handling the button press
 */
menu_t* wheelMenuButton(menu_t* menu, wheelMenuRenderer_t* renderer, const buttonEvt_t* evt)
{
    if (evt->down && menu->currentItem)
    {
        menuItem_t* item      = menu->currentItem->val;
        wheelItemInfo_t* info = findInfo(renderer, item->options ? item->options[0] : item->label);

        if (info && info->scroll != NO_SCROLL)
        {
            switch (evt->button)
            {
                case PB_UP:
                {
                    if (info->scroll & SCROLL_VERT)
                    {
                        renderer->timer = 0;
                        return (info->scroll & SCROLL_REVERSE) ? menuNavigateToPrevOption(menu)
                                                               : menuNavigateToNextOption(menu);
                    }

                    break;
                }

                case PB_DOWN:
                {
                    if (info->scroll & SCROLL_VERT)
                    {
                        renderer->timer = 0;
                        return (info->scroll & SCROLL_REVERSE) ? menuNavigateToNextOption(menu)
                                                               : menuNavigateToPrevOption(menu);
                    }
                    break;
                }

                case PB_LEFT:
                {
                    if (info->scroll & SCROLL_HORIZ)
                    {
                        renderer->timer = 0;
                        return (info->scroll & SCROLL_REVERSE) ? menuNavigateToNextOption(menu)
                                                               : menuNavigateToPrevOption(menu);
                    }
                    break;
                }

                case PB_RIGHT:
                {
                    if (info->scroll & SCROLL_HORIZ)
                    {
                        renderer->timer = 0;
                        return (info->scroll & SCROLL_REVERSE) ? menuNavigateToPrevOption(menu)
                                                               : menuNavigateToNextOption(menu);
                    }
                    break;
                }

                case PB_A:
                {
                    renderer->timer = 0;
                    return menuSelectCurrentItem(menu);
                }

                case PB_B:
                case PB_SELECT:
                case PB_START:
                default:
                    break;
            }
        }
    }

    return menu;
}

/**
 * @brief Inform the wheel menu that the touchpad has been released, selecting the current item
 *
 * @param menu The menu released
 * @param renderer The wheel menu renderer
 * @return menu_t* The current top-level menu, as returned by menuButton()
 */
menu_t* wheelMenuTouchRelease(menu_t* menu, wheelMenuRenderer_t* renderer)
{
    if (!renderer->touched)
    {
        return menu;
    }

    renderer->touched = false;

    if (!renderer->active)
    {
        return menu;
    }

    if (menu->currentItem)
    {
        menuItem_t* cur = menu->currentItem->val;
        renderer->timer = 0;

        if (!renderer->zoomed && (menuItemHasOptions(cur) || menuItemIsSetting(cur)))
        {
            menuSelectCurrentItem(menu);
            renderer->zoomed = true;
            if (menuItemHasOptions(cur))
            {
                renderer->zoomValue = cur->currentOpt;
            }
            else if (menuItemIsSetting(cur))
            {
                renderer->zoomValue = cur->currentSetting;
            }
            return menu;
        }
        else if (!menuItemHasSubMenu(cur) && !menuItemIsBack(cur))
        {
            if (renderer->zoomed && renderer->zoomValue != UINT8_MAX)
            {
                if (menuItemHasOptions(cur))
                {
                    if (cur->currentOpt != renderer->zoomValue)
                    {
                        menuSetCurrentOption(menu, renderer->zoomValue);
                    }
                }
                else if (menuItemIsSetting(cur))
                {
                    if (cur->currentSetting != renderer->zoomValue)
                    {
                        menuSetCurrentOption(menu, renderer->zoomValue);
                    }
                }
            }
            // Don't stay active after the touch is released
            renderer->active = false;

            menuSelectCurrentItem(menu);

            while (NULL != menu->parentMenu)
            {
                menu = menu->parentMenu;
            }

            renderer->zoomed = false;
            return menu;
        }

        return menuSelectCurrentItem(menu);
    }
    else
    {
        renderer->zoomed = false;
        renderer->active = false;

        // No selection! Go back
        if (menu->parentMenu)
        {
            return menu->parentMenu;
        }
        else
        {
            // There's no higher level menu, just close
            return menu;
        }
    }
}

/**
 * @brief Returns whether or not the wheel menu is active and should be drawn
 *
 * @param menu The menu
 * @param renderer The wheel menu renderer
 * @return true if the wheel menu is active
 * @return false if the wheel menu is not active
 */
bool wheelMenuActive(menu_t* menu, wheelMenuRenderer_t* renderer)
{
    return renderer && renderer->active;
}

/**
 * @brief Get an item's info if it exists, or return NULL
 *
 * @param renderer The wheel menu renderer
 * @param label The menu item label to retrieve the info for
 * @return wheelItemInfo_t* The item info, or NULL if one does not exist
 */
static wheelItemInfo_t* findInfo(wheelMenuRenderer_t* renderer, const char* label)
{
    node_t* node = renderer->itemInfos.first;
    while (node != NULL)
    {
        wheelItemInfo_t* info = node->val;

        if (info->label == label)
        {
            return info;
        }

        node = node->next;
    }

    return NULL;
}

/**
 * @brief Get an item's info, creating a new entry if not found
 *
 * @param renderer The wheel menu renderer
 * @param label The menu item label to retrieve or create the info for
 * @return wheelItemInfo_t* A valid item info for the given label
 */
static wheelItemInfo_t* findOrAddInfo(wheelMenuRenderer_t* renderer, const char* label)
{
    wheelItemInfo_t* info = findInfo(renderer, label);

    if (NULL == info)
    {
        info        = heap_caps_calloc(1, sizeof(wheelItemInfo_t), MALLOC_CAP_SPIRAM);
        info->label = label;

        // Defaults for colors
        info->unselectedBg = renderer->unselBgColor;
        info->selectedBg   = renderer->selBgColor;
        info->shapeFlags   = WM_SHAPE_DEFAULT;

        // We only add the item to the list if it's not already there
        push(&renderer->itemInfos, info);
    }

    return info;
}

static int cmpDrawInfo(const void* a, const void* b)
{
    return ((const wheelDrawInfo_t*)b)->drawOrder - ((const wheelDrawInfo_t*)a)->drawOrder;
}
