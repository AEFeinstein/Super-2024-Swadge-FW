#ifndef _SOKO_MODE_H_
#define _SOKO_MODE_H_

#include "swadge2024.h"
#include "soko_input.h"

extern swadgeMode_t sokoMode;

typedef enum
{
    SOKO_MENU,
    SOKO_LEVELPLAY,
} sokoScreen_t;

typedef enum
{
    SKE_NONE=0,
    SKE_PLAYER=1,
    SKE_CRATE=2,
} sokoEntityType_t;

typedef enum
{
    SK_EMPTY = 0,
    SK_WALL = 1,
    SK_GOAL = 2,
} sokoTile_t;

typedef struct 
{
    sokoEntityType_t type;
    uint16_t x;
    uint16_t y;
} sokoEntity_t;

typedef struct
{
    uint8_t width;
    uint8_t height;
    uint8_t entityCount;
    uint16_t playerIndex;//we could have multiple players...
    sokoTile_t tiles[20][20];
    sokoEntity_t entities[25];//todo: pointer and runtime array size
} sokoLevel_t;





typedef struct
{
    //meta
    menu_t* menu;                               ///< The menu structure
    menuLogbookRenderer_t* menuLogbookRenderer; ///< Renderer for the menu
    font_t ibm;                                 ///< The font used in the menu and game
    sokoScreen_t screen;                        ///< The screen being displayed

    //game settings
    uint16_t maxPush;                           ///< Maximum number of crates the player can push. Use 0 for no limit.

    //input
    sokoGameplayInput_t input;

    //current level
    sokoLevel_t currentLevel;
    bool allCratesOnGoal;

} soko_t;

// soko_t* soko;

#endif