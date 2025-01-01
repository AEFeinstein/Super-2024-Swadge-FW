/**
 * @file 3dPrimitives.h
 * @author Jeremy Stintzcum (Jeremy.Stintzcum@gmail.com)
 * @brief A header that include all the 3D rendering types
 * @version 0.1
 * @date 2024-12-31
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once

#include "cVectors.h"

//==============================================================================
// Math
//==============================================================================

typedef struct
{
    float x, y, z;
} vec3d_t;

//==============================================================================
// Geometry
//==============================================================================

typedef struct
{
    vec3d_t p[3];
} triangle_t;

typedef struct
{
    cVector_t tris; // List of triangles
} mesh_t;

//==============================================================================
// Function Declarations
//==============================================================================

// Load Tris into mesh