#include "shapes.h"
#include "palette.h"

#include "pinball_point.h"

/**
 * @brief TODO doc
 *
 * @param tableData
 * @param scene
 * @return int32_t
 */
int32_t readPointFromFile(uint8_t* tableData, jsScene_t* scene)
{
    jsPoint_t* point = &scene->points[scene->numPoints++];
    uint32_t dIdx    = 0;
    point->id        = readInt16(tableData, &dIdx);
    point->groupId   = readInt8(tableData, &dIdx);
    point->group     = addToGroup(scene, point, point->groupId);
    point->pos.x     = readInt16(tableData, &dIdx);
    point->pos.y     = readInt16(tableData, &dIdx);
    point->type      = readInt8(tableData, &dIdx);

    return dIdx;
}
