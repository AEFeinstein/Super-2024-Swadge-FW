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
    uint16_t spokeR           = renderer->spokeR;
    paletteColor_t selBgColor = renderer->selBgColor;

    // If we haven't customized the back option to be around the ring, then we use the center
    // So, remove one from the ring items to compensate
    uint8_t ringItems    = menu->items->length - ((menu->parentMenu && !renderer->customBack) ? 1 : 0);
    uint16_t anchorAngle = (360 + renderer->anchorAngle - (360 / ringItems / 2)) % 360;

    uint16_t itemGapSq = 0;
    uint16_t altGapSq  = 0;
    uint16_t targetSq  = ((renderer->unselR * 2) * (renderer->unselR * 2));
    int16_t inR        = renderer->unselR / 3;
    int16_t outR       = -renderer->unselR / 3;
    uint16_t minAlt    = UINT16_MAX;
    uint16_t minReg    = UINT16_MAX;

    /*while ((itemGapSq < targetSq || altGapSq < targetSq) && (spokeR + renderer->unselR < TFT_HEIGHT))
    {
        spokeR++;
        for (int n = 0; n < 2; n++)
        {
            bool alt = n;
            if ((alt && altGapSq < targetSq) || (!alt && itemGapSq < targetSq))
            {
                continue;
            }
            uint16_t r0 = alt ? spokeR + inR : spokeR;
            uint16_t r1 = alt ? spokeR + outR : spokeR;

            uint16_t x0 = getCos1024(anchorAngle) * r0 / 1024;
            uint16_t y0 = getSin1024(anchorAngle) * r0 / 1024;
            uint16_t x1 = getCos1024((anchorAngle + 360 / ringItems) % 360) * r1 / 1024;
            uint16_t y1 = getSin1024((anchorAngle + 360 / ringItems) % 360) * r1 / 1024;

            uint16_t sq = (x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0);
            if (alt)
            {
                altGapSq = sq;
                ESP_LOGI("Wheel", "Alternating gap is %" PRIu16 " wtith spokeR=%" PRIu16, altGapSq, spokeR);
                minAlt = spokeR;
            }
            else
            {
                itemGapSq = sq;
                ESP_LOGI("Wheel", "Regular gap is %" PRIu16 " wtith spokeR=%" PRIu16, itemGapSq, spokeR);
                minReg = spokeR;
            }
        }
    }

    bool alternate = (minAlt < minReg);
    spokeR = (alternate ? minAlt : minReg);*/
    bool alternate = false;

    wheelItemInfo_t* backInfo = NULL;

    bool foundSelected       = false;
    wsg_t* selIcon           = NULL;
    paletteColor_t selItemBg = renderer->selBgColor;
    uint16_t selX            = 0;
    uint16_t selY            = 0;

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

                // Next item!
                node = node->next;
                continue;
            }

            // Calculate where this sector starts
            uint16_t centerAngle = (anchorAngle + info->position * 360 / ringItems + (180 / ringItems)) % 360;

            bool cur          = (renderer->touched && menu->currentItem == node);
            paletteColor_t bg = cur ? info->selectedBg : info->unselectedBg;

            if (ringItems > 8)
            {
                alternate = true;
            }
            uint16_t itemSpokeR = alternate ? (((info->position % 2) == 1) ? spokeR + inR : spokeR + outR) : spokeR;

            uint16_t centerX = renderer->x + getCos1024(centerAngle) * itemSpokeR / 1024;
            uint16_t centerY = renderer->y - getSin1024(centerAngle) * itemSpokeR / 1024;

            if (renderer->touched && menu->currentItem == node)
            {
                // Don't draw the selected item yet
                foundSelected = true;
                selItemBg     = info->selectedBg;
                selIcon       = info->icon;
                selX          = centerX;
                selY          = centerY;
            }
            else
            {
                drawCircleFilled(centerX, centerY, renderer->unselR, bg);
                if (info->icon)
                {
                    drawWsgSimple(info->icon, centerX - info->icon->w / 2, centerY - info->icon->h / 2);
                }
                drawCircle(centerX, centerY, renderer->unselR, renderer->borderColor);
            }
        }

        node = node->next;
    }

    bool centerSel             = renderer->touched && (!menu->currentItem || menuItemIsBack(menu->currentItem->val));
    paletteColor_t backSelBg   = backInfo ? backInfo->selectedBg : renderer->selBgColor;
    paletteColor_t backUnselBg = backInfo ? backInfo->unselectedBg : renderer->unselBgColor;
    drawCircleFilled(renderer->x, renderer->y, centerSel ? renderer->selR : renderer->unselR,
                     centerSel ? backSelBg : backUnselBg);
    if (backInfo && backInfo->icon)
    {
        drawWsgSimple(backInfo->icon, renderer->x - backInfo->icon->w / 2, renderer->y - backInfo->icon->h / 2);
    }
    drawCircle(renderer->x, renderer->y, centerSel ? renderer->selR : renderer->unselR, renderer->borderColor);

    if (foundSelected)
    {
        drawCircleFilled(selX, selY, renderer->selR, selItemBg);
        if (selIcon)
        {
            drawWsgSimple(selIcon, selX - selIcon->w / 2, selY - selIcon->h / 2);
        }
        drawCircle(selX, selY, renderer->selR, renderer->borderColor);
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

    if (radius <= 512)
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
