/*! \file menuZorldoRenderer.h
 *
 * \section menuZorldoRenderer_design Design Philosophy
 *
 * A menu renderer takes the data in the menu_t data structure and renders it to the display. The menu data structure is
 * separated from the rendering logic so that it is possible to draw menus with differing themes.
 *
 * \section menuZorldoRenderer_usage Usage
 *
 * First a ::menu_t data structure must be created. For that usage, see menu.h.
 *
 * The menu renderer is initialized with initMenuZorldoRenderer() and deinitialized with deinitMenuZorldoRenderer().
 * Menu renderers must be deinitialized or they will leak memory.
 * When initializing the menu, fonts may be passed in as an argument, or the arguments may be \c NULL. If the Swadge
 * mode is loading fonts for itself, you can save RAM by using the same font in the menu renderer. If the Swadge mode
 * isn't using the same fonts, it's easier to have the menu renderer manage it's own fonts. Either way, you should avoid
 * loading the same font multiple times.
 *
 * The menu is drawn with drawMenuZorldo(). This will both draw over the entire display and light LEDs. The menu may be
 * drawn on top of later.
 *
 * \section menuZorldoRenderer_example Example
 *
 * See menu.h for examples on how to use menuZorldoRenderer
 */

#pragma once

#include "hdw-led.h"
#include "menu.h"
#include "fs_wsg.h"
#include "fs_font.h"
#include "vector2d.h"
#include "wsgPalette.h"

typedef struct
{
    float x;
    int16_t y;
    float speed; ///< Speed in px/us
    wsg_t wsg;
    bool mirrored;
} menuZorldoCloud_t;

/**
 * @brief A struct containing all the state data to render a Zorldo-style menu and LEDs
 */
typedef struct
{
    wsg_t corner;  ///< The corner cap for the border
    wsg_t ewi;     ///< Ewi Of Time
    wsg_t battery; ///< Battery container
    wsg_t tomi;    ///< Hey! Listen!

    wsg_t arrowPageUp;  ///< A single up arrow (previous page). Mirror it for down.
    wsg_t arrowRight;   ///< A single right arrow (next option). Mirror it for left.
    wsg_t arrowSubmenu; ///< A double right arrow (enter submenu). Mirror it for...you know.

    paletteColor_t textFillColor;    ///< The color to fill text with
    paletteColor_t textOutlineColor; ///< The color to outline text with
    font_t* titleFont;               ///< The font to render the title with
    font_t* menuFont;                ///< The font to render the menu with
    bool titleFontAllocated;         ///< true if this font was allocated by the renderer and should be freed by
                                     ///< deinitMenuZorldoRenderer()
    bool menuFontAllocated;          ///< true if this font was allocated by the renderer and should be freed by
                                     ///< deinitMenuZorldoRenderer()

    int32_t selectedMarqueeTimer; ///< The timer for marquee-ing the selected item text, if too long to fit
    int32_t shadowMarqueeTimer;   ///< The timer for marquee-ing the selected item text, if too long to fit
    int32_t pageArrowTimer;       ///< The timer for blinking page up/down arrows
    node_t* currentItem;          ///< The currently selected menu item, resets ::selectedMarqueeTimer when changed

    menuZorldoCloud_t clouds[4]; ///< Clouds to drift behind the menu
    float tomiYPos;              ///< Tomi's Y position
    float tomiYTarget;           ///< Y position Tomi is moving towards. Will match the selected menu item.
    float tomiXDegrees;          ///< Sine degrees for Tomi's x offset
    float tomiYDegrees;          ///< Sine degrees for Tomi's y offset

    led_t leds[CONFIG_NUM_LEDS]; ///< An array with the RGB LED state to be output
    bool ledsOn;                 ///< true if LEDs should be set by this renderer, false to leave LEDs alone

    bool drawBody;       ///< true to draw the body background, false to skip it
    uint16_t bodyHeight; ///< the height of the middle portion of the body, between top and bottom decorated parts
} menuZorldoRenderer_t;

menuZorldoRenderer_t* initMenuZorldoRenderer(font_t* titleFont, font_t* titleFontOutline, font_t* menuFont);
void deinitMenuZorldoRenderer(menuZorldoRenderer_t* renderer);
void drawMenuZorldoBody(uint16_t topLeftX, uint16_t topLeftY, menuZorldoRenderer_t* renderer, int64_t elapsedUs);
void drawMenuZorldo(menu_t* menu, menuZorldoRenderer_t* renderer, int64_t elapsedUs);
void setZorldoLedsOn(menuZorldoRenderer_t* renderer, bool ledsOn);
void setDrawMenuZorldoBody(menuZorldoRenderer_t* renderer, bool drawBody);
void setMenuZorldoBodyHeight(menuZorldoRenderer_t* renderer, int16_t height);
