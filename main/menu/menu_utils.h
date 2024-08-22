/*! \file menu_utils.h
 *
 * This file contains common utility functions to assist in manipulating menus and
 * making menu renderers.
 *
 */

#ifndef _MENU_UTILS_H_
#define _MENU_UTILS_H_

#include "menu.h"

const char* getMenuItemLabelText(char* buffer, int buflen, const menuItem_t* item);
bool menuItemIsSetting(const menuItem_t* item);
bool menuItemHasOptions(const menuItem_t* item);
bool menuItemHasPrev(const menuItem_t* item);
bool menuItemHasNext(const menuItem_t* item);
bool menuItemIsBack(const menuItem_t* item);
bool menuItemHasSubMenu(const menuItem_t* item);
void menuSavePosition(const char** out, int len, const menu_t* menu);
menu_t* menuRestorePosition(const char** in, int len, menu_t* menu);

#endif
