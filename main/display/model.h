#ifndef _MODEL_H_
#define _MODEL_H_

#include <stdint.h>
#include <float.h>

#include "palette.h"

typedef int8_t modelVert_t[3];
typedef uint16_t modelLine_t[2];

/**
 * @brief Structure representing a triangle and its color
 */
typedef struct
{
    /// @brief Index of each of the triangle's vertices
    uint16_t verts[3];

    // Triangle color
    paletteColor_t color;
} modelTri_t;

/**
 * @brief Structure representing a 3D model
 */
typedef struct
{
    /// @brief The number of vertices in \c verts
    uint16_t vertCount;

    /// @brief An array defining the coordinates of all vertices
    modelVert_t* verts;

    /// @brief The number of triangles in \c tris
    uint16_t triCount;

    /// @brief An array defining the vertices and color of all triangles
    modelTri_t* tris;

    /// @brief The number of lines in \c lines
    uint16_t lineCount;

    /// @brief An array defining the model's lines
    modelLine_t* lines;
} model_t;

/**
 * @brief Render a 3D model to the screen
 *
 * @param model The 3D model to draw
 * @param offset The location within the world
 * @param orient A quaternion representing the orientation of the model
 * @param x The X coordinate of the left side of the window to draw the model within
 * @param y The Y coordinate of the top side of the window to draw the model within
 * @param w The width of the window to draw the model within
 * @param h The height of the window to draw the model within
 */
void drawModel(const model_t* model, float orient[4]);

#endif
