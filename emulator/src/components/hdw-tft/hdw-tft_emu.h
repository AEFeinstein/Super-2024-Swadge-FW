#pragma once

const paletteColor_t* getLastTftBitmap(void);
uint32_t* getDisplayBitmap(uint16_t* width, uint16_t* height);
void setDisplayBitmapMultiplier(uint8_t multiplier);