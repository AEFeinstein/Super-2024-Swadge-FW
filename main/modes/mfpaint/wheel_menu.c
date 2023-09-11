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
 */
void wheelMenuSetItemInfo(wheelMenuRenderer_t* renderer, const char* label, const wsg_t* icon, uint8_t position)
{
    wheelItemInfo_t* info = malloc(sizeof(wheelItemInfo_t));
    info->label = label;
    info->icon = icon;
    info->position = position;

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

    // We need to draw the WSGs after all the fills, so just store the locations to make calculations easier
    struct {
        uint16_t x;
        uint16_t y;
        wsg_t* wsg;
    } iconsToDraw[menu->items->length];
    uint8_t wsgs = 0;

    while (node != NULL)
    {
        menuItem_t* item = node->val;
        // Find menu item by either its label or its first option label
        wheelItemInfo_t* info = findInfo(renderer, item->label ? item->label : *item->options);

        if (info != NULL)
        {
            // Calculate where this sector starts
            uint16_t startAngle = (renderer->anchorAngle + info->position * 360 / menu->items->length) % 360;
            uint16_t endAngle = (renderer->anchorAngle + (info->position + 1) * 360 / menu->items->length) % 360;

            // We'll use the center angle for drawing icons, filling in, etc.
            uint16_t centerAngle = (startAngle < endAngle) ? (startAngle + (endAngle - startAngle) / 2) : (startAngle + ((endAngle + 360) - startAngle) / 2);

            uint16_t r = (menu->currentItem == node) ? renderer->selR : renderer->unselR;

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

            if (menu->currentItem == node)
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

    if (!menu->currentItem)
    {
        // This special case is just to fill faster than flood fill, it should work fine
        drawCircleFilled(renderer->x, renderer->y, centerR, renderer->selBgColor);

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

menu_t* wheelMenuSetSelection(menu_t* menu, wheelMenuRenderer_t* renderer, uint16_t angle, uint16_t radius)
{
    // Check if the angle is actually before the starting angle
    if ((angle % 360) < renderer->anchorAngle)
    {
        // just add 360 to it -- so if e.g. angle is 15 and startAngle is 20, angle
        // would then be 375, and (375 - 20) = 355, so the offset is still < 360
        angle = (angle % 360) + 360;
    }

    if (radius <= (renderer->centerR * 1024 / renderer->selR))
    {
        if (!renderer->customBack)
        {
            menu_t* tmp = menuNavigateToItem(menu, mnuBackStr);
            if (tmp->currentItem && ((menuItem_t*)tmp->currentItem->val)->label != mnuBackStr)
            {
                // item wasn't changed as we wanted
                // Unselect
                tmp->currentItem = NULL;
            }

            return tmp;
        }
    }

    // Calculate the offset
    uint8_t index = (angle - renderer->anchorAngle) * menu->items->length / 360;

    if (index < menu->items->length)
    {
        node_t* node = renderer->itemInfos.first;

        // Find the item configured for that offset
        while (node != NULL)
        {
            wheelItemInfo_t* item = node->val;
            if (item && (item->position == index))
            {
                // Only navigate to it if it's not the current item already
                if (!menu->currentItem || ((menuItem_t*)menu->currentItem->val)->label != item->label)
                {
                    return menuNavigateToItem(menu, item->label);
                }

                break;
            }

            node = node->next;
        }
    }

    return menu;
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
