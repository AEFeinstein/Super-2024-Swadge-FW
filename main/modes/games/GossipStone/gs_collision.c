#include "gs_collision.h"

bool gs_AABBPointCheck(gs_entity_t* AABB, gs_entity_t* point)
{
    if (AABB->colliderType != GS_AABB || point->colliderType != GS_POINT)
    {
        // There were issues with the parameters.
        return false;
    }
    if (point->pos.x > AABB->pos.x - AABB->collider.AABB.halfWidth
        && point->pos.x < AABB->pos.x + AABB->collider.AABB.halfWidth
        && point->pos.y > AABB->pos.y - AABB->collider.AABB.halfHeight
        && point->pos.y < AABB->pos.y + AABB->collider.AABB.halfHeight)
    {
        return true;
    }
    return false;
}