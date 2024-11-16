//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include <stdbool.h>
#include "fs_wsg.h"
#include "paWsgManager.h"
#include "pango_typedef.h"

//==============================================================================
// Functions
//==============================================================================

void pa_initializeWsgManager(paWsgManager_t* self)
{
    pa_loadWsgs(self);
    pa_initializeSprites(self);
    pa_initializeTiles(self);
}

void pa_freeWsgManager(paWsgManager_t* self)
{
    for (uint16_t i = 0; i < PA_WSGS_SIZE; i++)
    {
        freeWsg(&self->wsgs[i]);
    }
}

void pa_loadWsgs(paWsgManager_t* self)
{
    loadWsg("pa-100.wsg", &self->wsgs[PA_WSG_PANGO_SOUTH], false);
    loadWsg("pa-101.wsg", &self->wsgs[PA_WSG_PANGO_WALK_SOUTH], false);
    loadWsg("pa-102.wsg", &self->wsgs[PA_WSG_PANGO_NORTH], false);
    loadWsg("pa-103.wsg", &self->wsgs[PA_WSG_PANGO_WALK_NORTH], false);
    loadWsg("pa-104.wsg", &self->wsgs[PA_WSG_PANGO_SIDE], false);
    loadWsg("pa-106.wsg", &self->wsgs[PA_WSG_PANGO_WALK_SIDE_1], false);
    loadWsg("pa-105.wsg", &self->wsgs[PA_WSG_PANGO_WALK_SIDE_2], false);
    loadWsg("pa-107.wsg", &self->wsgs[PA_WSG_PANGO_PUSH_SOUTH_1], false);
    loadWsg("pa-108.wsg", &self->wsgs[PA_WSG_PANGO_PUSH_SOUTH_2], false);
    loadWsg("pa-109.wsg", &self->wsgs[PA_WSG_PANGO_PUSH_NORTH_1], false);
    loadWsg("pa-110.wsg", &self->wsgs[PA_WSG_PANGO_PUSH_NORTH_2], false);
    loadWsg("pa-111.wsg", &self->wsgs[PA_WSG_PANGO_PUSH_SIDE_1], false);
    loadWsg("pa-112.wsg", &self->wsgs[PA_WSG_PANGO_PUSH_SIDE_2], false);
    loadWsg("pa-113.wsg", &self->wsgs[PA_WSG_PANGO_HURT], false);
    loadWsg("pa-114.wsg", &self->wsgs[PA_WSG_PANGO_WIN], false);
    loadWsg("pa-115.wsg", &self->wsgs[PA_WSG_PANGO_ICON], false);
    loadWsg("po-000.wsg", &self->wsgs[PA_WSG_PO_SOUTH], false);
    loadWsg("po-001.wsg", &self->wsgs[PA_WSG_PO_WALK_SOUTH], false);
    loadWsg("po-002.wsg", &self->wsgs[PA_WSG_PO_NORTH], false);
    loadWsg("po-003.wsg", &self->wsgs[PA_WSG_PO_WALK_NORTH], false);
    loadWsg("po-004.wsg", &self->wsgs[PA_WSG_PO_SIDE], false);
    loadWsg("po-006.wsg", &self->wsgs[PA_WSG_PO_WALK_SIDE_1], false);
    loadWsg("po-005.wsg", &self->wsgs[PA_WSG_PO_WALK_SIDE_2], false);
    loadWsg("po-007.wsg", &self->wsgs[PA_WSG_PO_PUSH_SOUTH_1], false);
    loadWsg("po-008.wsg", &self->wsgs[PA_WSG_PO_PUSH_SOUTH_2], false);
    loadWsg("po-009.wsg", &self->wsgs[PA_WSG_PO_PUSH_NORTH_1], false);
    loadWsg("po-010.wsg", &self->wsgs[PA_WSG_PO_PUSH_NORTH_2], false);
    loadWsg("po-011.wsg", &self->wsgs[PA_WSG_PO_PUSH_SIDE_1], false);
    loadWsg("po-012.wsg", &self->wsgs[PA_WSG_PO_PUSH_SIDE_2], false);
    loadWsg("po-013.wsg", &self->wsgs[PA_WSG_PO_HURT], false);
    loadWsg("po-014.wsg", &self->wsgs[PA_WSG_PO_WIN], false);
    loadWsg("po-015.wsg", &self->wsgs[PA_WSG_PO_ICON], false);
    loadWsg("px-000.wsg", &self->wsgs[PA_WSG_PIXEL_SOUTH], false);
    loadWsg("px-001.wsg", &self->wsgs[PA_WSG_PIXEL_WALK_SOUTH], false);
    loadWsg("px-002.wsg", &self->wsgs[PA_WSG_PIXEL_NORTH], false);
    loadWsg("px-003.wsg", &self->wsgs[PA_WSG_PIXEL_WALK_NORTH], false);
    loadWsg("px-004.wsg", &self->wsgs[PA_WSG_PIXEL_SIDE], false);
    loadWsg("px-006.wsg", &self->wsgs[PA_WSG_PIXEL_WALK_SIDE_1], false);
    loadWsg("px-005.wsg", &self->wsgs[PA_WSG_PIXEL_WALK_SIDE_2], false);
    loadWsg("px-007.wsg", &self->wsgs[PA_WSG_PIXEL_PUSH_SOUTH_1], false);
    loadWsg("px-008.wsg", &self->wsgs[PA_WSG_PIXEL_PUSH_SOUTH_2], false);
    loadWsg("px-009.wsg", &self->wsgs[PA_WSG_PIXEL_PUSH_NORTH_1], false);
    loadWsg("px-010.wsg", &self->wsgs[PA_WSG_PIXEL_PUSH_NORTH_2], false);
    loadWsg("px-011.wsg", &self->wsgs[PA_WSG_PIXEL_PUSH_SIDE_1], false);
    loadWsg("px-012.wsg", &self->wsgs[PA_WSG_PIXEL_PUSH_SIDE_2], false);
    loadWsg("px-013.wsg", &self->wsgs[PA_WSG_PIXEL_HURT], false);
    loadWsg("px-014.wsg", &self->wsgs[PA_WSG_PIXEL_WIN], false);
    loadWsg("px-015.wsg", &self->wsgs[PA_WSG_PIXEL_ICON], false);
    loadWsg("kr-000.wsg", &self->wsgs[PA_WSG_GIRL_SOUTH], false);
    loadWsg("kr-001.wsg", &self->wsgs[PA_WSG_GIRL_WALK_SOUTH], false);
    loadWsg("kr-002.wsg", &self->wsgs[PA_WSG_GIRL_NORTH], false);
    loadWsg("kr-003.wsg", &self->wsgs[PA_WSG_GIRL_WALK_NORTH], false);
    loadWsg("kr-004.wsg", &self->wsgs[PA_WSG_GIRL_SIDE], false);
    loadWsg("kr-006.wsg", &self->wsgs[PA_WSG_GIRL_WALK_SIDE_1], false);
    loadWsg("kr-005.wsg", &self->wsgs[PA_WSG_GIRL_WALK_SIDE_2], false);
    loadWsg("kr-007.wsg", &self->wsgs[PA_WSG_GIRL_PUSH_SOUTH_1], false);
    loadWsg("kr-008.wsg", &self->wsgs[PA_WSG_GIRL_PUSH_SOUTH_2], false);
    loadWsg("kr-009.wsg", &self->wsgs[PA_WSG_GIRL_PUSH_NORTH_1], false);
    loadWsg("kr-010.wsg", &self->wsgs[PA_WSG_GIRL_PUSH_NORTH_2], false);
    loadWsg("kr-011.wsg", &self->wsgs[PA_WSG_GIRL_PUSH_SIDE_1], false);
    loadWsg("kr-012.wsg", &self->wsgs[PA_WSG_GIRL_PUSH_SIDE_2], false);
    loadWsg("kr-013.wsg", &self->wsgs[PA_WSG_GIRL_HURT], false);
    loadWsg("kr-014.wsg", &self->wsgs[PA_WSG_GIRL_WIN], false);
    loadWsg("kr-015.wsg", &self->wsgs[PA_WSG_GIRL_ICON], false);
    // loadWsg("pa-tile-009.wsg", &self->wsgs[PA_WSG_BLOCK], false);
    // loadWsg("pa-tile-013.wsg", &self->wsgs[PA_WSG_BONUS_BLOCK], false);
    loadWsg("pa-en-004.wsg", &self->wsgs[PA_WSG_ENEMY_SOUTH], false);
    loadWsg("pa-en-006.wsg", &self->wsgs[PA_WSG_ENEMY_NORTH], false);
    loadWsg("pa-en-000.wsg", &self->wsgs[PA_WSG_ENEMY_SIDE_1], false);
    loadWsg("pa-en-001.wsg", &self->wsgs[PA_WSG_ENEMY_SIDE_2], false);
    loadWsg("pa-en-005.wsg", &self->wsgs[PA_WSG_ENEMY_DRILL_SOUTH], false);
    loadWsg("pa-en-007.wsg", &self->wsgs[PA_WSG_ENEMY_DRILL_NORTH], false);
    loadWsg("pa-en-008.wsg", &self->wsgs[PA_WSG_ENEMY_STUN], false);
    loadWsg("pa-en-002.wsg", &self->wsgs[PA_WSG_ENEMY_DRILL_SIDE_1], false);
    loadWsg("pa-en-003.wsg", &self->wsgs[PA_WSG_ENEMY_DRILL_SIDE_2], false);
    loadWsg("break-000.wsg", &self->wsgs[PA_WSG_BREAK_BLOCK], false);
    loadWsg("break-001.wsg", &self->wsgs[PA_WSG_BREAK_BLOCK_1], false);
    loadWsg("break-002.wsg", &self->wsgs[PA_WSG_BREAK_BLOCK_2], false);
    loadWsg("break-003.wsg", &self->wsgs[PA_WSG_BREAK_BLOCK_3], false);
    loadWsg("blockfragment.wsg", &self->wsgs[PA_WSG_BLOCK_FRAGMENT], false);

    loadWsg("pa-tile-001.wsg", &self->wsgs[PA_WSG_WALL_0], false);
    loadWsg("pa-tile-002.wsg", &self->wsgs[PA_WSG_WALL_1], false);
    loadWsg("pa-tile-003.wsg", &self->wsgs[PA_WSG_WALL_2], false);
    loadWsg("pa-tile-004.wsg", &self->wsgs[PA_WSG_WALL_3], false);
    loadWsg("pa-tile-005.wsg", &self->wsgs[PA_WSG_WALL_4], false);
    loadWsg("pa-tile-006.wsg", &self->wsgs[PA_WSG_WALL_5], false);
    loadWsg("pa-tile-007.wsg", &self->wsgs[PA_WSG_WALL_6], false);
    loadWsg("pa-tile-008.wsg", &self->wsgs[PA_WSG_WALL_7], false);
    loadWsg("pa-tile-009.wsg", &self->wsgs[PA_WSG_BLOCK_BLUE], false);
    loadWsg("pa-tile-010.wsg", &self->wsgs[PA_WSG_BLOCK_MAGENTA], false);
    loadWsg("pa-tile-011.wsg", &self->wsgs[PA_WSG_BLOCK_RED], false);
    loadWsg("pa-tile-012.wsg", &self->wsgs[PA_WSG_BLOCK_ORANGE], false);
    loadWsg("pa-tile-013.wsg", &self->wsgs[PA_WSG_BLOCK_YELLOW], false);
    loadWsg("pa-tile-014.wsg", &self->wsgs[PA_WSG_BLOCK_GREEN], false);
    loadWsg("pa-tile-015.wsg", &self->wsgs[PA_WSG_BLOCK_TITLESCREEN], false);

    loadWsg("pa-hotdog.wsg", &self->wsgs[PA_WSG_HOTDOG], false);
}

