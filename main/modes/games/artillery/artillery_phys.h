#pragma once

//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include "hdw-btn.h"
#include "hdw-tft.h"
#include "palette.h"
#include "linked_list.h"
#include "vector2d.h"
#include "geometryFl.h"
#include "quaternions.h"
#include "font.h"

//==============================================================================
// Defines
//==============================================================================

// There are 32 zones, one for each bit in a int32_t
#define NUM_ZONES 32

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    CT_TANK,
    CT_SHELL,
    CT_OBSTACLE,
} circType_t;

typedef enum
{
    AMMO_NORMAL,
    AMMO_BIG_EXPLODE,
    AMMO_THREE,
    AMMO_FIVE,
    AMMO_SNIPER,
    AMMO_MACHINE_GUN,
    AMMO_BOUNCY,
    AMMO_JACKHAMMER,
    AMMO_HILL_MAKER,
    AMMO_JUMP,
} artilleryAmmoType_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    float x0;
    float y0;
    float x1;
    float y1;
} aabb_t;

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
    float barrelAngle;
    float targetBarrelAngle;
    vecFl_t relBarrelTip;
    float shotPower;
    artilleryAmmoType_t ammo;
    const char* ammoLabel;
    int32_t score;

    // Shell data
    int32_t bounces;
    int32_t explosionRadius;
    float explosionVel;
    struct _physCirc_t* owner;
} physCirc_t;

typedef struct
{
    int32_t zonemask;
    lineFl_t l;
    vecFl_t unitNormal;
    bool isTerrain;
    lineFl_t destination;
    aabb_t aabb;
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
    bool shouldStepForeground;

    vecFl_t g;
    vecFl_t bounds;
    rectangleFl_t zones[NUM_ZONES];
    list_t lines;
    list_t circles;
    list_t explosions;

    uint32_t cameraTimer;
    buttonBit_t cameraBtn;
    vec_t camera;
    list_t cameraTargets;

    bool shotFired;
    bool terrainMoving;
    int32_t playerSwapTimerUs;

    int16_t surfacePoints[TFT_WIDTH];
} physSim_t;

//==============================================================================
// Function Declarations
//==============================================================================

physSim_t* initPhys(float w, float h, int32_t groundLevel, float gx, float gy, bool generateTerrain);
void deinitPhys(physSim_t* phys);

void physAddWorldBounds(physSim_t* phys);
void physRemoveAllObjects(physSim_t* phys);

void drawPhysBackground(physSim_t* phys, int16_t x, int16_t y, int16_t w, int16_t h);
void drawPhysOutline(physSim_t* phys, physCirc_t** players, font_t* font, int32_t moveTimeLeftUs);

void physStepBackground(physSim_t* phys);
bool physStep(physSim_t* phys, int32_t elapsedUs);

void physSpawnPlayers(physSim_t* phys, int32_t numPlayers, physCirc_t* players[], paletteColor_t* colors);
physCirc_t* physAddPlayer(physSim_t* phys, vecFl_t pos, float barrelAngle, paletteColor_t baseColor,
                          paletteColor_t accentColor);

void setBarrelAngle(physCirc_t* circ, float angle);
void setShotPower(physCirc_t* circ, float power);
void fireShot(physSim_t* phys, physCirc_t* circ);

void adjustCpuShot(physSim_t* ad, physCirc_t* cpu, physCirc_t* target);
