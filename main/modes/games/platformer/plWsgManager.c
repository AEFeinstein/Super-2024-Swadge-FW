//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include <stdbool.h>
#include "fs_wsg.h"
#include "plWsgManager.h"
#include "platformer_typedef.h"

//==============================================================================
// Functions
//==============================================================================

void pl_initializeWsgManager(plWsgManager_t* self)
{
    pl_loadWsgs(self);
    pl_initializeSprites(self);
    pl_initializeTiles(self);
}

void pl_freeWsgManager(plWsgManager_t* self)
{
    for (uint16_t i = 0; i < PL_WSGS_SIZE; i++)
    {
        freeWsg(&self->wsgs[i]);
    }
}

void pl_loadWsgs(plWsgManager_t* self)
{
    loadWsg("sprite000.wsg", &self->wsgs[PL_WSG_PLAYER_IDLE], false);
    loadWsg("sprite001.wsg", &self->wsgs[PL_WSG_PLAYER_WALK1], false);
    loadWsg("sprite002.wsg", &self->wsgs[PL_WSG_PLAYER_WALK2], false);
    loadWsg("sprite003.wsg", &self->wsgs[PL_WSG_PLAYER_WALK3], false);
    loadWsg("sprite004.wsg", &self->wsgs[PL_WSG_PLAYER_JUMP], false);
    loadWsg("sprite005.wsg", &self->wsgs[PL_WSG_PLAYER_SLIDE], false);
    loadWsg("sprite006.wsg", &self->wsgs[PL_WSG_PLAYER_HURT], false);
    loadWsg("sprite007.wsg", &self->wsgs[PL_WSG_PLAYER_CLIMB], false);
    loadWsg("sprite008.wsg", &self->wsgs[PL_WSG_PLAYER_WIN], false);
    loadWsg("sprite009.wsg", &self->wsgs[PL_WSG_ENEMY_BASIC], false);
    loadWsg("sprite012.wsg", &self->wsgs[PL_WSG_DUSTBUNNY_IDLE], false);
    loadWsg("sprite013.wsg", &self->wsgs[PL_WSG_DUSTBUNNY_CHARGE], false);
    loadWsg("sprite014.wsg", &self->wsgs[PL_WSG_DUSTBUNNY_JUMP], false);
    loadWsg("sprite015.wsg", &self->wsgs[PL_WSG_GAMING_1], false);
    loadWsg("sprite016.wsg", &self->wsgs[PL_WSG_GAMING_2], false);
    loadWsg("sprite017.wsg", &self->wsgs[PL_WSG_GAMING_3], false);
    loadWsg("sprite018.wsg", &self->wsgs[PL_WSG_MUSIC_1], false);
    loadWsg("sprite019.wsg", &self->wsgs[PL_WSG_MUSIC_2], false);
    loadWsg("sprite020.wsg", &self->wsgs[PL_WSG_MUSIC_3], false);
    loadWsg("sprite021.wsg", &self->wsgs[PL_WSG_WARP_1], false);
    loadWsg("sprite022.wsg", &self->wsgs[PL_WSG_WARP_2], false);
    loadWsg("sprite023.wsg", &self->wsgs[PL_WSG_WARP_3], false);
    loadWsg("sprite024.wsg", &self->wsgs[PL_WSG_WAPL_WSG_1], false);
    loadWsg("sprite025.wsg", &self->wsgs[PL_WSG_WAPL_WSG_2], false);
    loadWsg("sprite026.wsg", &self->wsgs[PL_WSG_WAPL_WSG_DIVE], false);
    loadWsg("sprite027.wsg", &self->wsgs[PL_WSG_1UP_1], false);
    loadWsg("sprite028.wsg", &self->wsgs[PL_WSG_1UP_2], false);
    loadWsg("sprite029.wsg", &self->wsgs[PL_WSG_1UP_3], false);
    loadWsg("sprite030.wsg", &self->wsgs[PL_WSG_WAVEBALL_1], false);
    loadWsg("sprite031.wsg", &self->wsgs[PL_WSG_WAVEBALL_2], false);
    loadWsg("sprite032.wsg", &self->wsgs[PL_WSG_WAVEBALL_3], false);
    loadWsg("sprite033.wsg", &self->wsgs[PL_WSG_ENEMY_BUSH_L2], false);
    loadWsg("sprite034.wsg", &self->wsgs[PL_WSG_ENEMY_BUSH_L3], false);
    loadWsg("sprite035.wsg", &self->wsgs[PL_WSG_DUSTBUNNY_L2_IDLE], false);
    loadWsg("sprite036.wsg", &self->wsgs[PL_WSG_DUSTBUNNY_L2_CHARGE], false);
    loadWsg("sprite037.wsg", &self->wsgs[PL_WSG_DUSTBUNNY_L2_JUMP], false);
    loadWsg("sprite038.wsg", &self->wsgs[PL_WSG_DUSTBUNNY_L3_IDLE], false);
    loadWsg("sprite039.wsg", &self->wsgs[PL_WSG_DUSTBUNNY_L3_CHARGE], false);
    loadWsg("sprite040.wsg", &self->wsgs[PL_WSG_DUSTBUNNY_L3_JUMP], false);
    loadWsg("sprite041.wsg", &self->wsgs[PL_WSG_WAPL_WSG_L2_1], false);
    loadWsg("sprite042.wsg", &self->wsgs[PL_WSG_WAPL_WSG_L2_2], false);
    loadWsg("sprite043.wsg", &self->wsgs[PL_WSG_WAPL_WSG_L2_DIVE], false);
    loadWsg("sprite044.wsg", &self->wsgs[PL_WSG_WAPL_WSG_L3_1], false);
    loadWsg("sprite045.wsg", &self->wsgs[PL_WSG_WAPL_WSG_L3_2], false);
    loadWsg("sprite046.wsg", &self->wsgs[PL_WSG_WAPL_WSG_L3_DIVE], false);
    loadWsg("sprite047.wsg", &self->wsgs[PL_WSG_CHECKPOINT_INACTIVE], false);
    loadWsg("sprite048.wsg", &self->wsgs[PL_WSG_CHECKPOINT_ACTIVE_1], false);
    loadWsg("sprite049.wsg", &self->wsgs[PL_WSG_CHECKPOINT_ACTIVE_2], false);
    loadWsg("tile039.wsg", &self->wsgs[PL_WSG_BOUNCE_BLOCK], false);
    loadWsg("tile066.wsg", &self->wsgs[PL_WSG_CONTAINER_1], false);
    loadWsg("tile034.wsg", &self->wsgs[PL_WSG_BRICK_BLOCK], false);
    loadWsg("tile032.wsg", &self->wsgs[PL_WSG_GRASS], false);
    loadWsg("tile033.wsg", &self->wsgs[PL_WSG_GROUND], false);
    loadWsg("tile034.wsg", &self->wsgs[PL_WSG_BRICK_BLOCK], false);
    loadWsg("tile035.wsg", &self->wsgs[PL_WSG_BLOCK], false);
    loadWsg("tile036.wsg", &self->wsgs[PL_WSG_METAL_BLOCK], false);
    loadWsg("tile037.wsg", &self->wsgs[PL_WSG_METAL_PIPE_H], false);
    loadWsg("tile038.wsg", &self->wsgs[PL_WSG_METAL_PIPE_V], false);
    loadWsg("tile039.wsg", &self->wsgs[PL_WSG_BOUNCE_BLOCK], false);
    loadWsg("tile040.wsg", &self->wsgs[PL_WSG_DIRT_PATH], false);
    loadWsg("tile041.wsg", &self->wsgs[PL_WSG_GIRDER], false);
    loadWsg("tile059.wsg", &self->wsgs[PL_WSG_GOAL_100PTS], false);
    loadWsg("tile060.wsg", &self->wsgs[PL_WSG_GOAL_500PTS], false);
    loadWsg("tile061.wsg", &self->wsgs[PL_WSG_GOAL_1000PTS], false);
    loadWsg("tile062.wsg", &self->wsgs[PL_WSG_GOAL_2000PTS], false);
    loadWsg("tile063.wsg", &self->wsgs[PL_WSG_GOAL_5000PTS], false);
    loadWsg("tile064.wsg", &self->wsgs[PL_WSG_CONTAINER_1], false);
    loadWsg("tile065.wsg", &self->wsgs[PL_WSG_CONTAINER_2], false);
    loadWsg("tile066.wsg", &self->wsgs[PL_WSG_CONTAINER_3], false);
    loadWsg("tile067.wsg", &self->wsgs[PL_WSG_COIN_1], false);
    loadWsg("tile068.wsg", &self->wsgs[PL_WSG_COIN_2], false);
    loadWsg("tile069.wsg", &self->wsgs[PL_WSG_COIN_3], false);
    loadWsg("tile070.wsg", &self->wsgs[PL_WSG_LADDER], false);
    loadWsg("tile080.wsg", &self->wsgs[PL_WSG_BG_GOAL_ZONE], false);
    loadWsg("tile081.wsg", &self->wsgs[PL_WSG_BG_ARROW_L], false);
    loadWsg("tile082.wsg", &self->wsgs[PL_WSG_BG_ARROW_R], false);
    loadWsg("tile083.wsg", &self->wsgs[PL_WSG_BG_ARROW_U], false);
    loadWsg("tile084.wsg", &self->wsgs[PL_WSG_BG_ARROW_D], false);
    loadWsg("tile085.wsg", &self->wsgs[PL_WSG_BG_ARROW_LU], false);
    loadWsg("tile086.wsg", &self->wsgs[PL_WSG_BG_ARROW_RU], false);
    loadWsg("tile087.wsg", &self->wsgs[PL_WSG_BG_ARROW_LD], false);
    loadWsg("tile088.wsg", &self->wsgs[PL_WSG_BG_ARROW_RD], false);
    loadWsg("tile089.wsg", &self->wsgs[PL_WSG_BG_CLOUD_LD], false);
    loadWsg("tile090.wsg", &self->wsgs[PL_WSG_BG_CLOUD_M], false);
    loadWsg("tile091.wsg", &self->wsgs[PL_WSG_BG_CLOUD_RD], false);
    loadWsg("tile092.wsg", &self->wsgs[PL_WSG_BG_CLOUD_LU], false);
    loadWsg("tile093.wsg", &self->wsgs[PL_WSG_BG_CLOUD_RU], false);
    loadWsg("tile094.wsg", &self->wsgs[PL_WSG_BG_CLOUD_D], false);
    loadWsg("tile095.wsg", &self->wsgs[PL_WSG_BG_CLOUD], false);
    loadWsg("tile096.wsg", &self->wsgs[PL_WSG_BG_TALL_GRASS], false);
    loadWsg("tile097.wsg", &self->wsgs[PL_WSG_BG_MOUNTAIN_L], false);
    loadWsg("tile098.wsg", &self->wsgs[PL_WSG_BG_MOUNTAIN_U], false);
    loadWsg("tile099.wsg", &self->wsgs[PL_WSG_BG_MOUNTAIN_R], false);
    loadWsg("tile100.wsg", &self->wsgs[PL_WSG_BG_MOUNTAIN], false);
    loadWsg("tile101.wsg", &self->wsgs[PL_WSG_BG_METAL], false);
    loadWsg("tile102.wsg", &self->wsgs[PL_WSG_BG_CHAINS], false);
    loadWsg("tile103.wsg", &self->wsgs[PL_WSG_BG_WALL], false);
}

