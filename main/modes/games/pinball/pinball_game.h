#pragma once

#include "hdw-btn.h"
#include "macros.h"
#include "pinball_typedef.h"

uint8_t readInt8(uint8_t* data, uint32_t* idx);
uint16_t readInt16(uint8_t* data, uint32_t* idx);
list_t* addToGroup(jsScene_t* scene, void* obj, uint8_t groupId);

void jsSceneInit(jsScene_t* scene);
void jsSceneDestroy(jsScene_t* scene);
void jsButtonPressed(jsScene_t* scene, buttonEvt_t* event);
