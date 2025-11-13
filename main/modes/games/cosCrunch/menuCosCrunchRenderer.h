#pragma once

#include "menu.h"
#include "wsg.h"

typedef struct
{
    struct
    {
        wsg_t pin;
        wsg_t fold;
        wsg_t arrowLeft;
        wsg_t arrowRight;
    } wsg;

    font_t* titleFont;
    font_t* titleFontOutline;
    font_t* menuFont;
} menuCosCrunchRenderer_t;

menuCosCrunchRenderer_t* initMenuCosCrunchRenderer(font_t* titleFont, font_t* titleFontOutline, font_t* menuFont);
void deinitMenuCosCrunchRenderer(menuCosCrunchRenderer_t* renderer);
void drawMenuCosCrunch(menu_t* menu, menuCosCrunchRenderer_t* renderer, int64_t elapsedUs);
