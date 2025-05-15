#ifndef _IMAGE_PROCESSOR_H_
#define _IMAGE_PROCESSOR_H_

#include <stdbool.h>
#include "assets_preprocessor.h"

bool process_image(processorInput_t* arg);

extern const assetProcessor_t imageProcessor;

#endif /* _IMAGE_PROCESSOR_H_ */