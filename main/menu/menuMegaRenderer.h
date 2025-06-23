/*! \file menuMegaRenderer.h
 *
 * \section menuMegaRenderer_design Design Philosophy
 *
 * A menu renderer takes the data in the menu_t data structure and renders it to the display. The menu data structure is
 * separated from the rendering logic so that it is possible to draw menus with differing themes.
 *
 * \section menuMegaRenderer_usage Usage
 *
 * First a ::menu_t data structure must be created. For that usage, see menu.h.
 *
 * The menu renderer is initialized with initmenuMegaRenderer() and deinitialized with deinitmenuMegaRenderer().
 * Menu renderers must be deinitialized or they will leak memory.
 * When initializing the menu, fonts may be passed in as an argument, or the arguments may be \c NULL. If the Swadge
 * mode is loading fonts for itself, you can save RAM by using the same font in the menu renderer. If the Swadge mode
 * isn't using the same fonts, it's easier to have the menu renderer manage it's own fonts. Either way, you should avoid
 * loading the same font multiple times.
 *
 * The menu is drawn with drawMenuMega(). This will both draw over the entire display and light LEDs. The menu may be
 * drawn on top of later.
 *
 * \section menuMegaRenderer_example Example
 *
 * See menu.h for examples on how to use menuMegaRenderer
 */

#ifndef _MENU_MEGA_RENDERER_H_
#define _MENU_MEGA_RENDERER_H_

#include "hdw-led.h"
#include "menu.h"
#include "fs_wsg.h"
#include "fs_font.h"

/**
 * @brief A struct containing all the state data to render a mega-style menu and LEDs
 */
typedef struct
{
    wsg_t back;
    wsg_t bg;
    wsg_t body;
    wsg_t down;
    wsg_t item;
    wsg_t item_sel;
    wsg_t next;
    wsg_t prev;
    wsg_t submenu;
    wsg_t up;
    wsg_t batt[4]; ///< Images for the battery levels

    font_t* titleFont;              ///< The font to render the title with
    font_t* titleFontOutline;       ///< The font to render the title outline with
    font_t* menuFont;               ///< The font to render the menu with
    bool titleFontAllocated;        ///< true if this font was allocated by the renderer and should be freed by
                                    ///< deinitMenuManiaRenderer()
    bool titleFontOutlineAllocated; ///< true if this font was allocated by the renderer and should be freed by
                                    ///< deinitMenuManiaRenderer()
    bool menuFontAllocated;         ///< true if this font was allocated by the renderer and should be freed by
                                    ///< deinitMenuManiaRenderer()

    int32_t selectedMarqueeTimer; ///< The timer for marquee-ing the selected item text, if too long to fit
    int32_t pageArrowTimer;       ///< The timer for blinking page up/down arrows

    led_t leds[CONFIG_NUM_LEDS]; ///< An array with the RGB LED state to be output
    bool ledsOn;

} menuMegaRenderer_t;

menuMegaRenderer_t* initMenuMegaRenderer(font_t* titleFont, font_t* titleFontOutline, font_t* menuFont);
void deinitMenuMegaRenderer(menuMegaRenderer_t* renderer);
void drawMenuMega(menu_t* menu, menuMegaRenderer_t* renderer, int64_t elapsedUs);
void setMegaLedsOn(menuMegaRenderer_t* renderer, bool ledsOn);

#endif