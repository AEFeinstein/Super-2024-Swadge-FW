#ifndef _MODE_RAY_H_
#define _MODE_RAY_H_

//==============================================================================
// Includes
//==============================================================================

#include "swadge.h"
#include "fp_math.h"
#include "starfield.h"
#include "esp_random.h"
#include "credits_utils.h"

//==============================================================================
// Defines
//==============================================================================

/** Time, in microseconds, to lock out buttons after showing dialog, death, or credits */
#define RAY_BUTTON_LOCKOUT_US 1000000

/** The number of total maps */
#define NUM_MAPS 4
/** The number of keys per map */
#define NUM_KEYS 3

/** The player's starting max health */
#define GAME_START_HEALTH 3

/** Microseconds per effect when standing in lava or on heal */
#define US_PER_FLOOR_EFFECT 500000

/** The number of bullets tracked at a given point in time */
#define MAX_RAY_BULLETS 32

/** The blink time for pause and dialog items */
#define BLINK_US 500000

/**
 * @brief Helper macro to check if a cell is of a given type
 * The type is the top three bits of the type
 *
 * @param cell The cell to check
 * @param type The type to check against (top three bits)
 */
#define CELL_IS_TYPE(cell, type) (((cell) & (0xE0)) == (type))

// Bits used for tile type construction, topmost bit
#define BG  0x00
#define OBJ 0x80
// Types of background, next two top bits
#define META  0x00
#define FLOOR 0x20
#define WALL  0x40
#define DOOR  0x60
// Types of objects, next two top bits
#define ITEM    0x00
#define ENEMY   0x20
#define BULLET  0x40
#define SCENERY 0x60

// The pixel size of each cell
#define CELL_SIZE 20

//==============================================================================
// Enums
//==============================================================================

/**
 * @brief Tile types. The top three bits are metadata, the bottom five bits are
 * a unique number per that metadata
 */
