//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include <stdio.h>

#include "hdw-tft.h"
#include "menu.h"
#include "bresenham.h"

//==============================================================================
// Const Variables
//==============================================================================

static const char* mnuBackStr = "Back";

//==============================================================================
// Function Prototypes
//==============================================================================

static void deinitSubMenu(menu_t* menu);

//==============================================================================
// Functions
//==============================================================================

/**
 * Initialize and return an empty menu. A menu is a collection of vertically
 * scrollable rows. The rows are separated into pages, and when a page
 * boundary is crossed the whole page scrolls.
 *
 * Rows may be single items, where the callback is called whenever it is
 * selected or multi items, which is a collection of horizontally scrollable
 * items and the callback is called whenever it scrolls or is selected.
 *
 * Rows may also be submenus. When a submenu is selected, the callback is not
 * called, and instead a the submenu is rendered. Each submenu automatically
 * has a "Back" item added to it, which returns to the parent menu
 *
 * @param title  The title to be displayed for this menu. The underlying memory
 *               isn't copied, so this string must persist for the lifetime of
 *               the menu.
 * @param font   The font to draw this menu with
 * @param cbFunc The function to call when a menu option is selected. The
 *               argument to the callback will be the same pointer
 * @return A pointer to the newly allocated menu_t. This must be deinitialized
 *         when the menu is not used anymore.
 */
menu_t* initMenu(const char* title, font_t* font, menuCb cbFunc)
{
    menu_t* menu      = calloc(1, sizeof(menu_t));
    menu->title       = title;
    menu->font        = font;
    menu->cbFunc      = cbFunc;
    menu->currentItem = NULL;
    menu->items       = calloc(1, sizeof(list_t));
    menu->parentMenu  = NULL;
    return menu;
}

/**
 * Deinitialize a menu and all connected menus, including submenus and parent
 * menus. This frees memory allocated for this menu, but not memory allocated
 * elsewhere, like the font or item labels
 *
 * @param menu The menu to deinitialize
 */
void deinitMenu(menu_t* menu)
{
    // Traverse to the top level menu
    while (menu->parentMenu)
    {
        menu = menu->parentMenu;
    }

    // Deinitialize the menu. This has recursion!
    deinitSubMenu(menu);
}

/**
 * Deinitialize a menu and all of its submenus recursively. This frees memory
 * allocated for this menu, but not memory allocated elsewhere, like the font or
 * item labels
 *
 * @param menu
 */
static void deinitSubMenu(menu_t* menu)
{
    // For each item
    node_t* itemNode = menu->items->first;
    while (itemNode)
    {
        menuItem_t* item = (menuItem_t*)itemNode->val;
        // If there is a submenu
        if (item->subMenu)
        {
            // Deinitialize it
            deinitSubMenu(item->subMenu);
        }
        itemNode = itemNode->next;
    }

    // Clear all items in the list
    clear(menu->items);
    // Free the list
    free(menu->items);
    // Free the menu
    free(menu);
}

/**
 * Add a submenu item to the menu. When this item is select, the submenu is
 * rendered. The ::menuCb is not called.
 *
 * All items added to this ::menu_t after calling startSubMenu() will be added
 * in this submenu until endSubMenu() is called. Submenus have no limitations on
 * how many submenus may be nested.
 *
 * Submenu items are rendered with an icon indicating that it is a submenu.
 *
 * @param menu The menu to create a submenu in
 * @param label The label for this submenu. The underlying memory isn't copied,
 *              so this string must persist for the lifetime of the menu
 * @return A pointer to the submenu
 */
menu_t* startSubMenu(menu_t* menu, const char* label)
{
    // Allocate a submenu
    menu_t* subMenu      = calloc(1, sizeof(menu_t));
    subMenu->title       = label;
    subMenu->font        = menu->font;
    subMenu->cbFunc      = menu->cbFunc;
    subMenu->currentItem = NULL;
    subMenu->items       = calloc(1, sizeof(list_t));
    subMenu->parentMenu  = menu;

    // Allocate a new menu item
    menuItem_t* newItem = calloc(1, sizeof(menuItem_t));
    newItem->label      = label;
    newItem->options    = NULL;
    newItem->numOptions = 0;
    newItem->currentOpt = 0;
    newItem->subMenu    = subMenu;
    push(menu->items, newItem);

    // If this is the first item, set it as the current
    if (1 == menu->items->length)
    {
        menu->currentItem = menu->items->first;
    }

    // Return the submenu
    return subMenu;
}

