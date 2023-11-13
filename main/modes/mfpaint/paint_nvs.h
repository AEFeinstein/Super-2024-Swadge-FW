#ifndef _PAINT_NVS_H_
#define _PAINT_NVS_H_

#include <stdint.h>
#include <stdbool.h>
#include <malloc.h>

#include "settingsManager.h"
#include "hdw-nvs.h"

#include "paint_common.h"
#include "paint_type.h"

#define PAINT_NS_DATA    "paint_img"
#define PAINT_NS_PALETTE "paint_pal"

// Settings bounds for the menu
const settingParam_t* paintGetInUseSlotBounds(void);
const settingParam_t* paintGetRecentSlotBounds(void);
const settingParam_t* paintGetEnableLedsBounds(void);
const settingParam_t* paintGetEnableBlinkBounds(void);
const settingParam_t* paintGetMigratedBounds(void);

// Getters / setters for the new separated values
int32_t paintGetInUseSlots(void);
void paintSetInUseSlots(int32_t inUseSlots);
// int32_t paintGetRecentSlot(void);
// void paintSetRecentSlot(int32_t recentSlot);
bool paintGetEnableLeds(void);
void paintSetEnableLeds(bool enableLeds);
bool paintGetEnableBlink(void);
void paintSetEnableBlink(bool enableBlink);

// void paintDebugIndex(int32_t index);
void paintLoadIndex(int32_t* dest);
void paintSaveIndex(int32_t index);
// void paintResetStorage(int32_t* index);
bool paintGetSlotInUse(int32_t index, uint8_t slot);
void paintClearSlot(int32_t* index, uint8_t slot);
void paintSetSlotInUse(int32_t* index, uint8_t slot);
bool paintGetAnySlotInUse(int32_t index);
uint8_t paintGetRecentSlot(int32_t index);
void paintSetRecentSlot(int32_t* index, uint8_t slot);
size_t paintGetStoredSize(const paintCanvas_t* canvas);
bool paintDeserialize(paintCanvas_t* dest, const uint8_t* data, size_t offset, size_t count);
size_t paintSerializeWsg(uint8_t* dest, const wsg_t* wsg);
size_t paintSerialize(uint8_t* dest, const paintCanvas_t* canvas, size_t offset, size_t count);
bool paintSave(int32_t* index, const paintCanvas_t* canvas, uint8_t slot);
bool paintLoad(int32_t* index, paintCanvas_t* canvas, uint8_t slot);
bool paintLoadDimensions(paintCanvas_t* canvas, uint8_t slot);

bool paintSaveNamed(const char* name, const paintCanvas_t* canvas);
bool paintLoadNamed(const char* name, paintCanvas_t* canvas);
bool paintLoadDimensionsNamed(const char* name, paintCanvas_t* canvas);
void paintDeleteNamed(const char* name);
bool paintSlotExists(const char* name);
bool paintGetLastSlot(char* out);
void paintSetLastSlot(const char* name);

uint8_t paintGetPrevSlotInUse(int32_t index, uint8_t slot);
uint8_t paintGetNextSlotInUse(int32_t index, uint8_t slot);
void paintDeleteSlot(int32_t* index, uint8_t slot);
bool paintDeleteIndex(void);

#endif
