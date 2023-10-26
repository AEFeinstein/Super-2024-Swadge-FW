// Includes
#include "model.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <esp_log.h>
#include <inttypes.h>

#include "hdw-imu.h"
#include "quaternions.h"
#include "hdw-tft.h"
#include "shapes.h"
#include "palette.h"

// If defined, the stack will be used to allocate vert/tri buffers, otherwise heap is used
//#define MODEL_USE_STACK

// do a funky typedef so we can still define trimap as a 2D array
typedef uint16_t trimap_t[3];

// Variables
#ifndef MODEL_USE_STACK
static uint16_t maxTris = 0;
static int16_t* verts_out = NULL;
static trimap_t* trimap = NULL;
#endif

// Static Function Prototypes
static void intcross(int* p, const int* a, const int* b);
static int zcompare(const int16_t *a, const int16_t* b);
static unsigned julery_isqrt(unsigned long val);
static void countScene(const scene_t* scene, uint16_t* verts, uint16_t* faces);
static void convertQuatToMatrix(float transform[4][4], const float rotate[4]);

// Function Definitions
static void intcross(int* p, const int* a, const int* b)
{
    float tx = a[1] * b[2] - a[2] * b[1];
    float ty = a[2] * b[0] - a[0] * b[2];
    p[2]     = a[0] * b[1] - a[1] * b[0];
    p[1]     = ty;
    p[0]     = tx;
}

static int zcompare(const int16_t *a, const int16_t* b)
{
	return (a[0] - b[0]) ? (a[0] - b[0]) : ((a[2] >> 8) - (b[2] >> 8));
}

static unsigned julery_isqrt(unsigned long val) {
    unsigned long temp, g=0, b = 0x8000, bshft = 15;
    do {
        if (val >= (temp = (((g << 1) + b)<<bshft--))) {
           g += b;
           val -= temp;
        }
    } while (b >>= 1);
    return g;
}

/**
 * @brief Count and return the total number of vertices and faces in a scene
 *
 * @param scene The scene to count
 * @param[out] verts A pointer to a uint16_t to be set to the total number of vertices
 * @param[out] faces A pointer to a uint16_t to be set to the total number of faces
 */
static void countScene(const scene_t* scene, uint16_t* verts, uint16_t* faces)
{
    uint16_t totalVerts = 0;
    uint16_t totalTris = 0;

    for (uint8_t i = 0; i < scene->modelCount; i++)
    {
        const modelPos_t* modelPos = &scene->models[i];
        if (NULL != modelPos->model)
        {
            uint16_t prevVerts = totalVerts;
            uint16_t prevTris = totalTris;
            totalVerts += modelPos->model->vertCount;
            totalTris += modelPos->model->triCount;

            if (totalVerts < prevVerts || totalTris < prevTris)
            {
                // Detect integer rollover and abort
                ESP_LOGE("Model",
                         "Too many verts/faces in scene: %" PRIu16 " + %" PRIu16 " verts"
                         " or %" PRIu16 " + %" PRIu16 " faces rolled over",
                         prevVerts, modelPos->model->vertCount,
                         prevTris, modelPos->model->triCount);
                return;
            }
        }
    }

    *verts = totalVerts;
    *faces = totalTris;
}

void initRenderer(const model_t* model)
{
    initRendererCustom(model->vertCount, model->triCount);
}

void initRendererCustom(uint16_t maxVerts, uint16_t maxFaces)
{
#ifdef MODEL_USE_STACK
    ESP_LOGI("Model", "Using stack for verts and faces! Woo");
#else
    // Free any existing buffers
    deinitRenderer();

    ESP_LOGI("Model", "Allocating %" PRIu64 " bytes for verts and faces", (uint64_t)((maxVerts + maxFaces) * 3 * sizeof(uint16_t)));

    // Allocate the new buffers
    maxTris = maxFaces;
    verts_out = malloc(maxVerts * 3 * sizeof(int16_t));
    trimap = malloc(maxFaces * 3 * sizeof(uint16_t));

    if (verts_out == NULL || trimap == NULL)
    {
        ESP_LOGI("Model", "Renderer could not allocate the buffers :(");
    }
#endif
}

void initRendererScene(const scene_t* scene)
{
    uint16_t totalVerts = 0;
    uint16_t totalTris = 0;
    countScene(scene, &totalVerts, &totalTris);
    initRendererCustom(totalVerts, totalTris);
}

void deinitRenderer(void)
{
#ifndef MODEL_USE_STACK
    if (NULL != verts_out)
    {
        free(verts_out);
        verts_out = NULL;
    }

    if (NULL != trimap)
    {
        free(trimap);
        trimap = NULL;
        maxTris = 0;
    }
#endif
}


