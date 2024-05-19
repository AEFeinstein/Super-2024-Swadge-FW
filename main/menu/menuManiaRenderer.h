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
 * When initializing the menu, fonts may be passed in as an argument, or the arguments may be \c NULL. If the Swadge
 * mode is loading fonts for itself, you can save RAM by using the same font in the menu renderer. If the Swadge mode
 * isn't using the same fonts, it's easier to have the menu renderer manage it's own fonts. Either way, you should avoid
 * loading the same font multiple times.
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
    font_t* titleFont;              ///< The font to render the title with
    font_t* titleFontOutline;       ///< The font to render the title outline with
    font_t* menuFont;               ///< The font to render the menu with
    font_t* menuFontOutline;        ///< The font to render the menu outline with
    bool titleFontAllocated;        ///< true if this font was allocated by the renderer and should be freed by
                                    ///< deinitMenuManiaRenderer()
    bool titleFontOutlineAllocated; ///< true if this font was allocated by the renderer and should be freed by
                                    ///< deinitMenuManiaRenderer()
    bool menuFontAllocated;         ///< true if this font was allocated by the renderer and should be freed by
                                    ///< deinitMenuManiaRenderer()
    bool menuFontOutlineAllocated;  ///< true if this font was allocated by the renderer and should be freed by
                                    ///< deinitMenuManiaRenderer()
    led_t leds[CONFIG_NUM_LEDS];    ///< An array with the RGB LED state to be output
    wsg_t batt[4];                  ///< Images for the battery levels
    int16_t innerOrbitAngle;        ///< Angle for the inner orbit circle
    int16_t outerOrbitAngle;        ///< Angle for the outer orbit circle
    int32_t innerOrbitTimer;        ///< Timer to rotate the inner orbit circle
    int32_t outerOrbitTimer;        ///< Timer to rotate the outer orbit circle
    int32_t ledDecayTimer;          ///< Timer to decay LEDs
    int32_t ledExciteTimer;         ///< Timer to excite LEDs
    int16_t currentLed;             ///< The current LED being excited
} menuManiaRenderer_t;

menuManiaRenderer_t* initMenuManiaRenderer(font_t* titleFont, font_t* titleFontOutline, font_t* menuFont,
                                           font_t* menuFontOutline);
void deinitMenuManiaRenderer(menuManiaRenderer_t* renderer);
void drawMenuMania(menu_t* menu, menuManiaRenderer_t* renderer, int64_t elapsedUs);

#endif