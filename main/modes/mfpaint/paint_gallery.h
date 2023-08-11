#ifndef _PAINT_GALLERY_H_
#define _PAINT_GALLERY_H_

#include <stdint.h>

#include "swadge2024.h"

#include "paint_common.h"

extern paintGallery_t* paintGallery;

void paintGallerySetup(bool screensaver);
void paintGalleryCleanup(void);
void paintGalleryMainLoop(int64_t elapsedUs);
void paintGalleryModeButtonCb(buttonEvt_t* evt);
void paintGalleryModeTouchCb(touch_event_t* evt);

void paintGalleryModePollTouch(void);

void paintGalleryDrawUi(void);
void paintGalleryAddInfoText(const char* text, int8_t row, bool center, char leftArrow, char rightArrow);
void paintGalleryDecreaseSpeed(void);
void paintGalleryIncreaseSpeed(void);

bool paintGalleryDoLoad(void);

#endif
