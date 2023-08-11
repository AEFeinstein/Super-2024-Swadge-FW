#ifndef _PAINT_NVS_H_
#define _PAINT_NVS_H_

#include <stdint.h>
#include <stdbool.h>
#include <malloc.h>

#include "nvs_manager.h"

#include "paint_common.h"
#include "paint_type.h"

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
size_t paintSerialize(uint8_t* dest, const paintCanvas_t* canvas, size_t offset, size_t count);
bool paintSave(int32_t* index, const paintCanvas_t* canvas, uint8_t slot);
bool paintLoad(int32_t* index, paintCanvas_t* canvas, uint8_t slot);
bool paintLoadDimensions(paintCanvas_t* canvas, uint8_t slot);
uint8_t paintGetPrevSlotInUse(int32_t index, uint8_t slot);
uint8_t paintGetNextSlotInUse(int32_t index, uint8_t slot);
void paintDeleteSlot(int32_t* index, uint8_t slot);
bool paintDeleteIndex(void);


#endif
