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
    int32_t x = ((self->pos.x - self->gameData->entityManager.camera.pos.x) >> DECIMAL_BITS)
                - self->gameData->assets[self->assetIndex].originX;
    int32_t y = ((self->pos.y - self->gameData->entityManager.camera.pos.y) >> DECIMAL_BITS)
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
        y         = ((self->pos.y - self->gameData->entityManager.camera.pos.y) >> DECIMAL_BITS)
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

void gs_drawFlame(gs_entity_t* self)
{
    if (!self->gameData->touchState[1].touched)
    {
        return;
    }
    gs_flame_t* fData = (gs_flame_t*)self->data;
    // use the right touch input to draw the sized flame.
    self->currentAnimationFrame
        = (self->gameData->assets[GS_FLAME_ASSET].numFrames) * self->gameData->touchState[1].position / 1023;
    self->currentAnimationFrame = self->gameData->assets[GS_FLAME_ASSET].numFrames - 1 - self->currentAnimationFrame;

    int32_t x = ((self->pos.x - self->gameData->entityManager.camera.pos.x) >> DECIMAL_BITS)
                - self->gameData->assets[self->assetIndex].originX;
    int32_t y = ((self->pos.y - self->gameData->entityManager.camera.pos.y) >> DECIMAL_BITS)
                - self->gameData->assets[self->assetIndex].originY;

    vec_t direction       = rotateVec2d((vec_t){0, 1 << DECIMAL_BITS}, fData->rotateDeg >> DECIMAL_BITS);
    vec_t directionScaled = mulVec2d(direction, 28 + self->currentAnimationFrame);
    drawWsg(&self->gameData->assets[self->assetIndex].frames[self->currentAnimationFrame], x, y, gs_randomInt(0, 1),
            false, fData->rotateDeg >> DECIMAL_BITS);
    drawCircleFilled(x + self->gameData->assets[self->assetIndex].originX + (directionScaled.x >> DECIMAL_BITS),
                     y + self->gameData->assets[self->assetIndex].originY + (directionScaled.y >> DECIMAL_BITS),
                     2 + self->currentAnimationFrame + gs_randomInt(-1, 1), c530);

    directionScaled = mulVec2d(direction, 24 + self->currentAnimationFrame);
    drawCircleFilled(x + self->gameData->assets[self->assetIndex].originX + (directionScaled.x >> DECIMAL_BITS)
                         + gs_randomInt(-1, 1),
                     y + self->gameData->assets[self->assetIndex].originY + (directionScaled.y >> DECIMAL_BITS)
                         + gs_randomInt(-1, 1),
                     1 + (self->currentAnimationFrame >> 1), c554);
}

void gs_updatePhysicsObject(gs_entity_t* self)
{
    gs_physics_t* pData = (gs_physics_t*)self->data;
    pData->vel.y++;
    self->pos.x += (pData->vel.x * self->gameData->elapsedUs) >> 15;
    self->pos.y += (pData->vel.y * self->gameData->elapsedUs) >> 15;

    gs_hitInfo_t hitInfo = {0};
    if (self->gameData->entityManager.tilemap) // The Prophecy level doesn't have a tilemap
    {
        gs_collisionCheck(self->gameData->entityManager.tilemap, self, &hitInfo);
        if (hitInfo.hit == false)
        {
            return;
        }
        // self->pos.x = hitInfo.pos.x + hitInfo.normal.x * self->halfWidth;
        // self->pos.y = hitInfo.pos.y + hitInfo.normal.y * self->halfHeight;

        // Reflect the velocity vector along the normal
        // See http://www.sunshine2k.de/articles/coding/vectorreflection/vectorreflection.html
        pData->vel = divVec2d(
            mulVec2d(subVec2d(pData->vel, mulVec2d(hitInfo.normal, (2 * dotVec2d(pData->vel, hitInfo.normal)))),
                     pData->bounceNumerator),
            pData->bounceDenominator);
    }
    // keep the physics object within the bounds of the level.
    if (self->pos.x < 256)
    {
        self->pos.x = 256;
    }
    else if (self->pos.x > 38144)
    {
        self->pos.x = 38144;
    }
}

