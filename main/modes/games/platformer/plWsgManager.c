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
    loadWsg(PULSE_036_WSG, &self->wsgs[PL_WSG_PLAYER_IDLE], false);
    loadWsg(PULSE_000_WSG, &self->wsgs[PL_WSG_PLAYER_WALK1], false);
    loadWsg(PULSE_001_WSG, &self->wsgs[PL_WSG_PLAYER_WALK2], false);
    loadWsg(PULSE_002_WSG, &self->wsgs[PL_WSG_PLAYER_WALK3], false);
    loadWsg(PULSE_003_WSG, &self->wsgs[PL_WSG_PLAYER_WALK4], false);
    loadWsg(PULSE_004_WSG, &self->wsgs[PL_WSG_PLAYER_WALK5], false);
    loadWsg(PULSE_005_WSG, &self->wsgs[PL_WSG_PLAYER_WALK6], false);
    loadWsg(PULSE_006_WSG, &self->wsgs[PL_WSG_PLAYER_WALK7], false);
    loadWsg(PULSE_007_WSG, &self->wsgs[PL_WSG_PLAYER_WALK8], false);
    loadWsg(PULSE_008_WSG, &self->wsgs[PL_WSG_PLAYER_WALK9], false);
    loadWsg(PULSE_009_WSG, &self->wsgs[PL_WSG_PLAYER_WALK10], false);
    loadWsg(PULSE_037_WSG, &self->wsgs[PL_WSG_PLAYER_JUMP], false);
    loadWsg(PULSE_038_WSG, &self->wsgs[PL_WSG_PLAYER_JUMP1], false);
    loadWsg(PULSE_041_WSG, &self->wsgs[PL_WSG_PLAYER_JUMP2], false);
    loadWsg(PULSE_043_WSG, &self->wsgs[PL_WSG_PLAYER_JUMP3], false);
    loadWsg(PULSE_046_WSG, &self->wsgs[PL_WSG_PLAYER_JUMP4], false);
    loadWsg(PULSE_024_WSG, &self->wsgs[PL_WSG_PLAYER_SHOOT_IDLE], false);
    loadWsg(PULSE_012_WSG, &self->wsgs[PL_WSG_PLAYER_SHOOT_WALK1], false);
    loadWsg(PULSE_013_WSG, &self->wsgs[PL_WSG_PLAYER_SHOOT_WALK2], false);
    loadWsg(PULSE_014_WSG, &self->wsgs[PL_WSG_PLAYER_SHOOT_WALK3], false);
    loadWsg(PULSE_015_WSG, &self->wsgs[PL_WSG_PLAYER_SHOOT_WALK4], false);
    loadWsg(PULSE_016_WSG, &self->wsgs[PL_WSG_PLAYER_SHOOT_WALK5], false);
    loadWsg(PULSE_017_WSG, &self->wsgs[PL_WSG_PLAYER_SHOOT_WALK6], false);
    loadWsg(PULSE_018_WSG, &self->wsgs[PL_WSG_PLAYER_SHOOT_WALK7], false);
    loadWsg(PULSE_019_WSG, &self->wsgs[PL_WSG_PLAYER_SHOOT_WALK8], false);
    loadWsg(PULSE_020_WSG, &self->wsgs[PL_WSG_PLAYER_SHOOT_WALK9], false);
    loadWsg(PULSE_021_WSG, &self->wsgs[PL_WSG_PLAYER_SHOOT_WALK10], false);
    loadWsg(PULSE_018_WSG, &self->wsgs[PL_WSG_PLAYER_SHOOT_JUMP], false);
    loadWsg(PULSE_019_WSG, &self->wsgs[PL_WSG_PLAYER_SHOOT_JUMP1], false);
    loadWsg(PULSE_020_WSG, &self->wsgs[PL_WSG_PLAYER_SHOOT_JUMP2], false);
    loadWsg(PULSE_021_WSG, &self->wsgs[PL_WSG_PLAYER_SHOOT_JUMP3], false);
    loadWsg(PULSE_021_WSG, &self->wsgs[PL_WSG_PLAYER_SHOOT_JUMP4], false);
    loadWsg(PULSE_005_WSG, &self->wsgs[PL_WSG_PLAYER_SLIDE], false);
    loadWsg(SPRITE_006_WSG, &self->wsgs[PL_WSG_PLAYER_HURT], false);
    loadWsg(SPRITE_007_WSG, &self->wsgs[PL_WSG_PLAYER_CLIMB], false);
    loadWsg(SPRITE_008_WSG, &self->wsgs[PL_WSG_PLAYER_WIN], false);
    loadWsg(SPRITE_009_WSG, &self->wsgs[PL_WSG_ENEMY_BASIC], false);
    loadWsg(SPRITE_012_WSG, &self->wsgs[PL_WSG_DUSTBUNNY_IDLE], false);
    loadWsg(SPRITE_013_WSG, &self->wsgs[PL_WSG_DUSTBUNNY_CHARGE], false);
    loadWsg(SPRITE_014_WSG, &self->wsgs[PL_WSG_DUSTBUNNY_JUMP], false);
    loadWsg(SPRITE_015_WSG, &self->wsgs[PL_WSG_GAMING_1], false);
    loadWsg(SPRITE_016_WSG, &self->wsgs[PL_WSG_GAMING_2], false);
    loadWsg(SPRITE_017_WSG, &self->wsgs[PL_WSG_GAMING_3], false);
    loadWsg(SPRITE_018_WSG, &self->wsgs[PL_WSG_MUSIC_1], false);
    loadWsg(SPRITE_019_WSG, &self->wsgs[PL_WSG_MUSIC_2], false);
    loadWsg(SPRITE_020_WSG, &self->wsgs[PL_WSG_MUSIC_3], false);
    loadWsg(SPRITE_021_WSG, &self->wsgs[PL_WSG_WARP_1], false);
    loadWsg(SPRITE_022_WSG, &self->wsgs[PL_WSG_WARP_2], false);
    loadWsg(SPRITE_023_WSG, &self->wsgs[PL_WSG_WARP_3], false);
    loadWsg(SPRITE_024_WSG, &self->wsgs[PL_WSG_WASP_1], false);
    loadWsg(SPRITE_025_WSG, &self->wsgs[PL_WSG_WASP_2], false);
    loadWsg(SPRITE_026_WSG, &self->wsgs[PL_WSG_WASP_DIVE], false);
    loadWsg(SPRITE_027_WSG, &self->wsgs[PL_WSG_1UP_1], false);
    loadWsg(SPRITE_028_WSG, &self->wsgs[PL_WSG_1UP_2], false);
    loadWsg(SPRITE_029_WSG, &self->wsgs[PL_WSG_1UP_3], false);
    loadWsg(SPRITE_030_WSG, &self->wsgs[PL_WSG_WAVEBALL_1], false);
    loadWsg(SPRITE_031_WSG, &self->wsgs[PL_WSG_WAVEBALL_2], false);
    loadWsg(SPRITE_032_WSG, &self->wsgs[PL_WSG_WAVEBALL_3], false);
    loadWsg(SPRITE_033_WSG, &self->wsgs[PL_WSG_ENEMY_BUSH_L2], false);
    loadWsg(SPRITE_034_WSG, &self->wsgs[PL_WSG_ENEMY_BUSH_L3], false);
    loadWsg(SPRITE_035_WSG, &self->wsgs[PL_WSG_DUSTBUNNY_L2_IDLE], false);
    loadWsg(SPRITE_036_WSG, &self->wsgs[PL_WSG_DUSTBUNNY_L2_CHARGE], false);
    loadWsg(SPRITE_037_WSG, &self->wsgs[PL_WSG_DUSTBUNNY_L2_JUMP], false);
    loadWsg(SPRITE_038_WSG, &self->wsgs[PL_WSG_DUSTBUNNY_L3_IDLE], false);
    loadWsg(SPRITE_039_WSG, &self->wsgs[PL_WSG_DUSTBUNNY_L3_CHARGE], false);
    loadWsg(SPRITE_040_WSG, &self->wsgs[PL_WSG_DUSTBUNNY_L3_JUMP], false);
    loadWsg(SPRITE_041_WSG, &self->wsgs[PL_WSG_WASP_L2_1], false);
    loadWsg(SPRITE_042_WSG, &self->wsgs[PL_WSG_WASP_L2_2], false);
    loadWsg(SPRITE_043_WSG, &self->wsgs[PL_WSG_WASP_L2_DIVE], false);
    loadWsg(SPRITE_044_WSG, &self->wsgs[PL_WSG_WASP_L3_1], false);
    loadWsg(SPRITE_045_WSG, &self->wsgs[PL_WSG_WASP_L3_2], false);
    loadWsg(SPRITE_046_WSG, &self->wsgs[PL_WSG_WASP_L3_DIVE], false);
    loadWsg(SPRITE_047_WSG, &self->wsgs[PL_WSG_CHECKPOINT_INACTIVE], false);
    loadWsg(SPRITE_048_WSG, &self->wsgs[PL_WSG_CHECKPOINT_ACTIVE_1], false);
    loadWsg(SPRITE_049_WSG, &self->wsgs[PL_WSG_CHECKPOINT_ACTIVE_2], false);
    loadWsg(TILE_039_WSG, &self->wsgs[PL_WSG_BOUNCE_BLOCK], false);
    loadWsg(TILE_066_WSG, &self->wsgs[PL_WSG_CONTAINER_1], false);
    loadWsg(TILE_034_WSG, &self->wsgs[PL_WSG_BRICK_BLOCK], false);
    loadWsg(TILE_032_WSG, &self->wsgs[PL_WSG_GRASS], false);
    loadWsg(TILE_033_WSG, &self->wsgs[PL_WSG_GROUND], false);
    loadWsg(TILE_034_WSG, &self->wsgs[PL_WSG_BRICK_BLOCK], false);
    loadWsg(TILE_035_WSG, &self->wsgs[PL_WSG_BLOCK], false);
    loadWsg(TILE_036_WSG, &self->wsgs[PL_WSG_METAL_BLOCK], false);
    loadWsg(TILE_037_WSG, &self->wsgs[PL_WSG_METAL_PIPE_H], false);
    loadWsg(TILE_038_WSG, &self->wsgs[PL_WSG_METAL_PIPE_V], false);
    loadWsg(TILE_039_WSG, &self->wsgs[PL_WSG_BOUNCE_BLOCK], false);
    loadWsg(TILE_040_WSG, &self->wsgs[PL_WSG_DIRT_PATH], false);
    loadWsg(TILE_041_WSG, &self->wsgs[PL_WSG_GIRDER], false);
    loadWsg(TILE_059_WSG, &self->wsgs[PL_WSG_GOAL_100PTS], false);
    loadWsg(TILE_060_WSG, &self->wsgs[PL_WSG_GOAL_500PTS], false);
    loadWsg(TILE_061_WSG, &self->wsgs[PL_WSG_GOAL_1000PTS], false);
    loadWsg(TILE_062_WSG, &self->wsgs[PL_WSG_GOAL_2000PTS], false);
    loadWsg(TILE_063_WSG, &self->wsgs[PL_WSG_GOAL_5000PTS], false);
    loadWsg(TILE_064_WSG, &self->wsgs[PL_WSG_CONTAINER_1], false);
    loadWsg(TILE_065_WSG, &self->wsgs[PL_WSG_CONTAINER_2], false);
    loadWsg(TILE_066_WSG, &self->wsgs[PL_WSG_CONTAINER_3], false);
    loadWsg(TILE_067_WSG, &self->wsgs[PL_WSG_COIN_1], false);
    loadWsg(TILE_068_WSG, &self->wsgs[PL_WSG_COIN_2], false);
    loadWsg(TILE_069_WSG, &self->wsgs[PL_WSG_COIN_3], false);
    loadWsg(TILE_070_WSG, &self->wsgs[PL_WSG_LADDER], false);
    loadWsg(TILE_080_WSG, &self->wsgs[PL_WSG_BG_GOAL_ZONE], false);
    loadWsg(TILE_081_WSG, &self->wsgs[PL_WSG_BG_ARROW_L], false);
    loadWsg(TILE_082_WSG, &self->wsgs[PL_WSG_BG_ARROW_R], false);
    loadWsg(TILE_083_WSG, &self->wsgs[PL_WSG_BG_ARROW_U], false);
    loadWsg(TILE_084_WSG, &self->wsgs[PL_WSG_BG_ARROW_D], false);
    loadWsg(TILE_085_WSG, &self->wsgs[PL_WSG_BG_ARROW_LU], false);
    loadWsg(TILE_086_WSG, &self->wsgs[PL_WSG_BG_ARROW_RU], false);
    loadWsg(TILE_087_WSG, &self->wsgs[PL_WSG_BG_ARROW_LD], false);
    loadWsg(TILE_088_WSG, &self->wsgs[PL_WSG_BG_ARROW_RD], false);
    loadWsg(TILE_089_WSG, &self->wsgs[PL_WSG_BG_CLOUD_LD], false);
    loadWsg(TILE_090_WSG, &self->wsgs[PL_WSG_BG_CLOUD_M], false);
    loadWsg(TILE_091_WSG, &self->wsgs[PL_WSG_BG_CLOUD_RD], false);
    loadWsg(TILE_092_WSG, &self->wsgs[PL_WSG_BG_CLOUD_LU], false);
    loadWsg(TILE_093_WSG, &self->wsgs[PL_WSG_BG_CLOUD_RU], false);
    loadWsg(TILE_094_WSG, &self->wsgs[PL_WSG_BG_CLOUD_D], false);
    loadWsg(TILE_095_WSG, &self->wsgs[PL_WSG_BG_CLOUD], false);
    loadWsg(TILE_096_WSG, &self->wsgs[PL_WSG_BG_TALL_GRASS], false);
    loadWsg(TILE_097_WSG, &self->wsgs[PL_WSG_BG_MOUNTAIN_L], false);
    loadWsg(TILE_098_WSG, &self->wsgs[PL_WSG_BG_MOUNTAIN_U], false);
    loadWsg(TILE_099_WSG, &self->wsgs[PL_WSG_BG_MOUNTAIN_R], false);
    loadWsg(TILE_100_WSG, &self->wsgs[PL_WSG_BG_MOUNTAIN], false);
    loadWsg(TILE_101_WSG, &self->wsgs[PL_WSG_BG_METAL], false);
    loadWsg(TILE_102_WSG, &self->wsgs[PL_WSG_BG_CHAINS], false);
    loadWsg(TILE_103_WSG, &self->wsgs[PL_WSG_BG_WALL], false);
}

