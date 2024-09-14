//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include <stdbool.h>
#include "fs_wsg.h"
#include "paWsgManager.h"

//==============================================================================
// Functions
//==============================================================================

void pa_initializeWsgManager(paWsgManager_t* self) {
    pa_loadWsgs(self);
    pa_initializeSprites(self);
    pa_initializeTiles(self);
}

void pa_freeWsgManager(paWsgManager_t* self){
    for (uint16_t i = 0; i < PA_WSGS_SIZE; i++)
    {
        freeWsg(&self->wsgs[i]);
    }
}

void pa_loadWsgs(paWsgManager_t* self){
    loadWsg("pa-000.wsg", &self->wsgs[PA_WSG_PANGO_SOUTH], false);
    loadWsg("pa-001.wsg", &self->wsgs[PA_WSG_PANGO_WALK_SOUTH], false);
    loadWsg("pa-002.wsg", &self->wsgs[PA_WSG_PANGO_NORTH], false);
    loadWsg("pa-003.wsg", &self->wsgs[PA_WSG_PANGO_WALK_NORTH], false);
    loadWsg("pa-004.wsg", &self->wsgs[PA_WSG_PANGO_SIDE], false);
    loadWsg("pa-006.wsg", &self->wsgs[PA_WSG_PANGO_WALK_SIDE_1], false);
    loadWsg("pa-005.wsg", &self->wsgs[PA_WSG_PANGO_WALK_SIDE_2], false);
    loadWsg("pa-007.wsg", &self->wsgs[PA_WSG_PANGO_PUSH_SOUTH_1], false);
    loadWsg("pa-008.wsg", &self->wsgs[PA_WSG_PANGO_PUSH_SOUTH_2], false);
    loadWsg("pa-009.wsg", &self->wsgs[PA_WSG_PANGO_PUSH_NORTH_1], false);
    loadWsg("pa-010.wsg", &self->wsgs[PA_WSG_PANGO_PUSH_NORTH_2], false);
    loadWsg("pa-011.wsg", &self->wsgs[PA_WSG_PANGO_PUSH_SIDE_1], false);
    loadWsg("pa-012.wsg", &self->wsgs[PA_WSG_PANGO_PUSH_SIDE_2], false);
    loadWsg("pa-013.wsg", &self->wsgs[PA_WSG_PANGO_HURT], false);
    loadWsg("pa-014.wsg", &self->wsgs[PA_WSG_PANGO_WIN], false);
    loadWsg("pa-015.wsg", &self->wsgs[PA_WSG_PANGO_ICON], false);
    loadWsg("pa-000.wsg", &self->wsgs[PA_WSG_PO_SOUTH], false);
    loadWsg("pa-001.wsg", &self->wsgs[PA_WSG_PO_WALK_SOUTH], false);
    loadWsg("pa-002.wsg", &self->wsgs[PA_WSG_PO_NORTH], false);
    loadWsg("pa-003.wsg", &self->wsgs[PA_WSG_PO_WALK_NORTH], false);
    loadWsg("pa-004.wsg", &self->wsgs[PA_WSG_PO_SIDE], false);
    loadWsg("pa-006.wsg", &self->wsgs[PA_WSG_PO_WALK_SIDE_1], false);
    loadWsg("pa-005.wsg", &self->wsgs[PA_WSG_PO_WALK_SIDE_2], false);
    loadWsg("pa-007.wsg", &self->wsgs[PA_WSG_PO_PUSH_SOUTH_1], false);
    loadWsg("pa-008.wsg", &self->wsgs[PA_WSG_PO_PUSH_SOUTH_2], false);
    loadWsg("pa-009.wsg", &self->wsgs[PA_WSG_PO_PUSH_NORTH_1], false);
    loadWsg("pa-010.wsg", &self->wsgs[PA_WSG_PO_PUSH_NORTH_2], false);
    loadWsg("pa-011.wsg", &self->wsgs[PA_WSG_PO_PUSH_SIDE_1], false);
    loadWsg("pa-012.wsg", &self->wsgs[PA_WSG_PO_PUSH_SIDE_2], false);
    loadWsg("pa-013.wsg", &self->wsgs[PA_WSG_PO_HURT], false);
    loadWsg("pa-014.wsg", &self->wsgs[PA_WSG_PO_WIN], false);
    loadWsg("pa-015.wsg", &self->wsgs[PA_WSG_PO_ICON], false);
    loadWsg("pa-000.wsg", &self->wsgs[PA_WSG_PIXEL_SOUTH], false);
    loadWsg("pa-001.wsg", &self->wsgs[PA_WSG_PIXEL_WALK_SOUTH], false);
    loadWsg("pa-002.wsg", &self->wsgs[PA_WSG_PIXEL_NORTH], false);
    loadWsg("pa-003.wsg", &self->wsgs[PA_WSG_PIXEL_WALK_NORTH], false);
    loadWsg("pa-004.wsg", &self->wsgs[PA_WSG_PIXEL_SIDE], false);
    loadWsg("pa-006.wsg", &self->wsgs[PA_WSG_PIXEL_WALK_SIDE_1], false);
    loadWsg("pa-005.wsg", &self->wsgs[PA_WSG_PIXEL_WALK_SIDE_2], false);
    loadWsg("pa-007.wsg", &self->wsgs[PA_WSG_PIXEL_PUSH_SOUTH_1], false);
    loadWsg("pa-008.wsg", &self->wsgs[PA_WSG_PIXEL_PUSH_SOUTH_2], false);
    loadWsg("pa-009.wsg", &self->wsgs[PA_WSG_PIXEL_PUSH_NORTH_1], false);
    loadWsg("pa-010.wsg", &self->wsgs[PA_WSG_PIXEL_PUSH_NORTH_2], false);
    loadWsg("pa-011.wsg", &self->wsgs[PA_WSG_PIXEL_PUSH_SIDE_1], false);
    loadWsg("pa-012.wsg", &self->wsgs[PA_WSG_PIXEL_PUSH_SIDE_2], false);
    loadWsg("pa-013.wsg", &self->wsgs[PA_WSG_PIXEL_HURT], false);
    loadWsg("pa-014.wsg", &self->wsgs[PA_WSG_PIXEL_WIN], false);
    loadWsg("pa-015.wsg", &self->wsgs[PA_WSG_PIXEL_ICON], false);
    loadWsg("pa-000.wsg", &self->wsgs[PA_WSG_GIRL_SOUTH], false);
    loadWsg("pa-001.wsg", &self->wsgs[PA_WSG_GIRL_WALK_SOUTH], false);
    loadWsg("pa-002.wsg", &self->wsgs[PA_WSG_GIRL_NORTH], false);
    loadWsg("pa-003.wsg", &self->wsgs[PA_WSG_GIRL_WALK_NORTH], false);
    loadWsg("pa-004.wsg", &self->wsgs[PA_WSG_GIRL_SIDE], false);
    loadWsg("pa-006.wsg", &self->wsgs[PA_WSG_GIRL_WALK_SIDE_1], false);
    loadWsg("pa-005.wsg", &self->wsgs[PA_WSG_GIRL_WALK_SIDE_2], false);
    loadWsg("pa-007.wsg", &self->wsgs[PA_WSG_GIRL_PUSH_SOUTH_1], false);
    loadWsg("pa-008.wsg", &self->wsgs[PA_WSG_GIRL_PUSH_SOUTH_2], false);
    loadWsg("pa-009.wsg", &self->wsgs[PA_WSG_GIRL_PUSH_NORTH_1], false);
    loadWsg("pa-010.wsg", &self->wsgs[PA_WSG_GIRL_PUSH_NORTH_2], false);
    loadWsg("pa-011.wsg", &self->wsgs[PA_WSG_GIRL_PUSH_SIDE_1], false);
    loadWsg("pa-012.wsg", &self->wsgs[PA_WSG_GIRL_PUSH_SIDE_2], false);
    loadWsg("pa-013.wsg", &self->wsgs[PA_WSG_GIRL_HURT], false);
    loadWsg("pa-014.wsg", &self->wsgs[PA_WSG_GIRL_WIN], false);
    loadWsg("pa-015.wsg", &self->wsgs[PA_WSG_GIRL_ICON], false);
    //loadWsg("pa-tile-009.wsg", &self->wsgs[PA_WSG_BLOCK], false);
    //loadWsg("pa-tile-013.wsg", &self->wsgs[PA_WSG_BONUS_BLOCK], false);
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
    loadWsg("pa-tile-009.wsg", &self->wsgs[PA_WSG_BLOCK], false);
    loadWsg("pa-tile-010.wsg", &self->wsgs[PA_WSG_SPAWN_BLOCK_0], false);
    loadWsg("pa-tile-011.wsg", &self->wsgs[PA_WSG_SPAWN_BLOCK_1], false);
    loadWsg("pa-tile-012.wsg", &self->wsgs[PA_WSG_SPAWN_BLOCK_2], false);
    loadWsg("pa-tile-013.wsg", &self->wsgs[PA_WSG_BONUS_BLOCK_0], false);
    loadWsg("pa-tile-014.wsg", &self->wsgs[PA_WSG_BONUS_BLOCK_1], false);
    loadWsg("pa-tile-015.wsg", &self->wsgs[PA_WSG_BONUS_BLOCK_2], false);
}

