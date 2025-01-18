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

typedef void (*bb_callbackFunction_t)(bb_entity_t* self);

typedef enum
{
    CRUMBLE_ANIM,       // A particle effect where garbage crumbles
    BUMP_ANIM,          // A particle effect where you bump things but they don't crumble
    ROCKET_ANIM,        // A vehicle to enter and depart the level
    FLAME_ANIM,         // A particle effect at the bottom of the rocket
    GARBOTNIK_FLYING,   // Frames of garbotnik flying
    HARPOON,            // A harpoon you throw
    EGG_LEAVES,         // They receive light and stimulate the egg
    EGG,                // When stimulated enough, they hatch into a bug!
    BU,                 // A bug 8
    BUG,                // A bug 9
    BUGG,               // A bug 10
    BUGGO,              // A bug 11
    BUGGY,              // A bug 12
    BUTT,               // A bug 13
    BB_MENU,            // Various sprites loaded as frames to create the parallax main menu
    BB_DEATH_DUMPSTER,  // Garbotnik's evil lair in space
    ATTACHMENT_ARM,     // The booster arm that detects the player and latches on.
    BB_WASHING_MACHINE, // It's heavy, and it has gravity
    BB_CAR,
    BB_SKELETON,    // Some dino bones embedded in garbage. Drops fuel.
    BB_FUEL,        // A zero-g glob of relish or crude dinosaur juice.
    BB_GRABBY_HAND, // A hand in the booster that grabs bugs.
    BB_DOOR,
    BB_DONUT,             // May be towed back to the booster to purchase WILES between dives.
    BB_SWADGE,            // Gives an immediate upgrade choice to the player when touched.
    BB_PANGO_AND_FRIENDS, // A sprite that flies up next to the rocket for a dialogue moment.
    BB_FOOD_CART, // Must be bumped 10 times to release a donut or a swadge. Also spawns a horde of bugs that may be
                  // ignored.
    BB_ARROW,     // Arrows for dialing in wiles.
    BB_WILE,
    BB_501KG,       // A huge bomb
    BB_DRILL_BOT,   // A driller that comes down then drills horizontally.
    BB_AMMO_SUPPLY, // A barrel of harpoons
    BB_PACIFIER,    // Calms down the bugs in range.
    BB_HOTDOG,      // icon for use in the radar screen.

    // These things do not have sprites in the sprites array. But we need the enum for the sake of the entity.
    // Some may have wsgs, but they cleverly load and unload their own WSGs.
    NO_SPRITE_STAR,          // a single white pixel for aesthetic beauty.
    NO_SPRITE_POI,           // Point of interest for the camera to follow in various cutscenes
    OVO_TALK,                // Need this enum actually for the entity but not the sprites array.
    BB_GAME_OVER,            // A screen after you fail.
    BB_RADAR_PING,           // Expanding circles when you press pause.
    BB_JANKY_BUG_DIG,        // used in the car fights. When bugs touch this the dirt "digs" toward the car fight arena.
    BB_SPIT,                 // projectile from the bug. Reuses fuel sprite with a palette swap.
    BB_DIVE_SUMMARY,         // A notepad that shows your dive stats.
    BB_EXPLOSION,            // A particle effect that deletes dirt, kills bugs, hurts and pushes garbotnik.
    BB_ATMOSPHERIC_ATOMIZER, // A particle effect that increases garbotnik's dragShift.
    BB_SPACE_LASER,          // A beam of pure damage straight down from the sky.
    BB_BRICK_TUTORIAL,       // A particular spot on levels 3 & 4 that makes garbotnik talk about breaking bricks.
    BB_GARBOTNIK_UI,         // The UI that shows the harpooon count and wile calldowns & cooldowns.
    BB_QUICKPLAY_CONFIRM,    // A screen that asks if you want to play quickplay.
} bb_spriteDef_t;

typedef enum
{ // The kinds of stuff that may be embedded into a garbage tile. Heck, now they can be in air too (car & washing
  // machine)
    NOTHING_EMBED,
    EGG_EMBED,
    WASHING_MACHINE_EMBED,
    BB_CAR_WITH_DONUT_EMBED,
    BB_CAR_WITH_SWADGE_EMBED,
    SKELETON_EMBED,
    DOOR_EMBED,
    BB_FOOD_CART_WITH_DONUT_EMBED,
    BB_FOOD_CART_WITH_SWADGE_EMBED,
    BRICK_TUTORIAL_EMBED,
} bb_embeddable_t;

typedef enum
{
    ONESHOT_ANIMATION,
    LOOPING_ANIMATION,
    NO_ANIMATION,
} bb_animationType_t;

void bb_trigger501kg(bb_entity_t* self);
void bb_triggerFaultyWile(bb_entity_t* self);
void bb_triggerAtmosphericAtomizerWile(bb_entity_t* self);
void bb_triggerDrillBotWile(bb_entity_t* self);
void bb_triggerAmmoSupplyWile(bb_entity_t* self);
void bb_triggerPacifierWile(bb_entity_t* self);
void bb_triggerSpaceLaserWile(bb_entity_t* self);

#endif