#pragma once

//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
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
// Structs
//==============================================================================

typedef struct
{
    int32_t zonemask;
    bool fixed;
    vecFl_t g;
    vecFl_t vel;
    vecFl_t acc;
    lineFl_t travelLine;
    lineFl_t travelLineBB;
    circleFl_t c;
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
} physSim_t;

//==============================================================================
// Function Declarations
//==============================================================================

physSim_t* initPhys(float w, float h, float gx, float gy);
void deinitPhys(physSim_t* phys);
void drawPhysOutline(physSim_t* phys);
void physStep(physSim_t* phys, int32_t elapsedUs);

void physAddLine(physSim_t* phys, float x1, float y1, float x2, float y2, bool isFixed);
void physAddCircle(physSim_t* phys, float x1, float y1, float r, bool isFixed);
