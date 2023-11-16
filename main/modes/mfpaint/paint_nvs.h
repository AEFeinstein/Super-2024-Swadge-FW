#ifndef _PAINT_NVS_H_
#define _PAINT_NVS_H_

#include <stdint.h>
#include <stdbool.h>
#include <malloc.h>

#include "settingsManager.h"
#include "hdw-nvs.h"

#include "paint_common.h"
#include "paint_type.h"
#include "paint_canvas.h"

#define PAINT_NS_DATA    "paint_img"
#define PAINT_NS_PALETTE "paint_pal"

// Settings bounds for the menu
const settingParam_t* paintGetEnableLedsBounds(void);
const settingParam_t* paintGetEnableBlinkBounds(void);

// Getters / setters for the new separated values
bool paintGetEnableLeds(void);
void paintSetEnableLeds(bool enableLeds);
bool paintGetEnableBlink(void);
void paintSetEnableBlink(bool enableBlink);

bool paintGetAnySlotInUse(void);
bool paintSaveNamed(const char* name, const paintCanvas_t* canvas);
bool paintLoadNamed(const char* name, paintCanvas_t* canvas);
void paintDeleteNamed(const char* name);
bool paintSlotExists(const char* name);
bool paintGetLastSlot(char* out);
void paintSetLastSlot(const char* name);

#endif
