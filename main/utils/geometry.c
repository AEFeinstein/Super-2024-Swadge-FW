#include "geometry.h"

/**
 * @brief Check if two circles intersect each other
 *
 * @param circle1 One circle to check for intersection
 * @param circle2 The other circle to check for intersection
 * @return true if the circles intersect or touch, false if they do not
 */
bool circleCircleIntersection(circle_t circle1, circle_t circle2)
{
    int32_t distX   = (circle1.pos.x - circle2.pos.x);
    int32_t distY   = (circle1.pos.y - circle2.pos.y);
    int32_t distRad = (circle1.radius + circle2.radius);
    // Compare distance between centers to the sum of the radii
    return ((distX * distX) + (distY * distY)) <= (distRad * distRad);
}

/**
 * @brief Check if a point is in a circle
 *
 * @param circle A circle to check for intersection
 * @param point A point to check if it's inside a circle
 * @return true if the point is in a circle intersect or touch, false if it isn't
 */
bool circlePointIntersection(circle_t circle, vec_t point)
{
    int32_t distX = (circle.pos.x - point.x);
    int32_t distY = (circle.pos.y - point.y);
    // Compare distance between centers to the radius
    return ((distX * distX) + (distY * distY)) <= (circle.radius * circle.radius);
}

/**
 * @brief Check if two rectangles intersect each other
 *
 * @param rect1 One rectangle to check for intersection
 * @param rect2 The other circle to check for intersection
 * @return true if the rectangles intersect or touch, false if they do not
 */
bool rectRectIntersection(rectangle_t rect1, rectangle_t rect2)
{
    return
        // rect1 left edge, rect2 right edge
        (rect1.pos.x) < (rect2.pos.x + rect2.width) &&
        // rect1 right edge, rect2 left edge
        (rect1.pos.x + rect1.width) > (rect2.pos.x) &&
        // rect1 top edge, rect2 bottom edge
        (rect1.pos.y) < (rect2.pos.y + rect2.height) &&
        // rect1 bottom edge, rect2 top edge
        (rect1.pos.y + rect1.height) > (rect2.pos.y);
}

/**
 * @brief Check if a circle and a rectangle intersect each other
 *
 * @param circle The circle to check for intersection
 * @param rect The rectangle to check for intersection
 * @return true if the shapes intersect or touch, false if they do not
 */
bool circleRectIntersection(circle_t circle, rectangle_t rect)
{
    // temporary variables to set edges for testing
    int32_t testX = circle.pos.x;
    int32_t testY = circle.pos.y;

    // which edge is closest?
    if (circle.pos.x < rect.pos.x)
    {
        // test left edge
        testX = rect.pos.x;
    }
    else if (circle.pos.x > rect.pos.x + rect.width)
    {
        // right edge
        testX = rect.pos.x + rect.width;
    }

    if (circle.pos.y < rect.pos.y)
    {
        // top edge
        testY = rect.pos.y;
    }
    else if (circle.pos.y > rect.pos.y + rect.height)
    {
        // bottom edge
        testY = rect.pos.y + rect.height;
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
 * @brief TODO
 *
 * Adapted from https://www.jeffreythompson.org/collision-detection/line-circle.php
 *
 * @param circle
 * @param line
 * @param collisionVec
 * @return true
 * @return false
 */
bool circleLineIntersection(circle_t circle, line_t line, vec_t* collisionVec)
{
    // Check for the line ends
    if (circlePointIntersection(circle, line.p1))
    {
        collisionVec->x = circle.pos.x - line.p1.x;
        collisionVec->y = circle.pos.y - line.p1.y;
        return true;
    }
    if (circlePointIntersection(circle, line.p2))
    {
        collisionVec->x = circle.pos.x - line.p2.x;
        collisionVec->y = circle.pos.y - line.p2.y;
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
    if (closestPoint.x <= bb.p1.x || closestPoint.x > bb.p2.x || closestPoint.y < bb.p1.y || closestPoint.y > bb.p2.y)
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
        collisionVec->x = circle.pos.x - closestPoint.x;
        collisionVec->y = circle.pos.y - closestPoint.y;
        return true;
    }
    return false;
}