typedef enum __attribute__((packed))
{
    // Special empty type
    EMPTY = (BG | META | 0),
    // Special delete tile
    DELETE = (BG | META | 1),
    // Floor tiles
    BG_FLOOR_HOLE = (BG | FLOOR | 0),
    BG_FLOOR_1    = (BG | FLOOR | 1),
    BG_FLOOR_2    = (BG | FLOOR | 2),
    BG_FLOOR_3    = (BG | FLOOR | 3),
    BG_FLOOR_4    = (BG | FLOOR | 4),
    BG_FLOOR_5    = (BG | FLOOR | 5),
    BG_FLOOR_6    = (BG | FLOOR | 6),
    BG_FLOOR_7    = (BG | FLOOR | 7),
    BG_FLOOR_8    = (BG | FLOOR | 8),
    BG_FLOOR_9    = (BG | FLOOR | 9),
    BG_FLOOR_10   = (BG | FLOOR | 10),
    BG_FLOOR_11   = (BG | FLOOR | 11),
    BG_FLOOR_12   = (BG | FLOOR | 12),
    BG_FLOOR_13   = (BG | FLOOR | 13),
    BG_FLOOR_14   = (BG | FLOOR | 14),
    BG_FLOOR_15   = (BG | FLOOR | 15),
    BG_FLOOR_16   = (BG | FLOOR | 16),
    BG_FLOOR_17   = (BG | FLOOR | 17),
    BG_FLOOR_18   = (BG | FLOOR | 18),
    BG_FLOOR_19   = (BG | FLOOR | 19),
    BG_FLOOR_20   = (BG | FLOOR | 20),
    BG_FLOOR_21   = (BG | FLOOR | 21),
    BG_FLOOR_22   = (BG | FLOOR | 22),
    BG_FLOOR_23   = (BG | FLOOR | 23),
    BG_FLOOR_24   = (BG | FLOOR | 24),
    BG_FLOOR_25   = (BG | FLOOR | 25),
    BG_FLOOR_26   = (BG | FLOOR | 26),
    BG_FLOOR_27   = (BG | FLOOR | 27),
    BG_FLOOR_28   = (BG | FLOOR | 28),
    BG_FLOOR_29   = (BG | FLOOR | 29),
    BG_FLOOR_30   = (BG | FLOOR | 30),
    BG_FLOOR_31   = (BG | FLOOR | 31),
    // Wall tiles
    BG_WALL_TARGET = (BG | WALL | 0),
    BG_WALL_1      = (BG | WALL | 1),
    BG_WALL_2      = (BG | WALL | 2),
    BG_WALL_3      = (BG | WALL | 3),
    BG_WALL_4      = (BG | WALL | 4),
    BG_WALL_5      = (BG | WALL | 5),
    BG_WALL_6      = (BG | WALL | 6),
    BG_WALL_7      = (BG | WALL | 7),
    BG_WALL_8      = (BG | WALL | 8),
    BG_WALL_9      = (BG | WALL | 9),
    BG_WALL_10     = (BG | WALL | 10),
    BG_WALL_11     = (BG | WALL | 11),
    BG_WALL_12     = (BG | WALL | 12),
    BG_WALL_13     = (BG | WALL | 13),
    BG_WALL_14     = (BG | WALL | 14),
    BG_WALL_15     = (BG | WALL | 15),
    BG_WALL_16     = (BG | WALL | 16),
    BG_WALL_17     = (BG | WALL | 17),
    BG_WALL_18     = (BG | WALL | 18),
    BG_WALL_19     = (BG | WALL | 19),
    BG_WALL_20     = (BG | WALL | 20),
    BG_WALL_21     = (BG | WALL | 21),
    BG_WALL_22     = (BG | WALL | 22),
    BG_WALL_23     = (BG | WALL | 23),
    BG_WALL_24     = (BG | WALL | 24),
    BG_WALL_25     = (BG | WALL | 25),
    BG_WALL_26     = (BG | WALL | 26),
    BG_WALL_27     = (BG | WALL | 27),
    BG_WALL_28     = (BG | WALL | 28),
    BG_WALL_29     = (BG | WALL | 29),
    BG_WALL_30     = (BG | WALL | 30),
    BG_WALL_31     = (BG | WALL | 31),
    // Door tiles
    BG_DOOR_BUSH    = (BG | DOOR | 0),
    BG_DOOR_CRACK_H = (BG | DOOR | 1),
    BG_DOOR_CRACK_V = (BG | DOOR | 2),
    BG_DOOR_3       = (BG | DOOR | 3),
    BG_DOOR_4       = (BG | DOOR | 4),
    BG_DOOR_5       = (BG | DOOR | 5),
    BG_DOOR_6       = (BG | DOOR | 6),
    BG_DOOR_7       = (BG | DOOR | 7),
    BG_DOOR_8       = (BG | DOOR | 8),
    BG_DOOR_9       = (BG | DOOR | 9),
    BG_DOOR_10      = (BG | DOOR | 10),
    BG_DOOR_11      = (BG | DOOR | 11),
    BG_DOOR_12      = (BG | DOOR | 12),
    BG_DOOR_13      = (BG | DOOR | 13),
    BG_DOOR_14      = (BG | DOOR | 14),
    BG_DOOR_15      = (BG | DOOR | 15),
    BG_DOOR_16      = (BG | DOOR | 16),
    BG_DOOR_17      = (BG | DOOR | 17),
    BG_DOOR_18      = (BG | DOOR | 18),
    BG_DOOR_19      = (BG | DOOR | 19),
    BG_DOOR_20      = (BG | DOOR | 20),
    BG_DOOR_21      = (BG | DOOR | 21),
    BG_DOOR_22      = (BG | DOOR | 22),
    BG_DOOR_23      = (BG | DOOR | 23),
    BG_DOOR_24      = (BG | DOOR | 24),
    BG_DOOR_25      = (BG | DOOR | 25),
    BG_DOOR_26      = (BG | DOOR | 26),
    BG_DOOR_27      = (BG | DOOR | 27),
    BG_DOOR_28      = (BG | DOOR | 28),
    BG_DOOR_29      = (BG | DOOR | 29),
    BG_DOOR_30      = (BG | DOOR | 30),
    BG_DOOR_31      = (BG | DOOR | 31),
    // Self and Enemies
    OBJ_ENEMY_START_POINT = (OBJ | ENEMY | 0),
    OBJ_ENEMY_BOX         = (OBJ | ENEMY | 1),
    OBJ_ENEMY_2           = (OBJ | ENEMY | 2),
    OBJ_ENEMY_3           = (OBJ | ENEMY | 3),
    OBJ_ENEMY_4           = (OBJ | ENEMY | 4),
    OBJ_ENEMY_5           = (OBJ | ENEMY | 5),
    OBJ_ENEMY_6           = (OBJ | ENEMY | 6),
    OBJ_ENEMY_7           = (OBJ | ENEMY | 7),
    OBJ_ENEMY_8           = (OBJ | ENEMY | 8),
    OBJ_ENEMY_9           = (OBJ | ENEMY | 9),
    OBJ_ENEMY_10          = (OBJ | ENEMY | 10),
    OBJ_ENEMY_11          = (OBJ | ENEMY | 11),
    OBJ_ENEMY_12          = (OBJ | ENEMY | 12),
    OBJ_ENEMY_13          = (OBJ | ENEMY | 13),
    OBJ_ENEMY_14          = (OBJ | ENEMY | 14),
    OBJ_ENEMY_15          = (OBJ | ENEMY | 15),
    OBJ_ENEMY_16          = (OBJ | ENEMY | 16),
    OBJ_ENEMY_17          = (OBJ | ENEMY | 17),
    OBJ_ENEMY_18          = (OBJ | ENEMY | 18),
    OBJ_ENEMY_19          = (OBJ | ENEMY | 19),
    OBJ_ENEMY_20          = (OBJ | ENEMY | 20),
    OBJ_ENEMY_21          = (OBJ | ENEMY | 21),
    OBJ_ENEMY_22          = (OBJ | ENEMY | 22),
    OBJ_ENEMY_23          = (OBJ | ENEMY | 23),
    OBJ_ENEMY_24          = (OBJ | ENEMY | 24),
    OBJ_ENEMY_25          = (OBJ | ENEMY | 25),
    OBJ_ENEMY_26          = (OBJ | ENEMY | 26),
    OBJ_ENEMY_27          = (OBJ | ENEMY | 27),
    OBJ_ENEMY_28          = (OBJ | ENEMY | 28),
    OBJ_ENEMY_29          = (OBJ | ENEMY | 29),
    OBJ_ENEMY_30          = (OBJ | ENEMY | 30),
    OBJ_ENEMY_31          = (OBJ | ENEMY | 31),
    // Item pickups
    OBJ_ITEM_EWI        = (OBJ | ITEM | 0),
    OBJ_ITEM_BOMB       = (OBJ | ITEM | 1),
    OBJ_ITEM_BOOTS      = (OBJ | ITEM | 2),
    OBJ_ITEM_SHIELD     = (OBJ | ITEM | 3),
    OBJ_ITEM_BOW        = (OBJ | ITEM | 4),
    OBJ_ITEM_BOOMERANG  = (OBJ | ITEM | 5),
    OBJ_ITEM_TURNTABLES = (OBJ | ITEM | 6),
    OBJ_ITEM_LULLABY    = (OBJ | ITEM | 7),
    OBJ_ITEM_HEART      = (OBJ | ITEM | 8),
    OBJ_ITEM_MPOINT_1   = (OBJ | ITEM | 9),
    OBJ_ITEM_MPOINT_5   = (OBJ | ITEM | 10),
    OBJ_ITEM_MPOINT_10  = (OBJ | ITEM | 11),
    OBJ_ITEM_MPOINT_20  = (OBJ | ITEM | 12),
    OBJ_ITEM_13         = (OBJ | ITEM | 13),
    OBJ_ITEM_14         = (OBJ | ITEM | 14),
    OBJ_ITEM_15         = (OBJ | ITEM | 15),
    OBJ_ITEM_16         = (OBJ | ITEM | 16),
    OBJ_ITEM_17         = (OBJ | ITEM | 17),
    OBJ_ITEM_18         = (OBJ | ITEM | 18),
    OBJ_ITEM_19         = (OBJ | ITEM | 19),
    OBJ_ITEM_20         = (OBJ | ITEM | 20),
    OBJ_ITEM_21         = (OBJ | ITEM | 21),
    OBJ_ITEM_22         = (OBJ | ITEM | 22),
    OBJ_ITEM_23         = (OBJ | ITEM | 23),
    OBJ_ITEM_24         = (OBJ | ITEM | 24),
    OBJ_ITEM_25         = (OBJ | ITEM | 25),
    OBJ_ITEM_26         = (OBJ | ITEM | 26),
    OBJ_ITEM_27         = (OBJ | ITEM | 27),
    OBJ_ITEM_28         = (OBJ | ITEM | 28),
    OBJ_ITEM_29         = (OBJ | ITEM | 29),
    OBJ_ITEM_30         = (OBJ | ITEM | 30),
    OBJ_ITEM_31         = (OBJ | ITEM | 31),
    // Bullets
    OBJ_BULLET_ARROW     = (OBJ | BULLET | 0),
    OBJ_BULLET_BOMB      = (OBJ | BULLET | 1),
    OBJ_BULLET_BOOMERANG = (OBJ | BULLET | 2),
    OBJ_BULLET_SWORD     = (OBJ | BULLET | 3),
    OBJ_BULLET_4         = (OBJ | BULLET | 4),
    OBJ_BULLET_5         = (OBJ | BULLET | 5),
    OBJ_BULLET_6         = (OBJ | BULLET | 6),
    OBJ_BULLET_7         = (OBJ | BULLET | 7),
    OBJ_BULLET_8         = (OBJ | BULLET | 8),
    OBJ_BULLET_9         = (OBJ | BULLET | 9),
    OBJ_BULLET_10        = (OBJ | BULLET | 10),
    OBJ_BULLET_11        = (OBJ | BULLET | 11),
    OBJ_BULLET_12        = (OBJ | BULLET | 12),
    OBJ_BULLET_13        = (OBJ | BULLET | 13),
    OBJ_BULLET_14        = (OBJ | BULLET | 14),
    OBJ_BULLET_15        = (OBJ | BULLET | 15),
    OBJ_BULLET_16        = (OBJ | BULLET | 16),
    OBJ_BULLET_17        = (OBJ | BULLET | 17),
    OBJ_BULLET_18        = (OBJ | BULLET | 18),
    OBJ_BULLET_19        = (OBJ | BULLET | 19),
    OBJ_BULLET_20        = (OBJ | BULLET | 20),
    OBJ_BULLET_21        = (OBJ | BULLET | 21),
    OBJ_BULLET_22        = (OBJ | BULLET | 22),
    OBJ_BULLET_23        = (OBJ | BULLET | 23),
    OBJ_BULLET_24        = (OBJ | BULLET | 24),
    OBJ_BULLET_25        = (OBJ | BULLET | 25),
    OBJ_BULLET_26        = (OBJ | BULLET | 26),
    OBJ_BULLET_27        = (OBJ | BULLET | 27),
    OBJ_BULLET_28        = (OBJ | BULLET | 28),
    OBJ_BULLET_29        = (OBJ | BULLET | 29),
    OBJ_BULLET_30        = (OBJ | BULLET | 30),
    OBJ_BULLET_31        = (OBJ | BULLET | 31),
    // Scenery
    OBJ_SCENERY_SHOP_BOMB = (OBJ | SCENERY | 0),
    OBJ_SCENERY_1         = (OBJ | SCENERY | 1),
    OBJ_SCENERY_2         = (OBJ | SCENERY | 2),
    OBJ_SCENERY_3         = (OBJ | SCENERY | 3),
    OBJ_SCENERY_4         = (OBJ | SCENERY | 4),
    OBJ_SCENERY_5         = (OBJ | SCENERY | 5),
    OBJ_SCENERY_6         = (OBJ | SCENERY | 6),
    OBJ_SCENERY_7         = (OBJ | SCENERY | 7),
    OBJ_SCENERY_8         = (OBJ | SCENERY | 8),
    OBJ_SCENERY_9         = (OBJ | SCENERY | 9),
    OBJ_SCENERY_10        = (OBJ | SCENERY | 10),
    OBJ_SCENERY_11        = (OBJ | SCENERY | 11),
    OBJ_SCENERY_12        = (OBJ | SCENERY | 12),
    OBJ_SCENERY_13        = (OBJ | SCENERY | 13),
    OBJ_SCENERY_14        = (OBJ | SCENERY | 14),
    OBJ_SCENERY_15        = (OBJ | SCENERY | 15),
    OBJ_SCENERY_16        = (OBJ | SCENERY | 16),
    OBJ_SCENERY_17        = (OBJ | SCENERY | 17),
    OBJ_SCENERY_18        = (OBJ | SCENERY | 18),
    OBJ_SCENERY_19        = (OBJ | SCENERY | 19),
    OBJ_SCENERY_20        = (OBJ | SCENERY | 20),
    OBJ_SCENERY_21        = (OBJ | SCENERY | 21),
    OBJ_SCENERY_22        = (OBJ | SCENERY | 22),
    OBJ_SCENERY_23        = (OBJ | SCENERY | 23),
    OBJ_SCENERY_24        = (OBJ | SCENERY | 24),
    OBJ_SCENERY_25        = (OBJ | SCENERY | 25),
    OBJ_SCENERY_26        = (OBJ | SCENERY | 26),
    OBJ_SCENERY_27        = (OBJ | SCENERY | 27),
    OBJ_SCENERY_28        = (OBJ | SCENERY | 28),
    OBJ_SCENERY_29        = (OBJ | SCENERY | 29),
    OBJ_SCENERY_30        = (OBJ | SCENERY | 30),
    OBJ_SCENERY_31        = (OBJ | SCENERY | 31),
} rayMapCellType_t;

