#pragma once

#define DECIMAL_BITS 4
#define BOARD_SIZE 5

typedef struct dn_entity_t dn_entity_t;

typedef enum
{
    ALPHA_ORTHO,       // Orthographic Alpha posed for match start.
    WHITE_CHESS_ORTHO, // Orthographic Chess King posed for match start.
    BLACK_CHESS_ORTHO, // Orthographic Chess King posed for match start.
} dn_spriteDef_t;

typedef enum
{
    ONESHOT_ANIMATION,
    LOOPING_ANIMATION,
    NO_ANIMATION,
} dn_animationType_t;