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
#include "wsgPalette.h"

/**
 * @brief A struct containing all the state data to render a mega-style menu and LEDs
 */
typedef struct
{
    wsg_t back;           ///< TODO doc
    wsg_t bg;             ///< TODO doc
    wsg_t body;           ///< TODO doc
    wsg_t down;           ///< TODO doc
    wsg_t item;           ///< TODO doc
    wsg_t item_sel;       ///< TODO doc
    wsg_t next;           ///< TODO doc
    wsg_t prev;           ///< TODO doc
    wsg_t submenu;        ///< TODO doc
    wsg_t up;             ///< TODO doc
    wsg_t batt[4];        ///< Images for the battery levels
    wsgPalette_t palette; ///< TODO doc

    font_t* titleFont;               ///< The font to render the title with
    font_t* titleFontOutline;        ///< The font to render the title outline with
    font_t* menuFont;                ///< The font to render the menu with
    bool titleFontAllocated;         ///< true if this font was allocated by the renderer and should be freed by
                                     ///< deinitMenuMegaRenderer()
    bool titleFontOutlineAllocated;  ///< true if this font was allocated by the renderer and should be freed by
                                     ///< deinitMenuMegaRenderer()
    bool menuFontAllocated;          ///< true if this font was allocated by the renderer and should be freed by
                                     ///< deinitMenuMegaRenderer()
    paletteColor_t textFillColor;    ///< TODO doc
    paletteColor_t textOutlineColor; ///< TODO doc

    int32_t selectedMarqueeTimer; ///< The timer for marquee-ing the selected item text, if too long to fit
    int32_t pageArrowTimer;       ///< The timer for blinking page up/down arrows
    node_t* currentItem;          ///< TODO doc

    const paletteColor_t* bgColors; ///< TODO doc
    int32_t numBgColors;            ///< TODO doc
    int32_t bgColorTimer;           ///< TODO doc
    int32_t bgColorDeg;             ///< TODO doc
    int32_t bgColorIdx;             ///< TODO doc

    led_t leds[CONFIG_NUM_LEDS]; ///< An array with the RGB LED state to be output
    bool ledsOn;                 ///< TODO doc
} menuMegaRenderer_t;

menuMegaRenderer_t* initMenuMegaRenderer(font_t* titleFont, font_t* titleFontOutline, font_t* menuFont);
void deinitMenuMegaRenderer(menuMegaRenderer_t* renderer);
void drawMenuMega(menu_t* menu, menuMegaRenderer_t* renderer, int64_t elapsedUs);
void setMegaLedsOn(menuMegaRenderer_t* renderer, bool ledsOn);
void recolorMenuMegaRenderer(menuMegaRenderer_t* renderer, paletteColor_t textFill, paletteColor_t textOutline,
                             paletteColor_t c1, paletteColor_t c2, paletteColor_t c3, paletteColor_t c4,
                             paletteColor_t c5, paletteColor_t c6, paletteColor_t c7, paletteColor_t c8,
                             const paletteColor_t* bgColors, int32_t numBgColors);

#endif