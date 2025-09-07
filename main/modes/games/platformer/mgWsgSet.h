#ifndef _WSGSET_H_
#define _WSGSET_H_

//==============================================================================
// Includes
//==============================================================================
#include <stdint.h>

//==============================================================================
// Structs
//==============================================================================
typedef struct
{
    uint16_t tile_solid_visible_noninteractive_startingTileIndex;
    cnfsFileIdx_t tile_solid_visible_noninteractive_firstFilename;
    uint16_t tile_solid_visible_noninteractive_numWsgsToLoad;

    uint16_t tile_solid_visible_interactive_startingTileIndex;
    cnfsFileIdx_t tile_solid_visible_interactive_firstFilename;
    uint16_t tile_solid_visible_interactive_numWsgsToLoad;

    uint16_t tile_nonsolid_visible_interactive_startingTileIndex;
    cnfsFileIdx_t tile_nonsolid_visible_interactive_firstFilename;
    uint16_t tile_nonsolid_visible_interactive_numWsgsToLoad;

    uint16_t tile_nonsolid_visible_noninteractive_startingTileIndex;
    cnfsFileIdx_t tile_nonsolid_visible_noninteractive_firstFilename;
    uint16_t tile_nonsolid_visible_noninteractive_numWsgsToLoad;
} mgWsgSet_t;

#endif