void pa_initializeSprites(paWsgManager_t* self)
{
    self->sprites[PA_SP_PLAYER_SOUTH].wsg     = &self->wsgs[PA_WSG_PANGO_SOUTH];
    self->sprites[PA_SP_PLAYER_SOUTH].originX = 8;
    self->sprites[PA_SP_PLAYER_SOUTH].originY = 16;

    self->sprites[PA_SP_PLAYER_WALK_SOUTH].wsg     = &self->wsgs[PA_WSG_PANGO_WALK_SOUTH];
    self->sprites[PA_SP_PLAYER_WALK_SOUTH].originX = 8;
    self->sprites[PA_SP_PLAYER_WALK_SOUTH].originY = 16;

    self->sprites[PA_SP_PLAYER_NORTH].wsg     = &self->wsgs[PA_WSG_PANGO_NORTH];
    self->sprites[PA_SP_PLAYER_NORTH].originX = 8;
    self->sprites[PA_SP_PLAYER_NORTH].originY = 16;

    self->sprites[PA_SP_PLAYER_WALK_NORTH].wsg     = &self->wsgs[PA_WSG_PANGO_WALK_NORTH];
    self->sprites[PA_SP_PLAYER_WALK_NORTH].originX = 8;
    self->sprites[PA_SP_PLAYER_WALK_NORTH].originY = 16;

    self->sprites[PA_SP_PLAYER_SIDE].wsg     = &self->wsgs[PA_WSG_PANGO_SIDE];
    self->sprites[PA_SP_PLAYER_SIDE].originX = 8;
    self->sprites[PA_SP_PLAYER_SIDE].originY = 16;

    self->sprites[PA_SP_PLAYER_WALK_SIDE_1].wsg     = &self->wsgs[PA_WSG_PANGO_WALK_SIDE_1];
    self->sprites[PA_SP_PLAYER_WALK_SIDE_1].originX = 8;
    self->sprites[PA_SP_PLAYER_WALK_SIDE_1].originY = 16;

    self->sprites[PA_SP_PLAYER_WALK_SIDE_2].wsg     = &self->wsgs[PA_WSG_PANGO_WALK_SIDE_2];
    self->sprites[PA_SP_PLAYER_WALK_SIDE_2].originX = 8;
    self->sprites[PA_SP_PLAYER_WALK_SIDE_2].originY = 16;

    self->sprites[PA_SP_PLAYER_PUSH_SOUTH_1].wsg     = &self->wsgs[PA_WSG_PANGO_PUSH_SOUTH_1];
    self->sprites[PA_SP_PLAYER_PUSH_SOUTH_1].originX = 8;
    self->sprites[PA_SP_PLAYER_PUSH_SOUTH_1].originY = 16;

    self->sprites[PA_SP_PLAYER_PUSH_SOUTH_2].wsg     = &self->wsgs[PA_WSG_PANGO_PUSH_SOUTH_2];
    self->sprites[PA_SP_PLAYER_PUSH_SOUTH_2].originX = 8;
    self->sprites[PA_SP_PLAYER_PUSH_SOUTH_2].originY = 16;

    self->sprites[PA_SP_PLAYER_PUSH_NORTH_1].wsg     = &self->wsgs[PA_WSG_PANGO_PUSH_NORTH_1];
    self->sprites[PA_SP_PLAYER_PUSH_NORTH_1].originX = 8;
    self->sprites[PA_SP_PLAYER_PUSH_NORTH_1].originY = 16;

    self->sprites[PA_SP_PLAYER_PUSH_NORTH_2].wsg     = &self->wsgs[PA_WSG_PANGO_PUSH_NORTH_2];
    self->sprites[PA_SP_PLAYER_PUSH_NORTH_2].originX = 8;
    self->sprites[PA_SP_PLAYER_PUSH_NORTH_2].originY = 16;

    self->sprites[PA_SP_PLAYER_PUSH_SIDE_1].wsg     = &self->wsgs[PA_WSG_PANGO_PUSH_SIDE_1];
    self->sprites[PA_SP_PLAYER_PUSH_SIDE_1].originX = 8;
    self->sprites[PA_SP_PLAYER_PUSH_SIDE_1].originY = 16;

    self->sprites[PA_SP_PLAYER_PUSH_SIDE_2].wsg     = &self->wsgs[PA_WSG_PANGO_PUSH_SIDE_2];
    self->sprites[PA_SP_PLAYER_PUSH_SIDE_2].originX = 8;
    self->sprites[PA_SP_PLAYER_PUSH_SIDE_2].originY = 16;

    self->sprites[PA_SP_PLAYER_HURT].wsg     = &self->wsgs[PA_WSG_PANGO_HURT];
    self->sprites[PA_SP_PLAYER_HURT].originX = 8;
    self->sprites[PA_SP_PLAYER_HURT].originY = 16;

    self->sprites[PA_SP_PLAYER_WIN].wsg     = &self->wsgs[PA_SP_PLAYER_WIN];
    self->sprites[PA_SP_PLAYER_WIN].originX = 8;
    self->sprites[PA_SP_PLAYER_WIN].originY = 16;

    self->sprites[PA_SP_PLAYER_ICON].wsg     = &self->wsgs[PA_SP_PLAYER_ICON];
    self->sprites[PA_SP_PLAYER_ICON].originX = 8;
    self->sprites[PA_SP_PLAYER_ICON].originY = 16;

    self->sprites[PA_SP_BLOCK].wsg     = &self->wsgs[PA_WSG_BLOCK_BLUE];
    self->sprites[PA_SP_BLOCK].originX = 8;
    self->sprites[PA_SP_BLOCK].originY = 8;

    self->sprites[PA_SP_BONUS_BLOCK].wsg     = &self->wsgs[PA_WSG_BLOCK_RED];
    self->sprites[PA_SP_BONUS_BLOCK].originX = 8;
    self->sprites[PA_SP_BONUS_BLOCK].originY = 8;

    self->sprites[PA_SP_ENEMY_SOUTH].wsg     = &self->wsgs[PA_WSG_ENEMY_SOUTH];
    self->sprites[PA_SP_ENEMY_SOUTH].originX = 8;
    self->sprites[PA_SP_ENEMY_SOUTH].originY = 16;

    self->sprites[PA_SP_ENEMY_NORTH].wsg     = &self->wsgs[PA_WSG_ENEMY_NORTH];
    self->sprites[PA_SP_ENEMY_NORTH].originX = 8;
    self->sprites[PA_SP_ENEMY_NORTH].originY = 16;

    self->sprites[PA_SP_ENEMY_SIDE_1].wsg     = &self->wsgs[PA_WSG_ENEMY_SIDE_1];
    self->sprites[PA_SP_ENEMY_SIDE_1].originX = 8;
    self->sprites[PA_SP_ENEMY_SIDE_1].originY = 16;

    self->sprites[PA_SP_ENEMY_SIDE_2].wsg     = &self->wsgs[PA_WSG_ENEMY_SIDE_2];
    self->sprites[PA_SP_ENEMY_SIDE_2].originX = 8;
    self->sprites[PA_SP_ENEMY_SIDE_2].originY = 16;

    self->sprites[PA_SP_ENEMY_DRILL_SOUTH].wsg     = &self->wsgs[PA_WSG_ENEMY_DRILL_SOUTH];
    self->sprites[PA_SP_ENEMY_DRILL_SOUTH].originX = 8;
    self->sprites[PA_SP_ENEMY_DRILL_SOUTH].originY = 16;

    self->sprites[PA_SP_ENEMY_DRILL_NORTH].wsg     = &self->wsgs[PA_WSG_ENEMY_DRILL_NORTH];
    self->sprites[PA_SP_ENEMY_DRILL_NORTH].originX = 8;
    self->sprites[PA_SP_ENEMY_DRILL_NORTH].originY = 16;

    self->sprites[PA_SP_ENEMY_STUN].wsg     = &self->wsgs[PA_WSG_ENEMY_STUN];
    self->sprites[PA_SP_ENEMY_STUN].originX = 8;
    self->sprites[PA_SP_ENEMY_STUN].originY = 16;

    self->sprites[PA_SP_ENEMY_DRILL_SIDE_1].wsg     = &self->wsgs[PA_WSG_ENEMY_DRILL_SIDE_1];
    self->sprites[PA_SP_ENEMY_DRILL_SIDE_1].originX = 8;
    self->sprites[PA_SP_ENEMY_DRILL_SIDE_1].originY = 16;

    self->sprites[PA_SP_ENEMY_DRILL_SIDE_2].wsg     = &self->wsgs[PA_WSG_ENEMY_DRILL_SIDE_2];
    self->sprites[PA_SP_ENEMY_DRILL_SIDE_2].originX = 8;
    self->sprites[PA_SP_ENEMY_DRILL_SIDE_2].originY = 16;

    self->sprites[PA_SP_BREAK_BLOCK].wsg     = &self->wsgs[PA_WSG_BREAK_BLOCK];
    self->sprites[PA_SP_BREAK_BLOCK].originX = 8;
    self->sprites[PA_SP_BREAK_BLOCK].originY = 8;

    self->sprites[PA_SP_BREAK_BLOCK_1].wsg     = &self->wsgs[PA_WSG_BREAK_BLOCK_1];
    self->sprites[PA_SP_BREAK_BLOCK_1].originX = 8;
    self->sprites[PA_SP_BREAK_BLOCK_1].originY = 8;

    self->sprites[PA_SP_BREAK_BLOCK_2].wsg     = &self->wsgs[PA_WSG_BREAK_BLOCK_2];
    self->sprites[PA_SP_BREAK_BLOCK_2].originX = 8;
    self->sprites[PA_SP_BREAK_BLOCK_2].originY = 8;

    self->sprites[PA_SP_BREAK_BLOCK_3].wsg     = &self->wsgs[PA_WSG_BREAK_BLOCK_3];
    self->sprites[PA_SP_BREAK_BLOCK_3].originX = 8;
    self->sprites[PA_SP_BREAK_BLOCK_3].originY = 8;

    self->sprites[PA_SP_BLOCK_FRAGMENT].wsg     = &self->wsgs[PA_WSG_BLOCK_FRAGMENT];
    self->sprites[PA_SP_BLOCK_FRAGMENT].originX = 3;
    self->sprites[PA_SP_BLOCK_FRAGMENT].originY = 3;

    self->sprites[PA_SP_HOTDOG].wsg     = &self->wsgs[PA_WSG_HOTDOG];
    self->sprites[PA_SP_HOTDOG].originX = 8;
    self->sprites[PA_SP_HOTDOG].originY = 8;
}

