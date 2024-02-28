#include "model.h"

#include "heatshrink_helper.h"
#include "small3dlib.h"

#include <esp_heap_caps.h>
#include <malloc.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define READ_16(buf, off) ((((buf)[off]) << 8) | (((buf)[off + 1])))
#define READ_32(buf, off) (((READ_16(buf, (off + 2))) << 16) | (READ_16(buf, off)))

bool loadModel(const char* name, model_t* model, bool useSpiRam)
{
    uint32_t decompressedSize = 0;
    uint8_t* decompressedBuf = readHeatshrinkFile(name, &decompressedSize, useSpiRam);

    if (NULL == decompressedBuf)
    {
        return false;
    }

    // Save the decompressed info into the model struct
    // The first 2 bytes are version, currently unused
    uint16_t version = (decompressedBuf[0] << 8) | decompressedBuf[1];

    if (version != 1)
    {
        return false;
    }

    // Next 2 bytes are vertex count
    uint16_t vertCount = (decompressedBuf[2] << 8) | decompressedBuf[3];

    // Next 2 bytes are face count
    uint16_t triCount = (decompressedBuf[4] << 8) | decompressedBuf[5];

    // Next 2 bytes are line count
    uint16_t lineCount = (decompressedBuf[6] << 8) | decompressedBuf[7];

    // Then, the verts are immediately after the triCount
    uint16_t vertOffset = 8;

    // Then, triangles are immediately vertexes, padded to a 4-byte offset
    uint16_t triOffset = vertOffset + 3 * vertCount;

    // Then, triangle colors are after those
    uint16_t triColOffset = (triOffset + 6 * triCount);

    // Then, lines are last
    uint16_t lineOffset = triColOffset + triCount;

    model->vertCount = vertCount;
    model->triCount = triCount;
    model->lineCount = lineCount;

    if (useSpiRam)
    {
        model->verts = heap_caps_malloc(sizeof(modelVert_t) * vertCount, MALLOC_CAP_SPIRAM);
        model->tris = (triCount > 0) ? heap_caps_malloc(sizeof(modelTri_t) * triCount, MALLOC_CAP_SPIRAM) : NULL;
        model->lines = (lineCount > 0) ? heap_caps_malloc(sizeof(modelLine_t) * lineCount, MALLOC_CAP_SPIRAM) : NULL;
    }
    else
    {
        model->verts = malloc(sizeof(modelVert_t) * vertCount);
        model->tris = (triCount > 0) ? malloc(sizeof(modelTri_t) * triCount) : NULL;
        model->lines = (lineCount > 0) ? malloc(sizeof(modelLine_t) * lineCount) : NULL;
    }

    uint16_t i;

    // Write all the verts into the struct
    for (i = 0; i < vertCount; i++)
    {
        model->verts[i][0] = (int8_t)decompressedBuf[vertOffset + i * sizeof(modelVert_t)];
        model->verts[i][1] = (int8_t)decompressedBuf[vertOffset + i * sizeof(modelVert_t) + 1];
        model->verts[i][2] = (int8_t)decompressedBuf[vertOffset + i * sizeof(modelVert_t) + 2];
    }

    // Write all the tris into the struct
    for (i = 0; i < triCount; i++)
    {
        model->tris[i].verts[0] = decompressedBuf[triOffset + i * 6] << 8 | decompressedBuf[triOffset + i * 6 + 1];
        model->tris[i].verts[1] = decompressedBuf[triOffset + i * 6 + 2] << 8 | decompressedBuf[triOffset + i * 6 + 3];
        model->tris[i].verts[2] = decompressedBuf[triOffset + i * 6 + 4] << 8 | decompressedBuf[triOffset + i * 6 + 5];
        model->tris[i].color = (paletteColor_t)decompressedBuf[triColOffset + i];
    }

    for (i = 0; i < lineCount; i++)
    {
        model->lines[i][0] = decompressedBuf[lineOffset + i * 4] << 8 | decompressedBuf[lineOffset + i * 4 + 1];
        model->lines[i][1] = decompressedBuf[lineOffset + i * 4 + 2] << 8 | decompressedBuf[lineOffset + i * 4 + 3];
    }

    free(decompressedBuf);

    return true;
}