void pa_initializeSprites(paWsgManager_t* self){
    self->sprites[PA_SP_PLAYER_SOUTH].wsg = &self->wsgs[PA_WSG_PANGO_SOUTH];
    self->sprites[PA_SP_PLAYER_SOUTH].originX         = 8;
    self->sprites[PA_SP_PLAYER_SOUTH].originY         = 16;
    
    self->sprites[PA_SP_PLAYER_WALK_SOUTH].wsg = &self->wsgs[PA_WSG_PANGO_WALK_SOUTH];
    self->sprites[PA_SP_PLAYER_WALK_SOUTH].originX         = 8;
    self->sprites[PA_SP_PLAYER_WALK_SOUTH].originY         = 16;
    
    self->sprites[PA_SP_PLAYER_NORTH].wsg = &self->wsgs[PA_WSG_PANGO_NORTH];
    self->sprites[PA_SP_PLAYER_NORTH].originX         = 8;
    self->sprites[PA_SP_PLAYER_NORTH].originY         = 16;
    
    self->sprites[PA_SP_PLAYER_WALK_NORTH].wsg = &self->wsgs[PA_WSG_PANGO_WALK_NORTH];
    self->sprites[PA_SP_PLAYER_WALK_NORTH].originX         = 8;
    self->sprites[PA_SP_PLAYER_WALK_NORTH].originY         = 16;
    
    self->sprites[PA_SP_PLAYER_SIDE].wsg = &self->wsgs[PA_WSG_PANGO_SIDE];
    self->sprites[PA_SP_PLAYER_SIDE].originX         = 8;
    self->sprites[PA_SP_PLAYER_SIDE].originY         = 16;
    
    self->sprites[PA_SP_PLAYER_WALK_SIDE_1].wsg = &self->wsgs[PA_WSG_PANGO_WALK_SIDE_1];
    self->sprites[PA_SP_PLAYER_WALK_SIDE_1].originX         = 8;
    self->sprites[PA_SP_PLAYER_WALK_SIDE_1].originY         = 16;
    
    self->sprites[PA_SP_PLAYER_WALK_SIDE_2].wsg = &self->wsgs[PA_WSG_PANGO_WALK_SIDE_2];
    self->sprites[PA_SP_PLAYER_WALK_SIDE_2].originX         = 8;
    self->sprites[PA_SP_PLAYER_WALK_SIDE_2].originY         = 16;
   
    self->sprites[PA_SP_PLAYER_PUSH_SOUTH_1].wsg = &self->wsgs[PA_WSG_PANGO_PUSH_SOUTH_1];
    self->sprites[PA_SP_PLAYER_PUSH_SOUTH_1].originX         = 8;
    self->sprites[PA_SP_PLAYER_PUSH_SOUTH_1].originY         = 16;
    
    self->sprites[PA_SP_PLAYER_PUSH_SOUTH_2].wsg = &self->wsgs[PA_WSG_PANGO_PUSH_SOUTH_2];
    self->sprites[PA_SP_PLAYER_PUSH_SOUTH_2].originX         = 8;
    self->sprites[PA_SP_PLAYER_PUSH_SOUTH_2].originY         = 16;

    self->sprites[PA_SP_PLAYER_PUSH_NORTH_1].wsg = &self->wsgs[PA_WSG_PANGO_PUSH_NORTH_1];
    self->sprites[PA_SP_PLAYER_PUSH_NORTH_1].originX         = 8;
    self->sprites[PA_SP_PLAYER_PUSH_NORTH_1].originY         = 16;

    self->sprites[PA_SP_PLAYER_PUSH_NORTH_2].wsg = &self->wsgs[PA_WSG_PANGO_PUSH_NORTH_2];
    self->sprites[PA_SP_PLAYER_PUSH_NORTH_2].originX         = 8;
    self->sprites[PA_SP_PLAYER_PUSH_NORTH_2].originY         = 16;

    self->sprites[PA_SP_PLAYER_PUSH_SIDE_1].wsg = &self->wsgs[PA_WSG_PANGO_PUSH_SIDE_1];
    self->sprites[PA_SP_PLAYER_PUSH_SIDE_1].originX         = 8;
    self->sprites[PA_SP_PLAYER_PUSH_SIDE_1].originY         = 16;

    self->sprites[PA_SP_PLAYER_PUSH_SIDE_2].wsg = &self->wsgs[PA_WSG_PANGO_PUSH_SIDE_2];
    self->sprites[PA_SP_PLAYER_PUSH_SIDE_2].originX         = 8;
    self->sprites[PA_SP_PLAYER_PUSH_SIDE_2].originY         = 16;

    self->sprites[PA_SP_PLAYER_HURT].wsg = &self->wsgs[PA_WSG_PANGO_HURT];
    self->sprites[PA_SP_PLAYER_HURT].originX         = 8;
    self->sprites[PA_SP_PLAYER_HURT].originY         = 16;

    self->sprites[PA_SP_PLAYER_WIN].wsg = &self->wsgs[PA_SP_PLAYER_WIN];
    self->sprites[PA_SP_PLAYER_WIN].originX         = 8;
    self->sprites[PA_SP_PLAYER_WIN].originY         = 16;

    self->sprites[PA_SP_PLAYER_ICON].wsg = &self->wsgs[PA_SP_PLAYER_ICON];
    self->sprites[PA_SP_PLAYER_ICON].originX         = 8;
    self->sprites[PA_SP_PLAYER_ICON].originY         = 16;
    
    self->sprites[PA_SP_BLOCK].wsg = &self->wsgs[PA_WSG_BLOCK];
    self->sprites[PA_SP_BLOCK].originX         = 8;
    self->sprites[PA_SP_BLOCK].originY         = 8;

    self->sprites[PA_SP_BONUS_BLOCK].wsg = &self->wsgs[PA_WSG_BONUS_BLOCK_0];
    self->sprites[PA_SP_BONUS_BLOCK].originX         = 8;
    self->sprites[PA_SP_BONUS_BLOCK].originY         = 8;

    self->sprites[PA_SP_ENEMY_SOUTH].wsg = &self->wsgs[PA_WSG_ENEMY_SOUTH];
    self->sprites[PA_SP_ENEMY_SOUTH].originX         = 8;
    self->sprites[PA_SP_ENEMY_SOUTH].originY         = 16;

    self->sprites[PA_SP_ENEMY_NORTH].wsg = &self->wsgs[PA_WSG_ENEMY_NORTH];
    self->sprites[PA_SP_ENEMY_NORTH].originX         = 8;
    self->sprites[PA_SP_ENEMY_NORTH].originY         = 16;

    self->sprites[PA_SP_ENEMY_SIDE_1].wsg = &self->wsgs[PA_WSG_ENEMY_SIDE_1];
    self->sprites[PA_SP_ENEMY_SIDE_1].originX         = 8;
    self->sprites[PA_SP_ENEMY_SIDE_1].originY         = 16;

    self->sprites[PA_SP_ENEMY_SIDE_2].wsg = &self->wsgs[PA_WSG_ENEMY_SIDE_2];
    self->sprites[PA_SP_ENEMY_SIDE_2].originX         = 8;
    self->sprites[PA_SP_ENEMY_SIDE_2].originY         = 16;

    self->sprites[PA_SP_ENEMY_DRILL_SOUTH].wsg = &self->wsgs[PA_WSG_ENEMY_DRILL_SOUTH];
    self->sprites[PA_SP_ENEMY_DRILL_SOUTH].originX         = 8;
    self->sprites[PA_SP_ENEMY_DRILL_SOUTH].originY         = 16;

    self->sprites[PA_SP_ENEMY_DRILL_NORTH].wsg = &self->wsgs[PA_WSG_ENEMY_DRILL_NORTH];
    self->sprites[PA_SP_ENEMY_DRILL_NORTH].originX         = 8;
    self->sprites[PA_SP_ENEMY_DRILL_NORTH].originY         = 16;

    self->sprites[PA_SP_ENEMY_STUN].wsg = &self->wsgs[PA_WSG_ENEMY_STUN];
    self->sprites[PA_SP_ENEMY_STUN].originX         = 8;
    self->sprites[PA_SP_ENEMY_STUN].originY         = 16;

    self->sprites[PA_SP_ENEMY_DRILL_SIDE_1].wsg = &self->wsgs[PA_WSG_ENEMY_DRILL_SIDE_1];
    self->sprites[PA_SP_ENEMY_DRILL_SIDE_1].originX         = 8;
    self->sprites[PA_SP_ENEMY_DRILL_SIDE_1].originY         = 16;

    self->sprites[PA_SP_ENEMY_DRILL_SIDE_2].wsg = &self->wsgs[PA_WSG_ENEMY_DRILL_SIDE_2];
    self->sprites[PA_SP_ENEMY_DRILL_SIDE_2].originX         = 8;
    self->sprites[PA_SP_ENEMY_DRILL_SIDE_2].originY         = 16;

    self->sprites[PA_SP_BREAK_BLOCK].wsg = &self->wsgs[PA_WSG_BREAK_BLOCK];
    self->sprites[PA_SP_BREAK_BLOCK].originX         = 8;
    self->sprites[PA_SP_BREAK_BLOCK].originY         = 8;

    self->sprites[PA_SP_BREAK_BLOCK_1].wsg = &self->wsgs[PA_WSG_BREAK_BLOCK_1];
    self->sprites[PA_SP_BREAK_BLOCK_1].originX         = 8;
    self->sprites[PA_SP_BREAK_BLOCK_1].originY         = 8;

    self->sprites[PA_SP_BREAK_BLOCK_2].wsg = &self->wsgs[PA_WSG_BREAK_BLOCK_2];
    self->sprites[PA_SP_BREAK_BLOCK_2].originX         = 8;
    self->sprites[PA_SP_BREAK_BLOCK_2].originY         = 8;

    self->sprites[PA_SP_BREAK_BLOCK_3].wsg = &self->wsgs[PA_WSG_BREAK_BLOCK_3];
    self->sprites[PA_SP_BREAK_BLOCK_3].originX         = 8;
    self->sprites[PA_SP_BREAK_BLOCK_3].originY         = 8;

    self->sprites[PA_SP_BLOCK_FRAGMENT].wsg = &self->wsgs[PA_WSG_BLOCK_FRAGMENT];
    self->sprites[PA_SP_BLOCK_FRAGMENT].originX         = 3;
    self->sprites[PA_SP_BLOCK_FRAGMENT].originY         = 3;
}

