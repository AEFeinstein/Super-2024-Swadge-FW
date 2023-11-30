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

#include <inttypes.h>
#include <malloc.h>
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
} wheelItemInfo_t;

typedef struct
{
    uint8_t drawOrder;
    uint16_t x;
    uint16_t y;
    bool selected;
    const wsg_t* wsg;
    paletteColor_t bgColor;
} wheelDrawInfo_t;

//==============================================================================
// Function Prototypes
//==============================================================================

static wheelItemInfo_t* findInfo(wheelMenuRenderer_t* renderer, const char* label);
static int cmpDrawInfo(const void* a, const void* b);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initializes and returns a new wheel menu with the default settings
 *
 * @param font The font to draw menu item labels with
 * @param anchorAngle The angle where the center of the first item will be anchored
 * @param textBox TODO doc
 * @return TODO doc
 */
wheelMenuRenderer_t* initWheelMenu(const font_t* font, uint16_t anchorAngle, const rectangle_t* textBox)
{
    wheelMenuRenderer_t* renderer = calloc(1, sizeof(wheelMenuRenderer_t));

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
 * @brief TODO doc
 *
 * @param renderer
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
 * @brief Sets the icon and ring position for the item with the given label
 *
 * @param renderer TODO doc
 * @param label The label of the item to set the icon and position for
 * @param icon  The icon to use when drawing the menu item
 * @param position The order of the item in the ring, centered around the anchor angle
 * @param scrollDir Which axis this item should be scrollable in
 */
void wheelMenuSetItemInfo(wheelMenuRenderer_t* renderer, const char* label, const wsg_t* icon, uint8_t position,
                          wheelScrollDir_t scrollDir)
{
    wheelItemInfo_t* info = findInfo(renderer, label);

    if (NULL == info)
    {
        info        = malloc(sizeof(wheelItemInfo_t));
        info->label = label;

        // Defaults for colors
        info->unselectedBg = renderer->unselBgColor;
        info->selectedBg   = renderer->selBgColor;

        // We only add the item to the list if it's not already there
        push(&renderer->itemInfos, info);
    }

    info->icon     = icon;
    info->position = position;
    info->scroll   = scrollDir;

    if (label == mnuBackStr)
    {
        renderer->customBack = true;
    }
}

/**
 * @brief TODO doc
 *
 * @param renderer
 * @param label
 * @param selectedBg
 * @param unselectedBg
 */
void wheelMenuSetItemColor(wheelMenuRenderer_t* renderer, const char* label, paletteColor_t selectedBg,
                           paletteColor_t unselectedBg)
{
    wheelItemInfo_t* info = findInfo(renderer, label);

    if (NULL == info)
    {
        // Initialize everything not set to NULL / 0
        info        = calloc(1, sizeof(wheelItemInfo_t));
        info->label = label;

        push(&renderer->itemInfos, info);
    }

    info->unselectedBg = unselectedBg;
    info->selectedBg   = selectedBg;
}

/**
 * @brief TODO doc
 *
 * @param menu
 * @param renderer
 * @param elapsedUs
 */
void drawWheelMenu(menu_t* menu, wheelMenuRenderer_t* renderer, int64_t elapsedUs)
{
    node_t* node = menu->items->first;

    // Draw background circle for the unselected area
    // drawCircleFilled(renderer->x, renderer->y, renderer->unselR, renderer->unselBgColor);

    // Add one for the convenience of looping
    uint16_t spokeR = renderer->spokeR;

    // If we haven't customized the back option to be around the ring, then we use the center
    // So, remove one from the ring items to compensate
    uint8_t ringItems    = menu->items->length - ((menu->parentMenu && !renderer->customBack) ? 1 : 0);
    uint16_t anchorAngle = (360 + renderer->anchorAngle - (360 / ringItems / 2)) % 360;

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

    // Just figure out what to draw where, then draw them all
    wheelDrawInfo_t drawInfos[ringItems + 1];
    uint8_t curDraw = 0;

    if (renderer->zoomed)
    {
        if (curItem == NULL)
        {
            ESP_LOGW("Wheel", "Zoomed but there's no selected item?");
            renderer->zoomed = false;
        }
        else if (menuItemHasOptions(curItem))
        {
            ringItems = curItem->numOptions;
            for (int i = 0; i < curItem->numOptions; i++)
            {
                wheelItemInfo_t* optInfo = findInfo(renderer, curItem->options[i]);
                uint8_t optPos           = (optInfo && optInfo->position != UINT8_MAX) ? optInfo->position : i;

                if (curItem->currentOpt == i)
                {
                    drawInfos[curDraw].drawOrder = UINT8_MAX;
                    drawInfos[curDraw].selected  = true;
                    drawInfos[curDraw].bgColor   = optInfo ? optInfo->selectedBg : renderer->selBgColor;
                }
                else
                {
                    // We want to draw the selected one last, and the farthest away one first
                    // So, order the items by how close they are to the selected one
                    drawInfos[curDraw].drawOrder
                        = (curPosition > optPos) ? (curPosition - optPos) : (optPos - curPosition);
                    drawInfos[curDraw].selected = false;
                    drawInfos[curDraw].bgColor  = optInfo ? optInfo->unselectedBg : renderer->unselBgColor;
                }

                // Calculate where this sector starts
                uint16_t centerAngle = (anchorAngle + optPos * 360 / ringItems + (180 / ringItems)) % 360;

                if (ringItems > 8)
                {
                    alternate = true;
                }
                uint16_t itemSpokeR = alternate ? (((optPos % 2) == 0) ? spokeR + inR : spokeR + outR) : spokeR;

                drawInfos[curDraw].x   = renderer->x + getCos1024(centerAngle) * itemSpokeR / 1024;
                drawInfos[curDraw].y   = renderer->y - getSin1024(centerAngle) * itemSpokeR / 1024;
                drawInfos[curDraw].wsg = optInfo ? optInfo->icon : NULL;
                curDraw++;
            }

            // Make the info for the back / confirm icon
            drawInfos[curDraw].drawOrder = (!curItem || menuItemIsBack(curItem)) ? UINT8_MAX : 0;
            drawInfos[curDraw].x         = renderer->x;
            drawInfos[curDraw].y         = renderer->y;
            drawInfos[curDraw].selected  = renderer->zoomBackSelected;
            // TODO: Confirm icon?
            drawInfos[curDraw].wsg     = NULL;
            drawInfos[curDraw].bgColor = renderer->zoomBackSelected ? renderer->selBgColor : renderer->unselBgColor;
            curDraw++;
        }
        else if (menuItemIsSetting(curItem))
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

                /*if ((tickAngle < 2 || tickAngle > 358) || (tickAngle > 88 && tickAngle < 92))
                {
                    // Vertical center
                    textY -= renderer->font->height / 2;
                }
                else if (tickAngle < 180)
                {
                    // Justify upwards
                    textY -= renderer->font->height;
                }*/
                // Otherwise this is fine

                ESP_LOGD("Wheel", "tick loc: (%" PRIu16 ", %" PRIu16 ") ->  (%" PRIu16 ", %" PRIu16 ")", x0, y0, x1,
                         y1);

                drawLine(x0, y0, x1, y1, renderer->borderColor, 0);

                drawText(renderer->font, c000, tickLabel, textX, textY);
            }

            drawLine(renderer->x, renderer->y, renderer->x + getCos1024(curAngle) * (tickStart - 2) / 1024,
                     renderer->y + getSin1024(curAngle) * (tickStart - 2) / 1024, c500, 0);
        }
    }
    else
    {
        if (ringItems > 8)
        {
            unselR -= 10;

            if (ringItems > 12)
            {
                unselR -= 2;
                selR -= 5;
                alternate = true;
            }
        }

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

                    drawInfos[curDraw].drawOrder = ((curPosition > info->position) ? (curPosition - info->position)
                                                                                   : (info->position - curPosition));
                    drawInfos[curDraw].x         = renderer->x + getCos1024(centerAngle) * itemSpokeR / 1024;
                    drawInfos[curDraw].y         = renderer->y - getSin1024(centerAngle) * itemSpokeR / 1024;
                    drawInfos[curDraw].selected  = isCur;
                    drawInfos[curDraw].wsg       = info->icon;
                    drawInfos[curDraw].bgColor   = isCur ? info->selectedBg : info->unselectedBg;
                    curDraw++;

                    ESP_LOGD("Wheel", "drawOrder is %" PRIu8 " for %s", drawInfos[curDraw].drawOrder, item->label);
                }
            }
            node = node->next;
        }

        // Back info found
        if (backInfo)
        {
            drawInfos[curDraw].drawOrder = (menuItemIsBack(curItem) ? UINT8_MAX : 0);
            drawInfos[curDraw].x         = renderer->x;
            drawInfos[curDraw].y         = renderer->y;
            drawInfos[curDraw].selected  = (menuItemIsBack(curItem));
            drawInfos[curDraw].wsg       = backInfo->icon;
            drawInfos[curDraw].bgColor   = menuItemIsBack(curItem) ? backInfo->selectedBg : backInfo->unselectedBg;
            curDraw++;
        }
        else
        {
            drawInfos[curDraw].drawOrder = (!curItem || menuItemIsBack(curItem)) ? UINT8_MAX : 0;
            drawInfos[curDraw].x         = renderer->x;
            drawInfos[curDraw].y         = renderer->y;
            drawInfos[curDraw].selected  = (!curItem || menuItemIsBack(curItem));
            drawInfos[curDraw].wsg       = NULL;
            drawInfos[curDraw].bgColor
                = (!curItem || menuItemIsBack(curItem)) ? renderer->selBgColor : renderer->unselBgColor;
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
        drawCircleFilled(info->x, info->y, info->selected ? selR : unselR, info->bgColor);
        if (info->wsg)
        {
            drawWsgSimple(info->wsg, info->x - info->wsg->w / 2, info->y - info->wsg->h / 2);
        }
        drawCircle(info->x, info->y, info->selected ? selR : unselR, renderer->borderColor);
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
            uint16_t textW = textWidth(renderer->font, label);

            while (textW > renderer->textBox->width)
            {
                // Copy the real label first if we're using a static string
                if (label != buffer)
                {
                    snprintf(buffer, sizeof(buffer) - 1, "%s", label);
                    label = buffer;
                }

                char* ptr = buffer + strlen(buffer);
                // Shorten the text by one, and add trailing
                *ptr-- = '\0';

                for (uint8_t i = 0; i < 3 && ptr > buffer; i++)
                {
                    *ptr-- = '.';
                }

                textW = textWidth(renderer->font, label);
            }

            drawText(renderer->font, renderer->textColor, label,
                     renderer->textBox->x + (renderer->textBox->width - textW) / 2,
                     renderer->textBox->y + (renderer->textBox->height - renderer->font->height - 1) / 2);
        }
    }
}

