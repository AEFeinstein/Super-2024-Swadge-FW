//==============================================================================
// Includes
//==============================================================================
#include "dn_utility.h"
#include <math.h>

//==============================================================================
// Functions
//==============================================================================
vec_t dn_boardToWorldPos(dn_boardPos_t boardPos)
{
    vec_t worldPos;
    worldPos.x = 0xFFFF + ((((boardPos.x - boardPos.y - 1) * (DN_TILE_WIDTH >> 1)) << DN_DECIMAL_BITS));
    worldPos.y = 0xFFFF + ((((boardPos.x + boardPos.y) * DN_TILE_HEIGHT)) << DN_DECIMAL_BITS);
    return worldPos;
}

dn_assetIdx_t dn_getAssetIdx(dn_characterSet_t characterSet, dn_unitRank rank, dn_facingDir facingDir)
{
    switch (characterSet)
    {
        case DN_ALPHA_SET:
        {
            switch (rank)
            {
                case DN_PAWN:
                    return facingDir ? DN_BUCKET_HAT_DOWN_ASSET : DN_BUCKET_HAT_UP_ASSET;
                case DN_KING:
                    return facingDir ? DN_ALPHA_DOWN_ASSET : DN_ALPHA_UP_ASSET;
            }
            break;
        }
        case DN_CHESS_SET:
        {
            switch (rank)
            {
                case DN_PAWN:
                    return DN_PAWN_ASSET;
                case DN_KING:
                    return DN_KING_ASSET;
            }
        }
    }
    return -1; // Invalid asset index
}

void dn_setFloorPalette(wsgPalette_t* palette, paletteColor_t color)
{
    wsgPaletteSet(palette, c112, color);
    wsgPaletteSet(palette, c223, color);
}

/**
 * @brief Lerp between a and b by amount
 *
 * @param a One of two inputs
 * @param b One of two inputs
 * @param amount Lerp amount from 0 to 30000. 0 returns a, 30000 returns b.
 */
int dn_lerp(int a, int b, uint16_t amount)
{
    return a + ((b - a) * amount) / 30000;
}

//input 0, output 0
//input 30000, output 30000
int16_t dn_logRemap(int16_t x)
{
    if (x <= 0) return 0;

    float factor = pow(x / 30000.0, 0.4);  // Exponent > 1 flattens early
    return (int16_t)(30000 * factor);
}