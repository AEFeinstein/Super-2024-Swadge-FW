#ifndef BIGBUG_TYPEDEF_INCLUDED
#define BIGBUG_TYPEDEF_INCLUDED

#include <esp_heap_caps.h>

// #define TILE_SIZE 64
// #define BITSHIFT_TILE_SIZE 1024
// #define HALF_TILE 32
// #define BITSHIFT_HALF_TILE 512

#define TILE_SIZE          32
#define BITSHIFT_TILE_SIZE 512
#define HALF_TILE          16
#define BITSHIFT_HALF_TILE 256

#define DECIMAL_BITS 4
#define FIELD_WIDTH  (TFT_WIDTH << DECIMAL_BITS)
#define FIELD_HEIGHT (TFT_HEIGHT << DECIMAL_BITS)
#define HALF_WIDTH   (FIELD_WIDTH / 2)
#define HALF_HEIGHT  (FIELD_HEIGHT / 2)

typedef struct bb_t bb_t;
typedef struct bb_entity_t bb_entity_t;
typedef struct bb_tilemap_t bb_tilemap_t;
typedef struct bb_hitInfo_t bb_hitInfo_t;
typedef struct bb_camera_t bb_camera_t;
typedef struct bb_gameData_t bb_gameData_t;
typedef struct bb_midgroundTileInfo_t bb_midgroundTileInfo_t;
typedef struct bb_foregroundTileInfo_t bb_foregroundTileInfo_t;

typedef enum
{
    CRUMBLE_ANIM,      // A particle effect where garbage crumbles
    BUMP_ANIM,         // A particle effect where you bump things but they don't crumble
    ROCKET_ANIM,       // A vehicle to enter and depart the level
    FLAME_ANIM,        // A particle effect at the bottom of the rocket
    GARBOTNIK_FLYING,  // Frames of garbotnik flying
    HARPOON,           // A harpoon you throw
    EGG_LEAVES,        // They receive light and stimulate the egg
    EGG,               // When stimulated enough, they hatch into a bug!
    BU,                // A bug
    BUG,               // A bug
    BUGG,              // A bug
    BUGGO,             // A bug
    BUGGY,             // A bug
    BUTT,              // A bug
    BB_MENU,           // Various sprites loaded as frames to create the parallax main menu
    BB_DEATH_DUMPSTER, // Garbotnik's evil lair in space
    ATTACHMENT_ARM,    // The booster arm that detects the player and latches on.
    BB_WASHING_MACHINE,// It's heavy, and it has gravity
    BB_CAR_IDLE,       // A car with a nice reward inside.
    BB_CAR_ACTIVE,     // The car alarm going crazy drawing a horde encounter
    BB_SKELETON,       // Some dino bones embedded in garbage. Drops fuel.
    BB_FUEL,           // A zero-g glob of relish or crude dinosaur juice.

    // These things do not have sprites in the sprites array. But we need the enum for the sake of the entity.
    // Some may have wsgs, but they cleverly load and unload their own WSGs.
    NO_SPRITE_STAR, // a single white pixel for aesthetic beauty.
    NO_SPRITE_POI,  // Point of interest for the camera to follow in various cutscenes
    OVO_TALK,       // Need this enum actually for the entity but not the sprites array.
    BB_GAME_OVER,   // A screen after you fail.
    BB_RADAR_PING,  // Expanding circles when you press pause.
    WILE            // A metal ball that is thrown (not implemented)

} bb_spriteDef_t;

typedef enum
{ // The kinds of stuff that may be embedded into a garbage tile. Heck, now they can be in air too (car & washing machine)
    NOTHING_EMBED,
    EGG_EMBED,
    WASHING_MACHINE_EMBED,
    CAR_EMBED,
    SKELETON_EMBED
} bb_embeddable_t;

typedef enum
{
    ONESHOT_ANIMATION,
    LOOPING_ANIMATION,
    NO_ANIMATION
} bb_animationType_t;

#endif