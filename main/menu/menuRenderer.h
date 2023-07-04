#ifndef _MENU_RENDERER_H_
#define _MENU_RENDERER_H_

#include "menu.h"
#include "spiffs_font.h"
#include "spiffs_wsg.h"
#include "hdw-led.h"

typedef struct
{
    uint32_t periodUs;
    uint32_t timerUs;
    uint8_t maxBrightness;
    uint8_t brightness;
    bool isLighting;
} menuLed_t;

typedef struct
{
    wsg_t arrow;
    wsg_t arrowS;
    font_t* font;
    led_t leds[CONFIG_NUM_LEDS];
    menuLed_t ledTimers[CONFIG_NUM_LEDS];
} menuRender_t;

menuRender_t* initMenuRenderer(font_t* menuFont);
void deinitMenuRenderer(menuRender_t* renderer);

void drawMenuThemed(menu_t* menu, menuRender_t* renderer, int64_t elapsedUs);

#endif