void pl_initializeSprites(plWsgManager_t* self)
{
    self->sprites[PL_SP_PLAYER_IDLE].wsg      = &self->wsgs[PL_WSG_PLAYER_IDLE];
    self->sprites[PL_SP_PLAYER_IDLE].origin   = &origin_15_15;
    self->sprites[PL_SP_PLAYER_IDLE].hitBox   = &box_16_32;

    self->sprites[PL_SP_PLAYER_WALK1].wsg     = &self->wsgs[PL_WSG_PLAYER_WALK1];
    self->sprites[PL_SP_PLAYER_WALK1].origin  = &origin_15_15;
    self->sprites[PL_SP_PLAYER_WALK1].hitBox  = &box_16_32;

    self->sprites[PL_SP_PLAYER_WALK2].wsg     = &self->wsgs[PL_WSG_PLAYER_WALK2];
    self->sprites[PL_SP_PLAYER_WALK2].origin  = &origin_15_15;
    self->sprites[PL_SP_PLAYER_WALK2].hitBox  = &box_16_32;

    self->sprites[PL_SP_PLAYER_WALK3].wsg     = &self->wsgs[PL_WSG_PLAYER_WALK3];
    self->sprites[PL_SP_PLAYER_WALK3].origin  = &origin_15_15;
    self->sprites[PL_SP_PLAYER_WALK3].hitBox  = &box_16_32;

    self->sprites[PL_SP_PLAYER_WALK4].wsg     = &self->wsgs[PL_WSG_PLAYER_WALK4];
    self->sprites[PL_SP_PLAYER_WALK4].origin  = &origin_15_15;
    self->sprites[PL_SP_PLAYER_WALK4].hitBox  = &box_16_32;
    
    self->sprites[PL_SP_PLAYER_WALK5].wsg     = &self->wsgs[PL_WSG_PLAYER_WALK5];
    self->sprites[PL_SP_PLAYER_WALK5].origin  = &origin_15_15;
    self->sprites[PL_SP_PLAYER_WALK5].hitBox  = &box_16_32;
    
    self->sprites[PL_SP_PLAYER_WALK6].wsg     = &self->wsgs[PL_WSG_PLAYER_WALK6];
    self->sprites[PL_SP_PLAYER_WALK6].origin  = &origin_15_15;
    self->sprites[PL_SP_PLAYER_WALK6].hitBox  = &box_16_32;
    
    self->sprites[PL_SP_PLAYER_WALK7].wsg     = &self->wsgs[PL_WSG_PLAYER_WALK7];
    self->sprites[PL_SP_PLAYER_WALK7].origin  = &origin_15_15;
    self->sprites[PL_SP_PLAYER_WALK7].hitBox  = &box_16_32;

    self->sprites[PL_SP_PLAYER_WALK8].wsg     = &self->wsgs[PL_WSG_PLAYER_WALK8];
    self->sprites[PL_SP_PLAYER_WALK8].origin  = &origin_15_15;
    self->sprites[PL_SP_PLAYER_WALK8].hitBox  = &box_16_32;

    self->sprites[PL_SP_PLAYER_WALK9].wsg     = &self->wsgs[PL_WSG_PLAYER_WALK9];
    self->sprites[PL_SP_PLAYER_WALK9].origin  = &origin_15_15;
    self->sprites[PL_SP_PLAYER_WALK9].hitBox  = &box_16_32;

    self->sprites[PL_SP_PLAYER_WALK10].wsg     = &self->wsgs[PL_WSG_PLAYER_WALK10];
    self->sprites[PL_SP_PLAYER_WALK10].origin  = &origin_15_15;
    self->sprites[PL_SP_PLAYER_WALK10].hitBox  = &box_16_32;

    self->sprites[PL_SP_PLAYER_JUMP].wsg     = &self->wsgs[PL_WSG_PLAYER_JUMP];
    self->sprites[PL_SP_PLAYER_JUMP].origin  = &origin_15_15;
    self->sprites[PL_SP_PLAYER_JUMP].hitBox  = &box_16_32;

    self->sprites[PL_SP_PLAYER_JUMP1].wsg     = &self->wsgs[PL_WSG_PLAYER_JUMP1];
    self->sprites[PL_SP_PLAYER_JUMP1].origin  = &origin_15_15;
    self->sprites[PL_SP_PLAYER_JUMP1].hitBox  = &box_16_32;

    self->sprites[PL_SP_PLAYER_JUMP2].wsg     = &self->wsgs[PL_WSG_PLAYER_JUMP2];
    self->sprites[PL_SP_PLAYER_JUMP2].origin  = &origin_15_15;
    self->sprites[PL_SP_PLAYER_JUMP2].hitBox  = &box_16_32;

    self->sprites[PL_SP_PLAYER_JUMP3].wsg     = &self->wsgs[PL_WSG_PLAYER_JUMP3];
    self->sprites[PL_SP_PLAYER_JUMP3].origin  = &origin_15_15;
    self->sprites[PL_SP_PLAYER_JUMP3].hitBox  = &box_16_32;

    self->sprites[PL_SP_PLAYER_JUMP4].wsg     = &self->wsgs[PL_WSG_PLAYER_JUMP4];
    self->sprites[PL_SP_PLAYER_JUMP4].origin  = &origin_15_15;
    self->sprites[PL_SP_PLAYER_JUMP4].hitBox  = &box_16_32;

    self->sprites[PL_SP_PLAYER_SLIDE].wsg     = &self->wsgs[PL_WSG_PLAYER_SLIDE];
    self->sprites[PL_SP_PLAYER_SLIDE].origin  = &origin_15_15;
    self->sprites[PL_SP_PLAYER_SLIDE].hitBox  = &box_16_32;

    self->sprites[PL_SP_PLAYER_HURT].wsg     = &self->wsgs[PL_WSG_PLAYER_HURT];
    self->sprites[PL_SP_PLAYER_HURT].origin  = &origin_8_8;
    self->sprites[PL_SP_PLAYER_HURT].hitBox  = &box_16_16;

    self->sprites[PL_SP_PLAYER_CLIMB].wsg     = &self->wsgs[PL_WSG_PLAYER_CLIMB];
    self->sprites[PL_SP_PLAYER_CLIMB].origin  = &origin_8_8;
    self->sprites[PL_SP_PLAYER_CLIMB].hitBox  = &box_16_16;

    self->sprites[PL_SP_PLAYER_WIN].wsg     = &self->wsgs[PL_WSG_PLAYER_WIN];
    self->sprites[PL_SP_PLAYER_WIN].origin  = &origin_8_8;
    self->sprites[PL_SP_PLAYER_WIN].hitBox  = &box_16_16;

    self->sprites[PL_SP_ENEMY_BASIC].wsg     = &self->wsgs[PL_WSG_ENEMY_BASIC];
    self->sprites[PL_SP_ENEMY_BASIC].origin  = &origin_8_8;
    self->sprites[PL_SP_ENEMY_BASIC].hitBox  = &box_16_16;

    self->sprites[PL_SP_HITBLOCK_CONTAINER].wsg     = &self->wsgs[PL_WSG_CONTAINER_1];
    self->sprites[PL_SP_HITBLOCK_CONTAINER].origin  = &origin_8_8;
    self->sprites[PL_SP_HITBLOCK_CONTAINER].hitBox  = &box_16_16;

    self->sprites[PL_SP_HITBLOCK_BRICKS].wsg     = &self->wsgs[PL_WSG_BRICK_BLOCK];
    self->sprites[PL_SP_HITBLOCK_BRICKS].origin = &origin_8_8;
    self->sprites[PL_SP_HITBLOCK_BRICKS].hitBox  = &box_16_16;

    self->sprites[PL_SP_DUSTBUNNY_IDLE].wsg     = &self->wsgs[PL_WSG_DUSTBUNNY_IDLE];
    self->sprites[PL_SP_DUSTBUNNY_IDLE].origin  = &origin_8_8;
    self->sprites[PL_SP_DUSTBUNNY_IDLE].hitBox  = &box_16_16;

    self->sprites[PL_SP_DUSTBUNNY_CHARGE].wsg     = &self->wsgs[PL_WSG_DUSTBUNNY_CHARGE];
    self->sprites[PL_SP_DUSTBUNNY_CHARGE].origin  = &origin_8_8;
    self->sprites[PL_SP_DUSTBUNNY_CHARGE].hitBox  = &box_16_16;

    self->sprites[PL_SP_DUSTBUNNY_JUMP].wsg     = &self->wsgs[PL_WSG_DUSTBUNNY_JUMP];
    self->sprites[PL_SP_DUSTBUNNY_JUMP].origin  = &origin_8_8;
    self->sprites[PL_SP_DUSTBUNNY_JUMP].hitBox  = &box_16_16;

    self->sprites[PL_SP_GAMING_1].wsg     = &self->wsgs[PL_WSG_GAMING_1];
    self->sprites[PL_SP_GAMING_1].origin  = &origin_8_8;
    self->sprites[PL_SP_GAMING_1].hitBox  = &box_16_16;

    self->sprites[PL_SP_GAMING_2].wsg     = &self->wsgs[PL_WSG_GAMING_2];
    self->sprites[PL_SP_GAMING_2].origin  = &origin_8_8;
    self->sprites[PL_SP_GAMING_2].hitBox  = &box_16_16;

    self->sprites[PL_SP_GAMING_3].wsg     = &self->wsgs[PL_WSG_GAMING_3];
    self->sprites[PL_SP_GAMING_3].origin = &origin_8_8;
    self->sprites[PL_SP_GAMING_3].hitBox  = &box_16_16;

    self->sprites[PL_SP_MUSIC_1].wsg     = &self->wsgs[PL_WSG_MUSIC_1];
    self->sprites[PL_SP_MUSIC_1].origin  = &origin_8_8;
    self->sprites[PL_SP_MUSIC_1].hitBox  = &box_16_16;

    self->sprites[PL_SP_MUSIC_2].wsg     = &self->wsgs[PL_WSG_MUSIC_2];
    self->sprites[PL_SP_MUSIC_2].origin  = &origin_8_8;
    self->sprites[PL_SP_MUSIC_2].hitBox  = &box_16_16;

    self->sprites[PL_SP_MUSIC_3].wsg     = &self->wsgs[PL_WSG_MUSIC_3];
    self->sprites[PL_SP_MUSIC_3].origin  = &origin_8_8;
    self->sprites[PL_SP_MUSIC_3].hitBox  = &box_16_16;

    self->sprites[PL_SP_WARP_1].wsg     = &self->wsgs[PL_WSG_WARP_1];
    self->sprites[PL_SP_WARP_1].origin  = &origin_8_8;
    self->sprites[PL_SP_WARP_1].hitBox  = &box_16_16;

    self->sprites[PL_SP_WARP_2].wsg     = &self->wsgs[PL_WSG_WARP_2];
    self->sprites[PL_SP_WARP_2].origin  = &origin_8_8;
    self->sprites[PL_SP_WARP_2].hitBox  = &box_16_16;

    self->sprites[PL_SP_WARP_3].wsg     = &self->wsgs[PL_WSG_WARP_3];
    self->sprites[PL_SP_WARP_3].origin  = &origin_8_8;
    self->sprites[PL_SP_WARP_3].hitBox  = &box_16_16;

    self->sprites[PL_SP_WASP_1].wsg     = &self->wsgs[PL_WSG_WASP_1];
    self->sprites[PL_SP_WASP_1].origin  = &origin_8_8;
    self->sprites[PL_SP_WASP_1].hitBox  = &box_16_16;

    self->sprites[PL_SP_WASP_2].wsg     = &self->wsgs[PL_WSG_WASP_2];
    self->sprites[PL_SP_WASP_2].origin  = &origin_8_8;
    self->sprites[PL_SP_WASP_2].hitBox  = &box_16_16;

    self->sprites[PL_SP_WASP_DIVE].wsg     = &self->wsgs[PL_WSG_WASP_DIVE];
    self->sprites[PL_SP_WASP_DIVE].origin  = &origin_8_8;
    self->sprites[PL_SP_WASP_DIVE].hitBox  = &box_16_16;

    self->sprites[PL_SP_1UP_1].wsg     = &self->wsgs[PL_WSG_1UP_1];
    self->sprites[PL_SP_1UP_1].origin  = &origin_8_8;
    self->sprites[PL_SP_1UP_1].hitBox  = &box_16_16;

    self->sprites[PL_SP_1UP_2].wsg     = &self->wsgs[PL_WSG_1UP_2];
    self->sprites[PL_SP_1UP_2].origin  = &origin_8_8;
    self->sprites[PL_SP_1UP_2].hitBox  = &box_16_16;

    self->sprites[PL_SP_1UP_3].wsg     = &self->wsgs[PL_WSG_1UP_3];
    self->sprites[PL_SP_1UP_3].origin  = &origin_8_8;
    self->sprites[PL_SP_1UP_3].hitBox  = &box_16_16;

    self->sprites[PL_SP_WAVEBALL_1].wsg     = &self->wsgs[PL_WSG_WAVEBALL_1];
    self->sprites[PL_SP_WAVEBALL_1].origin  = &origin_8_8;
    self->sprites[PL_SP_WAVEBALL_1].hitBox  = &box_16_16;

    self->sprites[PL_SP_WAVEBALL_2].wsg     = &self->wsgs[PL_WSG_WAVEBALL_2];
    self->sprites[PL_SP_WAVEBALL_2].origin  = &origin_8_8;
    self->sprites[PL_SP_WAVEBALL_2].hitBox  = &box_16_16;

    self->sprites[PL_SP_WAVEBALL_3].wsg     = &self->wsgs[PL_WSG_WAVEBALL_3];
    self->sprites[PL_SP_WAVEBALL_3].origin  = &origin_8_8;
    self->sprites[PL_SP_WAVEBALL_3].hitBox  = &box_16_16;

    self->sprites[PL_SP_ENEMY_BUSH_L2].wsg     = &self->wsgs[PL_WSG_ENEMY_BUSH_L2];
    self->sprites[PL_SP_ENEMY_BUSH_L2].origin = &origin_8_8;
    self->sprites[PL_SP_ENEMY_BUSH_L2].hitBox  = &box_16_16;

    self->sprites[PL_SP_ENEMY_BUSH_L3].wsg     = &self->wsgs[PL_WSG_ENEMY_BUSH_L3];
    self->sprites[PL_SP_ENEMY_BUSH_L3].origin = &origin_8_8;
    self->sprites[PL_SP_ENEMY_BUSH_L3].hitBox  = &box_16_16;

    self->sprites[PL_SP_DUSTBUNNY_L2_IDLE].wsg     = &self->wsgs[PL_WSG_DUSTBUNNY_L2_IDLE];
    self->sprites[PL_SP_DUSTBUNNY_L2_IDLE].origin  = &origin_8_8;
    self->sprites[PL_SP_DUSTBUNNY_L2_IDLE].hitBox  = &box_16_16;

    self->sprites[PL_SP_DUSTBUNNY_L2_CHARGE].wsg     = &self->wsgs[PL_WSG_DUSTBUNNY_L2_CHARGE];
    self->sprites[PL_SP_DUSTBUNNY_L2_CHARGE].origin  = &origin_8_8;
    self->sprites[PL_SP_DUSTBUNNY_L2_CHARGE].hitBox  = &box_16_16;

    self->sprites[PL_SP_DUSTBUNNY_L2_JUMP].wsg     = &self->wsgs[PL_WSG_DUSTBUNNY_L2_JUMP];
    self->sprites[PL_SP_DUSTBUNNY_L2_JUMP].origin  = &origin_8_8;
    self->sprites[PL_SP_DUSTBUNNY_L2_JUMP].hitBox  = &box_16_16;

    self->sprites[PL_SP_DUSTBUNNY_L3_IDLE].wsg     = &self->wsgs[PL_WSG_DUSTBUNNY_L3_IDLE];
    self->sprites[PL_SP_DUSTBUNNY_L3_IDLE].origin  = &origin_8_8;
    self->sprites[PL_SP_DUSTBUNNY_L3_IDLE].hitBox  = &box_16_16;

    self->sprites[PL_SP_DUSTBUNNY_L3_CHARGE].wsg     = &self->wsgs[PL_WSG_DUSTBUNNY_L3_CHARGE];
    self->sprites[PL_SP_DUSTBUNNY_L3_CHARGE].origin  = &origin_8_8;
    self->sprites[PL_SP_DUSTBUNNY_L3_CHARGE].hitBox  = &box_16_16;

    self->sprites[PL_SP_DUSTBUNNY_L3_JUMP].wsg     = &self->wsgs[PL_WSG_DUSTBUNNY_L3_JUMP];
    self->sprites[PL_SP_DUSTBUNNY_L3_JUMP].origin  = &origin_8_8;
    self->sprites[PL_SP_DUSTBUNNY_L3_JUMP].hitBox  = &box_16_16;

    self->sprites[PL_SP_WASP_L2_1].wsg     = &self->wsgs[PL_WSG_WASP_L2_1];
    self->sprites[PL_SP_WASP_L2_1].origin  = &origin_8_8;
    self->sprites[PL_SP_WASP_L2_1].hitBox  = &box_16_16;

    self->sprites[PL_SP_WASP_L2_2].wsg     = &self->wsgs[PL_WSG_WASP_L2_2];
    self->sprites[PL_SP_WASP_L2_2].origin  = &origin_8_8;
    self->sprites[PL_SP_WASP_L2_2].hitBox  = &box_16_16;

    self->sprites[PL_SP_WASP_L2_DIVE].wsg     = &self->wsgs[PL_WSG_WASP_L2_DIVE];
    self->sprites[PL_SP_WASP_L2_DIVE].origin  = &origin_8_8;
    self->sprites[PL_SP_WASP_L2_DIVE].hitBox  = &box_16_16;

    self->sprites[PL_SP_WASP_L3_1].wsg     = &self->wsgs[PL_WSG_WASP_L3_1];
    self->sprites[PL_SP_WASP_L3_1].origin  = &origin_8_8;
    self->sprites[PL_SP_WASP_L3_1].hitBox  = &box_16_16;

    self->sprites[PL_SP_WASP_L3_2].wsg     = &self->wsgs[PL_WSG_WASP_L3_2];
    self->sprites[PL_SP_WASP_L3_2].origin  = &origin_8_8;
    self->sprites[PL_SP_WASP_L3_2].hitBox  = &box_16_16;

    self->sprites[PL_SP_WASP_L3_DIVE].wsg     = &self->wsgs[PL_WSG_WASP_L3_DIVE];
    self->sprites[PL_SP_WASP_L3_DIVE].origin  = &origin_8_8;
    self->sprites[PL_SP_WASP_L3_DIVE].hitBox  = &box_16_16;

    self->sprites[PL_SP_CHECKPOINT_INACTIVE].wsg     = &self->wsgs[PL_WSG_CHECKPOINT_INACTIVE];
    self->sprites[PL_SP_CHECKPOINT_INACTIVE].origin  = &origin_8_8;
    self->sprites[PL_SP_CHECKPOINT_INACTIVE].hitBox  = &box_16_16;

    self->sprites[PL_SP_CHECKPOINT_ACTIVE_1].wsg     = &self->wsgs[PL_WSG_CHECKPOINT_ACTIVE_1];
    self->sprites[PL_SP_CHECKPOINT_ACTIVE_1].origin  = &origin_8_8;
    self->sprites[PL_SP_CHECKPOINT_ACTIVE_1].hitBox  = &box_16_16;

    self->sprites[PL_SP_CHECKPOINT_ACTIVE_2].wsg     = &self->wsgs[PL_WSG_CHECKPOINT_ACTIVE_2];
    self->sprites[PL_SP_CHECKPOINT_ACTIVE_2].origin  = &origin_8_8;
    self->sprites[PL_SP_CHECKPOINT_ACTIVE_2].hitBox  = &box_16_16;

    self->sprites[PL_SP_BOUNCE_BLOCK].wsg     = &self->wsgs[PL_WSG_BOUNCE_BLOCK];
    self->sprites[PL_SP_BOUNCE_BLOCK].origin  = &origin_8_8;
    self->sprites[PL_SP_BOUNCE_BLOCK].hitBox  = &box_16_16;
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

void pl_remapPlayerShootWsg(plWsgManager_t* self) {
    for (uint16_t i = PL_SP_PLAYER_IDLE; i < (PL_SP_PLAYER_JUMP4 + 1); i++)
    {
        pl_remapWsgToSprite(self, i, PL_WSG_PLAYER_SHOOT_IDLE + i);
    }
}

void pl_remapPlayerNotShootWsg(plWsgManager_t* self) {
    for (uint16_t i = PL_SP_PLAYER_IDLE; i < (PL_SP_PLAYER_JUMP4 + 1); i++)
    {
        pl_remapWsgToSprite(self, i, PL_WSG_PLAYER_IDLE + i);
    }
}