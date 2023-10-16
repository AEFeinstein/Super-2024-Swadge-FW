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

//==============================================================================
// Function Prototypes
//==============================================================================

static wheelItemInfo_t* findInfo(wheelMenuRenderer_t* renderer, const char* label);

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

    renderer->centerR = TFT_HEIGHT / 12;
    renderer->unselR  = TFT_HEIGHT / 4;
    renderer->selR    = TFT_HEIGHT / 3;

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
    drawCircleFilled(renderer->x, renderer->y, renderer->unselR, renderer->unselBgColor);

    uint16_t centerR          = renderer->centerR;
    paletteColor_t selBgColor = renderer->selBgColor;

    // If we haven't customized the back option to be around the ring, then we use the center
    // So, remove one from the ring items to compensate
    uint8_t ringItems    = menu->items->length - ((menu->parentMenu && !renderer->customBack) ? 1 : 0);
    uint16_t anchorAngle = (360 + renderer->anchorAngle - (360 / ringItems / 2)) % 360;

    // We need to draw the WSGs after all the fills, so just store the locations to avoid calculating twice
    struct
    {
        uint16_t x;
        uint16_t y;
        const wsg_t* wsg;
        paletteColor_t bgColor;
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
            uint16_t startAngle = (anchorAngle + info->position * 360 / ringItems) % 360;
            uint16_t endAngle   = (anchorAngle + (info->position + 1) * 360 / ringItems) % 360;

            // We'll use the center angle for drawing icons, filling in, etc.
            uint16_t centerAngle = ((startAngle < endAngle) ? (startAngle + (endAngle - startAngle) / 2)
                                                            : (startAngle + ((endAngle + 360) - startAngle) / 2))
                                   % 360;

            uint16_t r = (renderer->touched && menu->currentItem == node) ? renderer->selR : renderer->unselR;

            // If there's an icon, or the item's eventual color doesn't match the normal BG color
            if (info->icon
                || ((renderer->touched && menu->currentItem == node) ? (info->selectedBg != selBgColor)
                                                                     : (info->unselectedBg != renderer->unselBgColor)))
            {
                iconsToDraw[wsgs].x = renderer->x + getCos1024(centerAngle) * (centerR + (r - centerR) / 2) / 1024
                                      - (info->icon ? info->icon->w / 2 : 0);
                iconsToDraw[wsgs].y = renderer->y - getSin1024(centerAngle) * (centerR + (r - centerR) / 2) / 1024
                                      - (info->icon ? info->icon->h / 2 : 0);
                iconsToDraw[wsgs].wsg = info->icon;
                iconsToDraw[wsgs].bgColor
                    = (renderer->touched && menu->currentItem == node) ? info->selectedBg : info->unselectedBg;
                ++wsgs;
            }

            if (renderer->touched && menu->currentItem == node)
            {
                if (info->selectedBg != selBgColor)
                {
                    selBgColor = info->selectedBg;
                }

                fillCircleSector(renderer->x, renderer->y, centerR, r, startAngle, endAngle, selBgColor);
            }
            else if (info->unselectedBg != renderer->unselBgColor)
            {
                fillCircleSector(renderer->x, renderer->y, centerR, r, startAngle, endAngle, info->unselectedBg);
            }

            drawLine(renderer->x + getCos1024(startAngle) * centerR / 1024,
                     renderer->y - getSin1024(startAngle) * centerR / 1024,
                     renderer->x + getCos1024(startAngle) * r / 1024, renderer->y - getSin1024(startAngle) * r / 1024,
                     renderer->borderColor, 0);

            drawLine(renderer->x + getCos1024(endAngle) * centerR / 1024,
                     renderer->y - getSin1024(endAngle) * centerR / 1024,
                     renderer->x + getCos1024(endAngle) * r / 1024,
                     renderer->y - getSin1024(endAngle) * r / 1024,
                     renderer->borderColor, 0);

            for (uint16_t ang = startAngle; ang != endAngle; ang = (ang + 1) % 360)
            {
                // Fill in the whole thing
                drawLine(renderer->x + getCos1024(ang) * (r + 1) / 1024,
                         renderer->y - getSin1024(ang) * (r + 1) / 1024,
                         renderer->x + getCos1024((ang + 1) % 360) * (r + 1) / 1024,
                         renderer->y - getSin1024((ang + 1) % 360) * (r + 1) / 1024,
                         renderer->borderColor, 0);
            }
        }

        node = node->next;
    }

    if (!renderer->touched || !menu->currentItem || (!renderer->customBack && menuItemIsBack(menu->currentItem->val)))
    {
        // Here, we handle the case that the

        if (renderer->touched)
        {
            // This special case is just to fill faster than flood fill, it should work fine
            drawCircleFilled(renderer->x, renderer->y, centerR, selBgColor);
        }

        // draw the center circle border after
        drawCircle(renderer->x, renderer->y, centerR, renderer->borderColor);
    }
    else
    {
        //
        drawCircleFilled(renderer->x, renderer->y, centerR, renderer->unselBgColor);

        // Draw the center circle first to establish bounds for the fill
        drawCircle(renderer->x, renderer->y, centerR, renderer->borderColor);
    }

    for (uint8_t i = 0; i < wsgs; i++)
    {
        if (iconsToDraw[i].bgColor != renderer->unselBgColor)
        {
            // Fill in the background
            /*floodFill(iconsToDraw[i].x, iconsToDraw[i].y, iconsToDraw[i].bgColor, renderer->x - renderer->selR,
                      renderer->y - renderer->selR, renderer->x + renderer->selR, renderer->y + renderer->selR);*/
        }

        if (iconsToDraw[i].wsg)
        {
            drawWsgSimple(iconsToDraw[i].wsg, iconsToDraw[i].x, iconsToDraw[i].y);
        }
    }

    if (renderer->textBox && menu->currentItem && renderer->touched)
    {
        char buffer[128]  = {0};
        const char* label = getMenuItemLabelText(buffer, sizeof(buffer) - 1, menu->currentItem->val);

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

    if (radius <= (renderer->centerR * 1024 / renderer->selR))
    {
        if (!renderer->customBack && menu->parentMenu)
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
        if (!menuItemHasSubMenu(menu->currentItem->val) && !menuItemIsBack(menu->currentItem->val))
        {
            // Don't stay active after the touch is released
            renderer->active = false;

            menuSelectCurrentItem(menu);

            while (NULL != menu->parentMenu)
            {
                menu = menu->parentMenu;
            }

            return menu;
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
