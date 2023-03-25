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
 * Menus can be drawn with drawMenu(). A menu will draw over the entire display but may be drawn over later.
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
 * \endcode
 *
 * Initialize a menu:
 * \code{.c}
 * // Allocate the menu
 * menu_t * menu;
 * menu = initMenu(demoName, &ibm, demoMenuCb);
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
 * // Add more items to the main menu, including a multi-item
 * addSingleItemToMenu(menu, demoMenu3);
 * addSingleItemToMenu(menu, demoMenu4);
 * addMultiItemToMenu(menu, demoOpts, ARRAY_SIZE(demoOpts));
 * addSingleItemToMenu(menu, demoMenu5);
 * addSingleItemToMenu(menu, demoMenu6);
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
 * Draw the menu:
 * \code{.c}
 * drawMenu(menu);
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
 * Deinitialize menu:
 * \code{.c}
 * // Free the menu
 * deinitMenu(menu);
 * \endcode
 */

#ifndef _MENU_H_
#define _MENU_H_

#include <stdint.h>
#include <stdbool.h>
#include "linked_list.h"
#include "hdw-btn.h"
#include "font.h"

#define NUM_MENU_ITEMS 5

typedef void (*menuCb)(const char*, bool selected);

typedef struct _menu_t menu_t;

typedef struct
{
    const char* label;
    const char* const* options;
    menu_t* subMenu;
    uint8_t numOptions;
    uint8_t currentOpt;
} menuItem_t;

typedef struct _menu_t
{
    const char* title;
    font_t* font;
    menuCb cbFunc;
    list_t* items;
    node_t* currentItem;
    menu_t* parentMenu;
} menu_t;

menu_t* initMenu(const char* title, font_t* font, menuCb cbFunc) __attribute__((warn_unused_result));
void deinitMenu(menu_t* menu);
menu_t* startSubMenu(menu_t* menu, const char* label) __attribute__((warn_unused_result));
menu_t* endSubMenu(menu_t* menu) __attribute__((warn_unused_result));
void addSingleItemToMenu(menu_t* menu, const char* label);
void removeSingleItemFromMenu(menu_t* menu, const char* label);
void addMultiItemToMenu(menu_t* menu, const char* const* labels, uint8_t numLabels);
void removeMultiItemFromMenu(menu_t* menu, const char* const* labels);
menu_t* menuButton(menu_t* menu, buttonEvt_t btn) __attribute__((warn_unused_result));

void drawMenu(menu_t* menu);

#endif