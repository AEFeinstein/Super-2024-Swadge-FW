/*! \file paint_menu.h
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

#include "palette.h"

typedef enum
{
    NO_SCROLL = 0,
    SCROLL_VERT = 1,
    SCROLL_HORIZ = 2,
    SCROLL_REVERSE = 4,
    SCROLL_VERT_R = SCROLL_VERT | SCROLL_REVERSE,
    SCROLL_HORIZ_R = SCROLL_HORIZ | SCROLL_REVERSE,
} wheelScrollDir_t;

typedef struct
{
    /// @brief The font to draw the menu labels with
    const font_t* font;

    /// @brief The list holding each item's information
    list_t itemInfos;

    /// @brief The angle around which the 0th menu item will be centered
    uint16_t anchorAngle;

    /// @brief The X position of the center of the menu
    uint16_t x;

    /// @brief The Y position of the center of the menu
    uint16_t y;

    /// @brief The radius of the center circle of the menu, or 0 if none
    uint16_t centerR;

    /// @brief The radius of unselected items' sectors
    uint16_t unselR;

    /// @brief The radius of the selected sector
    uint16_t selR;

    paletteColor_t textColor;
    paletteColor_t unselBgColor;
    paletteColor_t selBgColor;
    paletteColor_t borderColor;

    bool customBack;
    bool touched;
    bool active;
} wheelMenuRenderer_t;

wheelMenuRenderer_t* initWheelMenu(const font_t* font, uint16_t anchorAngle);
void deinitWheelMenu(wheelMenuRenderer_t* renderer);
void drawWheelMenu(menu_t* menu, wheelMenuRenderer_t* renderer, int64_t elapsedUs);

void wheelMenuSetItemInfo(wheelMenuRenderer_t* renderer, const char* label, const wsg_t* icon, uint8_t position, wheelScrollDir_t scrollDir);
void wheelMenuSetItemColor(wheelMenuRenderer_t* renderer, const char* label, paletteColor_t selectedBg, paletteColor_t unselectedBg);
menu_t* wheelMenuTouch(menu_t* menu, wheelMenuRenderer_t* renderer, uint16_t angle, uint16_t radius);
menu_t* wheelMenuButton(menu_t* menu, wheelMenuRenderer_t* renderer, const buttonEvt_t* evt);
menu_t* wheelMenuTouchRelease(menu_t* menu, wheelMenuRenderer_t* renderer);
bool wheelMenuActive(menu_t* menu, wheelMenuRenderer_t* renderer);

#endif
