#ifndef BIGBUG_TYPEDEF_INCLUDED
#define BIGBUG_TYPEDEF_INCLUDED

#define DECIMAL_BITS 4
#define FIELD_WIDTH   (TFT_WIDTH << DECIMAL_BITS)
#define FIELD_HEIGHT  (TFT_HEIGHT << DECIMAL_BITS)
#define HALF_WIDTH   (FIELD_WIDTH / 2)
#define HALF_HEIGHT  (FIELD_HEIGHT / 2)

typedef struct bb_t bb_t;
typedef struct bb_entityManager_t bb_entityManager_t;
typedef struct bb_tilemap_t bb_tilemap_t;
typedef struct bb_entity_t bb_entity_t;

typedef enum
{
    CRUMBLE_ANIMATION
} bb_spriteDef_t;

#endif