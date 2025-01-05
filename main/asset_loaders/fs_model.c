#include "model.h"

#include "heatshrink_helper.h"

#include <esp_heap_caps.h>
#include <malloc.h>
#include <stdbool.h>
#include <stdint.h>

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