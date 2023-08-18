#ifndef _MENU_QUICK_SETTINGS_RENDERER_H_
#define _MENU_QUICK_SETTINGS_RENDERER_H_

#include <stdint.h>

#include "swadge2024.h"
#include "font.h"
#include "wsg.h"
#include "linked_list.h"

typedef struct
{
    font_t* font;
    const wsg_t* defaultIcon;
    list_t iconMap;
} menuQuickSettingsRenderer_t;

menuQuickSettingsRenderer_t* initMenuQuickSettingsRenderer(font_t* font);
void deinitMenuQuickSettingsRenderer(menuQuickSettingsRenderer_t* renderer);
void drawMenuQuickSettings(menu_t* menu, menuQuickSettingsRenderer_t* renderer, int64_t elapsedUs);
void quickSettingsRendererAddIcon(menuQuickSettingsRenderer_t* renderer, const char* label, const wsg_t* onWsg,
                                  const wsg_t* offWsg);
#endif
