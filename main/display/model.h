/*! \file model.h
 *
 * \section model_design Design Philosophy
 *
 * This module provides a self-contained utility for rendering 3D models. Any \c .obj file placed
 * in \c /assets/ will automatically be converted into a binary \c .mdl file in SPIFFS. Models are
 * loaded from SPIFFS with loadModel() and unloaded with freeModel(). Once loaded, the model can
 * either be passed directly into drawModel(), or composed within a scene_t and passed to drawScene().
 *
 * Before rendering, either initRenderer(), initRendererCustom(), or initRendererScene() must be
 * called at least once. If necessary, the renderer may be reconfigured by making another call to
 * \c initRenderer*(). Once the renderer is no longer needed, it must be deinitialized by calling
 * deinitRenderer().
 *
 * \section model_example Example
 *
 * \code{.c}
 * #include <float.h>
 *
 * // For loadModel() and freeModel()
 * #include "spiffs_model.h"
 * // for model_t and all rendering functions
 * #include "model.h"
 * // For TFT_WIDTH, TFT_HEIGHT
 * #include "hdw-tft.h"
 * // For getting the swadge's orientation (LSM6DSL.fqQuat)
 * #include "hdw-imu.h"
 *
 * ////////////////////
 * // Initialization //
 * ////////////////////
 *
 * // Load the model data
 * model_t model;
 * if (!loadModel("donut.mdl", &model, false))
 * {
 *     // Error, model not loaded!
 *     return;
 * }
 *
 * // Initialize the renderer
 * initRenderer(&model);
 *
 * ////////////////////
 * // Frame Callback //
 * ////////////////////
 *
 * // Get the swadge's orientation from the accelerometer to rotate the model
 * float orient[4];
 * memcpy(orient, LSM6DSL.fqQuat, sizeof(orient));
 *
 * float scale = 1.0;
 * float translate[3] = {0};
 *
 * // Draw the model across the whole screen
 * drawModel(&model, orient, scale, translate, 0, 0, TFT_WIDTH, TFT_HEIGHT);
 *
 * ////////////////////
 * // Deinitializing //
 * ////////////////////
 *
 * // Deinitialize the renderer
 * deinitRenderer();
 *
 * // Free the model data
 * freeModel(&model);
 * \endcode
 *
 */
#ifndef _MODEL_H_
#define _MODEL_H_

#include <stdbool.h>
#include <stdint.h>
#include <float.h>

#include "palette.h"
#include "small3dlib.h"

// Defines
#define SCENE_MAX_OBJECTS 12

//==============================================================================
// Defines
//==============================================================================

#define SCENE_MAX_OBJECTS 16

//==============================================================================
// Types
//==============================================================================

/// @brief Type representing a vertex as an array of 3 int8_t for x, y, and z
typedef int8_t modelVert_t[3];

/// @brief Type representing a line as an array of 2 vertex indices
typedef uint16_t modelLine_t[2];

/**
 * @brief Structure representing a triangular face and its color
 */
