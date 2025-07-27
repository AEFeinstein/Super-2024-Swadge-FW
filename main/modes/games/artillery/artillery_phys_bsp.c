//==============================================================================
// Includes
//==============================================================================

#include <string.h>
#include <esp_log.h>
#include <esp_heap_caps.h>
#include "linked_list.h"
#include "artillery_phys_bsp.h"
#include "artillery_phys_objs.h"

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    aabb_t aabb;
    list_t circles;
    list_t lines;
} zone_t;

//==============================================================================
// Function Prototypes
//==============================================================================

static inline bool aabbIntersectX(aabb_t* zone, aabb_t* obj);
static inline bool aabbIntersectY(aabb_t* zone, aabb_t* obj);
static bool aabbIntersect(aabb_t* zone, aabb_t* obj);

static zone_t* createFirstZone(physSim_t* phys);
static int cmpFloat(const void* a, const void* b);
static vecFl_t findZoneMedian(zone_t* zone);
static void countObjectSplits(zone_t* zone, vecFl_t median, int32_t* hCount, int32_t* vCount);
static void assignObjectsToZone(zone_t* src, zone_t* dst);
static void insertZoneSorted(list_t* zList, zone_t* zone);
static void setPhysObjsZones(physSim_t* phys);

//==============================================================================
// Functions
//==============================================================================

static bool aabbIntersect(aabb_t* zone, aabb_t* obj)
{
    return aabbIntersectX(zone, obj) && aabbIntersectY(zone, obj);
}

static inline bool aabbIntersectX(aabb_t* zone, aabb_t* obj)
{
    return (zone->x0) <= (obj->x1) && //
           (zone->x1) >= (obj->x0);
}

static inline bool aabbIntersectY(aabb_t* zone, aabb_t* obj)
{
    return (zone->y0) <= (obj->y1) && //
           (zone->y1) >= (obj->y0);
}

/**
 * @brief TODO
 *
 * @param phys
 * @return zone_t*
 */
static zone_t* createFirstZone(physSim_t* phys)
{
    // Create the first zone, which is all objects
    zone_t* zone  = heap_caps_calloc(1, sizeof(zone_t), MALLOC_CAP_SPIRAM);
    zone->aabb.x1 = phys->bounds.x;
    zone->aabb.y1 = phys->bounds.y;

    // Add all circles to the first zone
    node_t* cNode = phys->circles.first;
    while (cNode)
    {
        push(&zone->circles, cNode->val);
        // Iterate
        cNode = cNode->next;
    }

    // Add all lines to the first zone
    node_t* lNode = phys->lines.first;
    while (lNode)
    {
        push(&zone->lines, lNode->val);
        // Iterate
        lNode = lNode->next;
    }

    return zone;
}

/**
 * @brief TODO
 *
 * @param a
 * @param b
 * @return int
 */
