//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include <stdio.h>

#include "menu.h"
#include "macros.h"

//==============================================================================
// Const Variables
//==============================================================================

const char* mnuBackStr = "Back";

//==============================================================================
// Function Prototypes
//==============================================================================

static void deinitSubMenu(menu_t* menu);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initialize and return an empty menu. A menu is a collection of vertically
 * scrollable rows. The rows are separated into pages, and when a page
 * boundary is crossed the whole page scrolls.
 *
 * Rows may be single items, or multi items, which are collections of horizontally
 * scrollable items. The callback is called whenever a row is moved to or selected,
 * and when a multi item is scrolled to or selected.
 *
 * Rows may also be submenus. When a submenu is selected, the callback is not
 * called, and instead a the submenu is rendered. Each submenu automatically
 * has a "Back" item added to it, which returns to the parent menu
 *
 * @param title  The title to be displayed for this menu. The underlying memory
 *               isn't copied, so this string must persist for the lifetime of
 *               the menu.
 * @param cbFunc The function to call when a menu option is selected. The
 *               argument to the callback will be the same pointer
 * @return A pointer to the newly allocated menu_t. This must be de-initialized
 *         when the menu is not used anymore.
 */
menu_t* initMenu(const char* title, menuCb cbFunc)
{
    menu_t* menu      = calloc(1, sizeof(menu_t));
    menu->title       = title;
    menu->cbFunc      = cbFunc;
    menu->currentItem = NULL;
    menu->items       = calloc(1, sizeof(list_t));
    menu->parentMenu  = NULL;
    menu->showBattery = false;
    return menu;
}

/**
 * @brief Deinitialize a menu and all connected menus, including submenus and parent
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
 * @brief Deinitialize a menu and all of its submenus recursively. This frees memory
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

        // Free the item
        free(item);

        // Move to the next
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
 * @brief Add a submenu item to the menu. When this item is select, the submenu is
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
 * @brief Finish adding items to a submenu and resume adding items to the parent menu.
 * This will automatically add a "Back" item to the submenu, which returns to
 * the parent menu. ::menuCb will not be called when "Back" is selected.
 *
 * During menu construction, each call to startSubMenu() must have a
 * corresponding call to endSubMenu()
 *
 * @param menu The menu to end a submenu in
 * @return A pointer to the parent menu to use for future function calls
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
 * @brief Add a single item entry to the menu. When this item is selected, the ::menuCb
 * callback is called with the given label as the argument.
 *
 * @param menu The menu to add a single item to
 * @param label The label for this item. The underlying memory isn't copied, so
 *              this string must persist for the lifetime of the menu
 * @return The new menuItem_t that was added to the menu
 */
menuItem_t* addSingleItemToMenu(menu_t* menu, const char* label)
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
    return newItem;
}

/**
 * @brief Remove a single item entry from the menu. This item is removed by pointer,
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
            removeEntry(menu->items, listNode);
            free(item);
            return;
        }
        listNode = listNode->next;
    }
}

/**
 * @brief Add a multiple item entry to the menu. The multiple items exist in a single
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
 * @param currentLabel The current label index to display
 */
void addMultiItemToMenu(menu_t* menu, const char* const* labels, uint8_t numLabels, uint8_t currentLabel)
{
    menuItem_t* newItem = calloc(1, sizeof(menuItem_t));
    newItem->label      = NULL;
    newItem->options    = labels;
    newItem->numOptions = numLabels;
    newItem->currentOpt = currentLabel;
    newItem->subMenu    = NULL;
    push(menu->items, newItem);

    // If this is the first item, set it as the current
    if (1 == menu->items->length)
    {
        menu->currentItem = menu->items->first;
    }
}

/**
 * @brief Remove a multi item entry from the menu. This item is removed by pointer,
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
 * @brief Add a settings entry to the menu. A settings entry is left-right scrollable
 * where an integer setting is incremented or decremented. The ::menuCb callback
 * will be called each time the settings change with the newly selected value as
 * the argument. The ::menuCb callback will also be called if the currently
 * displayed setting is selected with that value as the argument.
 *
 * Settings items are rendered with an icon indicating that it is horizontally
 * scrollable.
 *
 * @param menu The menu to add a settings item to
 * @param label The label for this setting. The integer setting will be drawn after it
 * @param bounds The bounds for this setting
 * @param val The starting value for the setting
 */