/**
 * @brief Types of events that trigger scripts
 */
typedef enum
{
    SHOOT_OBJS   = 0, ///< Objects were shot
    KILL         = 1, ///< Enemies were killed
    GET          = 2, ///< Items were gotten
    TOUCH        = 3, ///< Objects were touched
    SHOOT_WALLS  = 4, ///< Walls were shot
    ENTER        = 5, ///< Map cells were entered
    TIME_ELAPSED = 6, ///< Time elapsed
    NUM_IF_OP_TYPES,  ///< The number of IF operation types
} ifOp_t;

/**
 * @brief Types of things that scripts can do
 */
typedef enum
{
    OPEN    = 7,  ///< Open doors
    CLOSE   = 8,  ///< Close doors
    SPAWN   = 9,  ///< Spawn enemies or items
    DESPAWN = 10, ///< Despawn enemies or items
    DIALOG  = 11, ///< Show a dialog box
    WARP    = 12, ///< Warp to a location on a map
    WIN     = 13, ///< Win the game
    CAMERA  = 14, ///< Move the camera
    SHOP    = 15, ///< Try to buy an object
} thenOp_t;

/**
 * @brief A script argument, whether to AND or OR items in a list
 */
typedef enum
{
    AND = 0, ///< All items in the list must happen for the script to trigger
    OR  = 1, ///< Any item in the list may happen for the script to trigger
} andOr_t;

