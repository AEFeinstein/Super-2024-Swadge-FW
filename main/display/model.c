// Includes
#include "model.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <esp_log.h>
#include <inttypes.h>

#include "hdw-imu.h"
#include "quaternions.h"
#include "matrixMath.h"
#include "hdw-tft.h"
#include "shapes.h"
#include "palette.h"

// If defined, the stack will be used to allocate vert/tri buffers, otherwise heap is used
//#define MODEL_USE_STACK
//#define MODEL_DEBUG

#ifdef MODEL_DEBUG
#define DEBUG_MATRIX(m) do { \
    printf(#m "[4][4]:\n"); \
    for (int r = 0; r < 4; r++) \
    { \
        for (int c = 0; c < 4; c++) printf("%+03.2f ", m[r][c]); \
        printf("\n"); \
    } \
    printf("\n"); \
} while (false)
#else
#define DEBUG_MATRIX(m)
#endif

// do a funky typedef so we can still define trimap as a 2D array
typedef uint16_t trimap_t[3];

// private variable
bool _frameClipped = false;
// read-only access to _frameClipped
const bool* const frameClipped = &_frameClipped;

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

void countScene(const scene_t* scene, uint16_t* verts, uint16_t* faces)
{
    uint32_t totalVerts = 0;
    uint32_t totalTris = 0;

    for (int i = 0; i < scene->objectCount; i++)
    {
        const obj3d_t* object = &scene->objects[i];
        if (NULL != object && NULL != object->model)
        {
            totalVerts += object->model->vertCount;
            totalTris += object->model->triCount;

            if (totalVerts > UINT16_MAX || totalTris > UINT16_MAX)
            {
                ESP_LOGE("Model",
                         "Too many verts/faces in scene: %" PRIu32 " verts"
                         " or %" PRIu32 " faces exceeds max of 65535",
                         totalVerts,
                         totalTris);
                return;
            }
        }
    }

    *verts = (uint16_t)totalVerts;
    *faces = (uint16_t)totalTris;
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
    float scale3[3] = {scale, scale, scale};

    scene_t scene;
    scene.objects[0].model = model;
    scene.objectCount = 1;

    createTransformMatrix(scene.objects[0].transform, translate, orient, scale3);
    identityMatrix(scene.transform);

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

    // Reset triangle overflow flag
    _frameClipped = false;

    // where in the vert map each model's vertices starts
    uint16_t vertOffsets[16] = {0};

    for (uint8_t modelNum = 0; modelNum < scene->objectCount; modelNum++)
    {
        const obj3d_t* object = &scene->objects[modelNum];
        const model_t* model = object->model;
        vertOffsets[modelNum] = vertices;

        if (NULL == model)
        {
            // no model no render!
            continue;
        }

        DEBUG_MATRIX(scene->transform);
        DEBUG_MATRIX(object->transform);

        for( i = 0; i < model->vertCount; i++ )
        {
            // (probably no longer accurate) Performing the transform this way is about 700us.
            float bx = 1.0 * model->verts[i][0];
            float by = 1.0 * model->verts[i][1];
            float bz = 1.0 * model->verts[i][2];

            float xlvert[4] = {bx, by, bz, 1};
            multiplyMatrixVector(xlvert, object->transform, xlvert);
            multiplyMatrixVector(xlvert, scene->transform, xlvert);

            // TODO: Shouldn't this be part of the matrix?
            // Project the vert onto the screen
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
                _frameClipped = true;
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
    // QUESTION: Would it potentially be more efficient to do insertion-sort as we go? Perhaps a heap even
    // Doing that would also allow us to do a sort of memory-constrained culling of faces based on distance
    // And we could just draw as many of the closest tris as we have space for
    qsort(trimap, totalTrisThisFrame, sizeof( trimap[0] ), (void*)zcompare );

#ifdef MODEL_DEBUG
    static int maxTotalTris = 0;
    if (totalTrisThisFrame > maxTotalTris)
    {
        maxTotalTris = totalTrisThisFrame;
    }

    ESP_LOGI("Model", "Tris this frame: %d / %d, %d culled (%d%% used, max=%d%%)", totalTrisThisFrame, maxTris, (maxTris - totalTrisThisFrame), 100 * totalTrisThisFrame / maxTris, 100 * maxTotalTris / maxTris);
#endif

    for( i = 0; i < totalTrisThisFrame; i++)
    {
        // Get the face index
        int j = trimap[i][1];

        // Extract the model number from the top byte of trimap[][2]
        int modelNum = (trimap[i][2] >> 8) & 0xFF;
        const model_t* model = scene->objects[modelNum].model;
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