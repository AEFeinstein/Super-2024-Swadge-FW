//==============================================================================
// Includes
//==============================================================================

#include <string.h>
#include <esp_log.h>
#include <esp_heap_caps.h>
#include <esp_timer.h>
#include "linked_list.h"
#include "artillery_phys_bsp.h"
#include "artillery_phys_objs.h"

//==============================================================================
// Structs
//==============================================================================

/**
 * @brief A zone defined by an axis aligned bounding box, consisting of circles and lines
 */
typedef struct
{
    aabb_t aabb;    ///< An axis aligned bounding box for this zone
    list_t circles; ///< A list of all circles in this zone
    list_t lines;   ///< A list of all lines in this zone
} zone_t;

//==============================================================================
// Function Prototypes
//==============================================================================

static bool aabbIntersect(aabb_t* zone, aabb_t* obj);
static int cmpFloat(const void* a, const void* b);
static zone_t* createFirstZone(physSim_t* phys);
static vecFl_t findZoneMedian(zone_t* zone);
static void countMedianIntersections(zone_t* zone, vecFl_t median, int32_t* hCount, int32_t* vCount);
static void assignObjectsToZone(zone_t* src, zone_t* dst);
static void insertZoneSorted(list_t* zList, zone_t* zone);
static void setPhysObjsZones(physSim_t* phys);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Check if two axis aligned bounding boxes intersect
 *
 * @param zone An axis aligned bounding box
 * @param obj Another axis aligned bounding box
 * @return true if they intersect, false if they do not
 */
static bool aabbIntersect(aabb_t* zone, aabb_t* obj)
{
    return (zone->x0) <= (obj->x1) && //
           (zone->x1) >= (obj->x0) && //
           (zone->y0) <= (obj->y1) && //
           (zone->y1) >= (obj->y0);
}

/**
 * @brief Create the first zone which covers the entire space and contains all objects
 *
 * @param phys The physics simulation
 * @return The created zone. This must be heap_caps_free()'d later
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
 * @brief qsort comparator for two floating point numbers
 *
 * @param a A pointer to a floating point number
 * @param b Another pointer to a floating point number
 * @return Greater than 0 if a > b, less than zero if a < b, or zero if a == b
 */
static int cmpFloat(const void* a, const void* b)
{
    return *((const float*)a) - *((const float*)b);
}

/**
 * @brief Find the median point for all objects in a zone
 *
 * @param zone The zone to find the median point in
 * @return The median point for all objects in the zone
 */
static vecFl_t findZoneMedian(zone_t* zone)
{
    // Make lists of all X and Y points
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
        // Midpoint of a line is the average of the start and end points
        xVals[vIdx] = ((line->aabb.x0 + line->aabb.x1) / 2.0f);
        yVals[vIdx] = ((line->aabb.y0 + line->aabb.y1) / 2.0f);
        vIdx++;

        // Iterate
        lNode = lNode->next;
    }

    // Sort the points
    qsort(xVals, nPoints, sizeof(float), cmpFloat);
    qsort(yVals, nPoints, sizeof(float), cmpFloat);

    // Return the median
    // Note, this picks a lower median rather than averaging two points if nPoints is even
    return (vecFl_t){
        .x = xVals[nPoints / 2],
        .y = yVals[nPoints / 2],
    };
}

/**
 * @brief Count the number of objects that intersect with the horizontal and vertical medians
 *
 * A better split zone will have fewer objects intersecting the median
 *
 * @param zone The zone to count objects in
 * @param median The median point in the zone
 * @param hCount [OUT] The number of objects that intersect with median.x
 * @param vCount [OUT] The number of objects that intersect with median.y
 */