void pl_initializeSprites(plWsgManager_t* self)
{
    self->sprites[PL_SP_PLAYER_IDLE].wsg     = &self->wsgs[PL_SP_PLAYER_IDLE];
    self->sprites[PL_SP_PLAYER_IDLE].originX = 8;
    self->sprites[PL_SP_PLAYER_IDLE].originY = 8;

    self->sprites[PL_SP_PLAYER_WALK1].wsg     = &self->wsgs[PL_SP_PLAYER_WALK1];
    self->sprites[PL_SP_PLAYER_WALK1].originX = 8;
    self->sprites[PL_SP_PLAYER_WALK1].originY = 8;

    self->sprites[PL_SP_PLAYER_WALK2].wsg     = &self->wsgs[PL_SP_PLAYER_WALK2];
    self->sprites[PL_SP_PLAYER_WALK2].originX = 8;
    self->sprites[PL_SP_PLAYER_WALK2].originY = 8;

    self->sprites[PL_SP_PLAYER_WALK3].wsg     = &self->wsgs[PL_SP_PLAYER_WALK3];
    self->sprites[PL_SP_PLAYER_WALK3].originX = 8;
    self->sprites[PL_SP_PLAYER_WALK3].originY = 8;

    self->sprites[PL_SP_PLAYER_JUMP].wsg     = &self->wsgs[PL_SP_PLAYER_JUMP];
    self->sprites[PL_SP_PLAYER_JUMP].originX = 8;
    self->sprites[PL_SP_PLAYER_JUMP].originY = 8;

    self->sprites[PL_SP_PLAYER_SLIDE].wsg     = &self->wsgs[PL_SP_PLAYER_SLIDE];
    self->sprites[PL_SP_PLAYER_SLIDE].originX = 8;
    self->sprites[PL_SP_PLAYER_SLIDE].originY = 8;

    self->sprites[PL_SP_PLAYER_HURT].wsg     = &self->wsgs[PL_SP_PLAYER_HURT];
    self->sprites[PL_SP_PLAYER_HURT].originX = 8;
    self->sprites[PL_SP_PLAYER_HURT].originY = 8;

    self->sprites[PL_SP_PLAYER_CLIMB].wsg     = &self->wsgs[PL_SP_PLAYER_CLIMB];
    self->sprites[PL_SP_PLAYER_CLIMB].originX = 8;
    self->sprites[PL_SP_PLAYER_CLIMB].originY = 8;

    self->sprites[PL_SP_PLAYER_WIN].wsg     = &self->wsgs[PL_SP_PLAYER_WIN];
    self->sprites[PL_SP_PLAYER_WIN].originX = 8;
    self->sprites[PL_SP_PLAYER_WIN].originY = 8;

    self->sprites[PL_SP_ENEMY_BASIC].wsg     = &self->wsgs[PL_SP_ENEMY_BASIC];
    self->sprites[PL_SP_ENEMY_BASIC].originX = 8;
    self->sprites[PL_SP_ENEMY_BASIC].originY = 8;

    self->sprites[PL_SP_HITBLOCK_CONTAINER].wsg     = &self->wsgs[PL_SP_HITBLOCK_CONTAINER];
    self->sprites[PL_SP_HITBLOCK_CONTAINER].originX = 8;
    self->sprites[PL_SP_HITBLOCK_CONTAINER].originY = 8;

    self->sprites[PL_SP_HITBLOCK_BRICKS].wsg     = &self->wsgs[PL_SP_HITBLOCK_BRICKS];
    self->sprites[PL_SP_HITBLOCK_BRICKS].originX = 8;
    self->sprites[PL_SP_HITBLOCK_BRICKS].originY = 8;

    self->sprites[PL_SP_DUSTBUNNY_IDLE].wsg     = &self->wsgs[PL_SP_DUSTBUNNY_IDLE];
    self->sprites[PL_SP_DUSTBUNNY_IDLE].originX = 8;
    self->sprites[PL_SP_DUSTBUNNY_IDLE].originY = 8;

    self->sprites[PL_SP_DUSTBUNNY_CHARGE].wsg     = &self->wsgs[PL_SP_DUSTBUNNY_CHARGE];
    self->sprites[PL_SP_DUSTBUNNY_CHARGE].originX = 8;
    self->sprites[PL_SP_DUSTBUNNY_CHARGE].originY = 8;

    self->sprites[PL_SP_DUSTBUNNY_JUMP].wsg     = &self->wsgs[PL_SP_DUSTBUNNY_JUMP];
    self->sprites[PL_SP_DUSTBUNNY_JUMP].originX = 8;
    self->sprites[PL_SP_DUSTBUNNY_JUMP].originY = 8;

    self->sprites[PL_SP_GAMING_1].wsg     = &self->wsgs[PL_SP_GAMING_1];
    self->sprites[PL_SP_GAMING_1].originX = 8;
    self->sprites[PL_SP_GAMING_1].originY = 8;

    self->sprites[PL_SP_GAMING_2].wsg     = &self->wsgs[PL_SP_GAMING_2];
    self->sprites[PL_SP_GAMING_2].originX = 8;
    self->sprites[PL_SP_GAMING_2].originY = 8;

    self->sprites[PL_SP_GAMING_3].wsg     = &self->wsgs[PL_SP_GAMING_3];
    self->sprites[PL_SP_GAMING_3].originX = 8;
    self->sprites[PL_SP_GAMING_3].originY = 8;

    self->sprites[PL_SP_MUSIC_1].wsg     = &self->wsgs[PL_SP_MUSIC_1];
    self->sprites[PL_SP_MUSIC_1].originX = 8;
    self->sprites[PL_SP_MUSIC_1].originY = 8;

    self->sprites[PL_SP_MUSIC_2].wsg     = &self->wsgs[PL_SP_MUSIC_2];
    self->sprites[PL_SP_MUSIC_2].originX = 8;
    self->sprites[PL_SP_MUSIC_2].originY = 8;

    self->sprites[PL_SP_MUSIC_3].wsg     = &self->wsgs[PL_SP_MUSIC_3];
    self->sprites[PL_SP_MUSIC_3].originX = 8;
    self->sprites[PL_SP_MUSIC_3].originY = 8;

    self->sprites[PL_SP_WARP_1].wsg     = &self->wsgs[PL_SP_WARP_1];
    self->sprites[PL_SP_WARP_1].originX = 8;
    self->sprites[PL_SP_WARP_1].originY = 8;

    self->sprites[PL_SP_WARP_2].wsg     = &self->wsgs[PL_SP_WARP_2];
    self->sprites[PL_SP_WARP_2].originX = 8;
    self->sprites[PL_SP_WARP_2].originY = 8;

    self->sprites[PL_SP_WARP_3].wsg     = &self->wsgs[PL_SP_WARP_3];
    self->sprites[PL_SP_WARP_3].originX = 8;
    self->sprites[PL_SP_WARP_3].originY = 8;

    self->sprites[PL_SP_WASP_1].wsg     = &self->wsgs[PL_SP_WASP_1];
    self->sprites[PL_SP_WASP_1].originX = 8;
    self->sprites[PL_SP_WASP_1].originY = 8;

    self->sprites[PL_SP_WASP_2].wsg     = &self->wsgs[PL_SP_WASP_2];
    self->sprites[PL_SP_WASP_2].originX = 8;
    self->sprites[PL_SP_WASP_2].originY = 8;

    self->sprites[PL_SP_WASP_DIVE].wsg     = &self->wsgs[PL_SP_WASP_DIVE];
    self->sprites[PL_SP_WASP_DIVE].originX = 8;
    self->sprites[PL_SP_WASP_DIVE].originY = 8;

    self->sprites[PL_SP_1UP_1].wsg     = &self->wsgs[PL_SP_1UP_1];
    self->sprites[PL_SP_1UP_1].originX = 8;
    self->sprites[PL_SP_1UP_1].originY = 8;

    self->sprites[PL_SP_1UP_2].wsg     = &self->wsgs[PL_SP_1UP_2];
    self->sprites[PL_SP_1UP_2].originX = 8;
    self->sprites[PL_SP_1UP_2].originY = 8;

    self->sprites[PL_SP_1UP_3].wsg     = &self->wsgs[PL_SP_1UP_3];
    self->sprites[PL_SP_1UP_3].originX = 8;
    self->sprites[PL_SP_1UP_3].originY = 8;

    self->sprites[PL_SP_WAVEBALL_1].wsg     = &self->wsgs[PL_SP_WAVEBALL_1];
    self->sprites[PL_SP_WAVEBALL_1].originX = 8;
    self->sprites[PL_SP_WAVEBALL_1].originY = 8;

    self->sprites[PL_SP_WAVEBALL_2].wsg     = &self->wsgs[PL_SP_WAVEBALL_2];
    self->sprites[PL_SP_WAVEBALL_2].originX = 8;
    self->sprites[PL_SP_WAVEBALL_2].originY = 8;

    self->sprites[PL_SP_WAVEBALL_3].wsg     = &self->wsgs[PL_SP_WAVEBALL_3];
    self->sprites[PL_SP_WAVEBALL_3].originX = 8;
    self->sprites[PL_SP_WAVEBALL_3].originY = 8;

    self->sprites[PL_SP_ENEMY_BUSH_L2].wsg     = &self->wsgs[PL_SP_ENEMY_BUSH_L2];
    self->sprites[PL_SP_ENEMY_BUSH_L2].originX = 8;
    self->sprites[PL_SP_ENEMY_BUSH_L2].originY = 8;

    self->sprites[PL_SP_ENEMY_BUSH_L3].wsg     = &self->wsgs[PL_SP_ENEMY_BUSH_L3];
    self->sprites[PL_SP_ENEMY_BUSH_L3].originX = 8;
    self->sprites[PL_SP_ENEMY_BUSH_L3].originY = 8;

    self->sprites[PL_SP_DUSTBUNNY_L2_IDLE].wsg     = &self->wsgs[PL_SP_DUSTBUNNY_L2_IDLE];
    self->sprites[PL_SP_DUSTBUNNY_L2_IDLE].originX = 8;
    self->sprites[PL_SP_DUSTBUNNY_L2_IDLE].originY = 8;

    self->sprites[PL_SP_DUSTBUNNY_L2_CHARGE].wsg     = &self->wsgs[PL_SP_DUSTBUNNY_L2_CHARGE];
    self->sprites[PL_SP_DUSTBUNNY_L2_CHARGE].originX = 8;
    self->sprites[PL_SP_DUSTBUNNY_L2_CHARGE].originY = 8;

    self->sprites[PL_SP_DUSTBUNNY_L2_JUMP].wsg     = &self->wsgs[PL_SP_DUSTBUNNY_L2_JUMP];
    self->sprites[PL_SP_DUSTBUNNY_L2_JUMP].originX = 8;
    self->sprites[PL_SP_DUSTBUNNY_L2_JUMP].originY = 8;

    self->sprites[PL_SP_DUSTBUNNY_L3_IDLE].wsg     = &self->wsgs[PL_SP_DUSTBUNNY_L3_IDLE];
    self->sprites[PL_SP_DUSTBUNNY_L3_IDLE].originX = 8;
    self->sprites[PL_SP_DUSTBUNNY_L3_IDLE].originY = 8;

    self->sprites[PL_SP_DUSTBUNNY_L3_CHARGE].wsg     = &self->wsgs[PL_SP_DUSTBUNNY_L3_CHARGE];
    self->sprites[PL_SP_DUSTBUNNY_L3_CHARGE].originX = 8;
    self->sprites[PL_SP_DUSTBUNNY_L3_CHARGE].originY = 8;

    self->sprites[PL_SP_DUSTBUNNY_L3_JUMP].wsg     = &self->wsgs[PL_SP_DUSTBUNNY_L3_JUMP];
    self->sprites[PL_SP_DUSTBUNNY_L3_JUMP].originX = 8;
    self->sprites[PL_SP_DUSTBUNNY_L3_JUMP].originY = 8;

    self->sprites[PL_SP_WASP_L2_1].wsg     = &self->wsgs[PL_SP_WASP_L2_1];
    self->sprites[PL_SP_WASP_L2_1].originX = 8;
    self->sprites[PL_SP_WASP_L2_1].originY = 8;

    self->sprites[PL_SP_WASP_L2_2].wsg     = &self->wsgs[PL_SP_WASP_L2_2];
    self->sprites[PL_SP_WASP_L2_2].originX = 8;
    self->sprites[PL_SP_WASP_L2_2].originY = 8;

    self->sprites[PL_SP_WASP_L2_DIVE].wsg     = &self->wsgs[PL_SP_WASP_L2_DIVE];
    self->sprites[PL_SP_WASP_L2_DIVE].originX = 8;
    self->sprites[PL_SP_WASP_L2_DIVE].originY = 8;

    self->sprites[PL_SP_WASP_L3_1].wsg     = &self->wsgs[PL_SP_WASP_L3_1];
    self->sprites[PL_SP_WASP_L3_1].originX = 8;
    self->sprites[PL_SP_WASP_L3_1].originY = 8;

    self->sprites[PL_SP_WASP_L3_2].wsg     = &self->wsgs[PL_SP_WASP_L3_2];
    self->sprites[PL_SP_WASP_L3_2].originX = 8;
    self->sprites[PL_SP_WASP_L3_2].originY = 8;

    self->sprites[PL_SP_WASP_L3_DIVE].wsg     = &self->wsgs[PL_SP_WASP_L3_DIVE];
    self->sprites[PL_SP_WASP_L3_DIVE].originX = 8;
    self->sprites[PL_SP_WASP_L3_DIVE].originY = 8;

    self->sprites[PL_SP_CHECKPOINT_INACTIVE].wsg     = &self->wsgs[PL_SP_CHECKPOINT_INACTIVE];
    self->sprites[PL_SP_CHECKPOINT_INACTIVE].originX = 8;
    self->sprites[PL_SP_CHECKPOINT_INACTIVE].originY = 8;

    self->sprites[PL_SP_CHECKPOINT_ACTIVE_1].wsg     = &self->wsgs[PL_SP_CHECKPOINT_ACTIVE_1];
    self->sprites[PL_SP_CHECKPOINT_ACTIVE_1].originX = 8;
    self->sprites[PL_SP_CHECKPOINT_ACTIVE_1].originY = 8;

    self->sprites[PL_SP_CHECKPOINT_ACTIVE_2].wsg     = &self->wsgs[PL_SP_CHECKPOINT_ACTIVE_2];
    self->sprites[PL_SP_CHECKPOINT_ACTIVE_2].originX = 8;
    self->sprites[PL_SP_CHECKPOINT_ACTIVE_2].originY = 8;

    self->sprites[PL_SP_BOUNCE_BLOCK].wsg     = &self->wsgs[PL_SP_BOUNCE_BLOCK];
    self->sprites[PL_SP_BOUNCE_BLOCK].originX = 8;
    self->sprites[PL_SP_BOUNCE_BLOCK].originY = 8;
}