/**
 * @brief A script argument, whether the items in a list are ordered or not
 */
typedef enum
{
    IN_ORDER  = 0, ///< Items in the list must happen in the specific order
    ANY_ORDER = 1, ///< Items in the list may happen in any order
} order_t;

/**
 * @brief A script argument, whether the script repeats after triggering or not
 */
typedef enum
{
    ONCE   = 0, ///< The script only triggers once
    ALWAYS = 1, ///< The script resets after triggering
} repeat_t;

typedef enum
{
    NO_KEY,   ///< Key was not picked up
    KEY,      ///< Key was picked up and not used yet
    OPEN_KEY, ///< Key was both picked up and used
} rayKeyState_t;

/**
 * @brief The different screens that can be displayed
 */
typedef enum
{
    RAY_MENU,         ///< The main menu is being shown
    RAY_GAME,         ///< The game loop is being shown
    RAY_INSTRUMENT,   ///< An instrument is being played
    RAY_DIALOG,       ///< A dialog box is being shown
    RAY_PAUSE,        ///< The pause menu is being shown
    RAY_WARP_SCREEN,  ///< The warp screen animation is being shown
    RAY_DEATH_SCREEN, ///< The player has died
    RAY_CREDITS,      ///< Showing the credits for the game
} rayScreen_t;

