#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>

#include "obj_processor.h"
#include "heatshrink_encoder.h"
#include "fileUtils.h"
#include "heatshrink_util.h"


#define DEBUG_OBJ
#define BUFLEN_DEFAULT 1024

typedef float fvVert_t[6];
typedef int iv_t[3];
typedef int ln_t[2];

#ifdef DEBUG_OBJ
#define DEBUG_PRINT(...) printf(__VA_ARGS__)
#else
#define DEBUG_PRINT(...)
#endif

#define DEBUG_SIZE_CHANGE(arr, count, size) DEBUG_PRINT("Increased size of " # arr " to %d because it has %d members\n", size, count)

#define CHECK_ARR(arr, count, size, elmsize) do \
{ \
    if (count + 1 >= size) \
    { \
        size *= 2; \
        DEBUG_SIZE_CHANGE(arr, count, size); \
        void* _tmp##arr = realloc(arr, size * elmsize); \
        if (_tmp##arr != NULL) \
        { \
            arr = _tmp##arr; \
        } \
    } \
} while(0)

void process_obj(const char* infile, const char* outdir)
{
    /* Determine if the output file already exists */
    char outFilePath[128] = {0};
    strcat(outFilePath, outdir);
    strcat(outFilePath, "/");
    strcat(outFilePath, get_filename(infile));

    /* Change the file extension */
    char* dotptr = strrchr(outFilePath, '.');
    snprintf(&dotptr[1], strlen(dotptr), "mdl");

    /* Open input file */
    FILE * f = fopen( infile, "r" );
	char buffer[1024];
	char * line;

    /* Min and Max dimensions */
    float minB[3] = { 1e20, 1e20, 1e20 };
	float maxB[3] = {-1e20,-1e20,-1e20 };

    // Absolute max in any dimension
	float maxextent = -1e20;

    // Read verts
    int fvSize = BUFLEN_DEFAULT;
	fvVert_t* fvVerts = malloc(fvSize * sizeof(fvVert_t));
	int fvc = 0;

    int ivSize = BUFLEN_DEFAULT;
	iv_t* ivS = malloc(ivSize * sizeof(iv_t));
	int ivc = 0;

    int lnSize = BUFLEN_DEFAULT;
    ln_t* lines = malloc(lnSize * sizeof(ln_t));
    int lnc = 0;

	while (NULL != (line = fgets( buffer, sizeof(buffer)-1, f )))
	{
        /* Read vertex line */
		if( line[0] == 'v' )
		{
            // Make sure the fvVerts array has room for another one
            CHECK_ARR(fvVerts, fvc, fvSize, sizeof(fvVert_t));
			float fv[6];
			sscanf( line + 2, "%f %f %f %f %f %f", fv+0, fv+1, fv+2, fv+3, fv+4, fv+5 );

			if( fv[0] < minB[0] ) minB[0] = fv[0];
			if( fv[0] > maxB[0] ) maxB[0] = fv[0];

			if( fv[1] < minB[1] ) minB[1] = fv[1];
			if( fv[1] > maxB[1] ) maxB[1] = fv[1];

			if( fv[2] < minB[2] ) minB[2] = fv[2];
			if( fv[2] > maxB[2] ) maxB[2] = fv[2];

			memcpy( &fvVerts[fvc++], fv, sizeof( fv ) );
		}

        /* Read triangle line */
		if( line[0] == 'f' )
		{
            // Make sure the ivS array has enough room for another one
            CHECK_ARR(ivS, ivc, ivSize, sizeof(iv_t));

			int vv[3];
			sscanf( line + 2, "%d %d %d", vv+0, vv+1, vv+2 );

            // read values are 1-indexed, decrement to make them 0-indexed
			vv[0]--;
			vv[1]--;
			vv[2]--;

			memcpy( ivS[ivc++], vv, sizeof( vv ) );
		}

        /* Read line entry */
        if (line[0] == 'l')
        {
            CHECK_ARR(lines, lnc, lnSize, sizeof(ln_t));

            ln_t ln;
            sscanf( line + 2, "%d %d", &ln[0], &ln[1]);

            // Convert to 0-indexed
            ln[0]--;
            ln[1]--;

            memcpy(lines[lnc++], ln, sizeof(ln));
        }
        /* Ignore comments and also everything else */
	}

    DEBUG_PRINT("Read %d vertices and %d faces from obj\n", fvc, ivc);

    int iAliasedVert[fvc];

    // the final list of verts
    int cvsize = BUFLEN_DEFAULT;
	float* compverts = malloc(cvsize * sizeof(float) * 3);
	int cvct = 0;
	int i, j;
	for( i = 0; i < fvc; i++ )
	{
		float * fvcr = fvVerts[i];

        // Look for any previous verts that overlap this one close enough
		for( j = 0; j < cvct; j++ )
		{
			float dx = fvcr[0] - compverts[j*3 + 0];
			float dy = fvcr[1] - compverts[j*3 + 1];
			float dz = fvcr[2] - compverts[j*3 + 2];
			float diff = sqrtf( dx*dx + dy*dy + dz*dz );
			if( diff < 0.0001 )
			{
                // Match found!
				break;
			}
		}
		if( j == cvct )
		{
			// No match found - new one!
            CHECK_ARR(compverts, cvct, cvsize, sizeof(float) * 3);
			memcpy( &compverts[3 * cvct++], fvcr, sizeof(float) * 3 );
            /*DEBUG_PRINT("Final vert #%d: (%.2f, %.2f, %.2f) --> (%d, %d, %d)\n", cvct - 1,
                        fvVerts[j][0], fvVerts[j][1], fvVerts[j][2],
                        compverts[(cvct - 1) * 3], compverts[(cvct - 1) * 3 + 1], compverts[(cvct - 1) * 3 + 2]);*/
		}
		iAliasedVert[i] = j;
	}

    DEBUG_PRINT("Combined %d vertices, final number is %d vertices\n", fvc - cvct, cvct);

	if( -minB[0] > maxextent ) maxextent = -minB[0];
	if( -minB[1] > maxextent ) maxextent = -minB[1];
	if( -minB[2] > maxextent ) maxextent = -minB[2];
	if(  maxB[0] > maxextent ) maxextent =  maxB[0];
	if(  maxB[1] > maxextent ) maxextent =  maxB[1];
	if(  maxB[2] > maxextent ) maxextent =  maxB[2];

	float scale = 127.9 / maxextent;

    // Allocate raw file buffer
    // Structure: 2 bytes version
    //            2 bytes #verts
    //            2 bytes #tris
    //            2 bytes #lines
    //                verts 3B each (int8 x 3)
    //                triVerts: {2B v0, 2B v1, 2B v2}
    //                triCols: { 1B Color (uint8 palette) }
    //                lines: {2B v0, 2B v1}

    // face verts/colors
	int ivR[ivc][4];

	for( i = 0; i < ivc; i++ )
	{
		int i0 = ivS[i][0];
		int i1 = ivS[i][1];
		int i2 = ivS[i][2];

		float * color = &fvVerts[i0][3];
		int fc = ((int)(color[2] * 5.9)) + ((int)(color[1] * 5.9)) * 6 + ((int)(color[0] * 5.9)) * 36;
		int * face = ivR[i];

        face[0] = iAliasedVert[i0];
		face[1] = iAliasedVert[i1];
		face[2] = iAliasedVert[i2];
		face[3] = fc;
	}

    int outBufSize = 2 + 2 + 2 + 2 + (cvct * 3) + 3 + ivc * 7 + lnc * 4;
    uint8_t* outBuf = malloc(outBufSize);
    uint8_t* bp = outBuf;

    // Write version
    *bp++ = 0;
    *bp++ = 1;

    // Write number of verts
    *bp++ = (cvct >> 8) & 0xFF;
    *bp++ = cvct & 0xFF;

    // Write number of tris
    *bp++ = (ivc >> 8) & 0xFF;
    *bp++ = ivc & 0xFF;

    // Write number of lines
    *bp++ = (lnc >> 8) & 0xFF;
    *bp++ = lnc & 0xFF;

#ifdef DEBUG_OBJ
    printf("\nint8_t verts[] = {\n");

	for( i = 0; i < cvct; i++ )
	{
		printf("\t%d, %d, %d,\n",
               (int8_t)(compverts[i*3 + 0]*scale),
               (int8_t)(compverts[i*3 + 1]*scale),
               (int8_t)(compverts[i*3 + 2]*scale));
	}
	printf("};\nuint8_t tris[] = {\n");
	for( i = 0; i < ivc; i++ )
	{
		printf("\t%d, %d, %d, %d,\n", ivR[i][0], ivR[i][1], ivR[i][2], ivR[i][3]);
	}
	printf("};\n" );
#endif

    // Write verts
	for( i = 0; i < cvct; i++ )
	{
        int8_t outVert[3];

        outVert[0] = (int8_t)(compverts[i*3 + 0]*scale);
        outVert[1] = (int8_t)(compverts[i*3 + 1]*scale);
        outVert[2] = (int8_t)(compverts[i*3 + 2]*scale);

        *bp++ = outVert[0];
        *bp++ = outVert[1];
        *bp++ = outVert[2];
	}

	// Write triangle vertexes
	for( i = 0; i < ivc; i++ )
	{
        *bp++ = (ivR[i][0] >> 8) & 0xFF;
        *bp++ = ivR[i][0] & 0xFF;
        *bp++ = (ivR[i][1] >> 8) & 0xFF;
        *bp++ = ivR[i][1] & 0xFF;
        *bp++ = (ivR[i][2] >> 8) & 0xFF;
        *bp++ = ivR[i][2] & 0xFF;
	}

    // Write triangle colors
    for (i = 0; i < ivc; i++)
    {
        *bp++ = (uint8_t)(ivR[i][3] & 0xFF);
    }

    // Write lines
    for (i = 0; i < lnc; i++)
    {
        *bp++ = (lines[i][0] >> 8) & 0xFF;
        *bp++ = lines[i][0] & 0xFF;
        *bp++ = (lines[i][1] >> 8) & 0xFF;
        *bp++ = lines[i][1] & 0xFF;
    }

    printf("Writing compressed model to %s\n", outFilePath);
    // Write compressed output file
    writeHeatshrinkFile(outBuf, outBufSize, outFilePath);
}