/*! \file menuManiaRenderer.h
 *
 * \section menuManiaRenderer_design Design Philosophy
 *
 * A menu renderer takes the data in the menu_t data structure and renders it to the display. The menu data structure is
 * separated from the rendering logic so that it is possible to draw menus with differing themes.
 *
 * \section menuManiaRenderer_usage Usage
 *
 * First a ::menu_t data structure must be created. For that usage, see menu.h.
 *
 * The menu renderer is initialized with initMenuManiaRenderer() and deinitialized with deinitMenuManiaRenderer().
 * Menu renderers must be deinitialized or they will leak memory.
 * When initializing the menu, a font must be passed in as an argument. This renderer will not allocate its own font to
 * avoid allocating the same font twice for a given mode (once for the menu and again for the mode itself).
 *
 * The menu is drawn with drawMenuMania(). This will both draw over the entire display and light LEDs. The menu may be
 * drawn on top of later.
 *
 * \section menuManiaRenderer_example Example
 *
 * See menu.h for examples on how to use menuManiaRenderer
 */

#ifndef _MENU_MANIA_RENDERER_H_
#define _MENU_MANIA_RENDERER_H_

#include "menu.h"
#include "spiffs_font.h"
#include "spiffs_wsg.h"
#include "hdw-led.h"

/**
 * @brief A struct containing all the state data to render a mania-style menu and LEDs
 */
typedef struct
{
    font_t* titleFont;           ///< The font to render the menu with
    font_t* menuFont;            ///< The font to render the menu with
    led_t leds[CONFIG_NUM_LEDS]; ///< An array with the RGB LED state to be output
    wsg_t batt[4];               ///< Images for the battery levels
    wsg_t menu_bg;               ///< Background image for the menu
} menuManiaRenderer_t;

menuManiaRenderer_t* initMenuManiaRenderer(font_t* titleFont, font_t* menuFont);
void deinitMenuManiaRenderer(menuManiaRenderer_t* renderer);
void drawMenuMania(menu_t* menu, menuManiaRenderer_t* renderer, int64_t elapsedUs);

#endif