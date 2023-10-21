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
 * @brief Initializes the renderer, allocating temporary buffers needed to draw efficiently.
 *
 * Enough space will be allocated for drawing the given model based on its number of elements.
 * Other models may be rendered, as long as they do not have more vertices or triangles. If
 * multiple models with varying numbers of vertices and faces will be drawn, initRendererCustom()
 * should be called with the maximum numbe of vertices and maximum number of faces for any single
 * model.
 *
 */
void initRenderer(const model_t* model);

/**
 * @brief Initializes the renderer, allocating temporary buffers needed to draw efficiently.
 *
 * @param maxVerts The maximum number of vertices in a model that will be drawn
 * @param maxFaces The maximum number of lines in a model that will be drawn
 */
void initRendererCustom(uint16_t maxVerts, uint16_t maxFaces);

/**
 * @brief Deinitializes the renderer, clearing any temporary buffers that were allocated
 *
 */
void deinitRenderer(void);

/**
 * @brief Render a 3D model to the screen
 *
 * initRenderer() or initRendererCustom() must be called once before rendering anything.
 *
 * @param model The 3D model to draw
 * @param offset The location within the world
 * @param orient A quaternion representing the orientation of the model
 * @param scale The scale factor to apply to the model
 * @param translate A vector representing the x, y, and z translation of the model
 * @param x The X coordinate of the left side of the window to draw the model within
 * @param y The Y coordinate of the top side of the window to draw the model within
 * @param w The width of the window to draw the model within
 * @param h The height of the window to draw the model within
 */
void drawModel(const model_t* model, float orient[4], float scale, float translate[3], uint16_t x, uint16_t y, uint16_t w, uint16_t h);

#endif