void pl_initializeTiles(plWsgManager_t* self)
{
    self->tiles[0] = &self->wsgs[PL_WSG_GRASS];
    self->tiles[1] = &self->wsgs[PL_WSG_GROUND];
    self->tiles[2] = &self->wsgs[PL_WSG_BRICK_BLOCK];
    self->tiles[3] = &self->wsgs[PL_WSG_BLOCK];
    self->tiles[4] = &self->wsgs[PL_WSG_METAL_BLOCK];
    self->tiles[5] = &self->wsgs[PL_WSG_METAL_PIPE_H];
    self->tiles[6] = &self->wsgs[PL_WSG_METAL_PIPE_V];
    self->tiles[7] = &self->wsgs[PL_WSG_BOUNCE_BLOCK];
    self->tiles[8] = &self->wsgs[PL_WSG_DIRT_PATH];
    self->tiles[9] = &self->wsgs[PL_WSG_GIRDER];

    self->tiles[10] = &self->wsgs[0];
    self->tiles[11] = &self->wsgs[0];
    self->tiles[12] = &self->wsgs[0];
    self->tiles[13] = &self->wsgs[0];
    self->tiles[14] = &self->wsgs[0];
    self->tiles[15] = &self->wsgs[0];
    self->tiles[16] = &self->wsgs[0];
    self->tiles[17] = &self->wsgs[0];
    self->tiles[18] = &self->wsgs[0];
    self->tiles[19] = &self->wsgs[0];
    self->tiles[20] = &self->wsgs[0];
    self->tiles[21] = &self->wsgs[0];
    self->tiles[22] = &self->wsgs[0];
    self->tiles[23] = &self->wsgs[0];
    self->tiles[24] = &self->wsgs[0];
    self->tiles[25] = &self->wsgs[0];
    self->tiles[26] = &self->wsgs[0];

    self->tiles[27] = &self->wsgs[PL_WSG_GOAL_100PTS];
    self->tiles[28] = &self->wsgs[PL_WSG_GOAL_500PTS];
    self->tiles[29] = &self->wsgs[PL_WSG_GOAL_1000PTS];
    self->tiles[30] = &self->wsgs[PL_WSG_GOAL_2000PTS];
    self->tiles[31] = &self->wsgs[PL_WSG_GOAL_5000PTS];
    self->tiles[32] = &self->wsgs[PL_WSG_CONTAINER_1];
    self->tiles[33] = &self->wsgs[PL_WSG_CONTAINER_2];
    self->tiles[34] = &self->wsgs[PL_WSG_CONTAINER_3];
    self->tiles[35] = &self->wsgs[PL_WSG_COIN_1];
    self->tiles[36] = &self->wsgs[PL_WSG_COIN_2];
    self->tiles[37] = &self->wsgs[PL_WSG_COIN_3];
    self->tiles[38] = &self->wsgs[PL_WSG_LADDER];

    self->tiles[39] = &self->wsgs[0];
    self->tiles[40] = &self->wsgs[0];
    self->tiles[41] = &self->wsgs[0];
    self->tiles[42] = &self->wsgs[0];
    self->tiles[43] = &self->wsgs[0];
    self->tiles[44] = &self->wsgs[0];
    self->tiles[45] = &self->wsgs[0];
    self->tiles[46] = &self->wsgs[0];
    self->tiles[47] = &self->wsgs[0];

    self->tiles[48] = &self->wsgs[PL_WSG_BG_GOAL_ZONE];
    self->tiles[49] = &self->wsgs[PL_WSG_BG_ARROW_L];
    self->tiles[50] = &self->wsgs[PL_WSG_BG_ARROW_R];
    self->tiles[51] = &self->wsgs[PL_WSG_BG_ARROW_U];
    self->tiles[52] = &self->wsgs[PL_WSG_BG_ARROW_D];
    self->tiles[53] = &self->wsgs[PL_WSG_BG_ARROW_LU];
    self->tiles[54] = &self->wsgs[PL_WSG_BG_ARROW_RU];
    self->tiles[55] = &self->wsgs[PL_WSG_BG_ARROW_LD];
    self->tiles[56] = &self->wsgs[PL_WSG_BG_ARROW_RD];
    self->tiles[57] = &self->wsgs[PL_WSG_BG_CLOUD_LD];
    self->tiles[58] = &self->wsgs[PL_WSG_BG_CLOUD_M];
    self->tiles[59] = &self->wsgs[PL_WSG_BG_CLOUD_RD];
    self->tiles[60] = &self->wsgs[PL_WSG_BG_CLOUD_LU];
    self->tiles[61] = &self->wsgs[PL_WSG_BG_CLOUD_RU];
    self->tiles[62] = &self->wsgs[PL_WSG_BG_CLOUD_D];
    self->tiles[63] = &self->wsgs[PL_WSG_BG_CLOUD];
    self->tiles[64] = &self->wsgs[PL_WSG_BG_TALL_GRASS];
    self->tiles[65] = &self->wsgs[PL_WSG_BG_MOUNTAIN_L];
    self->tiles[66] = &self->wsgs[PL_WSG_BG_MOUNTAIN_U];
    self->tiles[67] = &self->wsgs[PL_WSG_BG_MOUNTAIN_R];
    self->tiles[68] = &self->wsgs[PL_WSG_BG_MOUNTAIN];
    self->tiles[69] = &self->wsgs[PL_WSG_BG_METAL];
    self->tiles[70] = &self->wsgs[PL_WSG_BG_CHAINS];
    self->tiles[71] = &self->wsgs[PL_WSG_BG_WALL];
}

