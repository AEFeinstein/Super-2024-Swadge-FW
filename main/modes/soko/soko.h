#ifndef _SOKO_MODE_H_
#define _SOKO_MODE_H_

#include "swadge2024.h"
#include "soko_input.h"
#include "soko_consts.h"

extern swadgeMode_t sokoMode;

typedef enum
{
    SOKO_OVERWORLD   = 0,
    SOKO_CLASSIC     = 1,
    SOKO_EULER       = 2,
    SOKO_LASERBOUNCE = 3
} soko_var_t;

typedef enum
{
    SOKO_MENU,
    SOKO_LEVELPLAY,
    SOKO_LOADNEWLEVEL
} sokoScreen_t;

typedef enum
{
    SKB_EMPTY             = 0,
    SKB_WALL              = 1,
    SKB_FLOOR             = 2,
    SKB_GOAL              = 3,
    SKB_NO_WALK           = 4,
    SKB_OBJSTART          = 201, // Object and Signal Bytes are over 200
    SKB_COMPRESS          = 202,
    SKB_PLAYER            = 203,
    SKB_CRATE             = 204,
    SKB_WARPINTERNAL      = 205,
    SKB_WARPINTERNALEXIT  = 206,
    SKB_WARPEXTERNAL      = 207,
    SKB_BUTTON            = 208,
    SKB_LASEREMITTER      = 209,
    SKB_LASERRECEIVEROMNI = 210,
    SKB_LASERRECEIVER     = 211,
    SKB_LASER90ROTATE     = 212,
    SKB_GHOSTBLOCK        = 213,
    SKB_OBJEND            = 230
} soko_bin_t; // Binary file byte value decode list
typedef struct soko_portal_s
{
    uint8_t x;
    uint8_t y;
    uint8_t index;
    bool levelCompleted; // use this to show completed levels later
} soko_portal_t;

typedef struct soko_goal_s
{
    uint8_t x;
    uint8_t y;
} soko_goal_t;

typedef enum
{
    SKS_INIT, ///< meta enum used for edge cases
    SKS_GAMEPLAY,
    SKS_VICTORY,
} sokoGameState_t;

/*
typedef enum
{
    SOKO_OVERWORLD = 0,
    SOKO_CLASSIC = 1,
    SOKO_EULER = 2
} soko_var_t;
*/

typedef enum
{
    SKE_NONE               = 0,
    SKE_PLAYER             = 1,
    SKE_CRATE              = 2,
    SKE_LASER_90           = 3,
    SKE_STICKY_CRATE       = 4,
    SKE_WARP               = 5,
    SKE_BUTTON             = 6,
    SKE_LASER_EMIT_UP      = 7,
    SKE_LASER_RECEIVE_OMNI = 8,
    SKE_LASER_RECEIVE      = 9,
    SKE_GHOST              = 10
} sokoEntityType_t;

typedef enum
{
    SKT_EMPTY         = 0,
    SKT_FLOOR         = 1,
    SKT_WALL          = 2,
    SKT_GOAL          = 3,
    SKT_PORTAL        = 4,
    SKT_LASER_EMIT    = 5, // To Be Removed
    SKT_LASER_RECEIVE = 6, // To Be Removed
    SKT_FLOOR_WALKED  = 7,
    SKT_NO_WALK       = 8
} sokoTile_t;

typedef struct
{
    bool sticky;   // For Crates, this determines if crates stick to players. For Buttons, this determines if the button
                   // stays down.
    bool trail;    // Crates leave Euler trails
    bool players;  // For Crates, allow player push. For Button, allow player press.
    bool crates;   // For Buttons, allow crate push. For Portals, allow crate transport.
    bool inverted; // For Buttons, invert default state of affected blocks. For ghost blocks, inverts default
                   // tangibility. Button and Ghostblock with both cancel.
    uint8_t* targetX;
    uint8_t* targetY;
    uint8_t targetCount;
    uint8_t hp;
} sokoEntityProperties_t; // this is a separate type so that it can be allocated as several different types with a void
                          // pointer and some aggressive casting.

typedef struct
{
    sokoEntityType_t type;
    uint16_t x;
    uint16_t y;
    sokoDirection_t facing;
    sokoEntityProperties_t* properties;
    bool propFlag;
} sokoEntity_t;

typedef struct sokoVec_s
{
    int16_t x;
    int16_t y;
} sokoVec_t;

