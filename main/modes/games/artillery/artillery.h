#pragma once

#include "swadge2024.h"
#include "artillery_types.h"

//==============================================================================
// Defines
//==============================================================================

#ifndef M_PIf
    #define M_PIf ((float)M_PI)
#endif

#define TANK_MOVE_TIME_US 3000000

#define PHYS_FPS          40
#define PHYS_TIME_STEP_US (1000000 / PHYS_FPS)
#define PHYS_TIME_STEP_S  (1 / (float)PHYS_FPS)

#define LAVA_ANIM_PERIOD 500000
#define LAVA_ANIM_BLINKS 3

//==============================================================================
// Externs
//==============================================================================

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

extern const trophyData_t artilleryTrophies[];

//==============================================================================
// Functions
//==============================================================================

void setDriveInMenu(bool visible);
void setAmmoInMenu(void);
void openAmmoMenu(void);
artilleryData_t* getArtilleryData(void);
void artilleryInitGame(artilleryGameType_t gameType, bool generateTerrain);