/**
 * @brief The different pause screens that can be displayed
 */
typedef enum
{
    RP_LOCAL_MAP,   ///< The map the player is currently in
    RP_WORLD_MAP,   ///< All the maps and how they connect
    RP_NUM_SCREENS, ///< The number of pause screens
} rayPauseScreen_t;

/**
 * @brief Enum for visited tiles
 */
typedef enum __attribute__((packed))
{
    NOT_VISITED,      ///< This map tile has not been visited
    VISITED,          ///< This map tile has been visited
    SCRIPT_DOOR_OPEN, ///< This map tile has been visited and has a permanently-open door
} rayTileState_t;

//==============================================================================
// Structs
//==============================================================================

/**
 * @brief A script argument, coordinates in the map
 */
typedef struct
{
    uint8_t x; ///< The X coordinate
    uint8_t y; ///< The Y coordinate
} rayMapCoordinates_t;

/**
 * @brief A script argument, some object that should be spawned
 */
typedef struct
{
    rayMapCellType_t type;   ///< The type of object to spawn
    rayMapCoordinates_t pos; ///< Where to spawn the object
    uint8_t id;              ///< The ID of the spawned object
} raySpawn_t;

/**
 * @brief A script. This has some condition that triggers it (if) and some event
 * that occurs when it is triggered (then). The trigger and event both have
 * arguments stored in a union
 */