static void countMedianIntersections(zone_t* zone, vecFl_t median, int32_t* hCount, int32_t* vCount)
{
    // Set counts to zero
    (*hCount) = 0;
    (*vCount) = 0;

    // Check intersections for all circles
    node_t* cNode = zone->circles.first;
    while (cNode)
    {
        physCirc_t* circ = cNode->val;

        if (circ->aabb.x0 <= median.x && median.x <= circ->aabb.x1)
        {
            // median.x intersects the object's AABB
            (*hCount)++;
        }

        if (circ->aabb.y0 <= median.y && median.y <= circ->aabb.y1)
        {
            // median.y intersects the object's AABB
            (*vCount)++;
        }

        // Iterate
        cNode = cNode->next;
    }

    // Check intersections for all lines
    node_t* lNode = zone->lines.first;
    while (lNode)
    {
        physLine_t* line = (physLine_t*)lNode->val;

        if (line->aabb.x0 <= median.x && median.x <= line->aabb.x1)
        {
            // median.x intersects the object's AABB
            (*hCount)++;
        }

        if (line->aabb.y0 <= median.y && median.y <= line->aabb.y1)
        {
            // median.y intersects the object's AABB
            (*vCount)++;
        }

        // Iterate
        lNode = lNode->next;
    }
}

/**
 * @brief Assign all objects from a source zone to a destination zone if they intersect with the destination zone's AABB
 *
 * @param src The source zone of objects to check
 * @param dst The destination zone to add intersecting objects to
 */
static void assignObjectsToZone(zone_t* src, zone_t* dst)
{
    // Check all source circles
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

    // Check all source lines
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
 * @brief Insert a zone into a list of zones, sorting by number of objects, greatest to least
 *
 * @param zList A list of zones to insert into
 * @param zone A zone to insert into the list
 */
static void insertZoneSorted(list_t* zList, zone_t* zone)
{
    // Iterate through the list of zones
    node_t* zNode = zList->first;
    while (zNode)
    {
        zone_t* listZone = zNode->val;

        // If the zone in the list has fewer objects than the zone being inserted
        if ((listZone->circles.length + listZone->lines.length) < (zone->circles.length + zone->lines.length))
        {
            // Add the new zone before the existing zone and return
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
 * @brief Set the zone bitmasks for all objects in the simulation
 *
 * @param phys The physics simulation
 */
static void setPhysObjsZones(physSim_t* phys)
{
    // For each circle
    node_t* cNode = phys->circles.first;
    while (cNode)
    {
        physCirc_t* circ = cNode->val;
        updateCircleProperties(phys, circ);

        // Iterate
        cNode = cNode->next;
    }

    // For each line
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
 * @brief TODO doc
 *
 * @param phys The physics simulation
 */
void createBspZones(physSim_t* phys)
{
    int32_t tStart = esp_timer_get_time();

    // Make a list of zones
    list_t zList = {0};

    // Add the first zone to the zone list
    push(&zList, createFirstZone(phys));

    // Iterate until there are NUM_ZONES zones
    while (zList.length < NUM_ZONES)
    {
        // Always split the largest (i.e. first) zone
        zone_t* zone = shift(&zList);

        // Find the median of objects in the zone
        vecFl_t median = findZoneMedian(zone);

        // Count up objects divided by the median, both horizontally and vertically
        int32_t hCount = 0;
        int32_t vCount = 0;
        countMedianIntersections(zone, median, &hCount, &vCount);

        // Decide how to split the zone. You want the fewest objects intersecting the median
        bool splitHorz;
        if (hCount < vCount)
        {
            splitHorz = true;
        }
        else if (vCount < hCount)
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
    }

    // Convert list of zones to saved array
    int32_t zIdx = 0;
    while (zList.first)
    {
        zone_t* zone = zList.first->val;

        phys->zones[zIdx].pos.x  = zone->aabb.x0;
        phys->zones[zIdx].pos.y  = zone->aabb.y0;
        phys->zones[zIdx].width  = zone->aabb.x1 - zone->aabb.x0;
        phys->zones[zIdx].height = zone->aabb.y1 - zone->aabb.y0;
        zIdx++;

        heap_caps_free(shift(&zList));
    }

    // Reassign objects to zones
    setPhysObjsZones(phys);

    uint32_t tElapsed = esp_timer_get_time() - tStart;
    ESP_LOGI("PHS", "%" PRIu32 "us to BSP", tElapsed);
}
