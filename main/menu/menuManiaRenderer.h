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
#include "fs_font.h"
#include "fs_wsg.h"
#include "hdw-led.h"

#define Y_SECTION_MARGIN 14
#define TITLE_BG_HEIGHT  40

/// The height of the title section, from the top of the TFT to the bottom of the title block
#define MANIA_TITLE_HEIGHT (TITLE_BG_HEIGHT + Y_SECTION_MARGIN)

/// The height of the body section, from the bottom of the title block to the bottom of the TFT
#define MANIA_BODY_HEIGHT (TFT_HEIGHT - MANIA_TITLE_HEIGHT)

typedef struct
{
    int16_t orbitAngle;       ///< Angle for the orbit circles
    int32_t orbitTimer;       ///< Timer to rotate the orbit circles
    int32_t orbitUsPerDegree; ///< Number of microseconds to wait before rotating by one degree
    int32_t orbitDirection;   ///< The direction to rotate, +1 or -1
    int16_t diameterAngle;    ///< Angle to grow and shrink the orbit circles
    int32_t diameterTimer;    ///< Timer to grow and shrink the orbit circles
    paletteColor_t color;     ///< The color of this ring
} maniaRing_t;

/**
 * @brief A struct containing all the state data to render a mania-style menu and LEDs
 */
typedef struct
{
    font_t* titleFont;              ///< The font to render the title with
    font_t* titleFontOutline;       ///< The font to render the title outline with
    font_t* menuFont;               ///< The font to render the menu with
    bool titleFontAllocated;        ///< true if this font was allocated by the renderer and should be freed by
                                    ///< deinitMenuManiaRenderer()
    bool titleFontOutlineAllocated; ///< true if this font was allocated by the renderer and should be freed by
                                    ///< deinitMenuManiaRenderer()
    bool menuFontAllocated;         ///< true if this font was allocated by the renderer and should be freed by
                                    ///< deinitMenuManiaRenderer()
    led_t leds[CONFIG_NUM_LEDS];    ///< An array with the RGB LED state to be output
    wsg_t batt[4];                  ///< Images for the battery levels

    maniaRing_t rings[2];

    int32_t ledDecayTimer;  ///< Timer to decay LEDs
    int32_t ledExciteTimer; ///< Timer to excite LEDs
    int16_t currentLed;     ///< The current LED being excited
    bool ledsOn;            ///< true to use the LEDs, false to keep them off

    menuItem_t* selectedItem;     ///< Reference to the selected item to tell when it changes
    int16_t selectedShadowIdx;    ///< The index to the color offset for the selected drop shadow
    int32_t selectedShadowTimer;  ///< The timer to change the color for the selected drop shadow
    int16_t selectedBounceIdx;    ///< The index to the bounce offset for the selected item
    int32_t selectedBounceTimer;  ///< The timer to bounce the offset for the selected item
    int32_t selectedValue;        ///< The option index or setting value to tell when it changes
    int32_t selectedMarqueeTimer; ///< The timer for marquee-ing the selected item text, if too long to fit
} menuManiaRenderer_t;

menuManiaRenderer_t* initMenuManiaRenderer(font_t* titleFont, font_t* titleFontOutline, font_t* menuFont);
void deinitMenuManiaRenderer(menuManiaRenderer_t* renderer);
void drawMenuMania(menu_t* menu, menuManiaRenderer_t* renderer, int64_t elapsedUs);
void setManiaLedsOn(menuManiaRenderer_t* renderer, bool ledsOn);

#endif