static int cmpFloat(const void* a, const void* b)
{
    // Extract floats
    float fA = *((const float*)a);
    float fB = *((const float*)b);

    // Compare floats
    if (fA < fB)
    {
        return -1;
    }
    if (fA > fB)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

/**
 * @brief TODO
 *
 * @param zone
 * @return vecFl_t
 */
static vecFl_t findZoneMedian(zone_t* zone)
{
    // Make lists of all X an dY points
    int32_t vIdx    = 0;
    int32_t nPoints = zone->circles.length + zone->lines.length;
    float xVals[nPoints];
    float yVals[nPoints];

    // Add circle points to the list
    node_t* cNode = zone->circles.first;
    while (cNode)
    {
        physCirc_t* circ = cNode->val;
        xVals[vIdx]      = circ->c.pos.x;
        yVals[vIdx]      = circ->c.pos.y;
        vIdx++;

        // Iterate
        cNode = cNode->next;
    }

    // Add line points ot the list
    node_t* lNode = zone->lines.first;
    while (lNode)
    {
        physLine_t* line = (physLine_t*)lNode->val;
        xVals[vIdx]      = ((line->aabb.x0 + line->aabb.x1) / 2.0f);
        yVals[vIdx]      = ((line->aabb.y0 + line->aabb.y1) / 2.0f);
        vIdx++;

        // Iterate
        lNode = lNode->next;
    }

    // Sort the points
    qsort(xVals, nPoints, sizeof(float), cmpFloat);
    qsort(yVals, nPoints, sizeof(float), cmpFloat);

    // Return the median
    return (vecFl_t){
        .x = xVals[nPoints / 2],
        .y = yVals[nPoints / 2],
    };
}

/**
 * @brief TODO
 *
 * Something seems broken here. What's the best way to determine split? objects not on the median line? nah. Balance
 * of objects to left and right? probably
 *
 * @param zone
 * @param median
 * @param hCount
 * @param vCount
 */
static void countObjectSplits(zone_t* zone, vecFl_t median, int32_t* hCount, int32_t* vCount)
{
    (*hCount) = 0;
    (*vCount) = 0;

    // Sum medians of all circles
    node_t* cNode = zone->circles.first;
    while (cNode)
    {
        physCirc_t* circ = cNode->val;

        if (!(circ->aabb.x0 <= median.x && median.x <= circ->aabb.x1))
        {
            // Midpoint does not split the objects box
            (*hCount)++;
        }

        if (!(circ->aabb.y0 <= median.y && median.y <= circ->aabb.y1))
        {
            // Midpoint does not split the objects box
            (*vCount)++;
        }

        // Iterate
        cNode = cNode->next;
    }

    // Sum medians of all lines
    node_t* lNode = zone->lines.first;
    while (lNode)
    {
        physLine_t* line = (physLine_t*)lNode->val;

        if (!(line->aabb.x0 <= median.x && median.x <= line->aabb.x1))
        {
            // Midpoint does not split the objects box
            (*hCount)++;
        }

        if (!(line->aabb.y0 <= median.y && median.y <= line->aabb.y1))
        {
            // Midpoint does not split the objects box
            (*vCount)++;
        }

        // Iterate
        lNode = lNode->next;
    }
}

/**
 * @brief TODO
 *
 * @param src
 * @param dst
 */
static void assignObjectsToZone(zone_t* src, zone_t* dst)
{
    node_t* cNode = src->circles.first;
    while (cNode)
    {
        // Add to the list if there is an AABB intersection
        physCirc_t* circ = cNode->val;
        if (aabbIntersect(&dst->aabb, &circ->aabb))
        {
            push(&dst->circles, circ);
        }

        // Iterate
        cNode = cNode->next;
    }

    // Sum medians of all lines
    node_t* lNode = src->lines.first;
    while (lNode)
    {
        // Add to the list if there is an AABB intersection
        physLine_t* line = (physLine_t*)lNode->val;
        if (aabbIntersect(&dst->aabb, &line->aabb))
        {
            push(&dst->lines, line);
        }

        // Iterate
        lNode = lNode->next;
    }
}

/**
 * @brief TODO
 *
 * @param zList
 * @param zone
 */
static void insertZoneSorted(list_t* zList, zone_t* zone)
{
    node_t* zNode = zList->first;
    while (zNode)
    {
        zone_t* listZone = zNode->val;

        // If the zone in the list has fewer objects than the zone being inserted
        if ((listZone->circles.length + listZone->lines.length) < (zone->circles.length + zone->lines.length))
        {
            // Add the new zone before the existing zone
            addBefore(zList, zone, zNode);
            return;
        }

        // Iterate
        zNode = zNode->next;
    }

    // In case this hasn't returned yet, add it to the end
    push(zList, zone);
}

/**
 * @brief TODO doc
 *
 * @param phys
 */
static void setPhysObjsZones(physSim_t* phys)
{
    node_t* cNode = phys->circles.first;
    while (cNode)
    {
        physCirc_t* circ = cNode->val;
        updateCircleProperties(phys, circ);

        // Iterate
        cNode = cNode->next;
    }

    // Add all lines to the first zone
    node_t* lNode = phys->lines.first;
    while (lNode)
    {
        physLine_t* line = lNode->val;
        updateLineProperties(phys, line);

        // Iterate
        lNode = lNode->next;
    }
}

/**
 * @brief TODO
 *
 * @param phys
 */
void createBspZones(physSim_t* phys)
{
    // Clear out zones
    memset(&phys->zones, 0, sizeof(phys->zones));

    // Make a list of zones
    list_t zList = {0};

    // Add the first zone to the zone list
    push(&zList, createFirstZone(phys));

    // Iterate until there are 32 zones
    while (zList.length < 32)
    {
        // Split the largest (i.e. first) zone
        zone_t* zone = shift(&zList);

        // Find the median of objects in the zone
        vecFl_t median = findZoneMedian(zone);
        // ESP_LOGI("BSP", "Mid: (%f, %f))", median.x, median.y);

        // Count up objects divided by the median, both horizontally and vertically
        int32_t hCount = 0;
        int32_t vCount = 0;
        countObjectSplits(zone, median, &hCount, &vCount);
        // ESP_LOGI("BSP", "H: %d, V: %d", hCount, vCount);

        // Decide how to split the zone
        bool splitHorz;
        if (hCount > vCount)
        {
            splitHorz = true;
        }
        else if (vCount > hCount)
        {
            // More items are divided along Y axis, split horizontally
            splitHorz = false;
        }
        else if ((zone->aabb.x1 - zone->aabb.x0) > (zone->aabb.y1 - zone->aabb.y0))
        {
            // Even divide, but wider zone, split vertically
            splitHorz = true;
        }
        else
        {
            // Even divide, but taller zone, split horizontally
            splitHorz = false;
        }

        // Create two new zones and add them to the list
        if (splitHorz)
        {
            // More items are split by the X axis
            zone_t* nzA  = heap_caps_calloc(1, sizeof(zone_t), MALLOC_CAP_SPIRAM);
            nzA->aabb    = zone->aabb;
            nzA->aabb.x1 = median.x;
            assignObjectsToZone(zone, nzA);
            insertZoneSorted(&zList, nzA);

            zone_t* nzB  = heap_caps_calloc(1, sizeof(zone_t), MALLOC_CAP_SPIRAM);
            nzB->aabb    = zone->aabb;
            nzB->aabb.x0 = median.x;
            assignObjectsToZone(zone, nzB);
            insertZoneSorted(&zList, nzB);
        }
        else
        {
            // More items are split by the Y axis
            zone_t* nzA  = heap_caps_calloc(1, sizeof(zone_t), MALLOC_CAP_SPIRAM);
            nzA->aabb    = zone->aabb;
            nzA->aabb.y1 = median.y;
            assignObjectsToZone(zone, nzA);
            insertZoneSorted(&zList, nzA);

            zone_t* nzB  = heap_caps_calloc(1, sizeof(zone_t), MALLOC_CAP_SPIRAM);
            nzB->aabb    = zone->aabb;
            nzB->aabb.y0 = median.y;
            assignObjectsToZone(zone, nzB);
            insertZoneSorted(&zList, nzB);
        }

        // Free the zone that was just split
        heap_caps_free(zone);

        // char dbg[512] = {0};
        // node_t* zNode = zList.first;
        // while (zNode)
        // {
        //     char tmp[32]    = {0};
        //     zone_t* dbgZone = zNode->val;
        //     sprintf(tmp, "%d, ", dbgZone->circles.length + dbgZone->lines.length);
        //     strcat(dbg, tmp);
        //     zNode = zNode->next;
        // }
        // ESP_LOGI("BSP", "Zone sizes: %s", dbg);
    }

    // Convert list of zones to saved array
    int32_t zIdx = 0;
    while (zList.first)
    {
        zone_t* zone = zList.first->val;
        // ESP_LOGI("BSP", "Zone size: %d", zone->circles.length + zone->lines.length);

        phys->zones[zIdx].pos.x  = zone->aabb.x0;
        phys->zones[zIdx].pos.y  = zone->aabb.y0;
        phys->zones[zIdx].width  = zone->aabb.x1 - zone->aabb.x0;
        phys->zones[zIdx].height = zone->aabb.y1 - zone->aabb.y0;
        zIdx++;

        heap_caps_free(shift(&zList));
    }

    // Reassign objects to zones
    setPhysObjsZones(phys);
}
