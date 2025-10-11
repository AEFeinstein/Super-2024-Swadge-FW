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

#pragma once

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
    wsg_t bg;             ///< The screen's background image with cutout hexagons
    wsg_t body_top;       ///< The top part of the menu's background image.
    wsg_t body_bottom;    ///< The bottom part of the menu's background image.
    wsg_t item;           ///< Background image for non-selected items
    wsg_t item_sel;       ///< Background image for the selected item
    wsg_t up;             ///< A single up arrow (previous page)
    wsg_t down;           ///< A single down arrow (next page)
    wsg_t next;           ///< A single right arrow (next option)
    wsg_t prev;           ///< A single left arrow (previous option)
    wsg_t submenu;        ///< A double right arrow (enter submenu)
    wsg_t back;           ///< A double left arrow (exit submenu)
    wsg_t batt[4];        ///< Images for the battery levels
    wsgPalette_t palette; ///< A palette to recolor menu images with

    paletteColor_t textFillColor;    ///< The color to fill text with
    paletteColor_t textOutlineColor; ///< The color to outline text with
    font_t* titleFont;               ///< The font to render the title with
    font_t* titleFontOutline;        ///< The font to render the title outline with
    font_t* menuFont;                ///< The font to render the menu with
    bool titleFontAllocated;         ///< true if this font was allocated by the renderer and should be freed by
                                     ///< deinitMenuMegaRenderer()
    bool titleFontOutlineAllocated;  ///< true if this font was allocated by the renderer and should be freed by
                                     ///< deinitMenuMegaRenderer()
    bool menuFontAllocated;          ///< true if this font was allocated by the renderer and should be freed by
                                     ///< deinitMenuMegaRenderer()

    int32_t selectedMarqueeTimer; ///< The timer for marquee-ing the selected item text, if too long to fit
    int32_t shadowMarqueeTimer;   ///< The timer for marquee-ing the selected item text, if too long to fit
    int32_t pageArrowTimer;       ///< The timer for blinking page up/down arrows
    node_t* currentItem;          ///< The currently selected menu item, resets ::selectedMarqueeTimer when changed

    const paletteColor_t* bgColors; ///< A list of colors to cycle through for the background hexagons
    int32_t numBgColors;            ///< The number of colors in ::bgColors
    int32_t bgColorTimer;           ///< A timer to increment ::bgColorDeg in order to cycle through ::bgColors
    int32_t bgColorDeg; ///< When cycling through ::bgColors, a sine wave is followed. This counts the degrees into the
                        ///< sine wave, 0 to 180
    int32_t yOff;       ///< An offset to the hexagons to make them scroll smoothly
    int32_t bgColorIdx; ///< The current index into ::bgColors

    led_t leds[CONFIG_NUM_LEDS]; ///< An array with the RGB LED state to be output
    bool ledsOn;                 ///< true if LEDs should be set by this renderer, false to leave LEDs alone

    bool drawBody;          ///< true to draw the sci-fi rectangle body background, false to skip it
    bool conveyorBeltStyle; ///< true to draw a sliding background and sliding lights
} menuMegaRenderer_t;

menuMegaRenderer_t* initMenuMegaRenderer(font_t* titleFont, font_t* titleFontOutline, font_t* menuFont);
void deinitMenuMegaRenderer(menuMegaRenderer_t* renderer);
void drawMenuBody(uint16_t topLeftX, uint16_t topLeftY, uint8_t expansionHeight, bool flipLR,
                  menuMegaRenderer_t* renderer);
void drawMenuMega(menu_t* menu, menuMegaRenderer_t* renderer, int64_t elapsedUs);
void setMegaLedsOn(menuMegaRenderer_t* renderer, bool ledsOn);
void setDrawBody(menuMegaRenderer_t* renderer, bool drawBody);
void recolorMenuMegaRenderer(menuMegaRenderer_t* renderer, paletteColor_t textFill, paletteColor_t textOutline,
                             paletteColor_t c1, paletteColor_t c2, paletteColor_t c3, paletteColor_t c4,
                             paletteColor_t c5, paletteColor_t c6, paletteColor_t c7, paletteColor_t c8,
                             const paletteColor_t* bgColors, int32_t numBgColors);
