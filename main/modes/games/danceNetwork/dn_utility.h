#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <vector2d.h>
#include "dn_typedef.h"
#include "wsgPalette.h"

vec_t dn_boardToWorldPos(dn_boardPos_t boardPos);

dn_assetIdx_t dn_getAssetIdx(dn_characterSet_t characterSet, dn_unitRank rank, dn_facingDir facingDir);

void dn_setFloorPalette(wsgPalette_t* palette, paletteColor_t color);

int16_t dn_lerp(int16_t a, int16_t b, uint16_t amount);

int16_t dn_logRemap(int16_t x);