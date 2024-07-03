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
    int32_t distX   = (circle1.x - circle2.x);
    int32_t distY   = (circle1.y - circle2.y);
    int32_t distRad = (circle1.radius + circle2.radius);
    // Compare distance between centers to the sum of the radii
    return ((distX * distX) + (distY * distY)) <= (distRad * distRad);
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
        (rect1.x) < (rect2.x + rect2.width) &&
        // rect1 right edge, rect2 left edge
        (rect1.x + rect1.width) > (rect2.x) &&
        // rect1 top edge, rect2 bottom edge
        (rect1.y) < (rect2.y + rect2.height) &&
        // rect1 bottom edge, rect2 top edge
        (rect1.y + rect1.height) > (rect2.y);
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
    int32_t testX = circle.x;
    int32_t testY = circle.y;

    // which edge is closest?
    if (circle.x < rect.x)
    {
        // test left edge
        testX = rect.x;
    }
    else if (circle.x > rect.x + rect.width)
    {
        // right edge
        testX = rect.x + rect.width;
    }

    if (circle.y < rect.y)
    {
        // top edge
        testY = rect.y;
    }
    else if (circle.y > rect.y + rect.height)
    {
        // bottom edge
        testY = rect.y + rect.height;
    }

    // get distance from closest edges
    int32_t distX    = circle.x - testX;
    int32_t distY    = circle.y - testY;
    int32_t distance = (distX * distX) + (distY * distY);

    // if the distance is less than the radius, collision!
    if (distance <= circle.radius * circle.radius)
    {
        return true;
    }
    return false;
}
