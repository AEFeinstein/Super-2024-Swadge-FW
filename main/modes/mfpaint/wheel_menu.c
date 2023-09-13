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

typedef struct
{
    const char* label;
    const wsg_t* icon;
    uint8_t position;
    wheelScrollDir_t scroll;
} wheelItemInfo_t;

static wheelItemInfo_t* findInfo(wheelMenuRenderer_t* renderer, const char* label);

/**
 * @brief Initializes and returns a new wheel menu with the default settings
 *
 * @param font The font to draw menu item labels with
 * @param anchorAngle The angle to start the first item at
 * @return wheelMenuRenderer_t*
 */
wheelMenuRenderer_t* initWheelMenu(const font_t* font, uint16_t anchorAngle)
{
    wheelMenuRenderer_t* renderer = calloc(1, sizeof(wheelMenuRenderer_t));

    renderer->font = font;
    renderer->anchorAngle = anchorAngle;

    renderer->textColor = c000;
    renderer->borderColor = c000;
    renderer->unselBgColor = c333;
    renderer->selBgColor = c555;

    renderer->centerR = TFT_HEIGHT / 12;
    renderer->unselR = TFT_HEIGHT / 4;
    renderer->selR = TFT_HEIGHT / 3;

    renderer->x = TFT_WIDTH / 2;
    renderer->y = TFT_HEIGHT / 2;

    renderer->active = false;

    return renderer;
}

void deinitWheelMenu(wheelMenuRenderer_t* renderer)
{
    node_t* info = NULL;
    while ((info = pop(&renderer->itemInfos)))
    {
        free(info);
    }

    free(renderer);
}

/**
 * @brief Sets the icon and ring position for the item with the given label
 *
 * @param label The label of the item to set the icon and position for
 * @param icon  The icon to use when drawing the menu item
 * @param position The order of the item in the ring, centered around the anchor angle
 * @param scrollDir Which axis this item should be scrollable in
 */
void wheelMenuSetItemInfo(wheelMenuRenderer_t* renderer, const char* label, const wsg_t* icon, uint8_t position, wheelScrollDir_t scrollDir)
{
    wheelItemInfo_t* info = malloc(sizeof(wheelItemInfo_t));
    info->label = label;
    info->icon = icon;
    info->position = position;
    info->scroll = scrollDir;

    if (label == mnuBackStr)
    {
        renderer->customBack = true;
    }

    push(&renderer->itemInfos, info);
}

void drawWheelMenu(menu_t* menu, wheelMenuRenderer_t* renderer, int64_t elapsedUs)
{
    node_t* node = menu->items->first;

    // Draw background circle for the unselected area
    drawCircleFilled(renderer->x, renderer->y, renderer->unselR, renderer->unselBgColor);

    uint16_t centerR = renderer->centerR; //menu->currentItem ? renderer->centerR : renderer->centerR + (renderer->unselR - renderer->centerR) / 2;
    uint16_t fillAngle = 0;

    // If we haven't customized the back option to be around the ring, then we use the center
    // So, remove one from the ring items to compensate
    uint8_t ringItems = menu->items->length - ((menu->parentMenu && !renderer->customBack) ? 1 : 0);

    // We need to draw the WSGs after all the fills, so just store the locations to avoid calculating twice
    struct {
        uint16_t x;
        uint16_t y;
        wsg_t* wsg;
    } iconsToDraw[ringItems];
    uint8_t wsgs = 0;

    while (node != NULL)
    {
        menuItem_t* item = node->val;
        // Find menu item by either its label or its first option label
        wheelItemInfo_t* info = findInfo(renderer, item->label ? item->label : *item->options);

        if (info != NULL)
        {
            // Calculate where this sector starts
            uint16_t startAngle = (renderer->anchorAngle + info->position * 360 / ringItems) % 360;
            uint16_t endAngle = (renderer->anchorAngle + (info->position + 1) * 360 / ringItems) % 360;

            // We'll use the center angle for drawing icons, filling in, etc.
            uint16_t centerAngle = (startAngle < endAngle)
                    ? (startAngle + (endAngle - startAngle) / 2)
                    : (startAngle + ((endAngle + 360) - startAngle) / 2);

            uint16_t r = (renderer->touched && menu->currentItem == node) ? renderer->selR : renderer->unselR;

            drawLine(renderer->x + getCos1024(startAngle) * centerR / 1024,
                     renderer->y - getSin1024(startAngle) * centerR / 1024,
                     renderer->x + getCos1024(startAngle) * r / 1024,
                     renderer->y - getSin1024(startAngle) * r / 1024,
                     renderer->borderColor, 0);

            drawLine(renderer->x + getCos1024(endAngle) * centerR / 1024,
                     renderer->y - getSin1024(endAngle) * centerR / 1024,
                     renderer->x + getCos1024(endAngle) * r / 1024,
                     renderer->y - getSin1024(endAngle) * r / 1024,
                     renderer->borderColor, 0);

            if (info->icon)
            {
                iconsToDraw[wsgs].x = renderer->x + getCos1024(centerAngle) * (centerR + (r - centerR) / 2) / 1024 - info->icon->w / 2;
                iconsToDraw[wsgs].y = renderer->y - getSin1024(centerAngle) * (centerR + (r - centerR) / 2) / 1024 - info->icon->h / 2;
                iconsToDraw[wsgs].wsg = info->icon;
                ++wsgs;
            }

            if (renderer->touched && menu->currentItem == node)
            {
                for (uint16_t ang = startAngle; ang != endAngle; ang = (ang + 1) % 360)
                {
                    drawLine(renderer->x + getCos1024(ang) * r / 1024,
                            renderer->y - getSin1024(ang) * r / 1024,
                            renderer->x + getCos1024((ang + 1) % 360) * r / 1024,
                            renderer->y - getSin1024((ang + 1) % 360) * r / 1024,
                            renderer->borderColor, 0);
                }

                fillAngle = centerAngle;
            }
        }

        node = node->next;
    }

    if (!renderer->touched || !menu->currentItem || (!renderer->customBack && menuItemIsBack(menu->currentItem->val)))
    {
        // This special case is just to fill faster than flood fill, it should work fine
        if (renderer->touched)
        {
            drawCircleFilled(renderer->x, renderer->y, centerR, renderer->selBgColor);
        }

        // draw the center circle border after
        drawCircle(renderer->x, renderer->y, centerR, renderer->borderColor);
    }
    else
    {
        // Draw the center circle first to establish bounds for the fill
        drawCircle(renderer->x, renderer->y, centerR, renderer->borderColor);

        // Color the background under the selected item, first the inner part
        floodFill(renderer->x + getCos1024(fillAngle) * (centerR + (renderer->unselR - centerR) / 2) / 1024,
                    renderer->y - getSin1024(fillAngle) * (centerR + (renderer->unselR - centerR) / 2) / 1024,
                    renderer->selBgColor,
                    renderer->x - renderer->selR, renderer->y - renderer->selR,
                    renderer->x + renderer->selR, renderer->y + renderer->selR);

        // And then color the outer part of the selected item
        floodFill(renderer->x + getCos1024(fillAngle) * (renderer->unselR + (renderer->selR - renderer->unselR) / 2) / 1024,
                    renderer->y - getSin1024(fillAngle) * (renderer->unselR + (renderer->selR - renderer->unselR) / 2) / 1024,
                    renderer->selBgColor,
                    renderer->x - renderer->selR, renderer->y - renderer->selR,
                    renderer->x + renderer->selR, renderer->y + renderer->selR);
    }

    for (uint8_t i = 0; i < wsgs; i++)
    {
        drawWsgSimple(iconsToDraw[i].wsg, iconsToDraw[i].x, iconsToDraw[i].y);
    }
}

