#include "model.h"

#include <stdbool.h>

bool loadModel(const char* name, model_t* model, bool useSpiRam);
void freeModel(model_t* model);