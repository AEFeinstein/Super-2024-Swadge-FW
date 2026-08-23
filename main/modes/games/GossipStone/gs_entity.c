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
    if (self->palleteIdx)
    {
        drawWsgPalette(&self->gameData->assets[self->assetIndex].frames[self->currentAnimationFrame], x, y,
                       &self->gameData->entityManager.palettes[self->palleteIdx], self->flipped, false, 0);
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
    for (int i = 0; i < 35; i++)
    {
        int32_t x = i * self->gameData->assets[self->assetIndex].frames[0].w;
        y         = ((self->pos.y - self->gameData->camera.pos.y) >> GS_DECIMAL_BITS)
                    - self->gameData->assets[self->assetIndex].originY;
        drawWsg(&self->gameData->assets[self->assetIndex].frames[self->currentAnimationFrame], x, y, self->flipped,
                false, 0);
    }
    drawRectFilled(0, y + self->gameData->assets[self->assetIndex].frames[0].h, TFT_WIDTH,
                   y + self->gameData->assets[self->assetIndex].frames[0].h + 10, c012);
}

void gs_updateGossip(gs_entity_t* self)
{
    gs_gossip_t* data = (gs_gossip_t*)self->data;
    if (data->progress < strlen(data->messageList[data->index]) * FRAMES_PER_CHAR)
    {
        data->progress++;
        if (data->progress == strlen(data->messageList[data->index]) * FRAMES_PER_CHAR)
        {
            data->gossipStone->currentAnimationFrame = 0;
            data->gossipStone->paused                = true;
        }
    }
    // make this check for shake later
    else if (self->gameData->btnDownState & PB_A)
    {
        if (self->gameData->submode == GS_PROPHECY_SUBMODE)
        {
            if (data->index < data->arr_size - 1)
            {
                data->index++;
                if (data->index == 6 || data->index == 8)
                {
                    data->gossipStone->palleteIdx = GS_BLUE_PALETTE;
                }
                else if (data->index == 10)
                {
                    data->gossipStone->palleteIdx = GS_RED_PALETTE;
                }
                else
                {
                    data->gossipStone->palleteIdx = GS_UNTOUCHED_PALETTE;
                }
            }
        }
        else
        {
            data->index = gs_randomInt(1, data->arr_size - 1);
        }
        data->progress            = 0;
        data->gossipStone->paused = false;
        if (self->gameData->submode == GS_GOSSIP_SUBMODE)
        {
            gs_recordProgress(self);
        }
    }
}

void gs_recordProgress(gs_entity_t* self)
{
    gs_gossip_t* data = (gs_gossip_t*)self->data;

    uint8_t NVSgroup = data->index / 32;
    uint8_t NVSbit   = data->index % 32;
    if (!gs_checkBit(self->gameData->gossipProgress[NVSgroup], NVSbit))
    {
        // set that bit to 1
        self->gameData->gossipProgress[NVSgroup] |= (1 << NVSbit);
        // save it to nvs
        char nvsKey[20];
        sprintf(nvsKey, "gossipProgress%d", NVSgroup);
        printf("nvsKey = '%s'\n", nvsKey);
        writeNvs32(nvsKey, self->gameData->gossipProgress[NVSgroup]);

        // update the trophy
        trophyUpdate(&(*self->gameData->trophyData)[0], trophyGetSavedValue(&(*self->gameData->trophyData)[0]) + 1,
                     true);
    }
}

void gs_drawGossip(gs_entity_t* self)
{
    gs_gossip_t* data = ((gs_gossip_t*)self->data);
    int16_t textX     = 18;
    int16_t textY     = 50;

    uint16_t typeAmount = data->progress / FRAMES_PER_CHAR;
    char display[typeAmount + 5];
    strncpy(display, data->messageList[data->index], typeAmount);
    display[typeAmount] = '\0';

    if (typeAmount > 0)
    {
        drawTextWordWrap(&self->gameData->font_gossip, c445, display, &textX, &textY, TFT_WIDTH - textX,
                         TFT_HEIGHT - textY);
    }
}
