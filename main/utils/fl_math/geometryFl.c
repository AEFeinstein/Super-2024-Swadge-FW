#include <stddef.h>
#include <math.h>
#include "geometryFl.h"
#include "macros.h"

/**
 * @brief Check if two circles intersect
 *
 * @param circle1        [IN]  One circle to check for intersection, assumed to be moving
 * @param circle2        [IN]  The other circle to check for intersection, assumed to be fixed
 * @param collisionPoint [OUT] The point at which the two circles collide, on the circumference of circle2. May be NULL
 * @param collisionVec   [OUT] A vector pointing from the second circle to the first in the direction of the collision.
 * May be NULL.
 * @return true if the circles intersect or touch, false if they do not
 */
bool circleCircleFlIntersection(circleFl_t circle1, circleFl_t circle2, vecFl_t* collisionPoint, vecFl_t* collisionVec)
{
    float distX   = (circle1.pos.x - circle2.pos.x);
    float distY   = (circle1.pos.y - circle2.pos.y);
    float distRad = (circle1.radius + circle2.radius);
    // Compare distance between centers to the sum of the radii
    bool intersection = ((distX * distX) + (distY * distY)) <= (distRad * distRad);
    if (intersection)
    {
        if (NULL != collisionVec)
        {
            *collisionVec = subVecFl2d(circle1.pos, circle2.pos);
            if (NULL != collisionPoint)
            {
                *collisionPoint = addVecFl2d(circle2.pos, mulVecFl2d(normVecFl2d(*collisionVec), circle2.radius));
            }
        }
    }
    return intersection;
}

/**
 * @brief Check if a point and a circle intersect
 *
 * @param circle       [IN]  A circle to check for intersection
 * @param point        [IN]  A point to check for intersection
 * @param collisionVec [OUT] A vector pointing from the point to the circle in the direction of the collision. May be
 * NULL.
 * @return true if the point intersects or touches the circle, false if it doesn't
 */
bool circlePointFlIntersection(circleFl_t circle, vecFl_t point, vecFl_t* collisionVec)
{
    float distX = (circle.pos.x - point.x);
    float distY = (circle.pos.y - point.y);
    // Compare distance between centers to the radius
    bool intersection = ((distX * distX) + (distY * distY)) <= (circle.radius * circle.radius);
    if (intersection && (NULL != collisionVec))
    {
        *collisionVec = subVecFl2d(circle.pos, point);
    }
    return intersection;
}

/**
 * @brief Check if two rectangles intersect
 *
 * Adapted from https://www.jeffreythompson.org/collision-detection/rect-rect.php
 *
 * @param rect1        [IN]  One rectangle to check for intersection
 * @param rect2        [IN]  The other rectangle to check for intersection
 * @param collisionVec [OUT] A vector pointing from the second rectangle to the first in the direction of the collision.
 * May be NULL.
 * @return true if the rectangles intersect or touch, false if they do not
 */
bool rectRectFlIntersection(rectangleFl_t rect1, rectangleFl_t rect2, vecFl_t* collisionVec)
{
    // Check for intersection
    bool intersecting =
        // rect1 left edge, rect2 right edge
        (rect1.pos.x) < (rect2.pos.x + rect2.width) &&
        // rect1 right edge, rect2 left edge
        (rect1.pos.x + rect1.width) > (rect2.pos.x) &&
        // rect1 top edge, rect2 bottom edge
        (rect1.pos.y) < (rect2.pos.y + rect2.height) &&
        // rect1 bottom edge, rect2 top edge
        (rect1.pos.y + rect1.height) > (rect2.pos.y);

    // If there is an intersection
    if (intersecting && NULL != collisionVec)
    {
        // Find the collision vector
        vecFl_t mid1 = {
            .x = rect1.pos.x + (rect1.width / 2),
            .y = rect1.pos.y + (rect1.height / 2),
        };
        vecFl_t mid2 = {
            .x = rect2.pos.x + (rect2.width / 2),
            .y = rect2.pos.y + (rect2.height / 2),
        };
        vecFl_t delta = subVecFl2d(mid1, mid2);

        // Determine vector by absolute value
        if (ABS(delta.x) > ABS(delta.y))
        {
            collisionVec->x = delta.x;
            collisionVec->y = 0;
        }
        else
        {
            collisionVec->x = 0;
            collisionVec->y = delta.y;
        }
    }
    return intersecting;
}

/**
 * @brief Check if a circle and a rectangle intersect
 *
 * Adapted from https://www.jeffreythompson.org/collision-detection/circle-rect.php
 *
 * @param circle       [IN]  The circle to check for intersection
 * @param rect         [IN]  The rectangle to check for intersection
 * @param collisionVec [OUT] A vector pointing from the rectangle to the circle in the direction of the collision. May
 * be NULL.
 * @return true if the shapes intersect or touch, false if they do not
 */
