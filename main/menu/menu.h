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

menu_t* initMenu(const char* title, font_t* font, menuCb cbFunc);
void deinitMenu(menu_t* menu);
menu_t* startSubMenu(menu_t* menu, const char* label);
menu_t* endSubMenu(menu_t* menu);
void addSingleItemToMenu(menu_t* menu, const char* label);
void removeSingleItemFromMenu(menu_t* menu, const char* label);
void addMultiItemToMenu(menu_t* menu, const char* const* labels, uint8_t numLabels);
void removeMultiItemFromMenu(menu_t* menu, const char* const* labels);
menu_t* menuButton(menu_t* menu, buttonEvt_t btn);

void drawMenu(menu_t* menu);

#endif