typedef struct
{
    bool isActive;         ///< true if the script is active, false if it is not
    int32_t resetTimerSec; ///< Timer to not re-trigger the script immediately
    ifOp_t ifOp;           ///< The type of condition that triggers the script
    /// A union of arguments for the condition that triggers the script
    union
    {
        uint32_t time; ///< A time in ms
        /// A struct of arguments which is a list of IDs
        struct
        {
            andOr_t andOr;      ///< Whether or not the IDs in the list should be AND'd or OR'd
            order_t order;      ///< Whether or not the order of IDs in the list matters
            repeat_t oneTime;   ///< Whether or not the script triggers once or repeatedly
            uint8_t numIds;     ///< The number of IDs in the list
            uint8_t* ids;       ///< A list of IDs
            bool* idsTriggered; ///< A list of triggered IDs
        } idList;
        /// A struct of arguments which is a list of map cells
        struct
        {
            andOr_t andOr;              ///< Whether or not the cells in the list should be AND'd or OR'd
            order_t order;              ///< Whether or not the order of cells in the list matters
            repeat_t oneTime;           ///< Whether or not the script triggers once or repeatedly
            uint8_t numCells;           ///< The number of cells in the list
            rayMapCoordinates_t* cells; ///< A list of cells
            bool* cellsTriggered;       ///< A list of triggered cells
        } cellList;
    } ifArgs;

    thenOp_t thenOp; ///< The type of event that happens
    /// A union of arguments for the event that the script triggers
    union
    {
        /// A struct of arguments which is a list of cells
        struct
        {
            uint8_t numCells;           ///< The number of cells in the list
            rayMapCoordinates_t* cells; ///< A list of cells
        } cellList;

        /// A struct of arguments which is a list of spawns
        struct
        {
            uint8_t numSpawns;  ///< The number of spawns in the list
            raySpawn_t* spawns; ///< A list of spawns
        } spawnList;

        /// A struct of arguments which is a list of IDs
        struct
        {
            uint8_t numIds; ///< The number of IDs in the list
            uint8_t* ids;   ///< A list of IDs
        } idList;

        char* text; ///< Text to be displayed

        /// A struct of arguments which is a map and cell destination to warp to
        struct
        {
            uint8_t mapId;           ///< The map ID to warp to
            rayMapCoordinates_t pos; ///< The position to warp to
        } warpDest;

        /// A single cell
        rayMapCoordinates_t cell;

        struct
        {
            uint8_t cost;         ///< The cost of the object
            rayMapCellType_t obj; ///< The object to buy
        } shop;

    } thenArgs;

} rayScript_t;

/**
 * @brief A single map cell
 */
typedef struct
{
    uint16_t closeTimer;     ///< Timer to close the door after opening
    q8_8 doorOpen;           ///< A timer for this cell, if it happens to be a door
    rayMapCellType_t type;   ///< The type of this cell
    int8_t openingDirection; ///< If the door is opening or closing
} rayMapCell_t;

/**
 * @brief An entire map
 */
typedef struct
{
    uint32_t w;                   ///< The width of the map
    uint32_t h;                   ///< The height of the map
    rayMapCell_t** tiles;         ///< A 2D array of tiles in the map
    rayTileState_t* visitedTiles; ///< A 1D array of all the visited tiles in the map, row-order
    list_t crackedWalls;          ///< A list of cracked walls to check bombs against
} rayMap_t;

/**
 * @brief A texture with a name
 */
typedef struct
{
    cnfsFileIdx_t fIdx; ///< The file index of the texture
    wsg_t texture;      ///< An image used as a texture
} loadedTexture_t;

/**
 * @brief Common data for all objects in a map
 */
typedef struct
{
    wsg_t* sprite; ///< The current sprite for this object
    q24_8 posX;    ///< The X position of this object
    q24_8 posY;    ///< The Y position of this object
    union
    {
        q24_8 radius; ///< The radius of this object
        struct
        {
            q24_8 w; ///< Bounding box width
            q24_8 h; ///< Bounding box height (must be negative when using radius instead of box)
        } box;
    } bound;
    rayMapCellType_t type;  ///< The object's type
    int32_t id;             ///< This object's ID
    bool spriteMirrored;    ///< Whether or not the sprite should be drawn mirrored
    int16_t spriteRotation; ///< Degrees rotation
} rayObjCommon_t;

/**
 * @brief Data for a bullet in the map. It has common data and velocity
 */
typedef struct
{
    rayObjCommon_t c; ///< Common object properties
    q24_8 velX;       ///< The X velocity of this bullet
    q24_8 velY;       ///< The Y velocity of this bullet
    q24_8 accX;       ///< The X acceleration of this bullet
    q24_8 accY;       ///< The Y acceleration of this bullet
    int32_t fuseUs;   ///< The fuse timer for a bomb
    bool returnToPlayer;
    int32_t animTimer;
} rayBullet_t;

/*
// Forward declaration
struct Node;

// You can now use pointers to struct Node
void processNode(struct Node* node);

// Full definition later in the file or a separate source file
struct Node {
    int id;
    struct Node* next; // Common use: self-referential structures
};
*/