/**
 * @brief TODO doc
 *
 * @param menu
 * @param renderer
 * @param angle
 * @param radius
 * @return menu_t*
 */
menu_t* wheelMenuTouch(menu_t* menu, wheelMenuRenderer_t* renderer, uint16_t angle, uint16_t radius)
{
    renderer->touched = true;
    renderer->active  = true;

    if (radius <= 512)
    {
        if (renderer->zoomed)
        {
            renderer->zoomBackSelected = true;
            return menu;
        }
        else if (!renderer->customBack && menu->parentMenu)
        {
            // Only navigate if we're not already on the "Back" item.
            if (!menu->currentItem || ((menuItem_t*)menu->currentItem->val)->label != mnuBackStr)
            {
                return menuNavigateToItem(menu, mnuBackStr);
            }

            // Just return the menu, the callback was already called
            return menu;
        }
        else
        {
            if (menu->currentItem)
            {
                menu->currentItem = NULL;
                menu->cbFunc(NULL, false, 0);
            }
            return menu;
        }
    }

    renderer->zoomBackSelected = false;

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

        if (radius > 512)
        {
            // If the angle is in the top half, just clamp it to wherever's closer
            uint16_t zoomAngle = (angle > 270) ? 0 : (angle > 180 ? 180 : angle);
            uint8_t selection  = (180 - zoomAngle) * (max - min) / 180 + min;
            ESP_LOGD("Wheel", "Selection is %" PRIu8 ", from %" PRIu16, selection, zoomAngle);

            if (menuItemHasOptions(cur))
            {
                cur->currentOpt = selection;
            }
            else
            {
                cur->currentSetting = selection;
            }
        }
    }
    else
    {
        // Compensate for the "Back" item unless it's included in the ring
        uint8_t ringItems    = menu->items->length - ((menu->parentMenu && !renderer->customBack) ? 1 : 0);
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
            node_t* node = menu->items->first;

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
                        return menuNavigateToItem(menu, info->label);
                    }

                    return menu;
                }

                node = node->next;
            }
        }
    }

    return menu;
}