void addSettingsItemToMenu(menu_t* menu, const char* label, const settingParam_t* bounds, int32_t val)
{
    menuItem_t* newItem     = calloc(1, sizeof(menuItem_t));
    newItem->label          = label;
    newItem->minSetting     = bounds->min;
    newItem->maxSetting     = bounds->max;
    newItem->currentSetting = val;
    push(menu->items, newItem);

    // If this is the first item, set it as the current
    if (1 == menu->items->length)
    {
        menu->currentItem = menu->items->first;
    }
}

/**
 * @brief Remove a settings item entry from the menu. This item is removed by
 * pointer, not by doing any string comparisons
 *
 * @param menu The menu to remove a multi item from
 * @param label The label to remove
 */
void removeSettingsItemFromMenu(menu_t* menu, const char* label)
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
 * @brief Adds a settings item entry to the menu with a specific list of options. The
 * entry will be left-right scrollable. The ::menuCb callback will be called
 * each time the setting-options item scrolls or is selected with the newly
 * selected label and setting value as the arguments.
 *
 * @param menu The menu to add a settings options item to
 * @param settingLabel The overall label for this setting. This is what will be
 *                     passed to the callback when the selected option changes,
 *                     and this value will be rendered preceding the selected
 *                     option's label.
 * @param optionLabels All of the labels for each option. The underlying memory
 *                     isn't copied, so these strings must persist for the
 *                     lifetime of the menu. The label value for the selected
 *                     option will be rendered following the settingLabel.
 * @param optionValues The corresponding settings values for each option. The
 *                     underlying memory isn't copied, so this array must
 *                     persist for the lifetime of the menu. These values will
 *                     not be rendered, but will be passed to the callback as
 *                     the \c value when the selected option is changed.
 * @param numOptions The number of options and labels for this settings item.
 * @param bounds The bounds for this setting
 * @param currentValue The current value of the setting. Must be one of the values
 *                     in \c optionValues.
 */
void addSettingsOptionsItemToMenu(menu_t* menu, const char* settingLabel, const char* const* optionLabels,
                                  const int32_t* optionValues, uint8_t numOptions, const settingParam_t* bounds,
                                  int32_t currentValue)
{
    menuItem_t* newItem     = calloc(1, sizeof(menuItem_t));
    newItem->label          = settingLabel;
    newItem->options        = optionLabels;
    newItem->settingVals    = optionValues;
    newItem->numOptions     = numOptions;
    newItem->minSetting     = bounds->min;
    newItem->maxSetting     = bounds->max;
    newItem->currentSetting = currentValue;

    // Set the current option to the first in case we can't find it
    newItem->currentOpt = 0;

    // Find the matching element in optionValues
    // We're doing this here because pretty much it's gonna happen somewhere.
    // Doing it this way makes it super easy to use anywhere and we don't need
    // to worry that much if the value happened to be changed to an invalid
    // option somewhere else -- we'll just reset it if it's changed.
    for (uint8_t i = 0; i < numOptions; i++)
    {
        if (currentValue == newItem->settingVals[i])
        {
            newItem->currentOpt = i;
            break;
        }
    }

    push(menu->items, newItem);

    // If this is the first item, set it as the current
    if (1 == menu->items->length)
    {
        menu->currentItem = menu->items->first;
    }
}

/**
 * @brief Remove a settings options item entry from the menu. This item is removed by
 * pointer, not by doing any string comparisons
 *
 * @param menu The menu to remove a multi item from
 * @param optionLabels The list of option labels to remove
 */
