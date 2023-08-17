#ifndef _SOKO_MODE_H_
#define _SOKO_MODE_H_

#include "swadge2024.h"

extern swadgeMode_t sokoMode;

typedef enum
{
    SOKO_MENU,
    SOKO_LEVELPLAY,
} sokoScreen_t;

typedef enum
{
    SK_EMPTY = 0,
    SK_WALL = 1,
    SK_GOAL = 2,
} sokoTile_t;

typedef struct
{
    uint8_t width;
    uint8_t height;
    sokoTile_t tiles[20][20];
} sokoLevel_t;

typedef struct
{
    menu_t* menu;                               ///< The menu structure
    menuLogbookRenderer_t* menuLogbookRenderer; ///< Renderer for the menu
    font_t ibm;                                 ///< The font used in the menu and game
    sokoScreen_t screen;                        ///< The screen being displayed

    //player input
    uint16_t btnState;      ///< The button state

    //current level
    sokoLevel_t currentLevel;

} soko_t;

// soko_t* soko;

#endif