void pl_remapWsgToSprite(plWsgManager_t* self, uint16_t spriteIndex, uint16_t wsgIndex)
{
    self->sprites[spriteIndex].wsg = &self->wsgs[wsgIndex];
}

void pl_remapWsgToTile(plWsgManager_t* self, uint16_t tileIndex, uint16_t wsgIndex)
{
    self->tiles[tileIndex] = &self->wsgs[wsgIndex];
}

void pl_remapPlayerCharacter(plWsgManager_t* self, uint16_t newBaseIndex)
{
    for (uint16_t i = 0; i < (PL_WSG_PLAYER_WIN + 1); i++)
    {
        pl_remapWsgToSprite(self, i, newBaseIndex + i);
    }
}

void pl_animateTiles(plWsgManager_t* self)
{
    self->globalTileAnimationTimer--;
    if (self->globalTileAnimationTimer < 0)
    {
        // Assumption: all animated tiles have 6 frames of animation
        self->globalTileAnimationFrame = ((self->globalTileAnimationFrame + 1) % 6);

        //pl_remapWsgToTile(self, 9, PA_WSG_BLOCK_BLUE + self->globalTileAnimationFrame);
        //pl_remapWsgToSprite(self, PA_SP_BONUS_BLOCK, PA_WSG_BLOCK_BLUE + self->globalTileAnimationFrame);

        self->globalTileAnimationTimer = 6;
    }
}

void pl_remapBlockTile(plWsgManager_t* self, uint16_t newBlockWsgIndex)
{
    pl_remapWsgToTile(self, 8, newBlockWsgIndex);
    //pl_remapWsgToSprite(self, PA_SP_BLOCK, newBlockWsgIndex);
}