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
    wsgPaletteSet(palette, c334, color);
}