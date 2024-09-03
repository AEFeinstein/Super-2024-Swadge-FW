#include <stdio.h>
#include "pinball_triangle.h"

/**
 * @brief TODO doc
 *
 * @param tableData
 * @param scene
 * @return uint32_t
 */
uint32_t readTriangleFromFile(uint8_t* tableData, jsScene_t* scene)
{
    jsTriangle_t* triangle = &scene->triangles[scene->numTriangles++];

    uint32_t dIdx     = 0;
    triangle->id      = readInt16(tableData, &dIdx);
    triangle->groupId = readInt8(tableData, &dIdx);
    triangle->group   = addToGroup(scene, triangle, triangle->groupId);
    triangle->p1.x    = readInt16(tableData, &dIdx);
    triangle->p1.y    = readInt16(tableData, &dIdx);
    triangle->p2.x    = readInt16(tableData, &dIdx);
    triangle->p2.y    = readInt16(tableData, &dIdx);
    triangle->p3.x    = readInt16(tableData, &dIdx);
    triangle->p3.y    = readInt16(tableData, &dIdx);

    triangle->blinkTimer = 0;
    triangle->isOn       = false;
    triangle->isBlinking = true;

    return dIdx;
}

/**
 * @brief TODO
 *
 * @param scene
 * @param dt
 */
void jsBlinkTriangles(jsScene_t* scene, float dt)
{
    for (int32_t tIdx = 0; tIdx < scene->numTriangles; tIdx++)
    {
        jsTriangle_t* tri = &scene->triangles[tIdx];
        if (tri->isBlinking)
        {
            tri->blinkTimer += dt;
            while (tri->blinkTimer > 0.33f)
            {
                tri->blinkTimer -= 0.33f;
                tri->isOn = !tri->isOn;
            }
        }
    }
}