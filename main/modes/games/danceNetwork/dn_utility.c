//==============================================================================
// Includes
//==============================================================================
#include "dn_utility.h"

//==============================================================================
// Functions
//==============================================================================
vec_t dn_boardToWorldPos(dn_boardPos_t boardPos)
{
    vec_t worldPos;
    worldPos.x = 0xFFFF + ((((boardPos.x - boardPos.y - 1) * (DN_TILE_WIDTH >> 1))<<DN_DECIMAL_BITS));
    worldPos.y = 0xFFFF + ((((boardPos.x + boardPos.y) * DN_TILE_HEIGHT))<<DN_DECIMAL_BITS);
    return worldPos;
}