/**
 * @brief TODO doc
 *
 * @param menu
 * @param renderer
 * @param evt
 * @return menu_t*
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
                        return (info->scroll & SCROLL_REVERSE) ? menuNavigateToPrevOption(menu)
                                                               : menuNavigateToNextOption(menu);
                    }

                    break;
                }

                case PB_DOWN:
                {
                    if (info->scroll & SCROLL_VERT)
                    {
                        return (info->scroll & SCROLL_REVERSE) ? menuNavigateToNextOption(menu)
                                                               : menuNavigateToPrevOption(menu);
                    }
                    break;
                }

                case PB_LEFT:
                {
                    if (info->scroll & SCROLL_HORIZ)
                    {
                        return (info->scroll & SCROLL_REVERSE) ? menuNavigateToNextOption(menu)
                                                               : menuNavigateToPrevOption(menu);
                    }
                    break;
                }

                case PB_RIGHT:
                {
                    if (info->scroll & SCROLL_HORIZ)
                    {
                        return (info->scroll & SCROLL_REVERSE) ? menuNavigateToPrevOption(menu)
                                                               : menuNavigateToNextOption(menu);
                    }
                    break;
                }

                case PB_A:
                {
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
 * @brief TODO doc
 *
 * @param menu
 * @param renderer
 * @return menu_t*
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
                    cur->currentOpt = renderer->zoomValue;
                }
                else if (menuItemIsSetting(cur))
                {
                    cur->currentSetting = renderer->zoomValue;
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
 * @brief TODO doc
 *
 * @param menu
 * @param renderer
 * @return true
 * @return false
 */
bool wheelMenuActive(menu_t* menu, wheelMenuRenderer_t* renderer)
{
    return renderer && renderer->active;
}

/**
 * @brief TODO doc
 *
 * @param renderer
 * @param label
 * @return wheelItemInfo_t*
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

static int cmpDrawInfo(const void* a, const void* b)
{
    return ((const wheelDrawInfo_t*)b)->drawOrder - ((const wheelDrawInfo_t*)a)->drawOrder;
}
