/*! \file wheel_menu.h
 *
 * The new and improved paint menu will work like this, thanks to the new touchpad:
 *
 * 1. When the touchpad is pressed, the canvas is saved (if not already otherwise?)
 * 2. A ring selector is drawn over the center of the screen. It is divided into 4 sectors, with
 *    a dead-zone circle in the center. When the touchpad is moved towards a ring, it is highlighted.
 * 3. There are multiple types of entries in the ring. Some will move to another menu when selected
 *    by releasing the touchpad, and some will allow pressing up/down and/or left/right on the D-pad
 *    to change values. If a sub-menu is selected, it can be exited by pressing B, or maybe there
 *    will be an "exit" ring?
 * 4. Maybe there shuold be an option to have a "sticky" menu -- instead of requiring you to hold
 *    the touchpad at the same time as going up/down, it would keep you in the menu until you hit B.
 * 5. "Tool Ring" Layout:
 *    - The top sector is the tool selector, with left/right.
 *    - The left sector is the color selector, with up/down. And, maybe A or Left edits the color?
 *    - The right sector is the tool size selector, with left/right.
 *    - The bottom sector is the "..." or "Settings" menu
 * 6. "Settings" layout:
 *    - Save
 *    - Load
 *    - New
 *    - Exit
 *    - ... edit palette?
 */
#ifndef _WHEEL_MENU_H_
#define _WHEEL_MENU_H_

#include "menu.h"
#include "menu_utils.h"
#include "wsg.h"
#include "geometry.h"
#include "palette.h"

/**
 * @brief Wheel scroll directions
 */
typedef enum
{
    NO_SCROLL      = 0,                             ///< Do not allow setting values to scroll with buttons
    SCROLL_VERT    = 1,                             ///< Allow scrolling setting value with Up/Down D-Pad buttons
    SCROLL_HORIZ   = 2,                             ///< Allow scrolling setting value with Let/Right D-Pad buttons
    SCROLL_REVERSE = 4,                             ///< Flip the direction of scrolling
    SCROLL_VERT_R  = SCROLL_VERT | SCROLL_REVERSE,  ///< Allow scrolling setting value with Up/Down, reversed
    SCROLL_HORIZ_R = SCROLL_HORIZ | SCROLL_REVERSE, ///< Allow scrolling setting value with Up/Down, reversed
    ZOOM_SUBMENU   = 8,                             ///< When zoomed to a settings item, display it as a submenu
    ZOOM_GAUGE     = 16,                            ///< When zoomed to a settings item, display it as a gauge
    ZOOM_CONVEYOR  = 32, ///< When zoomed to a settings item, display it as a rotating conveyor belt
    MASK_ZOOM      = 56, ///< Mask for all zoom options
} wheelScrollDir_t;

typedef enum
{
    WM_SHAPE_DEFAULT      = 0,
    WM_SHAPE_ROUNDED_RECT = 1,
    WM_SHAPE_SQUARE       = 2,
} wheelMenuShapeFlags_t;

/**
 * @brief Renderer for a menu wheel
 */
typedef struct
{
    const font_t* font;          ///< The font to draw the menu labels with
    const rectangle_t* textBox;  ///< A pointer to the text box to draw the selected item's label inside
    list_t itemInfos;            ///< The list holding each item's information
    uint16_t anchorAngle;        ///< The angle around which the 0th menu item will be centered
    uint16_t x;                  ///< The X position of the center of the menu
    uint16_t y;                  ///< The Y position of the center of the menu
    uint16_t spokeR;             ///< The radial offset of the ring items
    uint16_t unselR;             ///< The radius of unselected items' sectors
    uint16_t selR;               ///< The radius of the selected sector
    paletteColor_t textColor;    ///< Color of the selected menu item text label
    paletteColor_t unselBgColor; ///< Default background color of unselected items
    paletteColor_t selBgColor;   ///< Default background color of selected items
    paletteColor_t borderColor;  ///< Color of the circle borders
    bool customBack;             ///< Whether the center "back" circle has been customized
    bool touched;                ///< Whether the touchpad is currently touched
    bool active;                 ///< Whether the menu should be shown, regardless of touch
    bool zoomed;                 ///< Whether or not a settings item is selected
    bool zoomBackSelected;       ///< Whether or not the center is selected while zoomed
    uint8_t zoomValue;           ///< The current selected option/value if zoomed
    int32_t timer;               ///< The timer for animations
} wheelMenuRenderer_t;

wheelMenuRenderer_t* initWheelMenu(const font_t* font, uint16_t anchorAngle, const rectangle_t* textBox);
void deinitWheelMenu(wheelMenuRenderer_t* renderer);
void drawWheelMenu(menu_t* menu, wheelMenuRenderer_t* renderer, int64_t elapsedUs);

void wheelMenuSetColor(wheelMenuRenderer_t* renderer, paletteColor_t textColor);
void wheelMenuSetItemInfo(wheelMenuRenderer_t* renderer, const char* label, const wsg_t* icon, uint8_t position,
                          wheelScrollDir_t scrollDir);
void wheelMenuSetItemIcon(wheelMenuRenderer_t* renderer, const char* label, const wsg_t* icon);
void wheelMenuSetItemColor(wheelMenuRenderer_t* renderer, const char* label, paletteColor_t selectedBg,
                           paletteColor_t unselectedBg);
void wheelMenuSetItemTextIcon(wheelMenuRenderer_t* renderer, const char* label, const char* textIcon);
void wheelMenuSetItemSize(wheelMenuRenderer_t* renderer, const char* label, int16_t w, int16_t h,
                          wheelMenuShapeFlags_t shapeFlags);
menu_t* wheelMenuTouch(menu_t* menu, wheelMenuRenderer_t* renderer, uint16_t angle, uint16_t radius);
menu_t* wheelMenuButton(menu_t* menu, wheelMenuRenderer_t* renderer, const buttonEvt_t* evt);
menu_t* wheelMenuTouchRelease(menu_t* menu, wheelMenuRenderer_t* renderer);
bool wheelMenuActive(menu_t* menu, wheelMenuRenderer_t* renderer);

#endif