struct rayGame;
struct rayEnemy;

typedef bool (*rayEnemyMain_t)(struct rayGame* ray, struct rayEnemy* enemy, uint32_t elapsedUs);
typedef void (*rayEnemyCheckCollision_t)(struct rayGame* ray, struct rayEnemy* enemy, rectangle_t player, q24_8* deltaX,
                                         q24_8* deltaY);
typedef void (*rayEnemyGetShot_t)(struct rayGame* ray, struct rayEnemy* enemy, rayMapCellType_t bullet);
typedef rayMapCellType_t (*rayEnemyDropAfterDeath_t)(void);

/**
 * @brief Data for an enemy in the map. It has common data, state tracking, and textures
 */
typedef struct rayEnemy
{
    rayObjCommon_t c; ///< Common object properties
    int32_t health;   ///< The enemy's health
    rayEnemyMain_t mainFn;
    rayEnemyCheckCollision_t collisionFn;
    rayEnemyGetShot_t getShotFn;
    rayEnemyDropAfterDeath_t dropAfterDeathFn;
    void* state; ///< Enemy specific state
} rayEnemy_t;

/**
 * @brief The player's inventory
 */
typedef struct
{
    // Persistent keys (TODO)
    rayKeyState_t keys[NUM_MAPS][NUM_KEYS]; ///< The number of small keys the player currently has
    // Persistent inventory items
    bool haveEwiOfTime;
    bool haveBombs;
    bool haveJumpBoots;
    bool haveShield;
    bool haveBow;
    bool haveBoomerang;
    bool haveTurntables;
    bool haveDoriasLullaby;
    // Persistent pick-ups (TODO heart pieces, ammo upgrades, etc.)
} rayInventory_t;

/**
 * @brief A struct with all the player information saved to NVM
 */
typedef struct
{
    q24_8 posX;                 ///< The player's X position
    q24_8 posY;                 ///< The player's Y position
    q24_8 dirX;                 ///< The player's X direction
    q24_8 dirY;                 ///< The player's Y direction
    int32_t mapId;              ///< The ID of the current map
    bool mapsVisited[NUM_MAPS]; ///< Booleans for each map visited
    // Current status
    int32_t health;    ///< The player's current health
    int32_t maxHealth; ///< The player's current max health.
    rayInventory_t i;  ///< All the players items
    uint32_t mpoints;  ///< Currency
} rayPlayer_t;

/**
 * @brief A struct with all the player information NOT saved to NVM
 */
typedef struct
{
    int32_t swordAngle;
    int32_t swordTimerUs;

    int32_t shieldZone;
    int32_t shieldTimerUs;
    bool shieldTouched;

    struct touchState
    {
        int32_t initialTouchPos;
        int32_t lastTouchPos;
        bool drawingBow;
        bool settingBomb;
    } ts;

    q24_8 jumpPos;
    q24_8 jumpVel;

    vec_t lastGoodCell;          ///< The last known good cell, used for moving after falling into a hole
    int32_t fallTimerUs;         ///< Timer for falling down a hole
    int32_t fallRotationTimerUs; ///< Timer for animating falling down a hole

    bool pbaDown;           ///< If PB_A is down, used to differentiate short and long presses
    uint32_t pbaDownTimeUs; ///< The time PB_A has been held down

    wsg_t* sprite;
} rayPlayerState_t;

typedef struct
{
    uint32_t noteHistory;       ///< Notes are 0-7 (3 bits) and invalid (4th bit) so this stores a history of 8 notes
    const char* lastPlayedSong; ///< Text indicating the last song played
    int8_t lTouchZone;          ///< The current left touch zone, or -1
    int8_t rTouchZone;          ///< The current right touch zone, or -1
    uint8_t notesActive;        ///< Bitmask of actively played notes
} rayInstrumentState_t;

/**
 * @brief The entire game state
 *
 */
