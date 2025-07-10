#pragma once

//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include "hdw-btn.h"
#include "linked_list.h"
#include "vector2d.h"
#include "geometryFl.h"

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
    // Position
    int32_t zonemask;
    circleFl_t c;
    bool fixed;

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

    // Button input
    buttonBit_t moving; ///< Either the left, right, or no button held

    // Game data
    circType_t type;

    // Player data
    float barrelAngle;
    vecFl_t relBarrelTip;
    float shotPower;
    artilleryAmmoType_t ammo;

    // Shell data
    int32_t bounces;
} physCirc_t;

typedef struct
{
    int32_t zonemask;
    lineFl_t l;
    vecFl_t unitNormal;
} physLine_t;

typedef struct
{
    vecFl_t g;
    vecFl_t bounds;
    rectangleFl_t zones[NUM_ZONES];
    list_t lines;
    list_t circles;

    uint32_t cameraTimer;
    buttonBit_t cameraBtn;
    vec_t camera;
    physCirc_t* cameraTarget;

    bool shotFired;
    bool shotDone;
} physSim_t;

//==============================================================================
// Function Declarations
//==============================================================================

physSim_t* initPhys(float w, float h, float gx, float gy);
void deinitPhys(physSim_t* phys);
void drawPhysOutline(physSim_t* phys);
void physStep(physSim_t* phys, int32_t elapsedUs);
void physSetCameraButton(physSim_t* phys, buttonBit_t btn);
void physAdjustCamera(physSim_t* phys, uint32_t elapsedUs);

physLine_t* physAddLine(physSim_t* phys, float x1, float y1, float x2, float y2);
physCirc_t* physAddCircle(physSim_t* phys, float x1, float y1, float r, circType_t type);
void setBarrelAngle(physCirc_t* circ, float angle);
void setShotPower(physCirc_t* circ, float power);
void fireShot(physSim_t* phys, physCirc_t* circ);