void gs_drawTileMap(gs_entity_t* self)
{
}

void gs_collisionCheck(gs_entity_t* tilemap, gs_entity_t* ent, gs_hitInfo_t* hitInfo)
{
}

void gs_updateGossipStone(gs_entity_t* self)
{
    gs_gossipStone_t* gsData = (gs_gossipStone_t*)self->data;
    // angular physics
    if (self->gameData->touchState[0].touched)
    {
        // printf("elapsedUs: %d\n", self->gameData->touchState[0].position - 511);
        // printf("tmp %d\n", (self->gameData->touchState[0].position - 511) * self->gameData->elapsedUs >> 14);
        gsData->angVel += (self->gameData->touchState[0].position - 511) * self->gameData->elapsedUs >> 13;
    }
    // 5% angular drag per frame
    gsData->angVel = gsData->angVel * 19 / 20;
    // printf("ang vel: %d\n", gsData->angVel);
    // printf("elapsed trunc: %d\n",self->gameData->elapsedUs>>2);
    // printf("uhhh: %d\n", gsData->angVel * (self->gameData->elapsedUs>>2));
    // printf("heh? %d\n", gsData->angVel * (self->gameData->elapsedUs>>2) / 24);
    gsData->rotateDeg += gsData->angVel * (self->gameData->elapsedUs >> 2) / 1000000;
    gsData->rotateDeg %= 360 << DECIMAL_BITS;
    if (gsData->rotateDeg < 0)
    {
        gsData->rotateDeg += 360 << DECIMAL_BITS;
    }
    ((gs_flame_t*)gsData->flame->data)->rotateDeg = gsData->rotateDeg;

    // translational physics
    if (self->gameData->touchState[1].touched)
    {
        vec_t rocketForce = rotateVec2d((vec_t){0, 1 << DECIMAL_BITS}, (gsData->rotateDeg >> DECIMAL_BITS) + 180);
        rocketForce
            = mulVec2d(rocketForce, (1024 - self->gameData->touchState[1].position) * self->gameData->elapsedUs);
        gsData->vel = addVec2d(gsData->vel, divVec2d(rocketForce, 10000000));
    }
    //gravity
    gsData->vel.y += 100 * self->gameData->elapsedUs >> 18;
    self->pos.x += gsData->vel.x * self->gameData->elapsedUs >> 20;
    self->pos.y += gsData->vel.y * self->gameData->elapsedUs >> 20;

    gsData->flame->pos = self->pos;

    self->gameData->entityManager.camera.vel = self->gameData->entityManager.camera.pos;
    if (self->pos.y < 0xFFFF)
    {
        self->gameData->entityManager.camera.pos.y = self->pos.y - (TFT_HEIGHT << (DECIMAL_BITS - 1));
    }
    // ground collision
    if (self->pos.y > 66847) // 0xFFFF + (82 << DECIMAL_BITS))
    {
        self->pos.y   = 66847;
        gsData->vel.y = 0;
    }
    self->gameData->entityManager.camera.pos.x = self->pos.x - (TFT_WIDTH << (DECIMAL_BITS - 1));
    self->gameData->entityManager.camera.vel
        = subVec2d(self->gameData->entityManager.camera.pos, self->gameData->entityManager.camera.vel);
}

void gs_drawGossipStone(gs_entity_t* self)
{
    gs_gossipStone_t* gsData = (gs_gossipStone_t*)self->data;
    int32_t x                = ((self->pos.x - self->gameData->entityManager.camera.pos.x) >> DECIMAL_BITS)
                               - self->gameData->assets[self->assetIndex].originX;
    int32_t y                = ((self->pos.y - self->gameData->entityManager.camera.pos.y) >> DECIMAL_BITS)
                               - self->gameData->assets[self->assetIndex].originY;
    int32_t finalRot         = gsData->rotateDeg >> DECIMAL_BITS;
    finalRot                 = (finalRot + 45) % 360;
    if (self->palleteIdx)
    {
        drawWsgPalette(&self->gameData->assets[self->assetIndex].frames[self->currentAnimationFrame], x, y,
                       &self->gameData->entityManager.palettes[self->palleteIdx], self->flipped, false, finalRot);
    }
    else
    {
        drawWsg(&self->gameData->assets[self->assetIndex].frames[self->currentAnimationFrame], x, y, self->flipped,
                false, finalRot);
    }
}

