#pragma once

//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include "hdw-btn.h"
#include "linked_list.h"
#include "geometryFl.h"

//==============================================================================
// Defines
//==============================================================================

// There are 32 zones split into an 8x4 grid
#define NUM_ZONES_BIG 8
#define NUM_ZONES_LIL 4
#define NUM_ZONES     (NUM_ZONES_BIG * NUM_ZONES_LIL)

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    CT_TANK,
    CT_SHELL,
    CT_OBSTACLE,
} circType_t;

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

    // Button input
    buttonBit_t moving; ///< Either the left, right, or no button held

    // Game data
    circType_t type;
    float barrelAngle;
    vecFl_t relBarrelTip;
    float shotPower;
} physCirc_t;

typedef struct
{
    int32_t zonemask;
    lineFl_t l;
    vecFl_t unitNormal;
    vecFl_t unitSlope;
} physLine_t;

typedef struct
{
    vecFl_t g;
    float gMag;
    vecFl_t bounds;
    rectangleFl_t zones[NUM_ZONES];
    list_t lines;
    list_t circles;
} physSim_t;

//==============================================================================
// Function Declarations
//==============================================================================

physSim_t* initPhys(float w, float h, float gx, float gy);
void deinitPhys(physSim_t* phys);
void drawPhysOutline(physSim_t* phys);
void physStep(physSim_t* phys, int32_t elapsedUs);

physLine_t* physAddLine(physSim_t* phys, float x1, float y1, float x2, float y2);
physCirc_t* physAddCircle(physSim_t* phys, float x1, float y1, float r, circType_t type);
void setBarrelAngle(physCirc_t* circ, float angle);
void setShotPower(physCirc_t* circ, float power);
physCirc_t* fireShot(physSim_t* phys, physCirc_t* circ);