void pa_initializeTiles(paWsgManager_t* self)
{
    self->tiles[0]  = &self->wsgs[PA_WSG_WALL_0];
    self->tiles[1]  = &self->wsgs[PA_WSG_WALL_1];
    self->tiles[2]  = &self->wsgs[PA_WSG_WALL_2];
    self->tiles[3]  = &self->wsgs[PA_WSG_WALL_3];
    self->tiles[4]  = &self->wsgs[PA_WSG_WALL_4];
    self->tiles[5]  = &self->wsgs[PA_WSG_WALL_5];
    self->tiles[6]  = &self->wsgs[PA_WSG_WALL_6];
    self->tiles[7]  = &self->wsgs[PA_WSG_WALL_7];
    self->tiles[8]  = &self->wsgs[PA_WSG_BLOCK_BLUE];
    self->tiles[9]  = &self->wsgs[PA_WSG_BLOCK_RED];
    self->tiles[10] = &self->wsgs[PA_WSG_BLOCK_ORANGE];
    self->tiles[11] = &self->wsgs[PA_WSG_BLOCK_YELLOW];
    self->tiles[12] = &self->wsgs[PA_WSG_BLOCK_GREEN];
    self->tiles[13] = &self->wsgs[PA_WSG_BLOCK_MAGENTA];
    self->tiles[14] = &self->wsgs[PA_WSG_BLOCK_TITLESCREEN];
}

