#include "gs_entity.h"
#include "gs_utility.h"
#include "gs_gossip.h"
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

void* gs_findLastNodeOfType(gs_entity_t* self, gs_dataType_t type)
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
    return cur;
}

gs_entity_t* gs_findLastEntityOfType(gs_entity_t* self, gs_dataType_t type)
{
    node_t* node = (node_t*)gs_findLastNodeOfType(self, type);
    if (node)
    {
        return (gs_entity_t*)node->val;
    }
    return NULL;
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
    int8_t offset = -((self->gameData->entityManager.camera.pos.x >> DECIMAL_BITS)
                      % self->gameData->assets[self->assetIndex].frames[0].w);
    if (self->gameData->entityManager.camera.pos.x < 0)
    {
        offset -= self->gameData->assets[self->assetIndex].frames[0].w;
    }
    for (int i = 0; i < TFT_WIDTH / self->gameData->assets[self->assetIndex].frames[0].w + 1; i++)
    {
        int32_t x = i * self->gameData->assets[self->assetIndex].frames[0].w + offset;
        y         = ((self->pos.y - self->gameData->entityManager.camera.pos.y) >> DECIMAL_BITS)
                    - self->gameData->assets[self->assetIndex].originY;
        drawWsg(&self->gameData->assets[self->assetIndex].frames[self->currentAnimationFrame], x, y, self->flipped,
                false, 0);
    }
}

void gs_updateGossip(gs_entity_t* self)
{
    gs_gossip_t* data = (gs_gossip_t*)self->data;
    if (data->dialogueFinished)
    {
        return;
    }
    if (self->gameData->submode == GS_PROPHECY_SUBMODE && data->index == data->arr_size - 1
        && (self->gameData->touchState[0].touched || self->gameData->touchState[1].touched)
        && data->arr_size == PROPHECY_COUNT)
    {
        data->dialogueFinished = true;
        data->onDialogueFinished(self);
    }
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
        switch (self->gameData->submode)
        {
            case GS_GOSSIP_SUBMODE:
            case GS_AMA_SUBMODE:
                data->index = gs_randomInt(1, data->arr_size - 1);
                data->gossipStone->paused = false;
                break;
            case GS_PROPHECY_SUBMODE:
                if (data->index < data->arr_size - 1)
                {
                    data->index++;
                    if (data->messageList[data->index][0] == '5' || data->messageList[data->index][0] == '3')
                    {
                        data->gossipStone->palleteIdx = GS_BLUE_PALETTE;
                    }
                    else if (data->messageList[data->index][0] == '1')
                    {
                        data->gossipStone->palleteIdx = GS_RED_PALETTE;
                    }
                    else
                    {
                        data->gossipStone->palleteIdx = GS_UNTOUCHED_PALETTE;
                    }
                    data->gossipStone->paused = false;
                }
                else
                {
                    data->dialogueFinished = true;
                    data->onDialogueFinished(self);
                }
                break;
        }
        data->progress = 0;
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
    if (data->dialogueFinished)
    {
        return;
    }
    int16_t textX = 18;
    int16_t textY = 50;

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
    gs_flame_t* fData = (gs_flame_t*)self->data;
    if (!fData->flameOn)
    {
        return;
    }
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
    if (self->gameData->touchState[0].touched && gsData->rcsEnabled)
    {
        // printf("elapsedUs: %d\n", self->gameData->touchState[0].position - 511);
        // printf("tmp %d\n", (self->gameData->touchState[0].position - 511) * self->gameData->elapsedUs >> 14);
        gsData->angVel += (self->gameData->touchState[0].position - 511) * self->gameData->elapsedUs >> 14;
    }
    // angular drag per frame
    gsData->angVel = gsData->angVel * 39 / 40;
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

    gs_flame_t* fData = (gs_flame_t*)gsData->flame->data;
    fData->rotateDeg  = gsData->rotateDeg;

    // translational physics
    fData->flameOn = self->gameData->touchState[1].touched && gsData->throttleEnabled;
    if (fData->flameOn)
    {
        vec_t rocketForce = rotateVec2d((vec_t){0, 1 << DECIMAL_BITS}, (gsData->rotateDeg >> DECIMAL_BITS) + 180);
        rocketForce
            = mulVec2d(rocketForce, (1024 - self->gameData->touchState[1].position) * self->gameData->elapsedUs);
        gsData->vel = addVec2d(gsData->vel, divVec2d(rocketForce, 10000000));
    }
    // gravity
    gsData->vel.y += gsData->gravity * self->gameData->elapsedUs >> 18;
    self->pos.x += gsData->vel.x * self->gameData->elapsedUs >> 20;
    self->pos.y += gsData->vel.y * self->gameData->elapsedUs >> 20;

    gsData->flame->pos = self->pos;

    // printf("pos x: %d pos y: %d\n", self->pos.x, self->pos.y);

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
    sData->frameCount           = endFrame - sData->startFrame; // it's actually the frame count minus 1.
    self->currentAnimationFrame = sData->startFrame;
}

