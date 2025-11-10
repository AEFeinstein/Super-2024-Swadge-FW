#pragma once

//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include "hdw-btn.h"
#include "hdw-tft.h"
#include "hdw-led.h"
#include "palette.h"
#include "linked_list.h"
#include "vector2d.h"
#include "geometry.h"
#include "geometryFl.h"
#include "quaternions.h"
#include "font.h"

//==============================================================================
// Defines
//==============================================================================

// There are 32 zones, one for each bit in a int32_t
#define NUM_ZONES 32

#define MAX_SHOT_POWER 400

#define LED_EXPLOSION_US 500000

#define CLOUD_ROWS 2
#define CLOUD_COLS 6

#define CIRC_PER_CLOUD 3
#define NUM_CLOUDS     (CLOUD_ROWS * CLOUD_COLS)

//==============================================================================
// Enums
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
// Structs
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
// Function Declarations
//==============================================================================

physSim_t* initPhys(float w, float h, int32_t groundLevel, float gx, float gy, bool generateTerrain);
void deinitPhys(physSim_t* phys);

void physAddWorldBounds(physSim_t* phys);
void physRemoveAllObjects(physSim_t* phys);

void drawPhysBackground(physSim_t* phys, int16_t x, int16_t y, int16_t w, int16_t h);
void drawPhysOutline(physSim_t* phys, physCirc_t** players, font_t* font, int32_t turn);

void physStepBackground(physSim_t* phys);
void physStep(physSim_t* phys, int32_t elapsedUs, bool menuShowing, bool* playerMoved, bool* cameraMoved);

void physSpawnPlayers(physSim_t* phys, int32_t numPlayers, physCirc_t* players[], paletteColor_t* colors);
physCirc_t* physAddPlayer(physSim_t* phys, vecFl_t pos, int16_t barrelAngle, paletteColor_t baseColor,
                          paletteColor_t accentColor);

void setBarrelAngle(physCirc_t* circ, int16_t angle);
void setShotPower(physCirc_t* circ, float power);
void fireShot(physSim_t* phys, physCirc_t* player, physCirc_t* opponent, bool firstShot);

void adjustCpuShot(physSim_t* ad, physCirc_t* cpu, physCirc_t* target);

const artilleryAmmoAttrib_t* getAmmoAttributes(uint16_t* numAttributes);
const artilleryAmmoAttrib_t* getAmmoAttribute(uint16_t idx);
