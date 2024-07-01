// Includes
#include "model.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <esp_log.h>
#include <inttypes.h>

#include "hdw-imu.h"
#include "hdw-tft.h"
#include "shapes.h"
#include "palette.h"

// do a funky typedef so we can still define trimap as a 2D array
typedef uint16_t trimap_t[3];

// Variables
static uint16_t* verts_out = NULL;
static trimap_t* trimap = NULL;

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
	return a[0] - b[0];
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

void initRenderer(const model_t* model)
{
    initRendererCustom(model->vertCount, model->triCount);
}

void initRendererCustom(uint16_t maxVerts, uint16_t maxFaces)
{
    // Free any existing buffers
    deinitRenderer();

    ESP_LOGI("Model", "Allocating %" PRIu64 " bytes for verts and faces", (uint64_t)((maxVerts + maxFaces) * 3 * sizeof(uint16_t)));

    // Allocate the new buffers
    verts_out = malloc(maxVerts * 3 * sizeof(uint16_t));
    trimap = malloc(maxFaces * 3 * sizeof(uint16_t));

    if (verts_out == NULL || trimap == NULL)
    {
        ESP_LOGI("Model", "Renderer could not allocate the buffers :(");
    }
}

void deinitRenderer(void)
{
    if (NULL != verts_out)
    {
        free(verts_out);
        verts_out = NULL;
    }

    if (NULL != trimap)
    {
        free(trimap);
        trimap = NULL;
    }
}

void drawModel(const model_t* model, float orient[4], float scale, float translate[3], uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    float plusy[3] = { 0, 1, 0 };

	// Produce a model matrix from a quaternion.
	float plusx_out[3] = { 0.9 * scale, 0, 0 };
	float plusy_out[3] = { 0, 0.9 * scale, 0 };
	float plusz_out[3] = { 0, 0, 0.9 * scale };
	mathRotateVectorByQuaternion( plusy, orient, plusy );
	mathRotateVectorByQuaternion( plusy_out, orient, plusy_out );
	mathRotateVectorByQuaternion( plusx_out, orient, plusx_out );
	mathRotateVectorByQuaternion( plusz_out, orient, plusz_out );

	int i, vertices = 0;
    for( i = 0; i < model->vertCount; i++ )
	{
		// Performing the transform this way is about 700us.
        float bx = 1.0 * model->verts[i][0] + translate[0];
		float by = 1.0 * model->verts[i][1] + translate[1];
		float bz = 1.0 * model->verts[i][2] + translate[2];

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

    if (model->triCount > 0)
    {
        // Draw model with shaded triangles
        int totalTrisThisFrame = 0;

        for( i = 0; i < model->triCount; i++)
        {
            int tv1 = model->tris[i].verts[0] * 3;
            int tv2 = model->tris[i].verts[1] * 3;
            int tv3 = model->tris[i].verts[2] * 3;
            int col = model->tris[i].color;

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
            if( icrp[2] < 0 ) continue;
            int z = verts_out[tv1+2] + verts_out[tv2+2] + verts_out[tv3+2];

            int b = col % 6;
            int g = ( col / 6 ) % 6;
            int r = ( col / 36 ) % 6;

            //float fcrp[3] = { icrp[0], icrp[1], icrp[2] };
            int crpscalar = julery_isqrt( icrp[0] * icrp[0] + icrp[1] * icrp[1] + icrp[2] * icrp[2] );
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
            trimap[totalTrisThisFrame][2] = r * 36 + g * 6 + b;
            totalTrisThisFrame++;
        }

        qsort(trimap, totalTrisThisFrame, sizeof( trimap[0] ), (void*)zcompare );

        for( i = 0; i < totalTrisThisFrame; i++)
        {
            int j = trimap[i][1];
            int tv1 = model->tris[j].verts[0]*3;
            int tv2 = model->tris[j].verts[1]*3;
            int tv3 = model->tris[j].verts[2]*3;
            int tcol = trimap[i][2];

            drawTriangleOutlined(
                verts_out[tv1+0], verts_out[tv1+1],
                verts_out[tv2+0], verts_out[tv2+1],
                verts_out[tv3+0], verts_out[tv3+1],
                tcol, tcol );
        }
    }
    else if (model->lineCount > 0)
    {
        // Draw wireframe with lines
        for (i = 0; i < model->lineCount; i++)
        {
            uint16_t v1 = model->lines[i][0] * 3;
            uint16_t v2 = model->lines[i][1] * 3;

            float col = verts_out[v1 + 2] / 2000 + 8;
            if (col > 5)
                col = 5;
            else if (col < 0)
                continue;
            drawLineFast(verts_out[v1], verts_out[v1 + 1], verts_out[v2], verts_out[v2 + 1], col);
        }
    }
}
