#include "menu_utils.h"

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>

/**
 * @brief Write and return the proper text label to use for the given menu item
 *
 * If the menu item's label is a simple string, it will be returned directly.
 * If the menu item's label must be constructed from multiple strings, it
 * will be written to \c buffer, and a pointer to \c buffer will be returned.
 *
 * @param buffer   A buffer to use for constructing the label, if needed
 * @param buflen   The size of the buffer
 * @param item     The menu item whose label to return
 *
 * @return A pointer to the item's label text
 */
const char* getMenuItemLabelText(char* buffer, int buflen, const menuItem_t* item)
{
    if (item->minSetting != item->maxSetting)
    {
        // Handle concatenating the values
        if (item->options)
        {
            snprintf(buffer, buflen - 1, "%s%s", item->label, item->options[item->currentOpt]);
        }
        else
        {
            snprintf(buffer, buflen - 1, "%s: %" PRId32, item->label, item->currentSetting);
        }

        // Return the buffer passed to us
        return buffer;
    }
    else if (item->label)
    {
        // Just return the item label
        return item->label;
    }
    else if (item->options)
    {
        // Just return the option label
        return item->options[item->currentOpt];
    }
    else
    {
        // Return an empty string
        buffer[0] = '\0';
        return buffer;
    }
}

/**
 * @brief Returns whether a menu item is a settings item
 *
 * This will return true for items that were added with either addSettingsItemToMenu()
 * or addSettingsOptionsItemToMenu()
 *
 * @param item A pointer to the menu item to check
 * @return true if the item is associated with a setting
 * @return false if the item does not have a setting
 */
bool menuItemIsSetting(const menuItem_t* item)
{
    return (item->minSetting != item->maxSetting);
}

/**
 * @brief Returns whether this menu item has options
 *
 * This will return true for items that were added with either addMultiItemToMenu()
 * or addSettingsOptionsItemToMenu()
 *
 * @param item A pointer to the menu item to check
 * @return true if the item has multiple options
 * @return false if the item has a single label
 */
bool menuItemHasOptions(const menuItem_t* item)
{
    return (NULL != item->options);
}

/**
 * @brief Returns true if this item has any previous option
 *
 * @param item A pointer to the menu item to check
 * @return true if the item has an option before the current one
 * @return false if the item has the first option selected
 */
bool menuItemHasPrev(const menuItem_t* item)
{
    return (menuItemHasOptions(item) && (0 != item->currentOpt))
           || (menuItemIsSetting(item) && (item->currentSetting > item->minSetting));
}

/**
 * @brief Returns true if this item has any next option
 *
 * @param item A pointer to the menu item to check
 * @return true if the item has an option after the current one
 * @return false if the item has the last option selected
 */
bool menuItemHasNext(const menuItem_t* item)
{
    return (menuItemHasOptions(item) && (item->currentOpt != item->numOptions - 1))
           || (menuItemIsSetting(item) && (item->currentSetting < item->maxSetting));
}

/**
 * @brief Returns true if this item is the "Back" button on a sub-menu
 *
 * @param item A pointer to the menu item to check
 * @return true if the item is the "Back" item
 * @return false if the item is any other menu item
 */
bool menuItemIsBack(const menuItem_t* item)
{
    return item && (mnuBackStr == item->label);
}

/**
 * @brief Returns true if this item will enter a sub-menu when selected
 *
 * @param item A pointer to the menu item to check
 * @return true if the item has a sub-menu
 * @return false if the item does not have a sub-menu
 */
bool menuItemHasSubMenu(const menuItem_t* item)
{
    return (NULL != item->subMenu);
}

/**
 * @brief Stores the current menu position in a buffer
 *
 * The output array should contain at least as many items as the maximum nesting depth of the menu
 *
 * @param out A pointer to an array of char-pointers that will receive the state
 * @param len The maximum number of items to write into the out buffer.
 * @param menu The menu whose state to save
 */
void menuSavePosition(const char** out, int len, const menu_t* menu)
{
    const menu_t* curMenu = menu;

    // Just calculate the depth
    int depth = 0;
    while (curMenu->parentMenu)
    {
        curMenu = curMenu->parentMenu;
        depth++;
    }

    curMenu = menu;

    // Zero out the rest of the array if we won't be setting it
    if (depth + 1 < len)
    {
        memset(out + depth + 1, 0, sizeof(out) * (len - depth - 1));
    }

    const char** curOut = (out + depth);
    do
    {
        if (!curMenu->currentItem || !curMenu->currentItem->val)
        {
            break;
        }

        menuItem_t* curItem = (menuItem_t*)curMenu->currentItem->val;

        *curOut-- = curItem->label;
        curMenu = curMenu->parentMenu;
    } while (curOut >= out);
}

/**
 * @brief Restores the menu position from the given buffer
 *
 * @param in The char-pointer array that was set by menuSavePosition()
 * @param len The length of the char-poniter array
 * @param menu The menu
 *
 * @return A pointer to the currently-selected menu
 */
menu_t* menuRestorePosition(const char** in, int len, menu_t* menu)
{
    menu_t* curMenu = menu;

    // Go to the start of the menu first
    while (curMenu->parentMenu)
    {
        curMenu = curMenu->parentMenu;
    }

    // Now navigate down
    const char** curIn = in;
    const char* toSelect = NULL;
    while (curIn < (in + len) && *curIn && curMenu)
    {
        if (toSelect && menuItemHasSubMenu((menuItem_t*)curMenu->currentItem->val))
        {
            curMenu = menuSelectCurrentItem(curMenu);
        }

        menuNavigateToItem(curMenu, *curIn);

        toSelect = *(++curIn);
    }

    return curMenu ? curMenu : menu;
}