bool circleRectFlIntersection(circleFl_t circle, rectangleFl_t rect, vecFl_t* collisionVec)
{
    // temporary variables to set edges for testing
    float testX = circle.pos.x;
    float testY = circle.pos.y;

    // which edge is closest?
    if (circle.pos.x < rect.pos.x)
    {
        // test left edge
        testX = rect.pos.x;
        if (NULL != collisionVec)
        {
            collisionVec->x = -1;
            collisionVec->y = 0;
        }
    }
    else if (circle.pos.x > rect.pos.x + rect.width)
    {
        // right edge
        testX = rect.pos.x + rect.width;
        if (NULL != collisionVec)
        {
            collisionVec->x = 1;
            collisionVec->y = 0;
        }
    }

    if (circle.pos.y < rect.pos.y)
    {
        // top edge
        testY = rect.pos.y;
        if (NULL != collisionVec)
        {
            collisionVec->x = 0;
            collisionVec->y = -1;
        }
    }
    else if (circle.pos.y > rect.pos.y + rect.height)
    {
        // bottom edge
        testY = rect.pos.y + rect.height;
        if (NULL != collisionVec)
        {
            collisionVec->x = 0;
            collisionVec->y = 1;
        }
    }

    // get distance from closest edges
    float distX    = circle.pos.x - testX;
    float distY    = circle.pos.y - testY;
    float distance = (distX * distX) + (distY * distY);

    // if the distance is less than the radius, collision!
    if (distance <= circle.radius * circle.radius)
    {
        return true;
    }
    return false;
}

/**
 * @brief Check if a circle and a line intersect
 *
 * Adapted from https://www.jeffreythompson.org/collision-detection/line-circle.php
 *
 * @param circle       [IN]  A circle to test intersection
 * @param line         [IN]  A line to test intersection
 * @param checkEnds    [IN]  True to check the points at the end of the line, false to ignore them
 * @param cpOnLine     [OUT] The closest point on the line to the circle
 * @param collisionVec [OUT] A vector pointing from the line to the circle in the direction of the collision. This may
 * be NULL.
 * @return true if the circle intersects the line, false if it doesn't
 */
bool circleLineFlIntersection(circleFl_t circle, lineFl_t line, bool checkEnds, vecFl_t* cpOnLine,
                              vecFl_t* collisionVec)
{
    if (checkEnds)
    {
        // Check for the line ends
        if (circlePointFlIntersection(circle, line.p1, collisionVec))
        {
            if (NULL != cpOnLine)
            {
                *cpOnLine = line.p1;
            }
            return true;
        }
        if (circlePointFlIntersection(circle, line.p2, collisionVec))
        {
            if (NULL != cpOnLine)
            {
                *cpOnLine = line.p2;
            }
            return true;
        }
    }

    // Get the length of the line
    vecFl_t lineLens = subVecFl2d(line.p2, line.p1);
    // Find a dot product
    float dot = dotVecFl2d(subVecFl2d(circle.pos, line.p1), lineLens);
    // Find the closest point on the line to the circle
    vecFl_t closestPoint = addVecFl2d(line.p1, divVecFl2d(mulVecFl2d(lineLens, dot), sqMagVecFl2d(lineLens)));

    // To validate a collision, the closest point must be in the bounding box of the line (i.e. on the line)
    // Create a bounding box for the line
    lineFl_t bb;
    // Assign x points
    if (line.p1.x < line.p2.x)
    {
        bb.p1.x = line.p1.x;
        bb.p2.x = line.p2.x;
    }
    else
    {
        bb.p1.x = line.p2.x;
        bb.p2.x = line.p1.x;
    }

    // Assign the y points
    if (line.p1.y < line.p2.y)
    {
        bb.p1.y = line.p1.y;
        bb.p2.y = line.p2.y;
    }
    else
    {
        bb.p1.y = line.p2.y;
        bb.p2.y = line.p1.y;
    }

    // If the closest point is outside the bounding box
    if (closestPoint.x < bb.p1.x || closestPoint.x > bb.p2.x || closestPoint.y < bb.p1.y || closestPoint.y > bb.p2.y)
    {
        // There is no collision
        return false;
    }

    // Get the distance from the circle to the closest point on the line
    vecFl_t closestDist = subVecFl2d(closestPoint, circle.pos);
    float distSqr       = sqMagVecFl2d(closestDist);
    float radiusSqr     = circle.radius * circle.radius;

    // If it's less than the radius, we've got a collision!
    if (distSqr < radiusSqr)
    {
        if (NULL != collisionVec)
        {
            *collisionVec = subVecFl2d(circle.pos, closestPoint);
        }
        if (NULL != cpOnLine)
        {
            *cpOnLine = closestPoint;
        }
        return true;
    }
    return false;
}

