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

gs_entity_t* gs_findLastEntityOfType(gs_entity_t* self, gs_dataType_t type)
{
    node_t* cur = self->gameData->entityManager.entities->last;
    while (((gs_entity_t*)cur->val)->dataType != type)
    {
        cur = cur->prev;
        if (!cur)
        {
            return NULL;
        }
    }
    return (gs_entity_t*)cur->val;
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

void gs_drawSkyGradient(gs_entity_t* self)
{
    int32_t y;
    for(int i = 0; i < 35; i++)
    {
        int32_t x = i * self->gameData->assets[self->assetIndex].frames[0].w;
        y = ((self->pos.y - self->gameData->camera.pos.y) >> GS_DECIMAL_BITS)
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
    drawRectFilled(0, y + self->gameData->assets[self->assetIndex].frames[0].h, TFT_WIDTH, y + self->gameData->assets[self->assetIndex].frames[0].h + 10, c012);
}

void gs_updateGossip(gs_entity_t* self)
{
    gs_gossip_t* data = (gs_gossip_t*)self->data;
    if(data->progress < TYPING_FRAMES)
    {
        data->progress++;
    }
    //make this check for shake later
    else if (self->gameData->btnDownState && PB_A)
    {
        data->index = gs_randomInt(1, data->arr_size - 1);
        data->progress = 0;
    }
}

void gs_drawGossip(gs_entity_t* self)
{
    gs_gossip_t* data = ((gs_gossip_t*)self->data);
    int16_t textX = 18;
    int16_t textY = 30;

    uint16_t typeAmount = (strlen(data->messageList[data->index]) * data->progress) / TYPING_FRAMES;
    char display[typeAmount+5];
    strncpy(display, data->messageList[data->index], typeAmount);
    display[typeAmount] = '\0';

    if(typeAmount > 0)
    {
        drawTextWordWrap(&self->gameData->font_gossip, c445, display, &textX, &textY, TFT_WIDTH-textX,
            TFT_HEIGHT-textY);
    }
}