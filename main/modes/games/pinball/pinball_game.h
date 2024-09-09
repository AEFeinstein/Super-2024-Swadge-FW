#pragma once

#include "hdw-btn.h"
#include "macros.h"
#include "pinball_typedef.h"

uint8_t readInt8(uint8_t* data, uint32_t* idx);
uint16_t readInt16(uint8_t* data, uint32_t* idx);
list_t* addToGroup(pbScene_t* scene, void* obj, uint8_t groupId);

void pbSceneInit(pbScene_t* scene);
void pbSceneDestroy(pbScene_t* scene);
void pbButtonPressed(pbScene_t* scene, buttonEvt_t* event);
void pbRemoveBall(pbBall_t* ball, pbScene_t* scene);
void pbStartBall(pbScene_t* scene);
void pbGameTimers(pbScene_t* scene, int32_t elapsedUs);
void pbOpenLaunchTube(pbScene_t* scene, bool open);
void pbStartMultiball(pbScene_t* scene);
