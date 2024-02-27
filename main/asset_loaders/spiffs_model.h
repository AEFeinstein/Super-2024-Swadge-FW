#ifndef _SPIFFS_MODEL_H_
#define _SPIFFS_MODEL_H_

#include "model.h"
#include "small3dlib.h"

#include <stdbool.h>

bool loadModel(const char* name, model_t* model, bool useSpiRam);
bool loadObjInfo(const char* name, object3dInfo_t* objInfo, bool useSpiRam);
void freeObjInfo(object3dInfo_t* objInfo);
void freeModel(model_t* model);

#endif
