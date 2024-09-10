#include <stddef.h>
#include <math.h>
#include "geometry.h"
#include "macros.h"

/**
 * @brief Check if two circles intersect
 *
 * @param circle1      [IN]  One circle to check for intersection
 * @param circle2      [IN]  The other circle to check for intersection
 * @param collisionVec [OUT] A vector pointing from the second circle to the first in the direction of the collision.
 * May be NULL.
 * @return true if the circles intersect or touch, false if they do not
 */
bool circleCircleIntersection(circle_t circle1, circle_t circle2, vec_t* collisionVec)
{
    int32_t distX   = (circle1.pos.x - circle2.pos.x);
    int32_t distY   = (circle1.pos.y - circle2.pos.y);
    int32_t distRad = (circle1.radius + circle2.radius);
    // Compare distance between centers to the sum of the radii
    bool intersection = ((distX * distX) + (distY * distY)) <= (distRad * distRad);
    if (intersection && (NULL != collisionVec))
    {
        collisionVec->x = circle1.pos.x - circle2.pos.x;
        collisionVec->y = circle1.pos.y - circle2.pos.y;
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
bool circlePointIntersection(circle_t circle, vec_t point, vec_t* collisionVec)
{
    int32_t distX = (circle.pos.x - point.x);
    int32_t distY = (circle.pos.y - point.y);
    // Compare distance between centers to the radius
    bool intersection = ((distX * distX) + (distY * distY)) <= (circle.radius * circle.radius);
    if (intersection && (NULL != collisionVec))
    {
        *collisionVec = subVec2d(circle.pos, point);
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
bool rectRectIntersection(rectangle_t rect1, rectangle_t rect2, vec_t* collisionVec)
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
        vec_t mid1 = {
            .x = rect1.pos.x + (rect1.width / 2),
            .y = rect1.pos.y + (rect1.height / 2),
        };
        vec_t mid2 = {
            .x = rect2.pos.x + (rect2.width / 2),
            .y = rect2.pos.y + (rect2.height / 2),
        };
        vec_t delta = subVec2d(mid1, mid2);

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
bool circleRectIntersection(circle_t circle, rectangle_t rect, vec_t* collisionVec)
{
    // temporary variables to set edges for testing
    int32_t testX = circle.pos.x;
    int32_t testY = circle.pos.y;

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
    int32_t distX    = circle.pos.x - testX;
    int32_t distY    = circle.pos.y - testY;
    int32_t distance = (distX * distX) + (distY * distY);

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
 * @param collisionVec [OUT] A vector pointing from the line to the circle in the direction of the collision. This may
 * be NULL.
 * @return true if the circle intersects the line, false if it doesn't
 */
bool circleLineIntersection(circle_t circle, line_t line, vec_t* collisionVec)
{
    // Check for the line ends
    if (circlePointIntersection(circle, line.p1, collisionVec))
    {
        return true;
    }
    if (circlePointIntersection(circle, line.p2, collisionVec))
    {
        return true;
    }

    // Get the length of the line
    vec_t lineLens = subVec2d(line.p2, line.p1);
    // Find a dot product
    int32_t dot = dotVec2d(subVec2d(circle.pos, line.p1), lineLens);
    // Find the closest point on the line to the circle
    vec_t closestPoint = addVec2d(line.p1, divVec2d(mulVec2d(lineLens, dot), sqMagVec2d(lineLens)));

    // To validate a collision, the closest point must be in the bounding box of the line (i.e. on the line)
    // Create a bounding box for the line
    line_t bb;
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
    vec_t closestDist = subVec2d(closestPoint, circle.pos);
    int32_t distSqr   = sqMagVec2d(closestDist);
    int32_t radiusSqr = circle.radius * circle.radius;

    // If it's less than the radius, we've got a collision!
    if (distSqr < radiusSqr)
    {
        if (NULL != collisionVec)
        {
            collisionVec->x = circle.pos.x - closestPoint.x;
            collisionVec->y = circle.pos.y - closestPoint.y;
        }
        return true;
    }
    return false;
}

/**
 * @brief Check if two lines intersect
 *
 * Adapted from https://www.jeffreythompson.org/collision-detection/line-line.php
 *
 * @param line1 [IN] One line to check for intersection
 * @param line2 [IN] Another line to check for intersection
 * @return true if the lines intersect, false if they do not
 */
bool lineLineIntersection(line_t line1, line_t line2)
{
    int32_t l1_x_diff = line1.p2.x - line1.p1.x;
    int32_t l1_y_diff = line1.p2.y - line1.p1.y;
    int32_t l2_x_diff = line2.p2.x - line2.p1.x;
    int32_t l2_y_diff = line2.p2.y - line2.p1.y;

    int32_t lines_x_diff = line1.p1.x - line2.p1.x;
    int32_t lines_y_diff = line1.p1.y - line2.p1.y;

    // To check if two lines are touching, we have to calculate the distance to the point of intersection:
    int32_t uAn = ((l2_x_diff) * (lines_y_diff) - (l2_y_diff) * (lines_x_diff));
    int32_t uBn = ((l1_x_diff) * (lines_y_diff) - (l1_y_diff) * (lines_x_diff));
    int32_t uD  = ((l2_y_diff) * (l1_x_diff) - (l2_x_diff) * (l1_y_diff));

    // If there is a collision, uA and uB should both be in the range of 0-1.
    // Make sure sign bits match
    if ((0 != (0x80000000 & (uAn ^ uD))) || (0 != (0x80000000 & (uBn ^ uD))))
    {
        return false;
    }

    // Make sure ABS(uAn) <= ABS(uD) (i.e. dividing them is in the range 0-1)
    if (uAn >= 0)
    {
        if (uAn > uD)
        {
            return false;
        }
    }
    else if (uAn < uD)
    {
        return false;
    }

    // Make sure ABS(uBn) <= ABS(uD) (i.e. dividing them is in the range 0-1)
    if (uBn >= 0)
    {
        if (uBn > uD)
        {
            return false;
        }
    }
    else if (uBn < uD)
    {
        return false;
    }

    // Lines intersect!
    return true;
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
bool rectLineIntersection(rectangle_t rect, line_t line, vec_t* collisionVec)
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
    line_t tmpLine;

    // Check top first
    tmpLine.p1.x = rect.pos.x;
    tmpLine.p1.y = rect.pos.y;
    tmpLine.p2.x = rect.pos.x + rect.width;
    tmpLine.p2.y = rect.pos.y;
    if (lineLineIntersection(tmpLine, line))
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
    if (lineLineIntersection(tmpLine, line))
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
    if (lineLineIntersection(tmpLine, line))
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
    if (lineLineIntersection(tmpLine, line))
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

/**
 * @brief Initialize an arrow pointing from base to tip
 *
 * @param base The point of the base of the arrow
 * @param tip The point of the tip of the arrow
 * @param wingLen The length of the wings of the arrowhead
 * @return The initialized arrow
 */
arrow_t initArrow(vec_t base, vec_t tip, int32_t wingLen)
{
    arrow_t arrow = {
        .base = base,
        .tip  = tip,
    };

    // Find the magnitude of the shaft
    int32_t mag = sqrtf(sqMagVec2d(subVec2d(base, tip))) + 0.5f;

    // Not a valid arrow
    if (0 == mag)
    {
        return arrow;
    }

    // Create and scale the wings
    arrow.wing1 = divVec2d(mulVec2d(subVec2d(base, tip), wingLen), mag);
    arrow.wing2 = arrow.wing1;

    // Rotate the wings
    arrow.wing1 = rotateVec2d(arrow.wing1, -45);
    arrow.wing2 = rotateVec2d(arrow.wing2, 45);

    // Translate the wings to align with the tip
    arrow.wing1 = addVec2d(arrow.wing1, tip);
    arrow.wing2 = addVec2d(arrow.wing2, tip);

    return arrow;
}
