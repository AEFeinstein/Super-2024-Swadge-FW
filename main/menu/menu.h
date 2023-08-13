/*! \file menu.h
 *
 * \section menu_design Design Philosophy
 *
 * Menus are versatile ways to construct a UI where the user can select text labels.
 *
 * Menus are vertically scrollable and may have any number of rows.
 * Each row in the menu can be a single item selection, a horizontally scrolling multi-item selection, or a submenu.
 * Menus can have any number of submenus.
 *
 * When a menu is initialized a callback function must be provided. When menu options are selected, submenus are
 * entered, or multi-item selections are scrolled, the callback is called.
 *
 * The menu data structure is created and managed in menu.h, but graphical rendering is handled in
 * menuLogbookRenderer.h. The separation of data structure and renderer is intentional and makes it easier to render the
 * data structure in a number of styles. As of now, only menuLogbookRenderer.h is supplied.
 *
 * \section menu_usage Usage
 *
 * Menus can be initialized with initMenu() and deinitialized with deinitMenu().
 * If menus are not deinitialized, they will leak memory.
 * When initializing a menu, a function pointer must be provided which will be called when menu items are selected.
 *
 * Submenu construction may be started and ended with startSubMenu() and endSubMenu() respectively.
 * startSubMenu() will return a pointer to the submenu, and items can be added to it.
 * endSubMenu() will automatically add a "Back" item to the submenu and return a pointer to the parent menu.
 *
 * Items may be added to the menu with addSingleItemToMenu() or addMultiItemToMenu().
 *
 * Items may be removed from the menu with removeSingleItemFromMenu() or removeMultiItemFromMenu().
 *
 * Button events must be passed to the menu with menuButton().
 * These button presses should not be handled elsewhere simultaneously.
 *
 * Menus are drawn with a renderer, such as menuLogbookRenderer.h.
 *
 * \section menu_example Example
 *
 * Const strings for the example menu:
 * \code{.c}
 * static const char demoName[]  = "Demo";
 * static const char demoMenu1[] = "Menu 1";
 * static const char demoMenu2[] = "Menu 2";
 * static const char demoMenu3[] = "Menu 3";
 * static const char demoMenu4[] = "Menu 4";
 * static const char demoMenu5[] = "Menu 5";
 * static const char demoMenu6[] = "Menu 6";
 * static const char demoMenu7[] = "Menu 7";
 * static const char demoMenu8[] = "Menu 8";
 *
 * static const char opt1[]            = "opt1";
 * static const char opt2[]            = "opt2";
 * static const char opt3[]            = "opt3";
 * static const char opt4[]            = "opt4";
 * static const char* const demoOpts[] = {opt1, opt2, opt3, opt4};
 *
 * static const char demoSettingLabel[] = "Setting";
 * \endcode
 *
 * Initialize a menu and renderer:
 * \code{.c}
 * // Allocate the menu
 * menu_t * menu;
 * menu = initMenu(demoName, demoMenuCb);
 *
 * // Add single items
 * addSingleItemToMenu(menu, demoMenu1);
 * addSingleItemToMenu(menu, demoMenu2);
 *
 * // Start a submenu, add an item to it
 * menu = startSubMenu(menu, demoSubMenu1);
 * addSingleItemToMenu(menu, demoMenu7);
 *
 * // Add a submenu with an item to the submenu
 * menu = startSubMenu(menu, demoSubMenu2);
 * addSingleItemToMenu(menu, demoMenu8);
 *
 * // Finish the second submenu
 * menu = endSubMenu(menu);
 *
 * // Finish the first submenu
 * menu = endSubMenu(menu);
 *
 * // Add more items to the main menu, including a multi-item and settings item
 * addSingleItemToMenu(menu, demoMenu3);
 * addSingleItemToMenu(menu, demoMenu4);
 * addMultiItemToMenu(menu, demoOpts, ARRAY_SIZE(demoOpts), 0);
 * addSingleItemToMenu(menu, demoMenu5);
 * addSingleItemToMenu(menu, demoMenu6);
 * addSettingsItemToMenu(menu, demoSettingLabel, getTftBrightnessSettingBounds(), getTftBrightnessSetting());
 *
 * // Load a font
 * font_t logbook;
 * loadFont("logbook.font", &logbook, false);
 * // Initialize a renderer
 * menuLogbookRenderer_t* renderer = initMenuLogbookRenderer(&logbook);
 * \endcode
 *
 * Process button events:
 * \code{.c}
 * buttonEvt_t evt = {0};
 * while (checkButtonQueueWrapper(&evt))
 * {
 *     menu = menuButton(menu, evt);
 * }
 * \endcode
 *
 * Draw the menu from swadgeMode_t.fnMainLoop:
 * \code{.c}
 * drawMenuLogbook(mainMenu->menu, mainMenu->renderer, elapsedUs);
 * \endcode
 *
 * Receive menu callbacks:
 * \code{.c}
 * static void demoMenuCb(const char* label, bool selected)
 * {
 *     printf("%s %s\n", label, selected ? "selected" : "scrolled to");
 * }
 * \endcode
 *
 * Deinitialize menu and renderer:
 * \code{.c}
 * // Free the menu
 * deinitMenu(menu);
 * // Free the renderer
 * deinitMenuLogbookRenderer(renderer);
 * \endcode
 */

