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

// #define DEBUG_OBJ
#define BUFLEN_DEFAULT 1024

typedef float fvVert_t[6];
typedef int iv_t[3];
typedef int ln_t[2];
typedef float uv_t[3];
typedef int triUv_t[3];
typedef struct
{
    int index;
    char* name;
} mtl_t;

#ifdef DEBUG
    //#define DEBUG_OBJ
#endif

#ifdef DEBUG
    #define DEBUG_PRINT(...) printf(__VA_ARGS__)
#else
    #define DEBUG_PRINT(...)
#endif

#ifdef DEBUG
    #define DEBUG_BUF(buf, end)                                    \
        do                                                         \
        {                                                          \
            printf("Buffer at " __FILE__ ":%d:\n", __LINE__);      \
            for (uint8_t* _dbgBuf = buf; _dbgBuf < end; _dbgBuf++) \
            {                                                      \
                if ((_dbgBuf - buf) % 8 == 0)                      \
                {                                                  \
                    /*printf("\n%04X    ", (uint16_t)(_dbgBuf - buf));*/         \
                }                                                  \
                printf("%02X ", *_dbgBuf);                         \
            }                                                      \
            printf("\n\n");                                        \
        } while (0)
#else
    #define DEBUG_BUF(buf, end)
#endif

#define DEBUG_SIZE_CHANGE(arr, count, size) \
    DEBUG_PRINT("Increased size of " #arr " to %d because it has %d members\n", size, count)

#define CHECK_ARR(arr, count, size, elmsize)                \
    do                                                      \
    {                                                       \
        if (count + 1 >= size)                              \
        {                                                   \
            size *= 2;                                      \
            DEBUG_SIZE_CHANGE(arr, count, size);            \
            void* _tmp##arr = realloc(arr, size * elmsize); \
            if (_tmp##arr != NULL)                          \
            {                                               \
                arr = _tmp##arr;                            \
            }                                               \
        }                                                   \
    } while (0)

void process_obj(const char* infile, const char* outdir)
{
    /* Determine if the output file already exists */
    char outFilePath[128] = {0};
    strcat(outFilePath, outdir);
    strcat(outFilePath, "/");
    strcat(outFilePath, get_filename(infile));

    DEBUG_PRINT("Processing object file %s\n", get_filename(infile));

    /* Change the file extension */
    char* dotptr = strrchr(outFilePath, '.');
    snprintf(&dotptr[1], strlen(dotptr), "mdl");

    /* Open input file */
    FILE* f = fopen(infile, "r");
    char buffer[1024];
    char* line;

    /* Min and Max dimensions */
    float minB[3] = {1e20, 1e20, 1e20};
    float maxB[3] = {-1e20, -1e20, -1e20};

    // Absolute max in any dimension
    float maxextent = -1e20;

    // Read verts
    int fvSize        = BUFLEN_DEFAULT;
    fvVert_t* fvVerts = malloc(fvSize * sizeof(fvVert_t));
    int fvc           = 0;

    int ivSize = BUFLEN_DEFAULT;
    iv_t* ivS  = malloc(ivSize * sizeof(iv_t));
    int ivc    = 0;

    int lnSize  = BUFLEN_DEFAULT;
    ln_t* lines = malloc(lnSize * sizeof(ln_t));
    int lnc     = 0;

    int uvSize = BUFLEN_DEFAULT;
    uv_t* uvs  = malloc(uvSize * sizeof(uv_t));
    int uvc    = 0;

    int triUvSize   = BUFLEN_DEFAULT;
    triUv_t* triUvs = malloc(triUvSize * sizeof(triUv_t));
    int triUvc      = 0;

    int mtlSize = BUFLEN_DEFAULT;
    mtl_t* mtls = malloc(mtlSize * sizeof(mtl_t));
    int mtlc    = 0;

    int triMtlSize   = BUFLEN_DEFAULT;
    uint8_t* triMtls = malloc(triMtlSize * sizeof(uint8_t));
    int triMtlc      = 0;

    char* mtlLibName = NULL;
    int curMtl       = -1;

    while (NULL != (line = fgets(buffer, sizeof(buffer) - 1, f)))
    {
        /* Read vertex line */
        if (line[0] == 'v' && line[1] == ' ')
        {
            // Make sure the fvVerts array has room for another one
            CHECK_ARR(fvVerts, fvc, fvSize, sizeof(fvVert_t));
            float fv[6]   = {0, 0, 0, -1, -1, -1};
            int readCount = sscanf(line + 2, "%f %f %f %f %f %f", fv + 0, fv + 1, fv + 2, fv + 3, fv + 4, fv + 5);

            if (fv[0] < minB[0])
                minB[0] = fv[0];
            if (fv[0] > maxB[0])
                maxB[0] = fv[0];

            if (fv[1] < minB[1])
                minB[1] = fv[1];
            if (fv[1] > maxB[1])
                maxB[1] = fv[1];

            if (fv[2] < minB[2])
                minB[2] = fv[2];
            if (fv[2] > maxB[2])
                maxB[2] = fv[2];

            memcpy(&fvVerts[fvc++], fv, sizeof(fv));

            // TODO pretty sure there's some way to get normals from
        }

        if (line[0] == 'v' && line[1] == 't')
        {
            CHECK_ARR(uvs, uvc, uvSize, sizeof(uv_t));
            // Texture UV
            float uvIn[3] = {0};
            int readCount = sscanf(line + 3, "%f %f %f", uvIn + 0, uvIn + 1, uvIn + 2);

            memcpy(&uvs[uvc++], uvIn, sizeof(uv_t));
        }

        if (line[0] == 'v' && line[1] == 'n')
        {
            // Texture Normal
            static bool warned = false;
            if (!warned)
            {
                warned = true;
                printf("obj_processor.c: Warning: Unsupported line 'vn' in %s\n", get_filename(infile));
            }
        }

        /* Read face line */
        if (line[0] == 'f')
        {
            // Make sure the ivS array has enough room for another one
            CHECK_ARR(ivS, ivc, ivSize, sizeof(iv_t));

            // try to scan for face/texture/normal, then face/texture, then just face

            // vertex IDs
            int vv[4];

            // We need to support any of these formats
            // f v1 v2 v3 ...
            // f v1//vn1 v2//vn2 v3//vn3 ...
            // f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3 ...
            // Set the position to right at the first value
            const char* cur = line + 1;
            int vertFaces   = 0;
            int fieldNum    = 0;

            int vt[4]    = {0};
            int textures = 0;

            int vn[4]   = {0};
            int normals = 0;

            while (0 != *cur && '\n' != *cur && vertFaces < 4)
            {
                // Skip whitespace
                if (*cur == ' ')
                {
                    fieldNum = 0;
                    // Skip any more whitespace
                    while (*++cur == ' ')
                        ;
                }

                if (*cur == 'v')
                {
                    cur++;
                    continue;
                }

                if (*cur == '/')
                {
                    // Vert/Texture/Normal separator
                    cur++;
                    fieldNum++;
                    continue;
                }

                if ('0' <= *cur && *cur <= '9')
                {
                    // Number found, save it
                    int val = atoi(cur);
                    if (val == 0)
                    {
                        printf("obj_processor.c: Bad int value in line '%s': ---> '%s'\n", line, cur);
                    }
                    else
                    {
                        if (fieldNum == 0)
                        {
                            // Vert index
                            vv[vertFaces++] = val;
                        }
                        else if (fieldNum == 1)
                        {
                            // Texture index
                            vt[textures++] = val;
                        }
                        else if (fieldNum == 2)
                        {
                            // Normal index
                            vn[normals++] = val;
                        }
                    }

                    // either way, advance past the number
                    while ('0' <= *cur && *cur <= '9')
                    {
                        ++cur;
                    }

                    continue;
                }
            }

            if (vertFaces == 3 || vertFaces == 4)
            {
                if ((ivc + (vertFaces - 2)) > UINT16_MAX)
                {
                    fprintf(stderr, "obj_processor.c: ERR! Object too large with >= %d faces\n", UINT16_MAX);
                }

                // read values are 1-indexed, decrement to make them 0-indexed
                vv[0]--;
                vv[1]--;
                vv[2]--;

                if (vertFaces == 4)
                {
                    static bool warned = false;
                    if (!warned)
                    {
                        printf("obj_processor.c: WARN: Triangulating face with 4 vertices in %s, probably poorly! "
                               "Maybe triangulate it?\n",
                               get_filename(infile));
                    }
                    // Special case: We will split the face into 2 triangles here
                    // Wikipedia tells of a "slow" algorithm that can triangulate arbitrary faces...

                    // Make the fourth vertex index 0-indexed as well
                    vv[3]--;

                    // vv[0, 2, 3] will cover the other half of the quad face
                    // TODO woops, does this work for a concave quad?
                    ivS[ivc][0] = vv[0];
                    ivS[ivc][1] = vv[2];
                    ivS[ivc][2] = vv[3];

                    // And now we need to check the buffer size again, for the next vert
                    CHECK_ARR(ivS, ++ivc, ivSize, sizeof(iv_t));
                }

                // Copy the vert to the new one
                memcpy(ivS[ivc++], vv, sizeof(vv));
            }
            else if (vertFaces > 4)
            {
                printf("obj_processor.c: Ignoring face with >4 verts: '%s'\n", line);
                // go to next line
                continue;
            }
            else
            {
                printf("obj_processor.c: Ignoring face with %d verts: '%s'\n", vertFaces, line);
            }

            if (textures > 0)
            {
                // Add the UV texture indices for the triangle...
                vt[0]--;
                vt[1]--;
                vt[2]--;
                CHECK_ARR(triUvs, triUvc, triUvSize, sizeof(triUv_t));
                memcpy(triUvs[triUvc++], vt, sizeof(vt));
            }
            else if (triUvc > 0)
            {
                printf("obj_processor.c: !!!! There are no UVs on triangle number %d, but there were already %d with "
                       "UVs\n",
                       ivc, triUvc);
            }

            if (curMtl != -1)
            {
                uint8_t mtlValue = (curMtl & 0xFF);
                CHECK_ARR(triMtls, triMtlc, triMtlSize, sizeof(uint8_t));
                triMtls[triMtlc++] = mtlValue;
            }
        }

        /* Read line entry */
        if (line[0] == 'l')
        {
            CHECK_ARR(lines, lnc, lnSize, sizeof(ln_t));

            ln_t ln;
            sscanf(line + 2, "%d %d", &ln[0], &ln[1]);

            // Convert to 0-indexed
            ln[0]--;
            ln[1]--;

            memcpy(lines[lnc++], ln, sizeof(ln));
        }

        if (line[0] == 'm' && !strncmp(line, "mtllib", 6))
        {
            char* mtllib = line + 7;
            char* end    = mtllib;
            while (*end && *end != '\n')
            {
                end++;
            }

            // don't add 1, because we have to add the NUL terminator ourselves
            if (mtlLibName != NULL)
            {
                printf("obj_processor.c: WARN: Multiple mtllib declarations in file!\n");
            }

            int len    = end - mtllib;
            mtlLibName = malloc(len + 1);
            memcpy(mtlLibName, mtllib, len);
            mtlLibName[len] = '\0';
        }

        if (line[0] == 'u' && !strncmp(line, "usemtl", 6))
        {
            char* mtlname = line + 7;
            char* end     = mtlname;
            while (*end && *end != '\n')
            {
                end++;
            }

            // Search for the material in the index
            mtl_t* newMtl = NULL;
            for (int m = 0; m < mtlc; m++)
            {
                if (!strncmp(mtls[m].name, mtlname, strlen(mtls[m].name)))
                {
                    newMtl = &mtls[m];
                    break;
                }
            }

            // Not found in the index, add it!
            if (newMtl == NULL)
            {
                // Gotta add a new material to the entry
                CHECK_ARR(mtls, mtlc, mtlSize, sizeof(mtl_t));

                newMtl        = &(mtls[mtlc++]);
                newMtl->index = (mtlc - 1);
                newMtl->name  = malloc(end - mtlname + 1);
                memcpy(newMtl->name, mtlname, end - mtlname);
                newMtl->name[end - mtlname] = '\0';

                printf("New material: %s\n", newMtl->name);
            }

            // And set it to the current material
            curMtl = newMtl->index;
        }

        /* Ignore comments and also everything else */
    }

    DEBUG_PRINT("Read %d vertices and %d faces from obj\n", fvc, ivc);

    int iAliasedVert[fvc];

    // float mergeThresh = 0.0001;
    float mergeThresh = -1;

    // the final list of verts
    int cvsize       = BUFLEN_DEFAULT;
    float* compverts = malloc(cvsize * sizeof(float) * 3);
    int cvct         = 0;
    int i, j;
    for (i = 0; i < fvc; i++)
    {
        float* fvcr = fvVerts[i];

        // Look for any previous verts that overlap this one close enough
        for (j = 0; j < cvct; j++)
        {
            float dx   = fvcr[0] - compverts[j * 3 + 0];
            float dy   = fvcr[1] - compverts[j * 3 + 1];
            float dz   = fvcr[2] - compverts[j * 3 + 2];
            float diff = sqrtf(dx * dx + dy * dy + dz * dz);
            if (diff < mergeThresh)
            {
                // Match found!
                break;
            }
        }
        if (j == cvct)
        {
            // No match found - new one!
            CHECK_ARR(compverts, cvct, cvsize, sizeof(float) * 3);
            memcpy(&compverts[3 * cvct++], fvcr, sizeof(float) * 3);
            /*DEBUG_PRINT("Final vert #%d: (%.2f, %.2f, %.2f) --> (%d, %d, %d)\n", cvct - 1,
                        fvVerts[j][0], fvVerts[j][1], fvVerts[j][2],
                        compverts[(cvct - 1) * 3], compverts[(cvct - 1) * 3 + 1], compverts[(cvct - 1) * 3 + 2]);*/
        }
        iAliasedVert[i] = j;
    }

    DEBUG_PRINT("Combined %d vertices, final number is %d vertices\n", fvc - cvct, cvct);

    if (-minB[0] > maxextent)
        maxextent = -minB[0];
    if (-minB[1] > maxextent)
        maxextent = -minB[1];
    if (-minB[2] > maxextent)
        maxextent = -minB[2];
    if (maxB[0] > maxextent)
        maxextent = maxB[0];
    if (maxB[1] > maxextent)
        maxextent = maxB[1];
    if (maxB[2] > maxextent)
        maxextent = maxB[2];

    // float scale  = 512; // 127.9 / maxextent;
    float scale[3] = {1, 1, -1};
    float uscale   = 1;
    float vscale   = 1;

    int32_t minBounds[3]
        = {(int32_t)(minB[0] * scale[0]), (int32_t)(minB[1] * scale[1]), (int32_t)(minB[2] * scale[2])};
    int32_t maxBounds[3]
        = {(int32_t)(maxB[0] * scale[0]), (int32_t)(maxB[1] * scale[1]), (int32_t)(maxB[2] * scale[2])};

    // Allocate raw file buffer
    // V1:
    // Structure: 2 bytes version (0x00 0x01)
    //            2 bytes #verts
    //            2 bytes #tris
    //            2 bytes #lines
    //                verts 3B each (int8 x 3)
    //                triVerts: {2B v0, 2B v1, 2B v2}
    //                triCols: { 1B Color (uint8 palette) }
    //                lines: {2B v0, 2B v1}
    ////////////////////////////////////////////////////////
    // V2:
    // Structure: 2 bytes version (0x00 0x02)
    //            2 bytes #verts
    //            2 bytes #uvs
    //            2 bytes #tris
    //            1 byte #mtls
    //            3 bytes padding
    //                min bounds 12B (int32 x 3)
    //                max bounds 12B (int32 x 3)
    //                verts 12B each (int32 x 3)
    //                uvs 8B each (int32 x 2)
    //                triVerts: {2B v0, 2B v1, 2B v2} * triCount
    //                triCols: { 1B Color (uint8 palette) } * triCount, if not uvs
    //                triUvs: {2B v0, 2B v1, 2B v2} * triCount, if uvs
    //                triMats: {1B mtl # } * triCount, if #mtls
    //                mtllib name { null-terminated string }, if #mtls
    //                mats: { null-terminated string } (strings continue immediately after previous NUL), if #mtls

    // face verts/colors
    uint16_t ivR[ivc][4];

    for (i = 0; i < ivc; i++)
    {
        int i0 = ivS[i][0];
        int i1 = ivS[i][1];
        int i2 = ivS[i][2];

        uint16_t* face = ivR[i];
        face[0]   = iAliasedVert[i0];
        face[1]   = iAliasedVert[i1];
        face[2]   = iAliasedVert[i2];

        float* color = &fvVerts[i0][3];
        if (color[2] >= 0.0)
        {
            int fc = ((int)(color[2] * 5.9)) + ((int)(color[1] * 5.9)) * 6 + ((int)(color[0] * 5.9)) * 36;

            face[3] = fc;
        }
        else
        {
            // 0xFF isn't a valid color anyway
            face[3] = 0xFF;
        }
    }

    int strSizes = 0;
    if (mtlLibName != NULL)
    {
        strSizes += strlen(mtlLibName) + 1;
    }

    for (i = 0; i < mtlc; i++)
    {
        strSizes += strlen(mtls[i].name) + 1;
    }

    int triCols = (ivR[0][3] > -1) ? (ivc) : 0;

    int outBufSize = 2             // Version
                     + 2           // vert count
                     + 2           // UV count
                     + 2           // tri count
                     + 1           // material count
                     + 3           /* padding */
                     + 24          // min/max bounds
                     + (cvct * 12) // verts (deduplicated)
                     + (uvc * 8)   // UVs count
                     + 3           /* 3 is max padding */
                     + ivc * 6     // triangles
                     + triCols * 1     // triangle colors
                     + triUvc * 6  // triangle UVs
                     + triMtlc     // triangle materials
                     // ??? + (cvct * 2) // vertex
                     /* + lnc * 4*/ // (V1) lines
                     + strSizes;    // mtllib, usemtl index
    uint8_t* outBuf = malloc(outBufSize);
    uint8_t* bp     = outBuf;

    // Write version
    *bp++ = 0;
    *bp++ = 2;

    // Write number of verts
    *bp++ = (cvct >> 8) & 0xFF;
    *bp++ = cvct & 0xFF;

    // V2: Write number of UVs
    *bp++ = (uvc >> 8) & 0xFF;
    *bp++ = uvc & 0xFF;

    // Write number of tris
    *bp++ = (ivc >> 8) & 0xFF;
    *bp++ = ivc & 0xFF;

    // Write number of MTLs
    *bp++ = (mtlc)&0xFF;

    // Padding
    *bp++ = 0;
    *bp++ = 0;
    *bp++ = 0;

    // Bounds
    *bp++ = (minBounds[0] >> 24) & 0xFF;
    *bp++ = (minBounds[0] >> 16) & 0xFF;
    *bp++ = (minBounds[0] >> 8) & 0xFF;
    *bp++ = (minBounds[0]) & 0xFF;

    *bp++ = (minBounds[1] >> 24) & 0xFF;
    *bp++ = (minBounds[1] >> 16) & 0xFF;
    *bp++ = (minBounds[1] >> 8) & 0xFF;
    *bp++ = (minBounds[1]) & 0xFF;

    *bp++ = (minBounds[2] >> 24) & 0xFF;
    *bp++ = (minBounds[2] >> 16) & 0xFF;
    *bp++ = (minBounds[2] >> 8) & 0xFF;
    *bp++ = (minBounds[2]) & 0xFF;

    *bp++ = (maxBounds[0] >> 24) & 0xFF;
    *bp++ = (maxBounds[0] >> 16) & 0xFF;
    *bp++ = (maxBounds[0] >> 8) & 0xFF;
    *bp++ = (maxBounds[0]) & 0xFF;

    *bp++ = (maxBounds[1] >> 24) & 0xFF;
    *bp++ = (maxBounds[1] >> 16) & 0xFF;
    *bp++ = (maxBounds[1] >> 8) & 0xFF;
    *bp++ = (maxBounds[1]) & 0xFF;

    *bp++ = (maxBounds[2] >> 24) & 0xFF;
    *bp++ = (maxBounds[2] >> 16) & 0xFF;
    *bp++ = (maxBounds[2] >> 8) & 0xFF;
    *bp++ = (maxBounds[2]) & 0xFF;

#ifdef DEBUG_OBJ
    printf("\nint8_t verts[] = {\n");

    for (i = 0; i < cvct; i++)
    {
        printf("\t%d, %d, %d,\n", (int8_t)(compverts[i * 3 + 0] * scale[0]), (int8_t)(compverts[i * 3 + 1] * scale[1]),
               (int8_t)(compverts[i * 3 + 2] * scale[2]));
    }
    printf("};\nuint8_t tris[] = {\n");
    for (i = 0; i < ivc; i++)
    {
        printf("\t%d, %d, %d, %d,\n", ivR[i][0], ivR[i][1], ivR[i][2], ivR[i][3]);
    }
    printf("};\n");
#endif

    // Write verts
    for (i = 0; i < cvct; i++)
    {
        int32_t outVert[3];

        outVert[0] = (int32_t)(compverts[i * 3 + 0] * scale[0]);
        outVert[1] = (int32_t)(compverts[i * 3 + 1] * scale[1]);
        outVert[2] = (int32_t)(compverts[i * 3 + 2] * scale[2]);

        *bp++ = (outVert[0] >> 24) & 0xFF;
        *bp++ = (outVert[0] >> 16) & 0xFF;
        *bp++ = (outVert[0] >> 8) & 0xFF;
        *bp++ = outVert[0] & 0xFF;

        *bp++ = (outVert[1] >> 24) & 0xFF;
        *bp++ = (outVert[1] >> 16) & 0xFF;
        *bp++ = (outVert[1] >> 8) & 0xFF;
        *bp++ = outVert[1] & 0xFF;

        *bp++ = (outVert[2] >> 24) & 0xFF;
        *bp++ = (outVert[2] >> 16) & 0xFF;
        *bp++ = (outVert[2] >> 8) & 0xFF;
        *bp++ = outVert[2] & 0xFF;
    }
    DEBUG_BUF(outBuf, bp);

    // V2: Write UVs
    for (i = 0; i < uvc; i++)
    {
        int32_t outUv[2];
        outUv[0] = (int32_t)(uvs[i][0] * uscale);
        outUv[1] = (int32_t)(uvs[i][1] * vscale);

        *bp++ = (outUv[0] >> 24) & 0xFF;
        *bp++ = (outUv[0] >> 16) & 0xFF;
        *bp++ = (outUv[0] >> 8) & 0xFF;
        *bp++ = outUv[0] & 0xFF;

        *bp++ = (outUv[1] >> 24) & 0xFF;
        *bp++ = (outUv[1] >> 16) & 0xFF;
        *bp++ = (outUv[1] >> 8) & 0xFF;
        *bp++ = outUv[1] & 0xFF;
    }

    bool printTris = false;
    if (!strcmp(get_filename(infile), "levelModel.obj"))
    {
        printTris = true;
    }

    // Write triangle vertexes
    for (i = 0; i < ivc; i++)
    {
        //if (printTris)
        //{
            //printf("ivR[i][0] >> 8 & 0xFF == %u\n", (ivR[i][0] >> 8) & 0xFF);
            //printf("ivR[i][0] & 0xFF == %u\n", (ivR[i][0]) & 0xFF);
        //}

        *bp++ = (ivR[i][0] >> 8) & 0xFF;
        *bp++ = ivR[i][0] & 0xFF;

        *bp++ = (ivR[i][1] >> 8) & 0xFF;
        *bp++ = ivR[i][1] & 0xFF;
        *bp++ = (ivR[i][2] >> 8) & 0xFF;
        *bp++ = ivR[i][2] & 0xFF;

        printf("ivR[%d] = { %u, %u, %u } --> %X %X %X %X %X %X\n", i, ivR[i][0], ivR[i][1], ivR[i][2], *(bp-6), *(bp-5), *(bp-4), *(bp-3), *(bp-2), *(bp-1));
    }

    if (uvc == 0 && ivR[0][3] != 0xFF)
    {
        // Write triangle colors
        for (i = 0; i < triCols; i++)
        {
            *bp++ = (uint8_t)(ivR[i][3] & 0xFF);
        }
    }

    // V2: Write triangle UVs (if UVs)
    if (uvc > 0)
    {
        for (i = 0; i < triUvc; i++)
        {
            uint16_t outUvt[3] = {0};

            outUvt[0] = triUvs[i][0];
            outUvt[1] = triUvs[i][1];
            outUvt[2] = triUvs[i][2];

            *bp++ = (outUvt[0] >> 8) & 0xFF;
            *bp++ = outUvt[0] & 0xFF;
            *bp++ = (outUvt[1] >> 8) & 0xFF;
            *bp++ = outUvt[1] & 0xFF;
            *bp++ = (outUvt[2] >> 8) & 0xFF;
            *bp++ = outUvt[2] & 0xFF;
        }
    }

    DEBUG_BUF(outBuf, bp);
    // V2: Write Triangle Materials (if materials)
    for (i = 0; i < triMtlc; i++)
    {
        *bp++ = triMtls[i];
    }

    DEBUG_BUF(outBuf, bp);
    // Write mtllib name referenced, or NULL if none
    if (mtlLibName != NULL)
    {
        for (char* cur = mtlLibName; *cur; cur++)
        {
            *bp++ = *cur;
        }
        *bp++ = '\0';
    }
    else
    {
        *bp++ = '\0';
    }

    DEBUG_BUF(outBuf, bp);
    for (i = 0; i < mtlc; i++)
    {
        for (char* cur = mtls[i].name; *cur; cur++)
        {
            *bp++ = *cur;
        }
        *bp++ = '\0';
    }

    // And that's it! We wrote a file!

    // V1: Write lines
    /*for (i = 0; i < lnc; i++)
    {
        *bp++ = (lines[i][0] >> 8) & 0xFF;
        *bp++ = lines[i][0] & 0xFF;
        *bp++ = (lines[i][1] >> 8) & 0xFF;
        *bp++ = lines[i][1] & 0xFF;
    }*/

    printf("wrote %d uncompressed bytes for model %s (predicted was %d)\n", (int)(bp - outBuf), get_filename(infile),
           outBufSize);

    DEBUG_PRINT("Writing compressed model to %s\n", outFilePath);
    // Write compressed output file

    DEBUG_BUF(outBuf, bp);
    writeHeatshrinkFile(outBuf, outBufSize, outFilePath);

    // TODO: free everything? lol
    free(fvVerts);
    free(ivS);
    free(lines);
    free(uvs);
    free(triUvs);
    for (i = 0; i < mtlc; i++)
    {
        free(mtls[i].name);
    }
    free(mtls);
    free(triMtls);
    free(mtlLibName);
}