void drawModel(const model_t* model, const float orient[4], float scale, const float translate[3], uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    scene_t scene;
    scene.models[0].model = model;
    scene.modelCount = 1;
    memcpy(scene.models[0].orient, orient, sizeof(float) * 4);
    scene.models[0].scale = scale;
    memcpy(scene.models[0].translate, translate, sizeof(float) * 3);

    drawScene(&scene, x, y, w, h);
}

void drawScene(const scene_t* scene, uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
#ifdef MODEL_USE_STACK
    uint16_t maxVerts, maxTris;
    countScene(scene, &maxVerts, &maxTris);
    int16_t verts_out[maxVerts * 3];
    trimap_t trimap[maxTris];
#endif

    int vertices = 0;
    int totalTrisThisFrame = 0;
    int i;

    // were not used really
    /*float plusy[3] = { 0, 1, 0 };

    // Produce a model matrix from a quaternion.
    float plusx_out[3] = { 0.9, 0, 0 };
    float plusy_out[3] = { 0, 0.9, 0 };
    float plusz_out[3] = { 0, 0, 0.9 };
    const float sceneOrient[4] = {1, 0, 0, 0};
    mathRotateVectorByQuaternion( plusy, sceneOrient, plusy );
    mathRotateVectorByQuaternion( plusy_out, sceneOrient, plusy_out );
    mathRotateVectorByQuaternion( plusx_out, sceneOrient, plusx_out );
    mathRotateVectorByQuaternion( plusz_out, sceneOrient, plusz_out );*/

    // where in the vert map each model's vertices starts
    uint16_t vertOffsets[16] = {0};

    for (uint8_t modelNum = 0; modelNum < scene->modelCount; modelNum++)
    {
        const modelPos_t* modelPos = &scene->models[modelNum];
        const model_t* model = modelPos->model;
        vertOffsets[modelNum] = vertices;

        if (NULL == model)
        {
            // no model no render!
            continue;
        }

        float plusy[3] = { 0, 1, 0 };

        // Produce a model matrix from a quaternion.
        float plusx_out[3] = { 0.9 * modelPos->scale, 0, 0 };
        float plusy_out[3] = { 0, 0.9 * modelPos->scale, 0 };
        float plusz_out[3] = { 0, 0, 0.9 * modelPos->scale };
        mathRotateVectorByQuaternion( plusy, modelPos->orient, plusy );
        mathRotateVectorByQuaternion( plusy_out, modelPos->orient, plusy_out );
        mathRotateVectorByQuaternion( plusx_out, modelPos->orient, plusx_out );
        mathRotateVectorByQuaternion( plusz_out, modelPos->orient, plusz_out );

        for( i = 0; i < model->vertCount; i++ )
        {
            // Performing the transform this way is about 700us.
            float bx = 1.0 * model->verts[i][0] + modelPos->translate[0];
            float by = 1.0 * model->verts[i][1] + modelPos->translate[1];
            float bz = 1.0 * model->verts[i][2] + modelPos->translate[2];

            float xlvert[3] = {
                bx * plusx_out[0] + by * plusx_out[1] + bz * plusx_out[2],
                bx * plusy_out[0] + by * plusy_out[1] + bz * plusy_out[2],
                bx * plusz_out[0] + by * plusz_out[1] + bz * plusz_out[2]
            };

            verts_out[vertices*3+0] = x + xlvert[0] + w/2;
            // Convert from right-handed to left-handed coordinate frame.
            verts_out[vertices*3+1] = y - xlvert[1] + h/2;
            verts_out[vertices*3+2] = xlvert[2];
            vertices++;
        }

        if (model->triCount > 0 && totalTrisThisFrame < maxTris)
        {
            // Draw model with shaded triangles
            for( i = 0; i < model->triCount && totalTrisThisFrame < maxTris; i++)
            {
                int tv1 = (vertOffsets[modelNum] + model->tris[i].verts[0]) * 3;
                int tv2 = (vertOffsets[modelNum] + model->tris[i].verts[1]) * 3;
                int tv3 = (vertOffsets[modelNum] + model->tris[i].verts[2]) * 3;
                int col = model->tris[i].color;

                // Cull out-of-bounds faces
                // We consider a face out of bounds if the X or Y coordinates of all three vertices
                //   are outside of the bounds in the same direction.
                if ((verts_out[tv1+0] < x && verts_out[tv2+0] < x && verts_out[tv3+0] < x)
                    || (verts_out[tv1+0] > (x+w) && verts_out[tv2+0] > (x+w) && verts_out[tv3+0] > (x+w))
                    || (verts_out[tv1+1] < y && verts_out[tv2+1] < y && verts_out[tv3+1] < y)
                    || (verts_out[tv1+1] > (y+h) && verts_out[tv2+1] > (y+h) && verts_out[tv3+1] > (y+h)))
                {
                    continue;
                }

                int diff1[3] = {
                    verts_out[tv3+0] - verts_out[tv1+0],
                    verts_out[tv3+1] - verts_out[tv1+1],
                    verts_out[tv3+2] - verts_out[tv1+2] };
                int diff2[3] = {
                    verts_out[tv2+0] - verts_out[tv1+0],
                    verts_out[tv2+1] - verts_out[tv1+1],
                    verts_out[tv2+2] - verts_out[tv1+2] };

                // If we didn't need the normal, could do cross faster. int crossproduct = diff1[1] * diff2[0] - diff1[0] * diff2[1];

                int icrp[3];
                intcross( icrp, diff1, diff2 );
                // skip inside-out faces
                if( icrp[2] < 0 ) continue;
                int z = verts_out[tv1+2] + verts_out[tv2+2] + verts_out[tv3+2];

                int b = col % 6;
                int g = ( col / 6 ) % 6;
                int r = ( col / 36 ) % 6;

                //float fcrp[3] = { icrp[0], icrp[1], icrp[2] };
                int crpscalar = julery_isqrt( icrp[0] * icrp[0] + icrp[1] * icrp[1] + icrp[2] * icrp[2] );

                // TODO: This is probably not the root cause of the issue
                if (crpscalar == 0)
                {
                    crpscalar = 1;
                }

                icrp[0] = ( 1024 * icrp[0] ) / crpscalar;
                icrp[1] = ( 1024 * icrp[1] ) / crpscalar;
                icrp[2] = ( 1024 * icrp[2] ) / crpscalar;

                int isum = icrp[0] - icrp[1] + icrp[2];

                r = ( r * ( ( isum ) + 1200 ) * 100 ) >> 18;
                g = ( g * ( ( isum ) + 1200 ) * 100 ) >> 18;
                b = ( b * ( ( isum ) + 1200 ) * 100 ) >> 18;

                if( r < 0 ) r = 0;
                if( g < 0 ) g = 0;
                if( b < 0 ) b = 0;
                if( r > 5 ) r = 5;
                if( g > 5 ) g = 5;
                if( b > 5 ) b = 5;

                trimap[totalTrisThisFrame][0] = z;
                trimap[totalTrisThisFrame][1] = i;
                // Pack the model number into the unused top byte of trimap[][2] for now
                trimap[totalTrisThisFrame][2] = (modelNum << 8) | ((r * 36 + g * 6 + b) & 0xFF);
                totalTrisThisFrame++;
            }

            if (totalTrisThisFrame == maxTris)
            {
                ESP_LOGE("Model", "Not enough space for all triangles in scene after model %" PRIu8 ", vert %d. "
                        "Skipping the rest, stuff may look weird!", modelNum, i);
            }
        }
        else if (model->lineCount > 0)
        {
            // Draw wireframe with lines
            for (i = 0; i < model->lineCount; i++)
            {
                uint16_t v1 = (vertOffsets[modelNum] + model->lines[i][0]) * 3;
                uint16_t v2 = (vertOffsets[modelNum] + model->lines[i][1]) * 3;

                float col = verts_out[v1 + 2] / 2000 + 8;
                if (col > 5)
                    col = 5;
                else if (col < 0)
                    continue;
                drawLineFast(verts_out[v1], verts_out[v1 + 1], verts_out[v2], verts_out[v2 + 1], col);
            }
        }
    }

    // Sort all faces by Z-index, then by model number
    qsort(trimap, totalTrisThisFrame, sizeof( trimap[0] ), (void*)zcompare );

    for( i = 0; i < totalTrisThisFrame; i++)
    {
        // Get the face index
        int j = trimap[i][1];

        // Extract the model number from the top byte of trimap[][2]
        int modelNum = (trimap[i][2] >> 8) & 0xFF;
        const model_t* model = scene->models[modelNum].model;
        //ESP_LOGI("Model", "modelNum is %" PRIu16, (trimap[i][2] >> 8) & 0xFF);

        int tv1 = (vertOffsets[modelNum] + model->tris[j].verts[0])*3;
        int tv2 = (vertOffsets[modelNum] + model->tris[j].verts[1])*3;
        int tv3 = (vertOffsets[modelNum] + model->tris[j].verts[2])*3;

        // Use only the low byte of trimap[][2] for color
        int tcol = trimap[i][2] & 0xFF;

        drawTriangleOutlinedBounded(
            verts_out[tv1+0], verts_out[tv1+1],
            verts_out[tv2+0], verts_out[tv2+1],
            verts_out[tv3+0], verts_out[tv3+1],
            tcol, tcol,
            x, y, x + w, y + h );
    }
}