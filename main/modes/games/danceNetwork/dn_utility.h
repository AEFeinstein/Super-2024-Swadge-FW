#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <vector2d.h>
#include "dn_typedef.h"

vec_t dn_boardToWorldPos(dn_boardPos_t boardPos);

dn_assetIdx_t dn_getAssetIdx(dn_characterSet_t characterSet, dn_unitRank rank, dn_facingDir facingDir);