typedef struct
{
    /// @brief Index of each of the triangle's vertices
    uint16_t verts[3];

    /// @brief The face's color
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
 * @brief Struct for holding all data nedeed to
 *
 */
typedef struct
{
    uint16_t vertCount;
    uint16_t triCount;
    uint16_t uvCount;

    uint8_t mtlCount;

    int32_t minBounds[3];
    int32_t maxBounds[3];

    /// @brief A list of (3 * vertCount) int32_ts
    int32_t* verts;

    /// @brief A list of (3 * triCount) vertex indices
    uint16_t* tris;

    /// @brief If true, use UVs. If false, use vertex colors
    bool useUvs;

    /// @brief A list of UV value pairs as (2 * uvCount) int32_ts
    int32_t* uvs;

    /// @brief A list of UV indices for each triangle vertex as (3 * triCount) uint16_ts
    uint16_t* triUvs;

    /// @brief A list of colors for each triangle
    paletteColor_t* triColors;

    /// @brief The name of the .mtl file referenced in this obj, if any
    const char* mtlLibName;

    /// @brief A list of (mtlCount) material names, dynamic strs
    const char** mtlNames;

    /// @brief A list of material indices for each triangle
    uint8_t* triMtls;
} object3dInfo_t;

/**
 * @brief Structure representing an object within a scene
 */
typedef struct
{
    /// @brief The 3D model to render
    const model_t* model;

    /// @brief The 4x4 transform matrix for this object's model
    float transform[4][4];
} obj3d_t;

/**
 * @brief Structure representing a scene
 */
typedef struct
{
    /// @brief The number of objects in \c objects to render
    uint8_t objectCount;

    /// @brief The transform matrix for the overall scene. This will be performed after the model transform.
    float transform[4][4];

    /// @brief An array of up to \c SCENE_MAX_OBJECTS objects to be rendered in the scene
    obj3d_t objects[SCENE_MAX_OBJECTS];
} scene_t;

// Variables

/**
 * @brief Pointer to a boolean value which is true when the last-drawn frame was clipped due to lack of memory
 */
extern const bool* const frameClipped;

//==============================================================================
// Function Prototypes
//==============================================================================

/**
 * @brief Initializes the renderer, allocating temporary buffers needed to draw efficiently.
 *
 * Enough space will be allocated for drawing the given model based on its number of elements.
 * Other models may be rendered, as long as they do not have more vertices or triangles. If
 * multiple models with varying numbers of vertices and faces will be drawn, initRendererCustom()
 * should be called with the maximum numbe of vertices and maximum number of faces for any single
 * model.
 *
 * Each initRenderer*() function may be called multiple times without calling deinitRenderer()
 * in-between each call.
 *
 * @param A model to use to determine the buffer space needed
 */
void initRenderer(const model_t* model);

/**
 * @brief Count and return the total number of vertices and faces in a scene
 *
 * @param scene The scene to count
 * @param[out] verts A pointer to a uint16_t to be set to the total number of vertices
 * @param[out] faces A pointer to a uint16_t to be set to the total number of faces
 */
void countScene(const scene_t* scene, uint16_t* verts, uint16_t* faces);

/**
 * @brief Initializes the renderer, allocating temporary buffers needed to draw efficiently.
 *
 * This function can be used to allocate slightly less memory than the maximum amount possible,
 * which means larger and/or a greater number of models could be rendered than normally possible.
 * Depending on the models used, around 20-40% of vertices are not actually drawn
 *
 * Each initRenderer*() function may be called multiple times without calling deinitRenderer()
 * in-between each call.
 *
 * @param maxVerts The maximum number of vertices in a model that will be drawn
 * @param maxFaces The maximum number of lines in a model that will be drawn
 */
void initRendererCustom(uint16_t maxVerts, uint16_t maxFaces);

/**
 * @brief Initializes the renderer, allocating temporary buffers sufficient to draw the given
 * scene efficiently.
 *
 * Each initRenderer*() function may be called multiple times without calling deinitRenderer()
 * in-between each call.
 *
 * @param scene The scene to allocate enough memory for
 */
void initRendererScene(const scene_t* scene);

/**
 * @brief Deinitializes the renderer, clearing any temporary buffers that were allocated.
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
void drawModel(const model_t* model, const float orient[4], float scale, const float translate[3], uint16_t x, uint16_t y,
               uint16_t w, uint16_t h);

/**
 * @brief Render a scene with multiple 3D models
 *
 * initRendererScene() or initRendererCustom() must be called once first before drawing the scene.
 *
 * @param scene The scene to draw
 * @param x The X coordinate of the left side of the location to draw the scene within
 * @param y The Y coordinate of the top side of the window to draw the scene within
 * @param w The width of the window to draw the model within
 * @param h The height of the window to draw the model within
 */
void drawScene(const scene_t* scene, uint16_t x, uint16_t y, uint16_t w, uint16_t h);

#endif
