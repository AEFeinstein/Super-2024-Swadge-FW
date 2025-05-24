#ifndef _IMAGE_PROCESSOR_H_
#define _IMAGE_PROCESSOR_H_

#include "assets_preprocessor.h"

/**
 * @brief The image processor converts PNG images to 8-bit images using the 216-color
 * web-safe palette plus one bit of transparency. Colors outside the palette will be
 * replaced with their closest value within the palette.
 *
 */
extern const assetProcessor_t imageProcessor;

#endif /* _IMAGE_PROCESSOR_H_ */