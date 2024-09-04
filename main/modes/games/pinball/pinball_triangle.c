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
 * @brief
 *
 * @param tri
 * @param elapsedUs
 */
void jsTriangleTimer(jsTriangle_t* tri, int32_t elapsedUs)
{
    if (tri->isBlinking)
    {
        RUN_TIMER_EVERY(tri->blinkTimer, 333333, elapsedUs, tri->isOn = !tri->isOn;);
    }
}
