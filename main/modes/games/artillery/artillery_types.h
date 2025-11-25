#pragma once

//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>

#include "hdw-btn.h"
#include "hdw-tft.h"
#include "hdw-led.h"

#include "palette.h"
#include "artillery_colors.h"

#include "geometry.h"
#include "geometryFl.h"
#include "vector2d.h"
#include "vectorFl2d.h"

#include "menu.h"
#include "menuMegaRenderer.h"
#include "menuSimpleRenderer.h"
#include "helpPages.h"

#include "linked_list.h"

#include "midiFileParser.h"

#include "p2pConnection.h"

//==============================================================================
// Defines
//==============================================================================

#define NUM_PLAYERS 2

// There are 32 zones, one for each bit in a int32_t
#define NUM_ZONES 32

#define CLOUD_ROWS 2
#define CLOUD_COLS 6

#define CIRC_PER_CLOUD 3
#define NUM_CLOUDS     (CLOUD_ROWS * CLOUD_COLS)

//==============================================================================
// Physics Enums
//==============================================================================

typedef enum
{
    CT_TANK,
    CT_SHELL,
    CT_OBSTACLE,
} circType_t;

typedef enum __attribute__((packed))
{
    NO_EFFECT,
    WALL_MAKER,
    HOMING_MISSILE,
    FLOOR_LAVA,
    CONFUSION,
    LASER,
    SNIPER,
    ROCKET_JUMP,
} ammoEffect_t;

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    AMS_MENU,
    AMS_CONNECTING,
    AMS_GAME,
    AMS_HELP,
    AMS_PAINT,
    AMS_GAME_OVER,
} artilleryModeState_t;

typedef enum
{
    AGS_TOUR,
    AGS_WAIT,
    AGS_MENU,
    AGS_MOVE,
    AGS_ADJUST,
    AGS_FIRE,
    AGS_LOOK,
    AGS_CPU_MOVE,
    AGS_CPU_ADJUST,
} artilleryGameState_t;

typedef enum
{
    AG_PASS_AND_PLAY,
    AG_WIRELESS,
    AG_CPU_PRACTICE,
} artilleryGameType_t;

typedef enum
{
    EYES_CC,
    EYES_UL,
    EYES_UC,
    EYES_UR,
    EYES_CR,
    EYES_DR,
    EYES_DC,
    EYES_DL,
    EYES_CL,
    EYES_DEAD,
} artilleryEye_t;

typedef enum __attribute__((packed))
{
    P2P_SET_COLOR,
    P2P_SET_WORLD,
    P2P_ADD_TERRAIN,
    P2P_SET_CLOUDS,
    P2P_FINISH_TOUR,
    P2P_SET_STATE,
    P2P_FIRE_SHOT,
} artilleryP2pPacketType_t;

typedef enum
{
    CPU_EASY,
    CPU_MEDIUM,
    CPU_HARD,
} artilleryCpuDifficulty_t;

// WARNING! Must match the order of trophies in artilleryTrophies[]
typedef enum
{
    AT_ROYAL_SAMPLER,
    AT_HITTING_YOURSELF,
    AT_TO_THE_MOON,
    AT_PASS_AND_PLAY,
    AT_P2P,
    AT_SKYNET,
    AT_SNIPER,
} artilleryTrophyType;

//==============================================================================
// Physics Structs
//==============================================================================

typedef struct
{
    float x0;
    float y0;
    float x1;
    float y1;
} aabb_t;

typedef struct
{
    paletteColor_t color;
    uint8_t radius;
    uint8_t numBounces;
    uint8_t numSpread;
    uint8_t numConsec;
    uint16_t score;
    float expVel;
    uint8_t expRadius;
    ammoEffect_t effect;
    const char* name;
    const char* help;
} artilleryAmmoAttrib_t;

struct _physCirc_t;

