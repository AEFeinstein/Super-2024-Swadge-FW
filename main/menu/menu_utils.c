#include "menu_utils.h"

#include <stdio.h>
#include <stdbool.h>

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
            snprintf(buffer, buflen - 1, "%s: %d", item->label, item->currentSetting);
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
    return (mnuBackStr == item->label);
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