void removeSettingsOptionsItemFromMenu(menu_t* menu, const char* const* optionLabels)
{
    node_t* listNode = menu->items->first;
    while (NULL != listNode)
    {
        menuItem_t* item = listNode->val;
        if (item->options == optionLabels)
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
 * @brief Helper function to call the callback when a menu is navigated or an item is selected
 *
 * @param menu The menu to call a callback for
 * @param item The item that was selected or scrolled to
 * @param selected true if the item was selected, false if it was only navigated to
 */
static void menuCallCallbackForItem(menu_t* menu, menuItem_t* item, bool selected)
{
    menu->cbFunc(
        // If the item is a non-setting item with options, pass the option label. Otherwise, the main label
        (item->options && !item->settingVals) ? item->options[item->currentOpt] : item->label,
        // Pass along selected
        selected,
        // If the item is a setting with options, pass the current option value. Otherwise, the regular
        // setting
        item->settingVals ? item->settingVals[item->currentOpt] : item->currentSetting);
}

/**
 * @brief Changes the selected item to the one with the given label, just as though it were scrolled to.
 *
 * @param menu The menu to change the selected item of
 * @param label The label of the menu item or an option to select
 * @return A pointer to the menu to use for future function calls. It may be a sub or parent menu.
 */
menu_t* menuNavigateToItem(menu_t* menu, const char* label)
{
    node_t* listNode = menu->items->first;
    while (NULL != listNode)
    {
        menuItem_t* item = listNode->val;

        if (item->label)
        {
            if (item->label == label)
            {
                menu->currentItem = listNode;
                menuCallCallbackForItem(menu, item, false);
                return menu;
            }
        }
        else if (item->options)
        {
            for (uint8_t i = 0; i < item->numOptions; i++)
            {
                if (item->options[i] == label)
                {
                    menu->currentItem = listNode;
                    item->currentOpt  = i;

                    if (item->settingVals)
                    {
                        item->currentSetting = item->settingVals[i];
                    }

                    menuCallCallbackForItem(menu, item, false);
                    return menu;
                }
            }
        }

        listNode = listNode->next;
    }

    return menu;
}

/**
 * @brief Navigate to the previous item in the menu. This is equivalent to pressing the UP button
 *
 * @param menu The menu to navigate in
 * @return A pointer to the menu to use for future function calls. It may be a sub or parent menu.
 */
menu_t* menuNavigateToPrevItem(menu_t* menu)
{
    if (NULL == menu->currentItem->prev)
    {
        menu->currentItem = menu->items->last;
    }
    else
    {
        menu->currentItem = menu->currentItem->prev;
    }

    // Call the callback for the move
    menuCallCallbackForItem(menu, menu->currentItem->val, false);

    return menu;
}

/**
 * @brief Navigate to the next item in the menu. This is equivalent to pressing the DOWN button
 *
 * @param menu The menu to navigate in
 * @return A pointer to the menu to use for future function calls. It may be a sub or parent menu.
 */
menu_t* menuNavigateToNextItem(menu_t* menu)
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

    // Call the callback for the move
    menuCallCallbackForItem(menu, menu->currentItem->val, false);
    return menu;
}

/**
 * @brief Navigate to the previous option in a menu item. This is equivalent to pressing the LEFT button
 *
 * @param menu The menu to navigate in
 * @return A pointer to the menu to use for future function calls. It may be a sub or parent menu.
 */
menu_t* menuNavigateToPrevOption(menu_t* menu)
{
    // Get a pointer to the item for convenience
    menuItem_t* item = menu->currentItem ? menu->currentItem->val : NULL;

    // Scroll options to the left, if applicable
    if (NULL == item)
    {
        return menu;
    }
    else if (item->options)
    {
        if (0 == item->currentOpt && !item->settingVals)
        {
            item->currentOpt = item->numOptions - 1;
        }
        else if (item->currentOpt > 0)
        {
            item->currentOpt--;
        }

        // Call the callback, not selected
        if (item->settingVals)
        {
            item->currentSetting = item->settingVals[item->currentOpt];
        }
    }
    else if (item->minSetting != item->maxSetting)
    {
        item->currentSetting = MAX(item->currentSetting - 1, item->minSetting);
    }
    else if (menu->parentMenu)
    {
        // If this item has a parent menu, return to it -- no callback
        return menu->parentMenu;
    }
    else
    {
        // Don't call the callback again for items without options
        return menu;
    }

    // Call the callback, not selected
    menuCallCallbackForItem(menu, item, false);
    return menu;
}

/**
 * @brief Navigate to the next option in a menu item. This is equivalent to pressing the RIGHT button
 *
 * @param menu The menu to navigate in
 * @return A pointer to the menu to use for future function calls. It may be a sub or parent menu.
 */
menu_t* menuNavigateToNextOption(menu_t* menu)
{
    menuItem_t* item = menu->currentItem ? menu->currentItem->val : NULL;

    // Scroll options to the right, if applicable
    if (NULL == item)
    {
        return menu;
    }
    else if (item->options)
    {
        if (item->numOptions - 1 == item->currentOpt && !item->settingVals)
        {
            item->currentOpt = 0;
        }
        else if (item->currentOpt + 1 < item->numOptions)
        {
            item->currentOpt++;
        }

        // Call the callback, not selected
        if (item->settingVals)
        {
            item->currentSetting = item->settingVals[item->currentOpt];
        }
    }
    else if (item->minSetting != item->maxSetting)
    {
        item->currentSetting = MIN(item->currentSetting + 1, item->maxSetting);
    }
    else if (item->subMenu)
    {
        // If this item has a submenu, enter it
        return item->subMenu;
    }
    else
    {
        // Don't call the callback again for items without options
        return menu;
    }

    menuCallCallbackForItem(menu, item, false);
    return menu;
}

/**
 * @brief Select the current item in a menu item. This is equivalent to pressing the A button.
 *
 * @param menu The menu to select an item in
 * @return A pointer to the menu to use for future function calls. It may be a sub or parent menu.
 */
menu_t* menuSelectCurrentItem(menu_t* menu)
{
    menuItem_t* item = menu->currentItem ? menu->currentItem->val : NULL;

    // Handle A button presses
    if (NULL == item)
    {
        return menu;
    }
    else if (item->subMenu)
    {
        // If this item has a submenu, call the callback, then enter it
        menu->cbFunc(item->label, true, 0);
        return item->subMenu;
    }
    else if (item->settingVals)
    {
        menu->cbFunc(item->label, true, item->settingVals[item->currentOpt]);
    }
    else if (item->minSetting != item->maxSetting)
    {
        // Call the callback, not selected
        menu->cbFunc(item->label, true, item->currentSetting);
    }
    else if (item->label)
    {
        if (item->label == mnuBackStr)
        {
            // If this is the back string, return the parent menu
            // Reset the current item when leaving a submenu
            menu->currentItem = menu->items->first;
            return menu->parentMenu;
        }
        else
        {
            // If this is a single item, call the callback
            menu->cbFunc(item->label, true, 0);
        }
    }
    else if (item->options)
    {
        // If this is a multi item, call the callback
        menu->cbFunc(item->options[item->currentOpt], true, 0);
    }

    // maybe return menu?
    return menu;
}

/**
 * @brief Performs the equivalent of scrolling to the given setting value or option index for the
 * currently selected menu item.
 *
 * @param menu The menu to scroll the selected item of
 * @param value The setting value or option index to scroll to
 * @return menu_t* The menu
 */
menu_t* menuSetCurrentOption(menu_t* menu, int32_t value)
{
    if (menu->currentItem)
    {
        menuItem_t* item = menu->currentItem->val;
        if (item->options)
        {
            item->currentOpt = CLAMP(value, 0, item->numOptions - 1);
        }
        else
        {
            item->currentSetting = CLAMP(value, item->minSetting, item->maxSetting);
        }

        menuCallCallbackForItem(menu, item, false);
    }

    return menu;
}

/**
 * @brief This must be called to pass button event from the Swadge mode to the menu.
 * If a button is passed here, it should not be handled anywhere else
 *
 * @param menu The menu to process button events for
 * @param evt The button event that occurred
 * @return A pointer to the menu to use for future function calls. It may be a sub or parent menu.
 */
menu_t* menuButton(menu_t* menu, buttonEvt_t evt)
{
    if (evt.down)
    {
        switch (evt.button)
        {
            case PB_UP:
            {
                // Scroll up
                return menuNavigateToPrevItem(menu);
            }
            case PB_DOWN:
            {
                return menuNavigateToNextItem(menu);
            }
            case PB_LEFT:
            {
                return menuNavigateToPrevOption(menu);
            }
            case PB_RIGHT:
            {
                return menuNavigateToNextOption(menu);
            }
            case PB_A:
            {
                return menuSelectCurrentItem(menu);
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
 * @brief Show or hide the battery on the menu. This should be called from the
 * root menu after the menu is constructed
 *
 * @param menu The menu to show a battery indicator on
 * @param showBattery true to show the battery, false to hide it
 */
void setShowBattery(menu_t* menu, bool showBattery)
{
    // Set this menu
    menu->showBattery = showBattery;

    // For each item
    node_t* itemNode = menu->items->first;
    while (itemNode)
    {
        menuItem_t* item = (menuItem_t*)itemNode->val;

        // If this item has a submenu
        if (NULL != item->subMenu)
        {
            // Set the battery there too (recursive)
            setShowBattery(item->subMenu, showBattery);
        }

        // Move to the next item
        itemNode = itemNode->next;
    }
}