typedef struct _physCirc_t
{
    // Position
    int32_t zonemask;
    circleFl_t c;
    bool fixed;
    aabb_t aabb;

    // Velocity
    vecFl_t vel;           ///< Velocity of the object, computed frame-by-frame
    lineFl_t travelLine;   ///< A line from the object's start to end point for this frame
    lineFl_t travelLineBB; ///< The bounding box for travelLine

    // Acceleration
    vecFl_t g; ///< Per-object gravity, shouldn't change
    /**
     * Static force exerted on this object by other objects, computed frame-by-frame.
     * This should be used instead of world gravity if physCirc_t.inContact is set.
     */
    vecFl_t staticForce;
    bool inContact; ///< true of staticForce is computed, false if it isn't
    vecFl_t slopeVec;
    vecFl_t contactNorm;
    vecFl_t lastContactNorm;

    // Common characteristics
    float bounciness; ///< Bounciness, from 0 (no bounce) to 1 (no velocity lost on bounce)
    paletteColor_t baseColor;
    paletteColor_t accentColor;

    // Game data
    circType_t type;

    // Player data
    buttonBit_t moving; ///< Either the left, right, or no button held
    int16_t barrelAngle;
    int16_t targetBarrelAngle;
    vecFl_t relBarrelTip;
    float shotPower;
    int16_t ammoIdx;
    int32_t score;
    int8_t shotsRemaining;
    int32_t shotTimer;
    int32_t lavaAnimTimer;
    list_t availableAmmo;

    // Shell data
    int32_t bounces;
    int32_t explosionRadius;
    float explosionVel;
    ammoEffect_t effect;
    struct _physCirc_t* owner;
    struct _physCirc_t* homingTarget;
} physCirc_t;

typedef struct
{
    uint32_t zonemask;
    lineFl_t l;
    vecFl_t unitNormal;
    bool isTerrain;
    lineFl_t destination;
    aabb_t aabb;
    bool isLava;
} physLine_t;

typedef struct
{
    circleFl_t circ;
    paletteColor_t color;
    int32_t expTimeUs;
    int32_t ttlUs;
} explosionAnim_t;

typedef struct
{
    bool isReady;
    bool shouldStepForeground; ///< This is set when the screen is drawn, every PHYS_TIME_STEP_US

    vecFl_t g;
    vecFl_t bounds;
    rectangleFl_t zones[NUM_ZONES];
    list_t lines;
    list_t circles;
    list_t explosions;
    circle_t clouds[NUM_CLOUDS * CIRC_PER_CLOUD];

    uint32_t cameraTimer;
    buttonBit_t cameraBtn;
    vec_t camera;
    list_t cameraTargets;
    list_t cameraTour;

    bool shotFired;
    bool terrainMoving;
    int32_t playerSwapTimerUs;

    int16_t surfacePoints[TFT_WIDTH];
    paletteColor_t surfaceColors[TFT_WIDTH];

    led_t ledColor;
    int32_t ledTimer;
} physSim_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    int32_t score;
    paletteColor_t baseColor;
    paletteColor_t accentColor;
    bool isPlayer;
} artilleryGameOverData_t;

typedef struct
{
    artilleryGameType_t gameType;

    // The physics simulation
    physSim_t* phys;

    // The players, pointers to objects in the simulation
    physCirc_t* players[NUM_PLAYERS];
    int32_t plIdx;
    int32_t cpuWaitTimer;
    int32_t turn;
    artilleryCpuDifficulty_t cpu;

    // The mode state (i.e. main menu, connecting, game)
    artilleryModeState_t mState;

    // In-game menu and renderer
    menu_t* modeMenu;
    menu_t* blankMenu;
    menuMegaRenderer_t* mRenderer;

    // The game state (i.e. moving, adjusting shot, etc.)
    artilleryGameState_t gState;

    // In-game menu and renderer
    menu_t* gameMenu;
    menuSimpleRenderer_t* smRenderer;

    // Help pages
    helpPageVars_t* help;
    helpPage_t* helpPages;

    // Timer to only allow a little bit of movement
    int32_t moveTimerUs;

    // Variables to handle button input when adjusting shots
    buttonBit_t adjButtonHeld;
    uint32_t adjButtonStartTimer;
    uint32_t adjButtonHeldTimer;
    int32_t tpLastPhi;
    int32_t tpCumulativeDiff;

    // Everything required for wireless communication
    p2pInfo p2p;
    const char* conStr;
    list_t p2pQueue;
    artilleryP2pPacketType_t expectedPacket;
    artilleryP2pPacketType_t lastTxType;

    // Variables for tank colors
    int32_t paintArrowBlinkTimer;
    int32_t myColorIdx;
    int32_t theirColorIdx;

    // Variables for game over
    artilleryGameOverData_t gameOverData[NUM_PLAYERS];

    // Audio
    midiFile_t bgms[4];
    uint32_t bgmIdx;

    // Eye LEDs
    artilleryEye_t eyeSlot;
    int32_t deadEyeTimer;

    // Fonts
    font_t font_oxanium;
    font_t font_oxaniumOutline;
    font_t font_pulseAux;
    font_t font_pulseAuxOutline;
} artilleryData_t;