void pa_initializeTiles(paWsgManager_t* self){
    self->tiles[0] = &self->wsgs[PA_WSG_WALL_0];
    self->tiles[1] = &self->wsgs[PA_WSG_WALL_1];
    self->tiles[2] = &self->wsgs[PA_WSG_WALL_2];
    self->tiles[3] = &self->wsgs[PA_WSG_WALL_3];
    self->tiles[4] = &self->wsgs[PA_WSG_WALL_4];
    self->tiles[5] = &self->wsgs[PA_WSG_WALL_5];
    self->tiles[6] = &self->wsgs[PA_WSG_WALL_6]; 
    self->tiles[7] = &self->wsgs[PA_WSG_WALL_7];
    self->tiles[8] = &self->wsgs[PA_WSG_BLOCK];
    self->tiles[9] = &self->wsgs[PA_WSG_SPAWN_BLOCK_0];
    self->tiles[10] = &self->wsgs[PA_WSG_SPAWN_BLOCK_1];
    self->tiles[11] = &self->wsgs[PA_WSG_SPAWN_BLOCK_2];
    self->tiles[12] = &self->wsgs[PA_WSG_BONUS_BLOCK_0];
    self->tiles[13] = &self->wsgs[PA_WSG_BONUS_BLOCK_1];
    self->tiles[14] = &self->wsgs[PA_WSG_BONUS_BLOCK_2];
}

void pa_remapWsgToSprite(paWsgManager_t* self, uint16_t spriteIndex, uint16_t wsgIndex){
    self->sprites[spriteIndex].wsg = &self->wsgs[wsgIndex];
}

void pa_remapWsgToTile(paWsgManager_t* self, uint16_t tileIndex, uint16_t wsgIndex){
    self->tiles[tileIndex] = &self->wsgs[wsgIndex];
}