menu_t* wheelMenuTouch(menu_t* menu, wheelMenuRenderer_t* renderer, uint16_t angle, uint16_t radius)
{
    renderer->touched = true;
    renderer->active = true;

    // Check if the angle is actually before the starting angle
    if ((angle % 360) < renderer->anchorAngle)
    {
        // just add 360 to it -- so if e.g. angle is 15 and startAngle is 20, angle
        // would then be 375, and (375 - 20) = 355, so the offset is still < 360
        angle = (angle % 360) + 360;
    }

    if (radius <= (renderer->centerR * 1024 / renderer->selR))
    {
        if (!renderer->customBack && menu->parentMenu)
        {
            return menuNavigateToItem(menu, mnuBackStr);
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

    // Compensate for the "Back" item unless it's included in the ring
    uint8_t ringItems = menu->items->length - ((menu->parentMenu && !renderer->customBack) ? 1 : 0);

    // Calculate the offset
    uint8_t index = (angle - renderer->anchorAngle) * ringItems / 360;

    if (index < ringItems)
    {
        node_t* node = menu->items->first;

        // Find the item configured for that offset
        while (node != NULL)
        {
            menuItem_t* menuItem = node->val;
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
    return menu;
}

menu_t* wheelMenuButton(menu_t* menu, wheelMenuRenderer_t* renderer, buttonEvt_t* evt)
{
    if (evt->down && menu->currentItem)
    {
        menuItem_t* item = menu->currentItem->val;
        wheelItemInfo_t* info = findInfo(renderer, item->options ? item->options[0] : item->label);

        if (info && info->scroll != NO_SCROLL)
        {
            switch (evt->button)
            {
                case PB_UP:
                {
                    if (info->scroll & SCROLL_VERT)
                    {
                        return (info->scroll & SCROLL_REVERSE)
                               ? menuNavigateToPrevOption(menu)
                               : menuNavigateToNextOption(menu);
                    }

                    break;
                }

                case PB_DOWN:
                {
                    if (info->scroll & SCROLL_VERT)
                    {
                        return (info->scroll & SCROLL_REVERSE)
                               ? menuNavigateToNextOption(menu)
                               : menuNavigateToPrevOption(menu);
                    }
                    break;
                }

                case PB_LEFT:
                {
                    if (info->scroll & SCROLL_HORIZ)
                    {
                        return (info->scroll & SCROLL_REVERSE)
                               ? menuNavigateToNextOption(menu)
                               : menuNavigateToPrevOption(menu);
                    }
                    break;
                }

                case PB_RIGHT:
                {
                    if (info->scroll & SCROLL_HORIZ)
                    {
                        return (info->scroll & SCROLL_REVERSE)
                               ? menuNavigateToPrevOption(menu)
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
        if (!menuItemHasSubMenu(menu->currentItem->val) && !menuItemIsBack(menu->currentItem->val))
        {
            // Don't stay active after the touch is released
            renderer->active = false;
        }

        return menuSelectCurrentItem(menu);
    }
    else
    {
        // No selection! Go back
        if (menu->parentMenu)
        {
            return menu->parentMenu;
        }
        else
        {
            // There's no higher level menu, just close
            renderer->active = false;
            return menu;
        }
    }
}

bool wheelMenuActive(menu_t* menu, wheelMenuRenderer_t* renderer)
{
    return renderer->active;
}

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
