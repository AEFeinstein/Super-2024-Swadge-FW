#pragma once

//==============================================================================
// Includes
//==============================================================================

#include "artillery_types.h"

#include "quaternions.h"

//==============================================================================
// Defines
//==============================================================================

#define MAX_SHOT_POWER 400

#define LED_EXPLOSION_US 500000

//==============================================================================
// Function Declarations
//==============================================================================

physSim_t* initPhys(float w, float h, int32_t groundLevel, float gx, float gy, bool generateTerrain);
void deinitPhys(physSim_t* phys);

void physAddWorldBounds(physSim_t* phys);
void physRemoveAllObjects(physSim_t* phys);

void drawPhysBackground(physSim_t* phys, int16_t x, int16_t y, int16_t w, int16_t h);
void drawPhysOutline(physSim_t* phys, physCirc_t** players, font_t* font, font_t* fontOutline, int32_t turn);

void physStepBackground(physSim_t* phys);
void physStep(physSim_t* phys, int32_t elapsedUs, bool menuShowing, bool* playerMoved, bool* cameraMoved);

void physSpawnPlayers(physSim_t* phys, int32_t numPlayers, physCirc_t* players[], paletteColor_t* colors);
physCirc_t* physAddPlayer(physSim_t* phys, vecFl_t pos, int16_t barrelAngle, paletteColor_t baseColor,
                          paletteColor_t accentColor);

void setBarrelAngle(physCirc_t* circ, int16_t angle);
void setShotPower(physCirc_t* circ, float power);
void fireShot(physSim_t* phys, physCirc_t* player, physCirc_t* opponent, bool firstShot);

void adjustCpuShot(physSim_t* ad, physCirc_t* cpu, physCirc_t* target, artilleryCpuDifficulty_t difficulty);

const artilleryAmmoAttrib_t* getAmmoAttributes(uint16_t* numAttributes);
const artilleryAmmoAttrib_t* getAmmoAttribute(uint16_t idx);