/**
 * @brief Find the points of intersection between a circle's circumference and a line. There may be zero (no
 * intersection), one (tangent), or two points of intersection.
 *
 * See https://cp-algorithms.com/geometry/circle-line-intersection.html
 *
 * @param circle [IN] The circle to check for intersection
 * @param line [IN] The line to check for intersection
 * @param intersection_1 [OUT] If it exists, the first intersection will be written to this pointer
 * @param intersection_2 [OUT] If it exists, the second intersection will be written to this pointer
 * @return The number of intersections, between 0 and 2. \p intersection_1 is filled before \p intersection_2.
 */
int16_t circleLineFlIntersectionPoints(circleFl_t circle, lineFl_t line, vecFl_t* intersection_1,
                                       vecFl_t* intersection_2)
{
    // Translate the line so that the circle is at the origin
    lineFl_t tLine = {
        .p1 = subVecFl2d(line.p1, circle.pos),
        .p2 = subVecFl2d(line.p2, circle.pos),
    };

    // Given parameters
    float r     = circle.radius;
    float denom = (tLine.p2.x - tLine.p1.x);
    if (0 == denom)
    {
        // circle & vertical line intersection
        float x = tLine.p1.x;
        if (x - r > EPSILON)
        {
            // No intersection
            return 0;
        }
        else if (fabsf(x - r) < EPSILON)
        {
            // One point
            intersection_1->x = x;
            intersection_1->y = 0;
            // Translate back
            *intersection_1 = addVecFl2d(*intersection_1, circle.pos);
            return 1;
        }
        else
        {
            // Find y from x
            float y = sqrtf((r * r) - (x * x));

            intersection_1->x = x;
            intersection_1->y = y;
            // Translate back
            *intersection_1   = addVecFl2d(*intersection_1, circle.pos);
            intersection_2->x = x;
            intersection_2->y = -y;
            // Translate back
            *intersection_2 = addVecFl2d(*intersection_2, circle.pos);
            return 2;
        }
    }
    else
    {
        float a = (tLine.p2.y - tLine.p1.y) / denom; // TODO div by 0
        float b = -1.0f;
        float c = tLine.p2.y - (tLine.p2.x * a);

        float ab2 = (a * a + b * b);
        float c2  = c * c;
        float r2  = r * r;

        float x0 = -a * c / ab2;
        float y0 = -b * c / ab2;
        if (c2 > r2 * ab2 + EPSILON)
        {
            // no points
            return 0;
        }
        else if (fabsf(c2 - r2 * ab2) < EPSILON)
        {
            // One point
            intersection_1->x = x0;
            intersection_1->y = y0;
            // Translate back
            *intersection_1 = addVecFl2d(*intersection_1, circle.pos);
            return 1;
        }
        else
        {
            // Two points
            float d    = r2 - c2 / ab2;
            float mult = sqrtf(d / ab2);
            float ax, ay, bx, by;
            ax = x0 + b * mult;
            bx = x0 - b * mult;
            ay = y0 - a * mult;
            by = y0 + a * mult;

            intersection_1->x = ax;
            intersection_1->y = ay;
            // Translate back
            *intersection_1   = addVecFl2d(*intersection_1, circle.pos);
            intersection_2->x = bx;
            intersection_2->y = by;
            // Translate back
            *intersection_2 = addVecFl2d(*intersection_2, circle.pos);
            return 2;
        }
    }
}

/**
 * @brief Check if two lines intersect
 *
 * Adapted from https://www.jeffreythompson.org/collision-detection/line-line.php
 *
 * @param a [IN] One line to check for intersection
 * @param b [IN] Another line to check for intersection
 * @return true if the lines intersect, false if they do not
 */
bool lineLineFlIntersection(lineFl_t a, lineFl_t b)
{
    float uA = ((b.p2.x - b.p1.x) * (a.p1.y - b.p1.y) - (b.p2.y - b.p1.y) * (a.p1.x - b.p1.x)) / //
               ((b.p2.y - b.p1.y) * (a.p2.x - a.p1.x) - (b.p2.x - b.p1.x) * (a.p2.y - a.p1.y));
    float uB = ((a.p2.x - a.p1.x) * (a.p1.y - b.p1.y) - (a.p2.y - a.p1.y) * (a.p1.x - b.p1.x)) / //
               ((b.p2.y - b.p1.y) * (a.p2.x - a.p1.x) - (b.p2.x - b.p1.x) * (a.p2.y - a.p1.y));

    if (uA >= 0 && uA <= 1 && uB >= 0 && uB <= 1)
    {
        return true;
    }
    return false;
}

