#pragma once

//==============================================================================
// Includes
//==============================================================================

#include <inttypes.h>

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    FC_AT_ATTENDEE,
    FC_AT_STAFF,
    FC_AT_VOLUNTEER,
    FC_AT_GUEST,
    FC_AT_PERFORMER,
    FC_AT_COSPLAYER,
    FC_AT_MAKER,
    FC_AT_PANELIST,
    FC_AT_PHOTOGRAPHER,
    FC_AT_GAMER,
    FC_AT_DEVELOPER,
    FC_AT_ARTIST,
    FC_AT_MUSICIAN,
    FC_AT_PARTIER,
    FC_AT_FURRY,
    FC_AT_PINBALL_WIZARD,
} fcAttendeeType_t;

typedef enum
{
    FC_CARD_SCROLLS, // Paper
    FC_CARD_PACMAN,
    FC_CARD_SHEET_MUSIC,
    FC_CARD_4, // FIXME
    FC_CARD_5, // FIXME
    FC_CARD_6, // FIXME
    FC_CARD_7, // FIXME
    FC_CARD_8, // FIXME
    FC_CARD_RED,
    FC_CARD_ORANGE,
    FC_CARD_YELLOW,
    FC_CARD_GREEN,
    FC_CARD_BLUE,
    FC_CARD_INDIGO,
    FC_CARD_VIOLET,
    FC_CARD_WHITE,
} fcProfileCards_t;

typedef enum
{
    FC_NONE,
    FC_BLUE,
    FC_RED,
    FC_YELLOW,
} fcTeams_t;

typedef enum
{
    FC_SHAPE_CYL_LRG,
    FC_SHAPE_CYL_LRG_HANDLE,
    FC_SHAPE_CYL_SML,
    FC_SHAPE_RND_SML,
    FC_SHAPE_ERLENMEYER,
    FC_SHAPE_HEART,
    FC_UNUSED_1,
    FC_UNUSED_2,
} fcShape_t;

typedef enum
{
    FC_CHARM_NONE,
    FC_CHARM_MUSIC_NOTE,
    FC_CHARM_HEART,
    FC_CHARM_SKULL,
    FC_CHARM_SMILEY,
    FC_CHARM_CONTROLLER,
    FC_CHARM_GUITER,
    FC_CHARM_GEM,
} fcCharm_t;

typedef enum
{
    FC_FILL_NONE,
    FC_FILL_CRYSTAL,
    FC_FILL_GRASS,
    FC_FILL_MARBLE,
    FC_FILL_DICE,
    FC_FILL_SEASHELLS,
    FC_FILL_PAINT,
    FC_FILL_TOY_CAR,
} fcFilling_t;

typedef enum
{
    FC_PED_NONE,
    FC_PED_WOOD,
    FC_PED_RUG,
    FC_PED_DOILY,
    FC_PED_NOTEBOOK,
    FC_PED_STONE,
    FC_PED_PLATE,
    FC_PED_MUSHROOM,
} fcPedestal_t;

typedef enum
{
    FC_COL_PINK,
    FC_COL_RED,
    FC_COL_BLUE,
    FC_COL_GREEN,
    FC_COL_YELLOW,
    FC_COL_PURPLE,
    FC_COL_GRAY,
    FC_COL_ORANGE,
} fcColor_t;

typedef enum
{
    FC_WING_NONE,
    FC_WING_NAVI,
    FC_WING_ROUND,
    FC_WING_TINY,
    FC_WING_SWIRLY,
    FC_WING_BAT,
    FC_WING_CURLY,
    FC_WING_MONARCH,
} fcWing_t;

typedef enum
{
    FC_BALL_CIRCLE,
    FC_BALL_OVAL,
    FC_BALL_TWO_SEG,
    FC_BALL_TOMI,
    FC_BALL_TEARDROP,
    FC_BALL_2, // FIXME
    FC_BALL_3, // FIXME
    FC_BALL_4, // FIXME
} fcBall_t;

typedef enum
{
    FC_AURA_NONE,
    FC_AURA_SPARKLE,
    FC_AURA_MUSIC_NOTES,
    FC_AURA_GLOW,
    FC_AURA_BUBBLE,
    FC_AURA_STINK,
    FC_AURA_1, // FIXME
    FC_AURA_2, // FIXME
} fcAura_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct __attribute__((packed))
{
    uint8_t shape    : 3;
    uint8_t charm    : 3;
    uint8_t filling  : 3;
    uint8_t pedestal : 3;
    uint8_t color    : 3;
    uint8_t wing     : 3;
    uint8_t ball     : 3;
    uint8_t aura     : 3;
} fcSPP_t;

typedef struct __attribute__((packed))
{
    int32_t sinceLastSeen;
    uint8_t years        : 4; // 1-15, 15+
    uint8_t attendeeType : 4;
    uint8_t cardBG       : 4;
    uint8_t team         : 2;
} profileCard_t;

typedef struct __attribute__((packed))
{
    int32_t packedName;
    profileCard_t pCard;
    fcSPP_t fairy;
} savedProfile_t;

/*
Trophies:
- OLDHEAD: Find someone with 15+ years
- Gets around: FInd one of each attendancer type
*/