void gs_updateStar(gs_entity_t* self)
{
    // fake movement during talking cutscene moment
    if (self->gameData->entityManager.gossipStone->updateFunction == NULL)
    {
        self->pos = subVec2d(self->pos, self->gameData->entityManager.camera.vel);
    }
}

void gs_updateFarStar(gs_entity_t* self)
{
    if (self->gameData->entityManager.camera.vel.x == 0 && self->gameData->entityManager.camera.vel.y == 0)
    {
        return;
    }
    gs_randomizeStarData(self);

    uint32_t longOdds = TFT_WIDTH * abs(self->gameData->entityManager.camera.vel.y);

    uint32_t total = longOdds + TFT_HEIGHT * abs(self->gameData->entityManager.camera.vel.x);
    uint32_t roll  = gs_randomInt(0, total);

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

void gs_enableFlightControls(gs_entity_t* self)
{
    gs_entity_t* gossipStone           = ((gs_gossip_t*)self->data)->gossipStone;
    gossipStone->currentAnimationFrame = 0;
    gossipStone->paused                = true;
    gs_gossipStone_t* gsData           = (gs_gossipStone_t*)gossipStone->data;
    gsData->rcsEnabled                 = true;
    gsData->throttleEnabled            = true;
}

void gs_updateMoon(gs_entity_t* self)
{
    // Just doing some prophecy scene management with the little moon that happens to be present for the scene.
    if (self->gameData->entityManager.gossipStone->pos.y < 30000)
    {
        self->updateFunction                                                                  = NULL;
        ((gs_gossipStone_t*)self->gameData->entityManager.gossipStone->data)->throttleEnabled = false;
        ((gs_gossipStone_t*)self->gameData->entityManager.gossipStone->data)->gravity         = 0;
        ((gs_flame_t*)((gs_gossipStone_t*)self->gameData->entityManager.gossipStone->data)->flame->data)->flameOn
            = false;
        gs_gossip_t* gData         = (gs_gossip_t*)self->gameData->entityManager.gossip->data;
        gData->gossipStone->paused = false;
        gData->messageList         = prophecyEndSceneList;
        gData->arr_size            = PROPHECY_END_SCENE_COUNT;
        gData->dialogueFinished    = false;
        gData->onDialogueFinished  = gs_spawnBigMoon;
        gData->index               = 0;
        gData->progress            = 0;
    }
}

void gs_updateBigMoon(gs_entity_t* self)
{
    // The big moon pulls the gossip stone in.
    gs_entity_t* gs = self->gameData->entityManager.gossipStone;
    vec_t gsVec     = subVec2d(gs->pos, self->pos);
    gsVec           = divVec2d(mulVec2d(gsVec, 199), 200);
    gs->pos         = addVec2d(self->pos, gsVec);

    if (sqMagVec2d(gsVec) > 17000000)
    {
        return;
    }
    gs_bigMoon_t* bmData = (gs_bigMoon_t*)self->data;
    bmData->deltaScale++;
    q24_8 increaseBy = gs_lerp(512, bmData->deltaScale, bmData->scale);
    bmData->scale += (increaseBy >> 9);
    if(bmData->scale > bmData->targetScale)
    {
        self->updateFunction = NULL;
        if(bmData->callback)
        {
            bmData->callback(self);
        }
    }
}

void gs_drawBigMoon(gs_entity_t* self)
{
    gs_bigMoon_t* bmData = (gs_bigMoon_t*)self->data;
    int32_t x            = ((self->pos.x - self->gameData->entityManager.camera.pos.x) >> DECIMAL_BITS);
    x -= FROM_FX_QN(
        MUL_FX_QN(TO_FX_QN(self->gameData->assets[self->assetIndex].originX, FRAC_BITS), bmData->scale, FRAC_BITS),
        FRAC_BITS);
    int32_t y = ((self->pos.y - self->gameData->entityManager.camera.pos.y) >> DECIMAL_BITS);
    y -= FROM_FX_QN(
        MUL_FX_QN(TO_FX_QN(self->gameData->assets[self->assetIndex].originY, FRAC_BITS), bmData->scale, FRAC_BITS),
        FRAC_BITS);
    drawWsgSmoothScaled(&self->gameData->assets[self->assetIndex].frames[self->currentAnimationFrame], x, y,
                        bmData->scale, bmData->scale);
}

void gs_spawnBigMoon(gs_entity_t* self)
{
    self->gameData->entityManager.gossipStone->updateFunction                 = gs_updateGossipStone;
    ((gs_gossipStone_t*)self->gameData->entityManager.gossipStone->data)->vel = (vec_t){0, 0};
    // Where the moon is spawning relative to the GossipStone.
    vec_t moonOffset = mulVec2d(
        (vec_t){self->gameData->entityManager.camera.vel.x / 16, self->gameData->entityManager.camera.vel.y / 16}, 200);
    // if it is so close that it would pop on screen
    if (sqMagVec2d(moonOffset) < 140 * 140)
    {
        // make the offset be the length of half the tft width.
        vec_t norm = (vec_t){self->gameData->entityManager.camera.vel.x, self->gameData->entityManager.camera.vel.y};
        fastNormVec(&norm.x, &norm.y);
        norm       = divVec2d(norm, 64);
        moonOffset = mulVec2d(norm, 140);
    }
    moonOffset.x *= 16;
    moonOffset.y *= 16;
    gs_entity_t* moon = gs_createEntityBefore(
        gs_findLastNodeOfType(self, GS_GOSSIP_STONE_DATA), &self->gameData->entityManager, 1, GS_NO_ANIMATION, false,
        GS_HI_RES_MOON_ASSET, 1, addVec2d(self->gameData->entityManager.gossipStone->pos, moonOffset), self->gameData);
    moon->updateFunction               = gs_updateBigMoon;
    moon->drawFunction                 = gs_drawBigMoon;
    moon->data                         = heap_caps_calloc(1, sizeof(gs_bigMoon_t), MALLOC_CAP_SPIRAM);
    moon->dataType                     = GS_BIG_MOON_DATA;
    ((gs_bigMoon_t*)moon->data)->scale = 1; // q24_8 for 0.125
    ((gs_bigMoon_t*)moon->data)->callback = gs_spawnLanding; // q24_8 for 0.125
    ((gs_bigMoon_t*)moon->data)->targetScale = 700;
}

void gs_spawnLanding(gs_entity_t* self)
{
    gs_entity_t* landing = gs_createEntityBefore(
        gs_findLastNodeOfType(self, GS_GOSSIP_STONE_DATA), &self->gameData->entityManager, 1, GS_NO_ANIMATION, false,
        GS_LANDING_ASSET, 1, self->gameData->entityManager.gossipStone->pos, self->gameData);
    landing->updateFunction               = gs_updateBigMoon;
    landing->drawFunction                 = gs_drawBigMoon;
    landing->data                         = heap_caps_calloc(1, sizeof(gs_bigMoon_t), MALLOC_CAP_SPIRAM);
    landing->dataType                     = GS_BIG_MOON_DATA;
    ((gs_bigMoon_t*)landing->data)->scale = 1;
    ((gs_bigMoon_t*)landing->data)->targetScale = 800;
}