/**
 * Finish adding items to a submenu and resume adding items to the parent menu.
 * This will automatically add a "Back" item to the submenu, which returns to
 * the parent menu. ::menuCb will not be called when "Back" is selected.
 *
 * During menu construction, each call to startSubMenu() must have a
 * corresponding call to endSubMenu()
 *
 * @param menu The menu to end a submenu in
 * @return TODO
 */
menu_t* endSubMenu(menu_t* menu)
{
    if (NULL != menu->parentMenu)
    {
        // Add the back
        addSingleItemToMenu(menu, mnuBackStr);

        // Return the parent menu
        return menu->parentMenu;
    }
    return menu;
}

/**
 * Add a single item entry to the menu. When this item is selected, the ::menuCb
 * callback is called with the given label as the argument.
 *
 * @param menu The menu to add a single item to
 * @param label The label for this item. The underlying memory isn't copied, so
 *              this string must persist for the lifetime of the menu
 */
void addSingleItemToMenu(menu_t* menu, const char* label)
{
    menuItem_t* newItem = calloc(1, sizeof(menuItem_t));
    newItem->label      = label;
    newItem->options    = NULL;
    newItem->numOptions = 0;
    newItem->currentOpt = 0;
    newItem->subMenu    = NULL;
    push(menu->items, newItem);

    // If this is the first item, set it as the current
    if (1 == menu->items->length)
    {
        menu->currentItem = menu->items->first;
    }
}

/**
 * Remove a single item entry from the menu. This item is removed by pointer,
 * not by doing a string comparison.
 *
 * @param menu The menu to remove a single item from
 * @param label The label for the item to remove
 */
void removeSingleItemFromMenu(menu_t* menu, const char* label)
{
    node_t* listNode = menu->items->first;
    while (NULL != listNode)
    {
        menuItem_t* item = listNode->val;
        if (item->label == label)
        {
            removeEntry(menu->items, listNode);
            if (menu->currentItem == listNode)
            {
                if (NULL != listNode->next)
                {
                    menu->currentItem = listNode->next;
                }
                else
                {
                    menu->currentItem = listNode->prev;
                }
            }
            free(item);
            return;
        }
        listNode = listNode->next;
    }
}

/**
 * Add a multiple item entry to the menu. The multiple items exist in a single
 * entry and are left-right scrollable. The ::menuCb callback will be called
 * each time the multi-item scrolls with the newly selected label as the
 * argument. The ::menuCb callback will also be called if the currently
 * displayed label is selected with that label as the argument.
 *
 * Multi items are rendered with an icon indicating that it is horizontally
 * scrollable.
 *
 * @param menu The menu to add a multi-item to
 * @param labels All of the labels for this multi-item. The underlying memory
 *               isn't copied, so this string must persist for the lifetime of
 *               the menu
 * @param numLabels The number of labels for this multi-item
 */
void addMultiItemToMenu(menu_t* menu, const char* const* labels, uint8_t numLabels)
{
    menuItem_t* newItem = calloc(1, sizeof(menuItem_t));
    newItem->label      = NULL;
    newItem->options    = labels;
    newItem->numOptions = numLabels;
    newItem->currentOpt = 0;
    newItem->subMenu    = NULL;
    push(menu->items, newItem);

    // If this is the first item, set it as the current
    if (1 == menu->items->length)
    {
        menu->currentItem = menu->items->first;
    }
}

/**
 * Remove a multi item entry from the menu. This item is removed by pointer,
 * not by doing any string comparisons.
 *
 * @param menu The menu to remove a multi item from
 * @param labels The labels to remove
 */
void removeMultiItemFromMenu(menu_t* menu, const char* const* labels)
{
    node_t* listNode = menu->items->first;
    while (NULL != listNode)
    {
        menuItem_t* item = listNode->val;
        if (item->options == labels)
        {
            removeEntry(menu->items, listNode);
            if (menu->currentItem == listNode)
            {
                if (NULL != listNode->next)
                {
                    menu->currentItem = listNode->next;
                }
                else
                {
                    menu->currentItem = listNode->prev;
                }
            }

            free(item);
            return;
        }
        listNode = listNode->next;
    }
}

/**
 * This must be called to pass button event from the Swadge Mode to the menu.
 * If a button is passed here, it should not be handled anywhere else
 *
 * @param menu The menu to process button events for
 * @param btn The button event that occurred
 * @return TODO
 */