void gs_randomizeStarData(gs_entity_t* self)
{
    gs_star_t* sData                  = (gs_star_t*)self->data;
    self->gameFramesPerAnimationFrame = gs_randomInt(5, 30);
    sData->linearPlayback             = gs_randomInt(0, 1);
    sData->startFrame                 = gs_randomInt(0, self->gameData->assets[self->assetIndex].numFrames - 1);
    uint8_t endFrame                  = sData->startFrame;
    if (gs_randomInt(0, 1) && sData->startFrame < self->gameData->assets[self->assetIndex].numFrames - 1)
    {
        endFrame = gs_randomInt(sData->startFrame + 1, self->gameData->assets[self->assetIndex].numFrames - 1);
    }
    sData->frameCount = endFrame - sData->startFrame; // it's actually the frame count minus 1.
}

void gs_updateFarStar(gs_entity_t* self)
{
    if (self->gameData->entityManager.camera.vel.x == 0 && self->gameData->entityManager.camera.vel.y == 0)
    {
        return;
    }
    gs_randomizeStarData(self);

    uint16_t longOdds = TFT_WIDTH * abs(self->gameData->entityManager.camera.vel.y);

    uint16_t total = longOdds + TFT_HEIGHT * abs(self->gameData->entityManager.camera.vel.x);
    uint16_t roll  = gs_randomInt(0, total);

    if (roll <= longOdds) // it warps to a long edge
    {
        if (self->gameData->entityManager.camera.vel.y < 0)
        {
            self->pos = addVec2d(self->gameData->entityManager.camera.pos,
                                 (vec_t){.x = gs_randomInt(0, TFT_WIDTH << DECIMAL_BITS), .y = -(16 << DECIMAL_BITS)});
        }
        else
        {
            self->pos = addVec2d(
                self->gameData->entityManager.camera.pos,
                (vec_t){.x = gs_randomInt(0, TFT_WIDTH << DECIMAL_BITS), .y = ((TFT_HEIGHT + 16) << DECIMAL_BITS)});
        }
    }
    else // it warps to a short edge
    {
        if (self->gameData->entityManager.camera.vel.x < 0)
        {
            self->pos = addVec2d(self->gameData->entityManager.camera.pos,
                                 (vec_t){.x = -(16 << DECIMAL_BITS), .y = gs_randomInt(0, TFT_HEIGHT << DECIMAL_BITS)});
        }
        else
        {
            self->pos = addVec2d(
                self->gameData->entityManager.camera.pos,
                (vec_t){.x = ((TFT_WIDTH + 16) << DECIMAL_BITS), .y = gs_randomInt(0, TFT_HEIGHT << DECIMAL_BITS)});
        }
    }
}

void gs_drawStar(gs_entity_t* self)
{
    gs_star_t* sData = (gs_star_t*)self->data;
    self->animationTimer++;
    if (self->animationTimer >= self->gameFramesPerAnimationFrame)
    {
        self->animationTimer = 0;

        if (sData->linearPlayback)
        {
            self->currentAnimationFrame++;
            if (self->currentAnimationFrame >= sData->startFrame + sData->frameCount)
                self->currentAnimationFrame = 0;
        }
        else
        {
            self->currentAnimationFrame = gs_randomInt(sData->startFrame, sData->startFrame + sData->frameCount);
        }
    }

    int32_t x = ((self->pos.x - self->gameData->entityManager.camera.pos.x) >> DECIMAL_BITS)
                - self->gameData->assets[self->assetIndex].originX;
    int32_t y = ((self->pos.y - self->gameData->entityManager.camera.pos.y) >> DECIMAL_BITS)
                - self->gameData->assets[self->assetIndex].originY;
    drawWsg(&self->gameData->assets[self->assetIndex].frames[self->currentAnimationFrame], x, y, self->flipped, false,
            0);
}
