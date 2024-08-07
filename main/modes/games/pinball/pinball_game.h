#pragma once

#include "hdw-btn.h"
#include "pinball_typedef.h"

uint8_t readInt8(uint8_t* data, uint32_t* idx);
uint16_t readInt16(uint8_t* data, uint32_t* idx);

void jsSceneInit(jsScene_t* scene);
void jsSimulate(jsScene_t* scene);
void jsSceneDraw(jsScene_t* scene);
void jsButtonPressed(jsScene_t* scene, buttonEvt_t* event);
