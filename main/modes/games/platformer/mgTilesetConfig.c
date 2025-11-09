#include <stdlib.h>
#include <stdbool.h>
#include "mgTilesetConfig.h"
#include "mega_pulse_ex_typedef.h"

bool mg_kineticDonutTileset_needsTransparency(uint8_t tileId)
{
    switch (tileId)
    {
        case MG_TILE_SOLID_VISIBLE_NONINTERACTIVE_20 ... MG_TILE_SOLID_VISIBLE_NONINTERACTIVE_6E:
            return false;
        case MG_TILE_NONSOLID_VISIBLE_INTERACTIVE_A0 ... MG_TILE_NONSOLID_VISIBLE_INTERACTIVE_A5:
            return true;
        case MG_TILE_NONSOLID_VISIBLE_NONINTERACTIVE_C0 ... MG_TILE_NONSOLID_VISIBLE_NONINTERACTIVE_C5:
            return true;
        default:
            return false;
    }
}

void mg_kineticDonutTileset_animateTiles(uint8_t tileId)
{
    // Nothing to do here... yet
}

bool mg_levelSelectTileset_needsTransparency(uint8_t tileId)
{
    switch (tileId)
    {
        case MG_TILE_SOLID_VISIBLE_NONINTERACTIVE_20 ... MG_TILE_SOLID_VISIBLE_NONINTERACTIVE_28:
            return true;
        default:
            return false;
    }
}

void mg_levelSelectTileset_animateTiles(uint8_t tileId)
{
    // Nothing to do here... yet
}