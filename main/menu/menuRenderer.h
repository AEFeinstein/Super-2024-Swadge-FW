#ifndef _MENU_RENDERER_H_
#define _MENU_RENDERER_H_

#include "menu.h"
#include "font.h"

typedef struct
{
    wsg_t arrow;
    wsg_t arrowS;
    font_t* font;
} menuRender_t;

menuRender_t* initMenuRenderer(font_t* menuFont);
void deinitMenuRenderer(menuRender_t* renderer);

void drawMenu(menu_t* menu, font_t* font);
void drawMenuThemed(menu_t* menu, menuRender_t* renderer);

#endif