#ifndef BIGBUG_TYPEDEF_INCLUDED
#define BIGBUG_TYPEDEF_INCLUDED

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
typedef struct bb_gameData_t bb_gameData_t;

typedef enum
{
    CRUMBLE_ANIM,
    BUMP_ANIM,   // A particle effect where you bump things
    ROCKET_ANIM, // A vehicle to enter and depart the level
    FLAME_ANIM,  //A particle effect at the bottom of the rocket
    GARBOTNIK_FLYING, //Frames of garbotnik flying
    WILE,        // A metal ball that is thrown
    HARPOON,     // A harpoon you throw
    FLY,         // A bug that flies
    ANT,         // A bug that crawls on the midground tile field
    BEETLE,      // A bug that walks on the the foreground tile field
} bb_spriteDef_t;

typedef enum
{
    ONESHOT_ANIMATION,
    LOOPING_ANIMATION,
    NO_ANIMATION
} bb_animationType_t;

#endif