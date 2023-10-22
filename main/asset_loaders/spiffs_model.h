#ifndef _SPIFFS_MODEL_H_
#define _SPIFFS_MODEL_H_

#include "model.h"

#include <stdbool.h>

bool loadModel(const char* name, model_t* model, bool useSpiRam);
void freeModel(model_t* model);

#endif