bool loadObjInfo(const char* name, object3dInfo_t* objInfo, bool useSpiRam)
{
    const uint32_t caps = useSpiRam ? MALLOC_CAP_SPIRAM : MALLOC_CAP_DEFAULT;
    uint32_t decompressedSize = 0;
    uint8_t* decompressedBuf = readHeatshrinkFile(name, &decompressedSize, useSpiRam);

    if (NULL == decompressedBuf)
    {
        return false;
    }

    // Save the decompressed info into the model struct
    // The first 2 bytes are version, currently unused
    uint16_t version = (decompressedBuf[0] << 8) | decompressedBuf[1];

    if (version != 2)
    {
        return false;
    }

    // Next 2 bytes are vertex count
    uint16_t vertCount = (decompressedBuf[2] << 8) | decompressedBuf[3];

    // V2: Next 2 bytes are UV count
    uint16_t uvCount = (decompressedBuf[4] << 8) | decompressedBuf[5];

    // Next 2 bytes are face count
    uint16_t triCount = (decompressedBuf[6] << 8) | decompressedBuf[7];

    // Just one byte for material count
    uint8_t mtlCount = decompressedBuf[8];

    uint16_t minBoundsOffset = 12;
    uint16_t maxBoundsOffset = minBoundsOffset + 12;

    // Then, 3 bytes of padding before the verts
    uint16_t vertOffset = maxBoundsOffset + 12;

    // Then, UVs are immediately after verts
    uint16_t uvOffset = vertOffset + 12 * vertCount;

    // Then, triangles are immediately after UVs
    uint16_t triOffset = uvOffset + 8 * uvCount;

    // Then, triangle colors are after those
    uint16_t triColOffset = (triOffset + 6 * triCount);

    // Then, triangle UVs are next
    uint16_t triUvOffset = (triColOffset + triCount);

    // Decide how many triangle UVs and materials there are
    uint16_t triUvCount = (uvCount > 0) ? (triCount) : 0;
    uint16_t triMtlCount = (mtlCount > 0) ? (triCount) : 0;

    // Triangle materials
    uint16_t triMatOffset = (triUvOffset + 6 * triUvCount);

    // Materials themselves (and mtllib name)
    uint16_t materialOffset = (triMatOffset + triMtlCount);

    bool useUvs = (uvCount > 0);

    objInfo->vertCount = vertCount;
    objInfo->triCount = triCount;
    objInfo->uvCount = uvCount;
    objInfo->mtlCount = mtlCount;

    objInfo->minBounds[0] = READ_32(decompressedBuf, minBoundsOffset);
    objInfo->minBounds[1] = READ_32(decompressedBuf, minBoundsOffset + 4);
    objInfo->minBounds[2] = READ_32(decompressedBuf, minBoundsOffset + 8);

    objInfo->maxBounds[0] = READ_32(decompressedBuf, maxBoundsOffset);
    objInfo->maxBounds[1] = READ_32(decompressedBuf, maxBoundsOffset + 4);
    objInfo->maxBounds[2] = READ_32(decompressedBuf, maxBoundsOffset + 8);

    objInfo->verts = heap_caps_malloc(sizeof(int32_t) * 3 * vertCount, caps);
    objInfo->tris = (triCount > 0) ? heap_caps_malloc(sizeof(uint16_t) * 3 * triCount, caps) : NULL;
    objInfo->uvs = heap_caps_malloc(sizeof(int32_t) * 2 * uvCount, caps);
    objInfo->triUvs = heap_caps_malloc(sizeof(uint16_t) * 2 * triUvCount, caps);
    objInfo->triColors = (useUvs) ? NULL : heap_caps_malloc(sizeof(paletteColor_t) * triCount, caps);
    objInfo->triMtls = (mtlCount > 0) ? heap_caps_malloc(sizeof(uint8_t) * triCount, caps) : 0;

    objInfo->useUvs = useUvs;

    uint16_t i;

    // Write all the verts into the struct
    for (i = 0; i < vertCount; i++)
    {
        objInfo->verts[i * 3] = (int32_t)READ_32(decompressedBuf, vertOffset + i * 12);
        objInfo->verts[i * 3 + 1] = (int32_t)READ_32(decompressedBuf, vertOffset + i * 12 + 4);
        objInfo->verts[i * 3 + 2] = (int32_t)READ_32(decompressedBuf, vertOffset + i * 12 + 8);
    }

    // Write all the UVs into the struct
    for (i = 0; i < uvCount; i++)
    {
        objInfo->uvs[i * 2] = (int32_t)READ_32(decompressedBuf, uvOffset + i * 8);
        objInfo->uvs[i * 2 + 1] = (int32_t)READ_32(decompressedBuf, uvOffset + i * 8 + 4);
    }

    // Write all the tris into the struct
    for (i = 0; i < triCount; i++)
    {
        objInfo->tris[i * 3] = (uint16_t)READ_16(decompressedBuf, triOffset + i * 6);
        objInfo->tris[i * 3 + 1] = (uint16_t)READ_16(decompressedBuf, triOffset + i * 6 + 2);
        objInfo->tris[i * 3 + 2] = (uint16_t)READ_16(decompressedBuf, triOffset + i * 6 + 4);

        if (!useUvs)
        {
            objInfo->triColors[i] = (paletteColor_t)(decompressedBuf[triColOffset + i]);
        }
    }

    // Write all the tri UVs into the struct
    for (i = 0; i < triUvCount; i++)
    {
        objInfo->triUvs[i * 3] = (uint16_t)READ_16(decompressedBuf, triUvOffset + i * 6);
        objInfo->triUvs[i * 3 + 1] = (uint16_t)READ_16(decompressedBuf, triUvOffset + i * 6 + 2);
        objInfo->triUvs[i * 3 + 2] = (uint16_t)READ_16(decompressedBuf, triUvOffset + i * 6 + 4);
    }

    for (i = 0; i < triMtlCount; i++)
    {
        objInfo->triMtls[i] = (uint8_t)decompressedBuf[triMatOffset + i];
    }

    // Allocate an array of string pointers
    objInfo->mtlNames = calloc(mtlCount, sizeof(const char*));

    char* cur = (char*)(&decompressedBuf[materialOffset]);
    const char* end = (char*)(&decompressedBuf[decompressedSize]);
    int mtlsRead = 0;
    while (cur < end && mtlsRead < ((mtlCount) ? (mtlCount + 1) : (0)))
    {
        if (*cur)
        {
            char* nextNul = cur;
            // Advance to the next NUL character
            while (*nextNul != '\0')
            {
                nextNul++;
            }

            if (nextNul >= end)
            {
                printf("ERROR! Tried to read MTL filename past end of data (now we're gonna leak some memory)\n");
                return false;
            }

            size_t len = nextNul - cur + 1;
            // Make a copy of the string and add it to the array
            char* str = malloc(len);
            memcpy(str, cur, len);

            cur = nextNul + 1;

            if (mtlsRead == 0)
            {
                // This is the mtllib filename
                objInfo->mtlLibName = str;
            }
            else
            {
                objInfo->mtlNames[mtlsRead - 1] = str;
            }

            mtlsRead++;
        }
        else
        {
            cur++;
        }
    }

    free(decompressedBuf);

    return true;
}

void freeObjInfo(object3dInfo_t* info)
{
    if (info->mtlLibName != NULL)
    {
        free(info->mtlLibName);
        info->mtlLibName = NULL;
    }

    for (int i = 0; i < info->mtlCount; i++)
    {
        free(info->mtlNames[i]);
        info->mtlNames[i] = NULL;
    }

    if (info->mtlNames != NULL)
    {
        free(info->mtlNames);
        info->mtlNames = NULL;
    }

    if (info->verts)
    {
        free(info->verts);
        info->verts = NULL;
    }

    if (info->uvs)
    {
        free(info->uvs);
        info->uvs = NULL;
    }

    if (info->tris)
    {
        free(info->tris);
        info->tris = NULL;
    }

    if (info->triUvs)
    {
        free(info->triUvs);
        info->triUvs = NULL;
    }

    if (info->triColors)
    {
        free(info->triColors);
        info->triColors = NULL;
    }

    if (info->triMtls)
    {
        free(info->triMtls);
        info->triMtls = NULL;
    }
}

void freeModel(model_t* model)
{
    free(model->verts);

    if (NULL != model->tris)
    {
        free(model->tris);
    }

    if (NULL != model->lines)
    {
        free(model->lines);
    }
}