typedef struct sokoCollision_s
{
    uint16_t x;
    uint16_t y;
    uint16_t entityFlag;
    uint16_t entityIndex;

} sokoCollision_t;

typedef struct
{
    wsg_t playerWSG;
    wsg_t playerUpWSG;
    wsg_t playerRightWSG;
    wsg_t playerLeftWSG;
    wsg_t playerDownWSG;
    wsg_t crateWSG;
    wsg_t stickyCrateWSG;
    paletteColor_t wallColor;
    paletteColor_t floorColor;

} sokoTheme_t;

typedef struct
{
    uint16_t levelScale;
    uint8_t width;
    uint8_t height;
    uint8_t entityCount;
    uint16_t playerIndex; // we could have multiple players...
    sokoTile_t tiles[SOKO_MAX_LEVELSIZE][SOKO_MAX_LEVELSIZE];
    sokoEntity_t entities[SOKO_MAX_ENTITY_COUNT]; // todo: pointer and runtime array size
    soko_var_t gameMode;
} sokoLevel_t;

typedef struct
{
    // meta
    menu_t* menu;                               ///< The menu structure
    menuLogbookRenderer_t* menuLogbookRenderer; ///< Renderer for the menu
    font_t ibm;                                 ///< The font used in the menu and game
    sokoScreen_t screen;                        ///< The screen being displayed

    // game settings
    uint16_t maxPush; ///< Maximum number of crates the player can push. Use 0 for no limit.
    sokoGameState_t state;

    // level
    char* levels[SOKO_LEVEL_COUNT]; ///< List of wsg filenames. not comitted to storing level data like this, but idk if
                                    ///< I need level names like picross.
    wsg_t levelWSG;                 ///< Current level

    // input
    sokoGameplayInput_t input;

    // current level
    sokoLevel_t currentLevel;
    bool allCratesOnGoal;

} soko_t;

typedef struct soko_abs_s soko_abs_t;
typedef struct soko_abs_s
{
    // meta
    menu_t* menu;                               ///< The menu structure
    menuLogbookRenderer_t* menuLogbookRenderer; ///< Renderer for the menu
    font_t ibm;                                 ///< The font used in the menu and game
    sokoScreen_t screen;                        ///< The screen being displayed

    char* levelFileText;
    char* levelNames[SOKO_LEVEL_COUNT];
    int levelIndeces[SOKO_LEVEL_COUNT];

    // game settings
    uint16_t maxPush; ///< Maximum number of crates the player can push. Use 0 for no limit.
    sokoGameState_t state;

    // theme settings
    sokoTheme_t* currentTheme; ///< Points to one of the other themes.
    sokoTheme_t overworldTheme;
    sokoTheme_t sokoDefaultTheme;

    // level
    char* levels[SOKO_LEVEL_COUNT]; ///< List of wsg filenames. not comitted to storing level data like this, but idk if
                                    ///< I need level names like picross.
    wsg_t levelWSG;                 ///< Current level
    uint8_t* levelBinaryData;

    soko_portal_t portals[SOKO_MAX_PORTALS];
    uint8_t portalCount;

    soko_goal_t goals[SOKO_MAX_GOALS];
    uint8_t goalCount;

    // input
    sokoGameplayInput_t input;

    // current level
    sokoLevel_t currentLevel;
    bool allCratesOnGoal;

    // camera features
    bool camEnabled;
    uint16_t camX;
    uint16_t camY;
    uint16_t camPadExtentX;
    uint16_t camPadExtentY;
    uint16_t camWidth;
    uint16_t camHeight;

    // game loop functions //Functions are moved into game struct so engine can support different game rules
    void (*gameLoopFunc)(soko_abs_t* self, int64_t elapsedUs);
    void (*sokoTryPlayerMovementFunc)(soko_abs_t* self);
    bool (*sokoTryMoveEntityInDirectionFunc)(soko_abs_t* self, sokoEntity_t* entity, int dx, int dy, uint16_t push);
    void (*drawTilesFunc)(soko_abs_t* self, sokoLevel_t* level);
    bool (*isVictoryConditionFunc)(soko_abs_t* self);
    sokoTile_t (*sokoGetTileFunc)(soko_abs_t* self, int x, int y);

    // Player Convenience Pointer
    sokoEntity_t* soko_player;

    bool loadNewLevelFlag;
    uint8_t loadNewLevelIndex;
    soko_var_t loadNewLevelVariant;

} soko_abs_t;

// soko_t* soko;

#endif