/*! \file menuLogbookRenderer.h
 *
 * \section menuLogbookRenderer_design Design Philosophy
 *
 * A menu renderer takes the data in the menu_t data structure and renders it to the display. The menu data structure is
 * separated from the rendering logic so that it is possible to draw menus with differing themes.
 *
 * \section menuLogbookRenderer_usage Usage
 *
 * First a ::menu_t data structure must be created. For that usage, see menu.h.
 *
 * The menu renderer is initialized with initMenuLogbookRenderer() and deinitialized with deinitMenuLogbookRenderer().
 * Menu renderers must be deinitialized or they will leak memory.
 * When initializing the menu, a font must be passed in as an argument. This renderer will not allocate its own font to
 * avoid allocating the same font twice for a given mode (once for the menu and again for the mode itself).
 *
 * The menu is drawn with drawMenuLogbook(). This will both draw over the entire display and light LEDs. The menu may be
 * drawn on top of later.
 *
 * \section menuLogbookRenderer_example Example
 *
 * See menu.h for examples on how to use menuLogbookRenderer
 */

#ifndef _MENU_LOGBOOK_RENDERER_RENDERER_H_
#define _MENU_LOGBOOK_RENDERER_RENDERER_H_

#include "menu.h"
#include "spiffs_font.h"
#include "spiffs_wsg.h"
#include "hdw-led.h"

/**
 * @brief A struct containing state data for a single LED when a menu is being rendered
 */
typedef struct
{
    uint32_t periodUs; ///< The time, in microseconds, for one step of this LED's fade in or out. Each fade takes
                       ///< between (2 * menuLogbookRenderer_LED_BRIGHTNESS_MIN) and (2 *
                       ///< (menuLogbookRenderer_LED_BRIGHTNESS_MIN + menuLogbookRenderer_LED_BRIGHTNESS_RANGE)) steps
    uint32_t timerUs;  ///< A microsecond timer for this LED's fade in and out
    uint8_t maxBrightness; ///< The maximum brightness for this LED's fade in
    uint8_t brightness;    ///< The LED's current brightness
    bool isLighting;       ///< true if the LED is fading in, false if it is fading out
} menuLed_t;

/**
 * @brief A struct containing all the state data to render a logbook-style menu and LEDs
 */
typedef struct
{
    wsg_t arrow;                          ///< An image of an arrow in normal color
    wsg_t arrowS;                         ///< An image of an arrow in selected color
    font_t* font;                         ///< The font to render the menu with
    led_t leds[CONFIG_NUM_LEDS];          ///< An array with the RGB LED state to be output
    menuLed_t ledTimers[CONFIG_NUM_LEDS]; ///< An array with the LED timers for animation
} menuLogbookRenderer_t;

menuLogbookRenderer_t* initMenuLogbookRenderer(font_t* menuFont);
void deinitMenuLogbookRenderer(menuLogbookRenderer_t* renderer);
void drawMenuLogbook(menu_t* menu, menuLogbookRenderer_t* renderer, int64_t elapsedUs);

#endif