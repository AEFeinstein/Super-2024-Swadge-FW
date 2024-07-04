#include <stdbool.h>

static bool collCheck(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2) {
    int hitMinX = x1;
    int hitMaxX = x1 + w1;
    int hitMinY = y1;
    int hitMaxY = y1 + h1;

    int hurtMinX = x2;
    int hurtMaxX = x2 + w2;
    int hurtMinY = y2;
    int hurtMaxY = y2 + h2;

    bool collisionX = (hitMinX < hurtMaxX) && (hitMaxX > hurtMinX);
    bool collisionY = (hitMinY < hurtMaxY) && (hitMaxY > hurtMinY);
    bool collision = collisionX && collisionY;

    return collision;
}