typedef struct rayGame
{
    rayScreen_t screen;           ///< The current screen being shown
    rayPauseScreen_t pauseScreen; ///< The current pause screen being shown

    menu_t* menu;                   ///< The main menu
    menuZorldoRenderer_t* renderer; ///< Renderer for the menu
    bool wasReset;                  ///< Flag to return to the main menu after wiping NVM

    rayMap_t map;      ///< The loaded map
    int32_t doorTimer; ///< A timer used to open doors

    vec_t camera;        ///< The position of the 2D camera
    vec_t cameraTarget;  ///< The target position of the 2D camera
    int32_t cameraTimer; ///< A timer to move the camera from current to target positions

    rayPlayer_t p;       ///< All the player's state, loaded from NVM
    rayPlayerState_t ps; ///< All the player's temporary state

    int32_t warpDestMapId; ///< The ID of the current map
    q24_8 warpDestPosX;    ///< The player's X position
    q24_8 warpDestPosY;    ///< The player's Y position
    int32_t warpTimerUs;   ///< Timer to display warp screen

    bool shouldShowCredits; ///< Set to true to show credits from the game

    rayBullet_t bullets[MAX_RAY_BULLETS]; ///< A list of all bullets
    list_t enemies;                       ///< A list of all enemies (moves, can be shot)
    list_t scenery;                       ///< A list of all scenery (doesn't move, can be shot)
    list_t items;                         ///< A list of all items (doesn't move, can be shot)

    uint32_t btnState; ///< The current button state

    list_t loadedTextures;    ///< A list of all currently loaded textures
    wsg_t* typeToTexMap[256]; ///< A map of rayMapCellType_t to texture

    font_t ibm;     ///< A font to draw the HUD
    font_t logbook; ///< A font to draw the menu

    const char* dialogText;     ///< A pointer to the current dialog text
    const char* nextDialogText; ///< A pointer to the next dialog text, if it doesn't fit in one box
    wsg_t* dialogPortrait;      ///< A portrait to draw above the dialog text

    int32_t btnLockoutUs; ///< A timer to block buttons when first showing a dialog
    int32_t blinkTimer;   ///< A timer to blink things on the pause menu
    bool blink;           ///< Boolean for two draw states on the pause menu

    list_t scripts[NUM_IF_OP_TYPES]; ///< An array of lists of scripts
    uint32_t scriptTimer;            ///< A microsecond timer to check for time based scripts
    uint32_t secondsSinceStart;      ///< The number of seconds since this map was loaded

    starfield_t starfield; ///< Starfield used for warp animation

    midiFile_t songs[NUM_MAPS + 1]; ///< Per-map background music, plus a boss theme
    midiFile_t sfx_door_open;       ///< SFX when a door opens
    midiFile_t sfx_e_damage;        ///< SFX when an enemy takes damage
    midiFile_t sfx_e_freeze;        ///< SFX when an enemy is frozen
    midiFile_t sfx_p_charge;        ///< SFX when the charge beam is shot
    midiFile_t sfx_p_damage;        ///< SFX when the player takes damage
    midiFile_t sfx_p_shoot;         ///< SFX when the a normal beam is shot
    midiFile_t sfx_e_block;         ///< SFX when an enemy blocks a shot
    midiFile_t sfx_e_dead;          ///< SFX when an enemy dies
    midiFile_t sfx_item_get;        ///< SFX when an item is obtained
    midiFile_t sfx_p_charge_start;  ///< SFX when the charge beam starts to charge
    midiFile_t sfx_p_missile;       ///< SFX when a missile is shot
    midiFile_t sfx_p_ice;           ///< SFX when the ice beam is shot
    midiFile_t sfx_p_xray;          ///< SFX when th xray beam is shot
    midiFile_t sfx_warp;            ///< SFX when the player warps
    midiFile_t sfx_lava_dmg;        ///< SFX when standing in lava
    midiFile_t sfx_health;          ///< SFX when picking up health
    midiFile_t sfx_game_over;       ///< SFX when the game is over

    int32_t ledTimer;     ///< A timer to change LED hue
    int32_t ledHue;       ///< The current LED hue
    int32_t targetLedHue; ///< The target LED hue

    credits_t credits; ///< Credits shown when the game is won

    const char* deathText; ///< Text shown on the death screen

    rayInstrumentState_t is; // The instrument state

    synthOscillator_t lOsc;            ///< An oscillator for the left touchpad
    synthOscillator_t rOsc;            ///< An oscillator for the right touchpad
    synthOscillator_t* oscillators[2]; ///< Access to both oscillators
} ray_t;

//==============================================================================
// Extern variables
//==============================================================================

extern swadgeMode_t rayMode;

extern const char rayName[];
extern const char* const rayMapNames[];
extern const paletteColor_t rayMapColors[];
extern const char RAY_NVS_KEY[];
extern const char* const RAY_NVS_VISITED_KEYS[];
extern const char MAGTROID_UNLOCK_KEY[];

//==============================================================================
// Functions
//==============================================================================

void rayFreeCurrentState(ray_t* ray);
void rayStartGame(void);
void raySwitchToScreen(rayScreen_t newScreen);

#endif