#ifndef _MENU_H_
#define _MENU_H_

#include <stdint.h>
#include <stdbool.h>
#include "linked_list.h"
#include "hdw-btn.h"
#include "font.h"
#include "settingsManager.h"

/**
 * @brief A callback which is called when a menu changes or items are selected
 * @param label A pointer to the label which was selected or scrolled to
 * @param selected true if the item was selected with the A button, false if it was scrolled to
 * @param value If a settings item was selected or scrolled, this is the new value for the setting
 */
typedef void (*menuCb)(const char* label, bool selected, uint32_t value);

typedef struct _menu_t menu_t;

/**
 * @brief The underlying data for an item in a menu. The item may be a single-select, multi-select, or submenu item.
 */
typedef struct
{
    const char* label;          ///< The label displayed if single-select or submenu
    const char* const* options; ///< The labels displayed if multi-select
    menu_t* subMenu;            ///< A pointer to a submenu, maybe NULL for single-select and multi-select
    uint8_t numOptions;         ///< The number of options for multi-select
    uint8_t currentOpt;         ///< The current selected option for multi-select
    uint8_t minSetting;         ///< The minimum value for settings items
    uint8_t maxSetting;         ///< The maximum value for settings items
    const int32_t* settingVals; ///< The setting value options for settings-options items
    uint8_t currentSetting;     // The current value for settings items
} menuItem_t;

/**
 * @brief The underlying data for a menu. This fundamentally is a list of menuItem_t
 */
typedef struct _menu_t
{
    const char* title;   ///< The title for this menu
    menuCb cbFunc;       ///< The callback function to call when menu items are selected
    list_t* items;       ///< A list_t of menu items to display
    node_t* currentItem; ///< The currently selected menu item
    menu_t* parentMenu;  ///< The parent menu, may be NULL if this is not a submenu
} menu_t;

/// @brief A string used to return to super-menus that says "Back"
extern const char* mnuBackStr;

menu_t* initMenu(const char* title, menuCb cbFunc) __attribute__((warn_unused_result));
void deinitMenu(menu_t* menu);
menu_t* startSubMenu(menu_t* menu, const char* label) __attribute__((warn_unused_result));
menu_t* endSubMenu(menu_t* menu) __attribute__((warn_unused_result));
void addSingleItemToMenu(menu_t* menu, const char* label);
void removeSingleItemFromMenu(menu_t* menu, const char* label);
void addMultiItemToMenu(menu_t* menu, const char* const* labels, uint8_t numLabels, uint8_t currentLabel);
void removeMultiItemFromMenu(menu_t* menu, const char* const* labels);
void addSettingsItemToMenu(menu_t* menu, const char* label, const settingParam_t* bounds, int32_t val);
void removeSettingsItemFromMenu(menu_t* menu, const char* label);
void addSettingsOptionsItemToMenu(menu_t* menu, const char* const* optionLabels, const int32_t* optionValues, uint8_t numOptions, const settingParam_t* bounds, int32_t currentOption);
void removeSettingsOptionsItemToMenu(menu_t menu, const char* const *optionLabels);
menu_t* menuButton(menu_t* menu, buttonEvt_t btn) __attribute__((warn_unused_result));

#endif