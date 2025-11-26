#pragma once

#include "swadge2024.h"
<<<<<<< HEAD
=======
#include "artillery_colors.h"
>>>>>>> origin/main
#include "artillery_phys.h"
#include "menuSimpleRenderer.h"

//==============================================================================
// Defines
//==============================================================================

#ifndef M_PIf
    #define M_PIf ((float)M_PI)
#endif

#define NUM_PLAYERS 2

#define TANK_MOVE_TIME_US 3000000

<<<<<<< HEAD
#define PHYS_FPS          60
=======
#define PHYS_FPS          40
>>>>>>> origin/main
#define PHYS_TIME_STEP_US (1000000 / PHYS_FPS)
#define PHYS_TIME_STEP_S  (1 / (float)PHYS_FPS)

#define LAVA_ANIM_PERIOD 500000
#define LAVA_ANIM_BLINKS 3

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
<<<<<<< HEAD
=======
    AGS_TOUR,
>>>>>>> origin/main
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

<<<<<<< HEAD
=======
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

>>>>>>> origin/main
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

    // The mode state (i.e. main menu, connecting, game)
    artilleryModeState_t mState;

    // In-game menu and renderer
    menu_t* modeMenu;
    menu_t* blankMenu;
    menuMegaRenderer_t* mRenderer;
<<<<<<< HEAD
    font_t* scoreFont;
=======
>>>>>>> origin/main

    // The game state (i.e. moving, adjusting shot, etc.)
    artilleryGameState_t gState;

    // In-game menu and renderer
    menu_t* gameMenu;
    menuSimpleRenderer_t* smRenderer;

<<<<<<< HEAD
=======
    // Help pages
    helpPageVars_t* help;
    helpPage_t* helpPages;

>>>>>>> origin/main
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
    bool p2pSetColorReceived;
    bool p2pSetWorldReceived;
    bool p2pAddTerrainReceived;
    bool p2pCloudsReceived;

    // Variables for tank colors
    int32_t paintArrowBlinkTimer;
    int32_t myColorIdx;
    int32_t theirColorIdx;

    // Variables for game over
    artilleryGameOverData_t gameOverData[NUM_PLAYERS];
<<<<<<< HEAD
} artilleryData_t;

extern const char load_ammo[];
extern swadgeMode_t artilleryMode;

void setDriveInMenu(bool visible);
=======

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

extern const char artilleryModeName[];

extern const char str_passAndPlay[];
extern const char str_wirelessConnect[];
extern const char str_cpuPractice[];
extern const char str_paintSelect[];
extern const char str_help[];
extern const char str_exit[];

extern const char str_load_ammo[];
extern const char str_drive[];
extern const char str_look_around[];
extern const char str_adjust[];
extern const char str_fire[];

extern swadgeMode_t artilleryMode;

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
extern const trophyData_t artilleryTrophies[];

void setDriveInMenu(bool visible);
void setAmmoInMenu(void);
void openAmmoMenu(void);
>>>>>>> origin/main
artilleryData_t* getArtilleryData(void);
void artilleryInitGame(artilleryGameType_t gameType, bool generateTerrain);