void pa_remapWsgToSprite(paWsgManager_t* self, uint16_t spriteIndex, uint16_t wsgIndex)
{
    self->sprites[spriteIndex].wsg = &self->wsgs[wsgIndex];
}

void pa_remapWsgToTile(paWsgManager_t* self, uint16_t tileIndex, uint16_t wsgIndex)
{
    self->tiles[tileIndex] = &self->wsgs[wsgIndex];
}

void pa_remapPlayerCharacter(paWsgManager_t* self, uint16_t newBaseIndex)
{
    for (uint16_t i = 0; i < (PA_SP_PLAYER_ICON + 1); i++)
    {
        pa_remapWsgToSprite(self, i, newBaseIndex + i);
    }
}

void pa_animateTiles(paWsgManager_t* self)
{
    self->globalTileAnimationTimer--;
    if (self->globalTileAnimationTimer < 0)
    {
        // Assumption: all animated tiles have 6 frames of animation
        self->globalTileAnimationFrame = ((self->globalTileAnimationFrame + 1) % 6);

        pa_remapWsgToTile(self, 9, PA_WSG_BLOCK_BLUE + self->globalTileAnimationFrame);

        self->globalTileAnimationTimer = 6;
    }
}

void pa_remapBlockTile(paWsgManager_t* self, uint16_t newBlockWsgIndex)
{
    pa_remapWsgToTile(self, 8, newBlockWsgIndex);
    pa_remapWsgToSprite(self, PA_SP_BLOCK, newBlockWsgIndex);
}