/**
 * @brief Find the intersection point between two infinitely long lines
 *
 * @param a One infinitely long line
 * @param b Another infinitely long line
 * @return The intersection, or (FLT_MAX, FLT_MAX) for parallel lines
 */
vecFl_t infLineIntersectionPoint(lineFl_t a, lineFl_t b)
{
    // Start with an invalid intersection
    vecFl_t intersect = {
        .x = FLT_MAX,
        .y = FLT_MAX,
    };

    // Find the two delta Xs
    float aDelX = (a.p2.x - a.p1.x);
    float bDelX = (b.p2.x - b.p1.x);

    if ((0 == aDelX) && (0 == bDelX))
    {
        // Lines are both vertical (parallel), do nothing
    }
    else if (0 == aDelX)
    {
        // A is vertical, B isn't
        float bSlope = (b.p2.y - b.p1.y) / bDelX;
        intersect.x  = a.p1.x;
        intersect.y  = bSlope * (intersect.x - b.p2.x) + b.p2.y;
    }
    else if (0 == bDelX)
    {
        // B is vertical, A isn't
        float aSlope = (a.p2.y - a.p1.y) / aDelX;
        intersect.x  = b.p1.x;
        intersect.y  = aSlope * (intersect.x - a.p2.x) + a.p2.y;
    }
    else
    {
        // Neither line is vertical
        float aSlope = (a.p2.y - a.p1.y) / aDelX;
        float bSlope = (b.p2.y - b.p1.y) / bDelX;
        float denom  = bSlope - aSlope;
        if (0 != denom)
        {
            float x     = (a.p2.y - (a.p2.x * aSlope) - b.p2.y + (b.p2.x * bSlope)) / denom;
            float y     = aSlope * (x - a.p2.x) + a.p2.y;
            intersect.x = x;
            intersect.y = y;
        }
    }
    return intersect;
}

/**
 * @brief Check if a line intersects with a rectangle
 *
 * Adapted from https://www.jeffreythompson.org/collision-detection/line-rect.php
 *
 * @param rect         [IN]  A rectangle to check for intersection
 * @param line         [IN]  A line to check for intersection
 * @param collisionVec [OUT] A vector pointing from the rectangle to the line in the direction of the collision. May be
 * NULL.
 * @return true if the line intersects the rectangle, false if it doesn't
 */
bool rectLineFlIntersection(rectangleFl_t rect, lineFl_t line, vecFl_t* collisionVec)
{
    if (NULL != collisionVec)
    {
        // Assume zeros
        collisionVec->x = 0;
        collisionVec->y = 0;
    }

    // Check if the line is entirely within the rectangle
    if ((rect.pos.x <= line.p1.x) && (line.p1.x <= (rect.pos.x + rect.width)))
    {
        if ((rect.pos.y <= line.p1.y) && (line.p1.y <= (rect.pos.y + rect.height)))
        {
            if ((rect.pos.x <= line.p2.x) && (line.p2.x <= (rect.pos.x + rect.width)))
            {
                if ((rect.pos.y <= line.p2.y) && (line.p2.y <= (rect.pos.y + rect.height)))
                {
                    // Collision vec of [0,0] is returned
                    return true;
                }
            }
        }
    }

    // Otherwise check if the line intersects each of the four sides of the rectangle
    lineFl_t tmpLine;

    // Check top first
    tmpLine.p1.x = rect.pos.x;
    tmpLine.p1.y = rect.pos.y;
    tmpLine.p2.x = rect.pos.x + rect.width;
    tmpLine.p2.y = rect.pos.y;
    if (lineLineFlIntersection(tmpLine, line))
    {
        if (NULL != collisionVec)
        {
            collisionVec->y = -1;
        }
        return true;
    }

    // Check right
    tmpLine.p1.x = rect.pos.x + rect.width;
    tmpLine.p1.y = rect.pos.y + rect.height;
    if (lineLineFlIntersection(tmpLine, line))
    {
        if (NULL != collisionVec)
        {
            collisionVec->x = 1;
        }
        return true;
    }

    // Check bottom
    tmpLine.p2.x = rect.pos.x;
    tmpLine.p2.y = rect.pos.y + rect.height;
    if (lineLineFlIntersection(tmpLine, line))
    {
        if (NULL != collisionVec)
        {
            collisionVec->y = 1;
        }
        return true;
    }

    // Check left
    tmpLine.p1.x = rect.pos.x;
    tmpLine.p1.y = rect.pos.y;
    if (lineLineFlIntersection(tmpLine, line))
    {
        if (NULL != collisionVec)
        {
            collisionVec->x = -1;
        }
        return true;
    }

    // No intersections
    return false;
}
