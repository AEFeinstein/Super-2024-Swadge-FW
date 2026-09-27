#pragma once

//==============================================================================
// Includes
//==============================================================================

#include <inttypes.h>
#include <stdbool.h>

#include "font.h"

//==============================================================================
// Defines
//==============================================================================

#define FC_MAX_NUM_FAIRIES 64

#define FC_NUM_YEAR_OPTIONS 16

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    FC_NAMESPACE,
    FC_USER_FAIRY,
    FC_USER_CARD,
    FC_SPP_SAVED,
    FC_SPP_NEXT_IDX,
} fcNvsKeys_t;

typedef enum
{
    FC_OPT_YEAR,
    FC_OPT_ATTENDEE,
    FC_OPT_CARD,
    FC_OPT_TEAM,
    FC_OPT_SHAPE,
    FC_OPT_CHARM,
    FC_OPT_FILLING,
    FC_OPT_PEDESTAL,
    FC_OPT_COLOR,
    FC_OPT_WING,
    FC_OPT_BALL,
    FC_OPT_AURA,
    FC_OPT_COUNT
} fcOptionTypes_t;

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
    FC_AT_COUNT
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
    FC_CARD_COUNT
} fcProfileCards_t;

typedef enum
{
    FC_BLUE,
    FC_RED,
    FC_YELLOW,
    FC_NONE // Count + unset
} fcTeams_t;

typedef enum
{
    FC_SHAPE_CYL_LRG,
    FC_SHAPE_CYL_LRG_HANDLE,
    FC_SHAPE_CYL_SML,
    FC_SHAPE_RND_SML,
    FC_SHAPE_ERLENMEYER,
    FC_SHAPE_HEART,
    FC_SHAPE_UNUSED_1,
    FC_SHAPE_UNUSED_2,
    FC_SHAPE_COUNT
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
    FC_CHARM_COUNT
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
    FC_FILL_COUNT
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
    FC_PED_COUNT
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
    FC_COL_COUNT
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
    FC_WING_COUNT
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
    FC_BALL_COUNT
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
    FC_AURA_COUNT
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
} fairy_t;

typedef struct __attribute__((packed))
{
    int32_t sinceLastSeen;
    uint8_t years        : 4; // 1-15, 15+
    uint8_t attendeeType : 4;
    uint8_t cardBG       : 4;
    uint8_t team         : 2;
    bool initialized   : 1; // If fairy is initialized
} profileCard_t;

typedef struct __attribute__((packed))
{
    int32_t packedName;
    profileCard_t pCard;
    fairy_t fairy;
} savedProfile_t;

//==============================================================================
// Consts
//==============================================================================

extern const int fairyMaxList[];
extern const char* const nvsStrs[];
extern const char* const fcOptionTypes[];
extern const char* const fcAttendeeText[];
extern const char* const fcCardText[];
extern const char* const fcTeamText[];
extern const char* const fcShapeText[];
extern const char* const fcCharmText[];
extern const char* const fcFillingText[];
extern const char* const fcPedestalText[];
extern const char* const fcColorsText[];
extern const char* const fcWingText[];
extern const char* const fcBallText[];
extern const char* const fcAuraText[];