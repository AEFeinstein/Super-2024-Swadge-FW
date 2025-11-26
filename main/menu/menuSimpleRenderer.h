/*! \file menuSimpleRenderer.h
 *
 * \section menuSimpleRenderer_design Design Philosophy
 *
 * A menu renderer takes the data in the menu_t data structure and renders it to the display. The menu data structure is
 * separated from the rendering logic so that it is possible to draw menus with differing themes.
 *
 * \section menuSimpleRenderer_usage Usage
 *
 * First a ::menu_t data structure must be created. For that usage, see menu.h.
 *
 * The menu renderer is initialized with initMenuSimpleRenderer() and deinitialized with deinitMenuSimpleRenderer().
 * Menu renderers must be deinitialized or they will leak memory.
 * When initializing the menu, fonts may be passed in as an argument, or the arguments may be \c NULL. If the Swadge
 * mode is loading fonts for itself, you can save RAM by using the same font in the menu renderer. If the Swadge mode
 * isn't using the same fonts, it's easier to have the menu renderer manage it's own fonts. Either way, you should avoid
 * loading the same font multiple times.
 *
 * The menu is drawn with drawMenuSimple(). This will both draw over the entire display and light LEDs. The menu may be
 * drawn on top of later.
 *
 * \section menuSimpleRenderer_example Example
 *
 * See menu.h for examples on how to use menuSimpleRenderer
 */

#pragma once

#include "menu.h"
#include "fs_font.h"

/**
 * @brief A struct containing all the state data to render a simple menu
 */
typedef struct
{
    font_t* font; ///< The font to render the title with
    int32_t numRows;

    paletteColor_t borderColor;  ///< The color of the background border
    paletteColor_t bgColor;      ///< The color of the screen background
    paletteColor_t rowTextColor; ///< The color of the row text
<<<<<<< HEAD
=======

    wsg_t arrow;        ///< The arrow to draw to indicate pages
    int32_t blinkTimer; ///< A timer used to blink the page arrows
>>>>>>> origin/main
} menuSimpleRenderer_t;

menuSimpleRenderer_t* initMenuSimpleRenderer(font_t* font, paletteColor_t border, paletteColor_t bg,
                                             paletteColor_t text, int32_t rows);
void deinitMenuSimpleRenderer(menuSimpleRenderer_t* renderer);
<<<<<<< HEAD
void drawMenuSimple(menu_t* menu, menuSimpleRenderer_t* renderer);
=======
void drawMenuSimple(menu_t* menu, menuSimpleRenderer_t* renderer, uint32_t elapsedUs);
>>>>>>> origin/main