menu_t* menuButton(menu_t* menu, buttonEvt_t btn)
{
    if (btn.down)
    {
        // Get a pointer to the item for convenience
        menuItem_t* item = menu->currentItem->val;

        switch (btn.button)
        {
            case PB_UP:
            {
                // Scroll up
                if (NULL == menu->currentItem->prev)
                {
                    menu->currentItem = menu->items->last;
                }
                else
                {
                    menu->currentItem = menu->currentItem->prev;
                }
                break;
            }
            case PB_DOWN:
            {
                // Scroll down
                if (NULL == menu->currentItem->next)
                {
                    menu->currentItem = menu->items->first;
                }
                else
                {
                    menu->currentItem = menu->currentItem->next;
                }
                break;
            }
            case PB_LEFT:
            {
                // Scroll options to the left, if applicable
                if (item->options)
                {
                    if (0 == item->currentOpt)
                    {
                        item->currentOpt = item->numOptions - 1;
                    }
                    else
                    {
                        item->currentOpt--;
                    }

                    // Call the callback, not selected
                    menu->cbFunc(item->options[item->currentOpt], false);
                }
                break;
            }
            case PB_RIGHT:
            {
                // Scroll options to the right, if applicable
                if (item->options)
                {
                    if (item->numOptions - 1 == item->currentOpt)
                    {
                        item->currentOpt = 0;
                    }
                    else
                    {
                        item->currentOpt++;
                    }

                    // Call the callback, not selected
                    menu->cbFunc(item->options[item->currentOpt], false);
                }
                break;
            }
            case PB_A:
            {
                // Handle A button presses
                if (item->subMenu)
                {
                    // If this item has a submenu, enter it
                    return item->subMenu;
                }
                else if (item->label == mnuBackStr)
                {
                    // If this is the back string, return the parent menu
                    // Reset the current item when leaving a submenu
                    menu->currentItem = menu->items->first;
                    return menu->parentMenu;
                }
                else if (item->label)
                {
                    // If this is a single item, call the callback
                    menu->cbFunc(item->label, true);
                }
                else if (item->options)
                {
                    // If this is a multi item, call the callback
                    menu->cbFunc(item->options[item->currentOpt], true);
                }
                break;
            }
            case PB_B:
            {
                // If there is a parent menu, go back
                if (menu->parentMenu)
                {
                    // Reset the current item when leaving a submenu
                    menu->currentItem = menu->items->first;
                    return menu->parentMenu;
                }
                break;
            }
            case PB_START:
            case PB_SELECT:
            case TB_0:
            case TB_1:
            case TB_2:
            case TB_3:
            case TB_4:
            default:
            {
                // Unused
                return menu;
            }
        }
    }
    return menu;
}

/**
 * Render a menu to the TFT. This should be called each main loop so that menu
 * animations are smooth. It will draw over the entire display, but may be drawn
 * over afterwards.
 *
 * @param menu The menu to render.
 */
void drawMenu(menu_t* menu)
{
    // Clear everything first
    clearPxTft();

#define ITEMS_PER_PAGE 5
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

    int16_t x = 20;
    int16_t y = 20;

    // Draw an underlined title
    drawText(menu->font, c555, menu->title, x, y);
    y += (menu->font->h + 2);
    drawLine(x, y, x + textWidth(menu->font, menu->title), y, c555, 0, 0, 0, 1, 1);
    y += 3;

    // TODO draw page indicators

    // Draw a page-worth of items
    for (uint8_t itemIdx = 0; itemIdx < ITEMS_PER_PAGE; itemIdx++)
    {
        menuItem_t* item = (menuItem_t*)pageStart->val;
        if (NULL != item)
        {
            // Draw an indicator if the current item is selected
            if (menu->currentItem->val == item)
            {
                drawText(menu->font, c555, "X", 0, y);
            }

            // Draw the label(s)
            if (item->label)
            {
                if (item->subMenu)
                {
                    char label[64] = {0};
                    snprintf(label, sizeof(label) - 1, "%s >>", item->label);
                    drawText(menu->font, c555, label, x, y);
                }
                else
                {
                    drawText(menu->font, c555, item->label, x, y);
                }
            }
            else if (item->options)
            {
                char label[64] = {0};
                snprintf(label, sizeof(label) - 1, "< %s >", item->options[item->currentOpt]);
                drawText(menu->font, c555, label, x, y);
            }

            // Move to the next row
            y += (menu->font->h + 2);
        }

        // Move to the next item
        pageStart = pageStart->next;
        if (NULL == pageStart)
        {
            break;
        }
    }
}
