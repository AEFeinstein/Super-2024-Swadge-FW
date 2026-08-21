#include "gs_entity.h"
#include "gs_utility.h"
#include "shapes.h"
#include <linked_list.h>
#include <limits.h>

void gs_setData(gs_entity_t* self, void* data, gs_dataType_t dataType)
{
    if (self->data != NULL)
    {
        heap_caps_free(self->data);
        self->data = NULL;
    }
    self->data     = data;
    self->dataType = dataType;
}

void gs_drawAsset(gs_entity_t* self)
{
    int32_t x = ((self->pos.x - self->gameData->camera.pos.x) >> GS_DECIMAL_BITS)
                - self->gameData->assets[self->assetIndex].originX;
    int32_t y = ((self->pos.y - self->gameData->camera.pos.y) >> GS_DECIMAL_BITS)
                - self->gameData->assets[self->assetIndex].originY;
    if (self->gray)
    {
        drawWsgPalette(&self->gameData->assets[self->assetIndex].frames[self->currentAnimationFrame], x, y,
                       &self->gameData->entityManager.palettes[GS_GRAYSCALE_PALETTE], self->flipped, false, 0);
    }
    else
    {
        drawWsg(&self->gameData->assets[self->assetIndex].frames[self->currentAnimationFrame], x, y, self->flipped,
                false, 0);
    }
}

void gs_drawNothing(gs_entity_t* self)
{
}