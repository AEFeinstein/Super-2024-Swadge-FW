//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include <stdbool.h>
#include "fs_wsg.h"
#include "mgWsgManager.h"
#include "mgTilesetConfig.h"
#include "mega_pulse_ex_typedef.h"
#include <esp_log.h>

//==============================================================================
// Functions
//==============================================================================

void mg_initializeWsgManager(mgWsgManager_t* self)
{
    self->wsgSetIndex          = MG_WSGSET_DEFAULT;
    self->transparencyFunction = &mg_dummyTileset_needsTransparency;

    mg_loadWsgs(self);
    mg_initializeSprites(self);
    mg_initializeTiles(self);

    mg_loadWsgSet(self, MG_WSGSET_KINETIC_DONUT);
}

void mg_freeWsgManager(mgWsgManager_t* self)
{
    for (uint16_t i = 0; i < MG_WSGS_SIZE; i++)
    {
        if (!(self->wsgs[i].w && self->wsgs[i].h))
        {
            continue;
        }

        freeWsg(&self->wsgs[i]);
    }
}

void mg_loadWsgs(mgWsgManager_t* self)
{
    for (uint16_t i = 0; i < MG_WSGS_SIZE; i++)
    {
        self->wsgs[i].w = 0;
        self->wsgs[i].h = 0;
    }

    loadWsg(PULSE_036_WSG, &self->wsgs[MG_WSG_PLAYER_IDLE], false);
    loadWsg(PULSE_000_WSG, &self->wsgs[MG_WSG_PLAYER_WALK1], false);
    loadWsg(PULSE_001_WSG, &self->wsgs[MG_WSG_PLAYER_WALK2], false);
    loadWsg(PULSE_002_WSG, &self->wsgs[MG_WSG_PLAYER_WALK3], false);
    loadWsg(PULSE_003_WSG, &self->wsgs[MG_WSG_PLAYER_WALK4], false);
    loadWsg(PULSE_004_WSG, &self->wsgs[MG_WSG_PLAYER_WALK5], false);
    loadWsg(PULSE_005_WSG, &self->wsgs[MG_WSG_PLAYER_WALK6], false);
    loadWsg(PULSE_006_WSG, &self->wsgs[MG_WSG_PLAYER_WALK7], false);
    loadWsg(PULSE_007_WSG, &self->wsgs[MG_WSG_PLAYER_WALK8], false);
    loadWsg(PULSE_008_WSG, &self->wsgs[MG_WSG_PLAYER_WALK9], false);
    loadWsg(PULSE_009_WSG, &self->wsgs[MG_WSG_PLAYER_WALK10], false);
    loadWsg(PULSE_037_WSG, &self->wsgs[MG_WSG_PLAYER_JUMP], false);
    loadWsg(PULSE_038_WSG, &self->wsgs[MG_WSG_PLAYER_JUMP1], false);
    loadWsg(PULSE_041_WSG, &self->wsgs[MG_WSG_PLAYER_JUMP2], false);
    loadWsg(PULSE_043_WSG, &self->wsgs[MG_WSG_PLAYER_JUMP3], false);
    loadWsg(PULSE_046_WSG, &self->wsgs[MG_WSG_PLAYER_JUMP4], false);
    loadWsg(PULSE_024_WSG, &self->wsgs[MG_WSG_PLAYER_SHOOT_IDLE], false);
    loadWsg(PULSE_012_WSG, &self->wsgs[MG_WSG_PLAYER_SHOOT_WALK1], false);
    loadWsg(PULSE_013_WSG, &self->wsgs[MG_WSG_PLAYER_SHOOT_WALK2], false);
    loadWsg(PULSE_014_WSG, &self->wsgs[MG_WSG_PLAYER_SHOOT_WALK3], false);
    loadWsg(PULSE_015_WSG, &self->wsgs[MG_WSG_PLAYER_SHOOT_WALK4], false);
    loadWsg(PULSE_016_WSG, &self->wsgs[MG_WSG_PLAYER_SHOOT_WALK5], false);
    loadWsg(PULSE_017_WSG, &self->wsgs[MG_WSG_PLAYER_SHOOT_WALK6], false);
    loadWsg(PULSE_018_WSG, &self->wsgs[MG_WSG_PLAYER_SHOOT_WALK7], false);
    loadWsg(PULSE_019_WSG, &self->wsgs[MG_WSG_PLAYER_SHOOT_WALK8], false);
    loadWsg(PULSE_020_WSG, &self->wsgs[MG_WSG_PLAYER_SHOOT_WALK9], false);
    loadWsg(PULSE_021_WSG, &self->wsgs[MG_WSG_PLAYER_SHOOT_WALK10], false);
    loadWsg(PULSE_018_WSG, &self->wsgs[MG_WSG_PLAYER_SHOOT_JUMP], false);
    loadWsg(PULSE_019_WSG, &self->wsgs[MG_WSG_PLAYER_SHOOT_JUMP1], false);
    loadWsg(PULSE_020_WSG, &self->wsgs[MG_WSG_PLAYER_SHOOT_JUMP2], false);
    loadWsg(PULSE_021_WSG, &self->wsgs[MG_WSG_PLAYER_SHOOT_JUMP3], false);
    loadWsg(PULSE_021_WSG, &self->wsgs[MG_WSG_PLAYER_SHOOT_JUMP4], false);
    loadWsg(PULSE_005_WSG, &self->wsgs[MG_WSG_PLAYER_SLIDE], false);
    loadWsg(PULSE_DMG_000_WSG, &self->wsgs[MG_WSG_PLAYER_HURT], false);
    loadWsg(SPRITE_007_WSG, &self->wsgs[MG_WSG_PLAYER_CLIMB], false);
    loadWsg(PULSE_WIN_WSG, &self->wsgs[MG_WSG_PLAYER_WIN], false);
    loadWsg(SPRITE_009_WSG, &self->wsgs[MG_WSG_ENEMY_BASIC], false);
    loadWsg(SPRITE_012_WSG, &self->wsgs[MG_WSG_DUSTBUNNY_IDLE], false);
    loadWsg(SPRITE_013_WSG, &self->wsgs[MG_WSG_DUSTBUNNY_CHARGE], false);
    loadWsg(SPRITE_014_WSG, &self->wsgs[MG_WSG_DUSTBUNNY_JUMP], false);
    loadWsg(SPRITE_015_WSG, &self->wsgs[MG_WSG_GAMING_1], false);
    loadWsg(SPRITE_016_WSG, &self->wsgs[MG_WSG_GAMING_2], false);
    loadWsg(SPRITE_017_WSG, &self->wsgs[MG_WSG_GAMING_3], false);
    loadWsg(SPRITE_018_WSG, &self->wsgs[MG_WSG_MUSIC_1], false);
    loadWsg(SPRITE_019_WSG, &self->wsgs[MG_WSG_MUSIC_2], false);
    loadWsg(SPRITE_020_WSG, &self->wsgs[MG_WSG_MUSIC_3], false);
    loadWsg(SPRITE_021_WSG, &self->wsgs[MG_WSG_WARP_1], false);
    loadWsg(SPRITE_022_WSG, &self->wsgs[MG_WSG_WARP_2], false);
    loadWsg(SPRITE_023_WSG, &self->wsgs[MG_WSG_WARP_3], false);
    loadWsg(SPRITE_024_WSG, &self->wsgs[MG_WSG_WASP_1], false);
    loadWsg(SPRITE_025_WSG, &self->wsgs[MG_WSG_WASP_2], false);
    loadWsg(SPRITE_026_WSG, &self->wsgs[MG_WSG_WASP_DIVE], false);
    loadWsg(SPRITE_027_WSG, &self->wsgs[MG_WSG_1UP_1], false);
    loadWsg(SPRITE_028_WSG, &self->wsgs[MG_WSG_1UP_2], false);
    loadWsg(SPRITE_029_WSG, &self->wsgs[MG_WSG_1UP_3], false);
    loadWsg(SHOT_BASIC_000_WSG, &self->wsgs[MG_WSG_WAVEBALL_1], false);
    loadWsg(SHOT_BASIC_001_WSG, &self->wsgs[MG_WSG_WAVEBALL_2], false);
    loadWsg(SHOT_BASIC_002_WSG, &self->wsgs[MG_WSG_WAVEBALL_3], false);
    loadWsg(SPRITE_033_WSG, &self->wsgs[MG_WSG_ENEMY_BUSH_L2], false);
    loadWsg(SPRITE_034_WSG, &self->wsgs[MG_WSG_ENEMY_BUSH_L3], false);
    loadWsg(SPRITE_035_WSG, &self->wsgs[MG_WSG_DUSTBUNNY_L2_IDLE], false);
    loadWsg(SPRITE_036_WSG, &self->wsgs[MG_WSG_DUSTBUNNY_L2_CHARGE], false);
    loadWsg(SPRITE_037_WSG, &self->wsgs[MG_WSG_DUSTBUNNY_L2_JUMP], false);
    loadWsg(SPRITE_038_WSG, &self->wsgs[MG_WSG_DUSTBUNNY_L3_IDLE], false);
    loadWsg(SPRITE_039_WSG, &self->wsgs[MG_WSG_DUSTBUNNY_L3_CHARGE], false);
    loadWsg(SPRITE_040_WSG, &self->wsgs[MG_WSG_DUSTBUNNY_L3_JUMP], false);
    loadWsg(SPRITE_041_WSG, &self->wsgs[MG_WSG_WASP_L2_1], false);
    loadWsg(SPRITE_042_WSG, &self->wsgs[MG_WSG_WASP_L2_2], false);
    loadWsg(SPRITE_043_WSG, &self->wsgs[MG_WSG_WASP_L2_DIVE], false);
    loadWsg(SPRITE_044_WSG, &self->wsgs[MG_WSG_WASP_L3_1], false);
    loadWsg(SPRITE_045_WSG, &self->wsgs[MG_WSG_WASP_L3_2], false);
    loadWsg(SPRITE_046_WSG, &self->wsgs[MG_WSG_WASP_L3_DIVE], false);
    loadWsg(SPRITE_047_WSG, &self->wsgs[MG_WSG_CHECKPOINT_INACTIVE], false);
    loadWsg(SPRITE_048_WSG, &self->wsgs[MG_WSG_CHECKPOINT_ACTIVE_1], false);
    loadWsg(SPRITE_049_WSG, &self->wsgs[MG_WSG_CHECKPOINT_ACTIVE_2], false);
    loadWsg(TILE_039_WSG, &self->wsgs[MG_WSG_BOUNCE_BLOCK], false);
    loadWsg(TILE_066_WSG, &self->wsgs[MG_WSG_CONTAINER_1], false);
    loadWsg(TILE_034_WSG, &self->wsgs[MG_WSG_BRICK_BLOCK], false);
    loadWsg(TILE_032_WSG, &self->wsgs[MG_WSG_GRASS], false);
    loadWsg(TILE_033_WSG, &self->wsgs[MG_WSG_GROUND], false);
    loadWsg(TILE_034_WSG, &self->wsgs[MG_WSG_BRICK_BLOCK], false);
    loadWsg(TILE_035_WSG, &self->wsgs[MG_WSG_BLOCK], false);
    loadWsg(TILE_036_WSG, &self->wsgs[MG_WSG_METAL_BLOCK], false);
    loadWsg(TILE_037_WSG, &self->wsgs[MG_WSG_METAL_PIPE_H], false);
    loadWsg(TILE_038_WSG, &self->wsgs[MG_WSG_METAL_PIPE_V], false);
    loadWsg(TILE_039_WSG, &self->wsgs[MG_WSG_BOUNCE_BLOCK], false);
    loadWsg(TILE_040_WSG, &self->wsgs[MG_WSG_DIRT_PATH], false);
    loadWsg(TILE_041_WSG, &self->wsgs[MG_WSG_GIRDER], false);
    loadWsg(TILE_059_WSG, &self->wsgs[MG_WSG_GOAL_100PTS], false);
    loadWsg(TILE_060_WSG, &self->wsgs[MG_WSG_GOAL_500PTS], false);
    loadWsg(TILE_061_WSG, &self->wsgs[MG_WSG_GOAL_1000PTS], false);
    loadWsg(TILE_062_WSG, &self->wsgs[MG_WSG_GOAL_2000PTS], false);
    loadWsg(TILE_063_WSG, &self->wsgs[MG_WSG_GOAL_5000PTS], false);
    loadWsg(TILE_064_WSG, &self->wsgs[MG_WSG_CONTAINER_1], false);
    loadWsg(TILE_065_WSG, &self->wsgs[MG_WSG_CONTAINER_2], false);
    loadWsg(TILE_066_WSG, &self->wsgs[MG_WSG_CONTAINER_3], false);
    loadWsg(TILE_067_WSG, &self->wsgs[MG_WSG_COIN_1], false);
    loadWsg(TILE_068_WSG, &self->wsgs[MG_WSG_COIN_2], false);
    loadWsg(TILE_069_WSG, &self->wsgs[MG_WSG_COIN_3], false);
    loadWsg(TILE_070_WSG, &self->wsgs[MG_WSG_LADDER], false);
    loadWsg(TILE_080_WSG, &self->wsgs[MG_WSG_BG_GOAL_ZONE], false);
    loadWsg(TILE_081_WSG, &self->wsgs[MG_WSG_BG_ARROW_L], false);
    loadWsg(TILE_082_WSG, &self->wsgs[MG_WSG_BG_ARROW_R], false);
    loadWsg(TILE_083_WSG, &self->wsgs[MG_WSG_BG_ARROW_U], false);
    loadWsg(TILE_084_WSG, &self->wsgs[MG_WSG_BG_ARROW_D], false);
    loadWsg(TILE_085_WSG, &self->wsgs[MG_WSG_BG_ARROW_LU], false);
    loadWsg(TILE_086_WSG, &self->wsgs[MG_WSG_BG_ARROW_RU], false);
    loadWsg(TILE_087_WSG, &self->wsgs[MG_WSG_BG_ARROW_LD], false);
    loadWsg(TILE_088_WSG, &self->wsgs[MG_WSG_BG_ARROW_RD], false);
    loadWsg(TILE_089_WSG, &self->wsgs[MG_WSG_BG_CLOUD_LD], false);
    loadWsg(TILE_090_WSG, &self->wsgs[MG_WSG_BG_CLOUD_M], false);
    loadWsg(TILE_091_WSG, &self->wsgs[MG_WSG_BG_CLOUD_RD], false);
    loadWsg(TILE_092_WSG, &self->wsgs[MG_WSG_BG_CLOUD_LU], false);
    loadWsg(TILE_093_WSG, &self->wsgs[MG_WSG_BG_CLOUD_RU], false);
    loadWsg(TILE_094_WSG, &self->wsgs[MG_WSG_BG_CLOUD_D], false);
    loadWsg(TILE_095_WSG, &self->wsgs[MG_WSG_BG_CLOUD], false);
    loadWsg(TILE_096_WSG, &self->wsgs[MG_WSG_BG_TALL_GRASS], false);
    loadWsg(TILE_097_WSG, &self->wsgs[MG_WSG_BG_MOUNTAIN_L], false);
    loadWsg(TILE_098_WSG, &self->wsgs[MG_WSG_BG_MOUNTAIN_U], false);
    loadWsg(TILE_099_WSG, &self->wsgs[MG_WSG_BG_MOUNTAIN_R], false);
    loadWsg(TILE_100_WSG, &self->wsgs[MG_WSG_BG_MOUNTAIN], false);
    loadWsg(TILE_101_WSG, &self->wsgs[MG_WSG_BG_METAL], false);
    loadWsg(TILE_102_WSG, &self->wsgs[MG_WSG_BG_CHAINS], false);
    loadWsg(TILE_103_WSG, &self->wsgs[MG_WSG_BG_WALL], false);
    loadWsg(HP_BOTTOM_ALPHA_WSG, &self->wsgs[MG_WSG_HP_BOTTOM_ALPHA], false);
    loadWsg(HP_MIDDLE_0_WSG, &self->wsgs[MG_WSG_HP_MIDDLE_0], false);
    loadWsg(HP_MIDDLE_1_WSG, &self->wsgs[MG_WSG_HP_MIDDLE_1], false);
    loadWsg(HP_MIDDLE_2_WSG, &self->wsgs[MG_WSG_HP_MIDDLE_2], false);
    loadWsg(HP_MIDDLE_3_WSG, &self->wsgs[MG_WSG_HP_MIDDLE_3], false);
    loadWsg(HP_MIDDLE_4_WSG, &self->wsgs[MG_WSG_HP_MIDDLE_4], false);
    loadWsg(HP_MIDDLE_5_WSG, &self->wsgs[MG_WSG_HP_MIDDLE_5], false);
    loadWsg(HP_MIDDLE_6_WSG, &self->wsgs[MG_WSG_HP_MIDDLE_6], false);
    loadWsg(HP_TOP_0_WSG, &self->wsgs[MG_WSG_HP_TOP_0], false);
    loadWsg(HP_TOP_1_WSG, &self->wsgs[MG_WSG_HP_TOP_1], false);
    loadWsg(HP_TOP_2_WSG, &self->wsgs[MG_WSG_HP_TOP_2], false);
    loadWsg(HP_TOP_3_WSG, &self->wsgs[MG_WSG_HP_TOP_3], false);
    loadWsg(HP_TOP_4_WSG, &self->wsgs[MG_WSG_HP_TOP_4], false);
    loadWsg(HP_TOP_5_WSG, &self->wsgs[MG_WSG_HP_TOP_5], false);
    loadWsg(HP_TOP_6_WSG, &self->wsgs[MG_WSG_HP_TOP_6], false);
    loadWsg(HP_BOTTOM_BIGMA_WSG, &self->wsgs[MG_WSG_HP_BOTTOM_BIGMA], false);
    loadWsg(HP_BOSS_MIDDLE_0_WSG, &self->wsgs[MG_WSG_HP_BOSS_MIDDLE_0], false);
    loadWsg(HP_BOSS_MIDDLE_1_WSG, &self->wsgs[MG_WSG_HP_BOSS_MIDDLE_1], false);
    loadWsg(HP_BOSS_MIDDLE_2_WSG, &self->wsgs[MG_WSG_HP_BOSS_MIDDLE_2], false);
    loadWsg(HP_BOSS_MIDDLE_3_WSG, &self->wsgs[MG_WSG_HP_BOSS_MIDDLE_3], false);
    loadWsg(HP_BOSS_MIDDLE_4_WSG, &self->wsgs[MG_WSG_HP_BOSS_MIDDLE_4], false);
    loadWsg(HP_BOSS_MIDDLE_5_WSG, &self->wsgs[MG_WSG_HP_BOSS_MIDDLE_5], false);
    loadWsg(HP_BOSS_MIDDLE_6_WSG, &self->wsgs[MG_WSG_HP_BOSS_MIDDLE_6], false);
    loadWsg(HP_BOSS_TOP_0_WSG, &self->wsgs[MG_WSG_HP_BOSS_TOP_0], false);
    loadWsg(HP_BOSS_TOP_1_WSG, &self->wsgs[MG_WSG_HP_BOSS_TOP_1], false);
    loadWsg(HP_BOSS_TOP_2_WSG, &self->wsgs[MG_WSG_HP_BOSS_TOP_2], false);
    loadWsg(HP_BOSS_TOP_3_WSG, &self->wsgs[MG_WSG_HP_BOSS_TOP_3], false);
    loadWsg(HP_BOSS_TOP_4_WSG, &self->wsgs[MG_WSG_HP_BOSS_TOP_4], false);
    loadWsg(HP_BOSS_TOP_5_WSG, &self->wsgs[MG_WSG_HP_BOSS_TOP_5], false);
    loadWsg(HP_BOSS_TOP_6_WSG, &self->wsgs[MG_WSG_HP_BOSS_TOP_6], false);
    loadWsg(PLACEHOLDER_30X_30_WSG, &self->wsgs[MG_WSG_CHARGIN_SCHMUCK_IDLE], false);
    loadWsg(PLACEHOLDER_30X_30_WSG, &self->wsgs[MG_WSG_CHARGIN_SCHMUCK_RUN1], false);
    loadWsg(PLACEHOLDER_30X_30_WSG, &self->wsgs[MG_WSG_CHARGIN_SCHMUCK_RUN2], false);
    loadWsg(PLACEHOLDER_24X_24_WSG, &self->wsgs[MG_WSG_TURRET_HORIZONTAL], false);
    loadWsg(PLACEHOLDER_24X_24_WSG, &self->wsgs[MG_WSG_TURRET_45DEG], false);
    loadWsg(PLACEHOLDER_24X_24_WSG, &self->wsgs[MG_WSG_TURRET_VERTICAL], false);
    loadWsg(TILE_TBD_WSG, &self->wsgs[MG_WSG_LEMON_SHOT], false);
    loadWsg(PLACEHOLDER_24X_24_WSG, &self->wsgs[MG_WSG_SHELDON_SHIELDY_SHIELD], false);
    loadWsg(PLACEHOLDER_24X_24_WSG, &self->wsgs[MG_WSG_SHELDON_SHIELDY_WALK1], false);
    loadWsg(PLACEHOLDER_24X_24_WSG, &self->wsgs[MG_WSG_SHELDON_SHIELDY_WALK2], false);
    loadWsg(PLACEHOLDER_24X_24_WSG, &self->wsgs[MG_WSG_SHELDON_SHIELDY_WALK3], false);
    loadWsg(PLACEHOLDER_24X_24_WSG, &self->wsgs[MG_WSG_SPIKY_MCGEE], false);
    loadWsg(PLACEHOLDER_24X_24_WSG, &self->wsgs[MG_WSG_AIR_TURRET], false);
    loadWsg(KD_BURGER_BOUNCE_UPRIGHT_WSG, &self->wsgs[MG_WSG_BOUNCE_PAD], false);
    loadWsg(KD_BURGER_BOUNCE_DIAGONAL_WSG, &self->wsgs[MG_WSG_BOUNCE_PAD_DIAGONAL], false);
    loadWsg(TILE_TBD_WSG, &self->wsgs[MG_WSG_MIXTAPE], false);
    loadWsg(PLACEHOLDER_30X_30_WSG, &self->wsgs[MG_WSG_BOSS_IDLE], false);
    loadWsg(PLACEHOLDER_30X_30_WSG, &self->wsgs[MG_WSG_BOSS_MOVE_1], false);
    loadWsg(PLACEHOLDER_30X_30_WSG, &self->wsgs[MG_WSG_BOSS_MOVE_2], false);
    loadWsg(PLACEHOLDER_30X_30_WSG, &self->wsgs[MG_WSG_BOSS_MOVE_3], false);
    loadWsg(WARP_WALL_WSG, &self->wsgs[MG_WSG_BOSS_DOOR], false);
    loadWsg(PULSE_SLIDE_WSG, &self->wsgs[MG_WSG_PLAYER_DASH_SLIDE], false);
    loadWsg(PULSE_DMG_001_WSG, &self->wsgs[MG_WSG_PLAYER_HURT_2], false);
    loadWsg(PULSE_DMG_002_WSG, &self->wsgs[MG_WSG_PLAYER_HURT_3], false);
}

void mg_initializeSprites(mgWsgManager_t* self)
{
    self->sprites[MG_SP_PLAYER_IDLE].wsg    = &self->wsgs[MG_WSG_PLAYER_IDLE];
    self->sprites[MG_SP_PLAYER_IDLE].origin = &origin_15_15;
    self->sprites[MG_SP_PLAYER_IDLE].hitBox = &box_16_32;

    self->sprites[MG_SP_PLAYER_WALK1].wsg    = &self->wsgs[MG_WSG_PLAYER_WALK1];
    self->sprites[MG_SP_PLAYER_WALK1].origin = &origin_15_15;
    self->sprites[MG_SP_PLAYER_WALK1].hitBox = &box_16_32;

    self->sprites[MG_SP_PLAYER_WALK2].wsg    = &self->wsgs[MG_WSG_PLAYER_WALK2];
    self->sprites[MG_SP_PLAYER_WALK2].origin = &origin_15_15;
    self->sprites[MG_SP_PLAYER_WALK2].hitBox = &box_16_32;

    self->sprites[MG_SP_PLAYER_WALK3].wsg    = &self->wsgs[MG_WSG_PLAYER_WALK3];
    self->sprites[MG_SP_PLAYER_WALK3].origin = &origin_15_15;
    self->sprites[MG_SP_PLAYER_WALK3].hitBox = &box_16_32;

    self->sprites[MG_SP_PLAYER_WALK4].wsg    = &self->wsgs[MG_WSG_PLAYER_WALK4];
    self->sprites[MG_SP_PLAYER_WALK4].origin = &origin_15_15;
    self->sprites[MG_SP_PLAYER_WALK4].hitBox = &box_16_32;

    self->sprites[MG_SP_PLAYER_WALK5].wsg    = &self->wsgs[MG_WSG_PLAYER_WALK5];
    self->sprites[MG_SP_PLAYER_WALK5].origin = &origin_15_15;
    self->sprites[MG_SP_PLAYER_WALK5].hitBox = &box_16_32;

    self->sprites[MG_SP_PLAYER_WALK6].wsg    = &self->wsgs[MG_WSG_PLAYER_WALK6];
    self->sprites[MG_SP_PLAYER_WALK6].origin = &origin_15_15;
    self->sprites[MG_SP_PLAYER_WALK6].hitBox = &box_16_32;

    self->sprites[MG_SP_PLAYER_WALK7].wsg    = &self->wsgs[MG_WSG_PLAYER_WALK7];
    self->sprites[MG_SP_PLAYER_WALK7].origin = &origin_15_15;
    self->sprites[MG_SP_PLAYER_WALK7].hitBox = &box_16_32;

    self->sprites[MG_SP_PLAYER_WALK8].wsg    = &self->wsgs[MG_WSG_PLAYER_WALK8];
    self->sprites[MG_SP_PLAYER_WALK8].origin = &origin_15_15;
    self->sprites[MG_SP_PLAYER_WALK8].hitBox = &box_16_32;

    self->sprites[MG_SP_PLAYER_WALK9].wsg    = &self->wsgs[MG_WSG_PLAYER_WALK9];
    self->sprites[MG_SP_PLAYER_WALK9].origin = &origin_15_15;
    self->sprites[MG_SP_PLAYER_WALK9].hitBox = &box_16_32;

    self->sprites[MG_SP_PLAYER_WALK10].wsg    = &self->wsgs[MG_WSG_PLAYER_WALK10];
    self->sprites[MG_SP_PLAYER_WALK10].origin = &origin_15_15;
    self->sprites[MG_SP_PLAYER_WALK10].hitBox = &box_16_32;

    self->sprites[MG_SP_PLAYER_JUMP].wsg    = &self->wsgs[MG_WSG_PLAYER_JUMP];
    self->sprites[MG_SP_PLAYER_JUMP].origin = &origin_15_15;
    self->sprites[MG_SP_PLAYER_JUMP].hitBox = &box_16_32;

    self->sprites[MG_SP_PLAYER_JUMP1].wsg    = &self->wsgs[MG_WSG_PLAYER_JUMP1];
    self->sprites[MG_SP_PLAYER_JUMP1].origin = &origin_15_15;
    self->sprites[MG_SP_PLAYER_JUMP1].hitBox = &box_16_32;

    self->sprites[MG_SP_PLAYER_JUMP2].wsg    = &self->wsgs[MG_WSG_PLAYER_JUMP2];
    self->sprites[MG_SP_PLAYER_JUMP2].origin = &origin_15_15;
    self->sprites[MG_SP_PLAYER_JUMP2].hitBox = &box_16_32;

    self->sprites[MG_SP_PLAYER_JUMP3].wsg    = &self->wsgs[MG_WSG_PLAYER_JUMP3];
    self->sprites[MG_SP_PLAYER_JUMP3].origin = &origin_15_15;
    self->sprites[MG_SP_PLAYER_JUMP3].hitBox = &box_16_32;

    self->sprites[MG_SP_PLAYER_JUMP4].wsg    = &self->wsgs[MG_WSG_PLAYER_JUMP4];
    self->sprites[MG_SP_PLAYER_JUMP4].origin = &origin_15_15;
    self->sprites[MG_SP_PLAYER_JUMP4].hitBox = &box_16_32;

    self->sprites[MG_SP_PLAYER_SLIDE].wsg    = &self->wsgs[MG_WSG_PLAYER_SLIDE];
    self->sprites[MG_SP_PLAYER_SLIDE].origin = &origin_15_15;
    self->sprites[MG_SP_PLAYER_SLIDE].hitBox = &box_16_32;

    self->sprites[MG_SP_PLAYER_DASH_SLIDE].wsg    = &self->wsgs[MG_WSG_PLAYER_DASH_SLIDE];
    self->sprites[MG_SP_PLAYER_DASH_SLIDE].origin = &origin_15_9;
    self->sprites[MG_SP_PLAYER_DASH_SLIDE].hitBox = &box_32_20;

    self->sprites[MG_SP_PLAYER_HURT].wsg    = &self->wsgs[MG_WSG_PLAYER_HURT];
    self->sprites[MG_SP_PLAYER_HURT].origin = &origin_15_15;
    self->sprites[MG_SP_PLAYER_HURT].hitBox = &box_16_32;

    self->sprites[MG_SP_PLAYER_CLIMB].wsg    = &self->wsgs[MG_WSG_PLAYER_CLIMB];
    self->sprites[MG_SP_PLAYER_CLIMB].origin = &origin_8_8;
    self->sprites[MG_SP_PLAYER_CLIMB].hitBox = &box_16_16;

    self->sprites[MG_SP_PLAYER_WIN].wsg    = &self->wsgs[MG_WSG_PLAYER_WIN];
    self->sprites[MG_SP_PLAYER_WIN].origin = &origin_15_15;
    self->sprites[MG_SP_PLAYER_WIN].hitBox = &box_16_32;

    self->sprites[MG_SP_ENEMY_BASIC].wsg    = &self->wsgs[MG_WSG_ENEMY_BASIC];
    self->sprites[MG_SP_ENEMY_BASIC].origin = &origin_8_8;
    self->sprites[MG_SP_ENEMY_BASIC].hitBox = &box_16_16;

    self->sprites[MG_SP_HITBLOCK_CONTAINER].wsg    = &self->wsgs[MG_WSG_CONTAINER_1];
    self->sprites[MG_SP_HITBLOCK_CONTAINER].origin = &origin_8_8;
    self->sprites[MG_SP_HITBLOCK_CONTAINER].hitBox = &box_16_16;

    self->sprites[MG_SP_HITBLOCK_BRICKS].wsg    = &self->wsgs[MG_WSG_BRICK_BLOCK];
    self->sprites[MG_SP_HITBLOCK_BRICKS].origin = &origin_8_8;
    self->sprites[MG_SP_HITBLOCK_BRICKS].hitBox = &box_16_16;

    self->sprites[MG_SP_DUSTBUNNY_IDLE].wsg    = &self->wsgs[MG_WSG_DUSTBUNNY_IDLE];
    self->sprites[MG_SP_DUSTBUNNY_IDLE].origin = &origin_8_8;
    self->sprites[MG_SP_DUSTBUNNY_IDLE].hitBox = &box_16_16;

    self->sprites[MG_SP_DUSTBUNNY_CHARGE].wsg    = &self->wsgs[MG_WSG_DUSTBUNNY_CHARGE];
    self->sprites[MG_SP_DUSTBUNNY_CHARGE].origin = &origin_8_8;
    self->sprites[MG_SP_DUSTBUNNY_CHARGE].hitBox = &box_16_16;

    self->sprites[MG_SP_DUSTBUNNY_JUMP].wsg    = &self->wsgs[MG_WSG_DUSTBUNNY_JUMP];
    self->sprites[MG_SP_DUSTBUNNY_JUMP].origin = &origin_8_8;
    self->sprites[MG_SP_DUSTBUNNY_JUMP].hitBox = &box_16_16;

    self->sprites[MG_SP_GAMING_1].wsg    = &self->wsgs[MG_WSG_GAMING_1];
    self->sprites[MG_SP_GAMING_1].origin = &origin_8_8;
    self->sprites[MG_SP_GAMING_1].hitBox = &box_16_16;

    self->sprites[MG_SP_GAMING_2].wsg    = &self->wsgs[MG_WSG_GAMING_2];
    self->sprites[MG_SP_GAMING_2].origin = &origin_8_8;
    self->sprites[MG_SP_GAMING_2].hitBox = &box_16_16;

    self->sprites[MG_SP_GAMING_3].wsg    = &self->wsgs[MG_WSG_GAMING_3];
    self->sprites[MG_SP_GAMING_3].origin = &origin_8_8;
    self->sprites[MG_SP_GAMING_3].hitBox = &box_16_16;

    self->sprites[MG_SP_MUSIC_1].wsg    = &self->wsgs[MG_WSG_MUSIC_1];
    self->sprites[MG_SP_MUSIC_1].origin = &origin_8_8;
    self->sprites[MG_SP_MUSIC_1].hitBox = &box_16_16;

    self->sprites[MG_SP_MUSIC_2].wsg    = &self->wsgs[MG_WSG_MUSIC_2];
    self->sprites[MG_SP_MUSIC_2].origin = &origin_8_8;
    self->sprites[MG_SP_MUSIC_2].hitBox = &box_16_16;

    self->sprites[MG_SP_MUSIC_3].wsg    = &self->wsgs[MG_WSG_MUSIC_3];
    self->sprites[MG_SP_MUSIC_3].origin = &origin_8_8;
    self->sprites[MG_SP_MUSIC_3].hitBox = &box_16_16;

    self->sprites[MG_SP_WARP_1].wsg    = &self->wsgs[MG_WSG_WARP_1];
    self->sprites[MG_SP_WARP_1].origin = &origin_8_8;
    self->sprites[MG_SP_WARP_1].hitBox = &box_16_16;

    self->sprites[MG_SP_WARP_2].wsg    = &self->wsgs[MG_WSG_WARP_2];
    self->sprites[MG_SP_WARP_2].origin = &origin_8_8;
    self->sprites[MG_SP_WARP_2].hitBox = &box_16_16;

    self->sprites[MG_SP_WARP_3].wsg    = &self->wsgs[MG_WSG_WARP_3];
    self->sprites[MG_SP_WARP_3].origin = &origin_8_8;
    self->sprites[MG_SP_WARP_3].hitBox = &box_16_16;

    self->sprites[MG_SP_WASP_1].wsg    = &self->wsgs[MG_WSG_WASP_1];
    self->sprites[MG_SP_WASP_1].origin = &origin_8_8;
    self->sprites[MG_SP_WASP_1].hitBox = &box_16_16;

    self->sprites[MG_SP_WASP_2].wsg    = &self->wsgs[MG_WSG_WASP_2];
    self->sprites[MG_SP_WASP_2].origin = &origin_8_8;
    self->sprites[MG_SP_WASP_2].hitBox = &box_16_16;

    self->sprites[MG_SP_WASP_DIVE].wsg    = &self->wsgs[MG_WSG_WASP_DIVE];
    self->sprites[MG_SP_WASP_DIVE].origin = &origin_8_8;
    self->sprites[MG_SP_WASP_DIVE].hitBox = &box_16_16;

    self->sprites[MG_SP_1UP_1].wsg    = &self->wsgs[MG_WSG_1UP_1];
    self->sprites[MG_SP_1UP_1].origin = &origin_8_8;
    self->sprites[MG_SP_1UP_1].hitBox = &box_16_16;

    self->sprites[MG_SP_1UP_2].wsg    = &self->wsgs[MG_WSG_1UP_2];
    self->sprites[MG_SP_1UP_2].origin = &origin_8_8;
    self->sprites[MG_SP_1UP_2].hitBox = &box_16_16;

    self->sprites[MG_SP_1UP_3].wsg    = &self->wsgs[MG_WSG_1UP_3];
    self->sprites[MG_SP_1UP_3].origin = &origin_8_8;
    self->sprites[MG_SP_1UP_3].hitBox = &box_16_16;

    self->sprites[MG_SP_WAVEBALL_1].wsg    = &self->wsgs[MG_WSG_WAVEBALL_1];
    self->sprites[MG_SP_WAVEBALL_1].origin = &origin_8_8;
    self->sprites[MG_SP_WAVEBALL_1].hitBox = &box_16_16;

    self->sprites[MG_SP_WAVEBALL_2].wsg    = &self->wsgs[MG_WSG_WAVEBALL_2];
    self->sprites[MG_SP_WAVEBALL_2].origin = &origin_8_8;
    self->sprites[MG_SP_WAVEBALL_2].hitBox = &box_16_16;

    self->sprites[MG_SP_WAVEBALL_3].wsg    = &self->wsgs[MG_WSG_WAVEBALL_3];
    self->sprites[MG_SP_WAVEBALL_3].origin = &origin_8_8;
    self->sprites[MG_SP_WAVEBALL_3].hitBox = &box_16_16;

    self->sprites[MG_SP_ENEMY_BUSH_L2].wsg    = &self->wsgs[MG_WSG_ENEMY_BUSH_L2];
    self->sprites[MG_SP_ENEMY_BUSH_L2].origin = &origin_8_8;
    self->sprites[MG_SP_ENEMY_BUSH_L2].hitBox = &box_16_16;

    self->sprites[MG_SP_ENEMY_BUSH_L3].wsg    = &self->wsgs[MG_WSG_ENEMY_BUSH_L3];
    self->sprites[MG_SP_ENEMY_BUSH_L3].origin = &origin_8_8;
    self->sprites[MG_SP_ENEMY_BUSH_L3].hitBox = &box_16_16;

    self->sprites[MG_SP_DUSTBUNNY_L2_IDLE].wsg    = &self->wsgs[MG_WSG_DUSTBUNNY_L2_IDLE];
    self->sprites[MG_SP_DUSTBUNNY_L2_IDLE].origin = &origin_8_8;
    self->sprites[MG_SP_DUSTBUNNY_L2_IDLE].hitBox = &box_16_16;

    self->sprites[MG_SP_DUSTBUNNY_L2_CHARGE].wsg    = &self->wsgs[MG_WSG_DUSTBUNNY_L2_CHARGE];
    self->sprites[MG_SP_DUSTBUNNY_L2_CHARGE].origin = &origin_8_8;
    self->sprites[MG_SP_DUSTBUNNY_L2_CHARGE].hitBox = &box_16_16;

    self->sprites[MG_SP_DUSTBUNNY_L2_JUMP].wsg    = &self->wsgs[MG_WSG_DUSTBUNNY_L2_JUMP];
    self->sprites[MG_SP_DUSTBUNNY_L2_JUMP].origin = &origin_8_8;
    self->sprites[MG_SP_DUSTBUNNY_L2_JUMP].hitBox = &box_16_16;

    self->sprites[MG_SP_DUSTBUNNY_L3_IDLE].wsg    = &self->wsgs[MG_WSG_DUSTBUNNY_L3_IDLE];
    self->sprites[MG_SP_DUSTBUNNY_L3_IDLE].origin = &origin_8_8;
    self->sprites[MG_SP_DUSTBUNNY_L3_IDLE].hitBox = &box_16_16;

    self->sprites[MG_SP_DUSTBUNNY_L3_CHARGE].wsg    = &self->wsgs[MG_WSG_DUSTBUNNY_L3_CHARGE];
    self->sprites[MG_SP_DUSTBUNNY_L3_CHARGE].origin = &origin_8_8;
    self->sprites[MG_SP_DUSTBUNNY_L3_CHARGE].hitBox = &box_16_16;

    self->sprites[MG_SP_DUSTBUNNY_L3_JUMP].wsg    = &self->wsgs[MG_WSG_DUSTBUNNY_L3_JUMP];
    self->sprites[MG_SP_DUSTBUNNY_L3_JUMP].origin = &origin_8_8;
    self->sprites[MG_SP_DUSTBUNNY_L3_JUMP].hitBox = &box_16_16;

    self->sprites[MG_SP_WASP_L2_1].wsg    = &self->wsgs[MG_WSG_WASP_L2_1];
    self->sprites[MG_SP_WASP_L2_1].origin = &origin_8_8;
    self->sprites[MG_SP_WASP_L2_1].hitBox = &box_16_16;

    self->sprites[MG_SP_WASP_L2_2].wsg    = &self->wsgs[MG_WSG_WASP_L2_2];
    self->sprites[MG_SP_WASP_L2_2].origin = &origin_8_8;
    self->sprites[MG_SP_WASP_L2_2].hitBox = &box_16_16;

    self->sprites[MG_SP_WASP_L2_DIVE].wsg    = &self->wsgs[MG_WSG_WASP_L2_DIVE];
    self->sprites[MG_SP_WASP_L2_DIVE].origin = &origin_8_8;
    self->sprites[MG_SP_WASP_L2_DIVE].hitBox = &box_16_16;

    self->sprites[MG_SP_WASP_L3_1].wsg    = &self->wsgs[MG_WSG_WASP_L3_1];
    self->sprites[MG_SP_WASP_L3_1].origin = &origin_8_8;
    self->sprites[MG_SP_WASP_L3_1].hitBox = &box_16_16;

    self->sprites[MG_SP_WASP_L3_2].wsg    = &self->wsgs[MG_WSG_WASP_L3_2];
    self->sprites[MG_SP_WASP_L3_2].origin = &origin_8_8;
    self->sprites[MG_SP_WASP_L3_2].hitBox = &box_16_16;

    self->sprites[MG_SP_WASP_L3_DIVE].wsg    = &self->wsgs[MG_WSG_WASP_L3_DIVE];
    self->sprites[MG_SP_WASP_L3_DIVE].origin = &origin_8_8;
    self->sprites[MG_SP_WASP_L3_DIVE].hitBox = &box_16_16;

    self->sprites[MG_SP_CHECKPOINT_INACTIVE].wsg    = &self->wsgs[MG_WSG_CHECKPOINT_INACTIVE];
    self->sprites[MG_SP_CHECKPOINT_INACTIVE].origin = &origin_8_8;
    self->sprites[MG_SP_CHECKPOINT_INACTIVE].hitBox = &box_16_16;

    self->sprites[MG_SP_CHECKPOINT_ACTIVE_1].wsg    = &self->wsgs[MG_WSG_CHECKPOINT_ACTIVE_1];
    self->sprites[MG_SP_CHECKPOINT_ACTIVE_1].origin = &origin_8_8;
    self->sprites[MG_SP_CHECKPOINT_ACTIVE_1].hitBox = &box_16_16;

    self->sprites[MG_SP_CHECKPOINT_ACTIVE_2].wsg    = &self->wsgs[MG_WSG_CHECKPOINT_ACTIVE_2];
    self->sprites[MG_SP_CHECKPOINT_ACTIVE_2].origin = &origin_8_8;
    self->sprites[MG_SP_CHECKPOINT_ACTIVE_2].hitBox = &box_16_16;

    self->sprites[MG_SP_BOUNCE_BLOCK].wsg    = &self->wsgs[MG_WSG_BOUNCE_BLOCK];
    self->sprites[MG_SP_BOUNCE_BLOCK].origin = &origin_8_8;
    self->sprites[MG_SP_BOUNCE_BLOCK].hitBox = &box_16_16;

    self->sprites[MG_SP_INVISIBLE_WARP_WALL].wsg    = &self->wsgs[0];
    self->sprites[MG_SP_INVISIBLE_WARP_WALL].origin = &origin_7_31;
    self->sprites[MG_SP_INVISIBLE_WARP_WALL].hitBox = &box_16_64;

    self->sprites[MG_SP_INVISIBLE_WARP_FLOOR].wsg    = &self->wsgs[0];
    self->sprites[MG_SP_INVISIBLE_WARP_FLOOR].origin = &origin_31_7;
    self->sprites[MG_SP_INVISIBLE_WARP_FLOOR].hitBox = &box_64_16;

    self->sprites[MG_SP_CHARGIN_SCHMUCK_IDLE].wsg    = &self->wsgs[MG_WSG_CHARGIN_SCHMUCK_IDLE];
    self->sprites[MG_SP_CHARGIN_SCHMUCK_IDLE].origin = &origin_15_15;
    self->sprites[MG_SP_CHARGIN_SCHMUCK_IDLE].hitBox = &box_30_30;

    self->sprites[MG_SP_CHARGIN_SCHMUCK_RUN1].wsg    = &self->wsgs[MG_WSG_CHARGIN_SCHMUCK_RUN1];
    self->sprites[MG_SP_CHARGIN_SCHMUCK_RUN1].origin = &origin_11_11;
    self->sprites[MG_SP_CHARGIN_SCHMUCK_RUN1].hitBox = &box_30_30;

    self->sprites[MG_SP_CHARGIN_SCHMUCK_RUN2].wsg    = &self->wsgs[MG_WSG_CHARGIN_SCHMUCK_RUN2];
    self->sprites[MG_SP_CHARGIN_SCHMUCK_RUN2].origin = &origin_11_11;
    self->sprites[MG_SP_CHARGIN_SCHMUCK_RUN2].hitBox = &box_30_30;

    self->sprites[MG_SP_TURRET_HORIZONTAL].wsg    = &self->wsgs[MG_WSG_TURRET_HORIZONTAL];
    self->sprites[MG_SP_TURRET_HORIZONTAL].origin = &origin_11_11;
    self->sprites[MG_SP_TURRET_HORIZONTAL].hitBox = &box_24_24;

    self->sprites[MG_SP_TURRET_45DEG].wsg    = &self->wsgs[MG_WSG_TURRET_45DEG];
    self->sprites[MG_SP_TURRET_45DEG].origin = &origin_11_11;
    self->sprites[MG_SP_TURRET_45DEG].hitBox = &box_24_24;

    self->sprites[MG_SP_TURRET_VERTICAL].wsg    = &self->wsgs[MG_WSG_TURRET_VERTICAL];
    self->sprites[MG_SP_TURRET_VERTICAL].origin = &origin_11_11;
    self->sprites[MG_SP_TURRET_VERTICAL].hitBox = &box_24_24;

    self->sprites[MG_SP_LEMON_SHOT].wsg    = &self->wsgs[MG_WSG_LEMON_SHOT];
    self->sprites[MG_SP_LEMON_SHOT].origin = &origin_8_8;
    self->sprites[MG_SP_LEMON_SHOT].hitBox = &box_16_16;

    self->sprites[MG_SP_SHELDON_SHIELDY_SHIELD].wsg    = &self->wsgs[MG_WSG_SHELDON_SHIELDY_SHIELD];
    self->sprites[MG_SP_SHELDON_SHIELDY_SHIELD].origin = &origin_11_11;
    self->sprites[MG_SP_SHELDON_SHIELDY_SHIELD].hitBox = &box_24_24;

    self->sprites[MG_SP_SHELDON_SHIELDY_WALK1].wsg    = &self->wsgs[MG_WSG_SHELDON_SHIELDY_WALK1];
    self->sprites[MG_SP_SHELDON_SHIELDY_WALK1].origin = &origin_11_11;
    self->sprites[MG_SP_SHELDON_SHIELDY_WALK1].hitBox = &box_24_24;

    self->sprites[MG_SP_SHELDON_SHIELDY_WALK2].wsg    = &self->wsgs[MG_WSG_SHELDON_SHIELDY_WALK2];
    self->sprites[MG_SP_SHELDON_SHIELDY_WALK2].origin = &origin_11_11;
    self->sprites[MG_SP_SHELDON_SHIELDY_WALK2].hitBox = &box_24_24;

    self->sprites[MG_SP_SHELDON_SHIELDY_WALK3].wsg    = &self->wsgs[MG_WSG_SHELDON_SHIELDY_WALK3];
    self->sprites[MG_SP_SHELDON_SHIELDY_WALK3].origin = &origin_11_11;
    self->sprites[MG_SP_SHELDON_SHIELDY_WALK3].hitBox = &box_24_24;

    self->sprites[MG_SP_SPIKY_MCGEE].wsg    = &self->wsgs[MG_WSG_SPIKY_MCGEE];
    self->sprites[MG_SP_SPIKY_MCGEE].origin = &origin_11_11;
    self->sprites[MG_SP_SPIKY_MCGEE].hitBox = &box_24_24;

    self->sprites[MG_SP_AIR_TURRET].wsg    = &self->wsgs[MG_WSG_AIR_TURRET];
    self->sprites[MG_SP_AIR_TURRET].origin = &origin_11_11;
    self->sprites[MG_SP_AIR_TURRET].hitBox = &box_24_24;

    self->sprites[MG_SP_BOUNCE_PAD].wsg    = &self->wsgs[MG_WSG_BOUNCE_PAD];
    self->sprites[MG_SP_BOUNCE_PAD].origin = &origin_15_15;
    self->sprites[MG_SP_BOUNCE_PAD].hitBox = &box_32_32;

    self->sprites[MG_SP_BOUNCE_PAD_DIAGONAL].wsg    = &self->wsgs[MG_WSG_BOUNCE_PAD_DIAGONAL];
    self->sprites[MG_SP_BOUNCE_PAD_DIAGONAL].origin = &origin_15_15;
    self->sprites[MG_SP_BOUNCE_PAD_DIAGONAL].hitBox = &box_32_32;

    self->sprites[MG_SP_MIXTAPE].wsg    = &self->wsgs[MG_WSG_MIXTAPE];
    self->sprites[MG_SP_MIXTAPE].origin = &origin_8_8;
    self->sprites[MG_SP_MIXTAPE].hitBox = &box_16_16;

    self->sprites[MG_SP_BOSS_IDLE].wsg    = &self->wsgs[MG_WSG_BOSS_IDLE];
    self->sprites[MG_SP_BOSS_IDLE].origin = &origin_15_15;
    self->sprites[MG_SP_BOSS_IDLE].hitBox = &box_16_32;

    self->sprites[MG_SP_BOSS_MOVE_1].wsg    = &self->wsgs[MG_WSG_BOSS_MOVE_1];
    self->sprites[MG_SP_BOSS_MOVE_1].origin = &origin_15_15;
    self->sprites[MG_SP_BOSS_MOVE_1].hitBox = &box_16_32;

    self->sprites[MG_SP_BOSS_MOVE_2].wsg    = &self->wsgs[MG_WSG_BOSS_MOVE_2];
    self->sprites[MG_SP_BOSS_MOVE_2].origin = &origin_15_15;
    self->sprites[MG_SP_BOSS_MOVE_2].hitBox = &box_16_32;

    self->sprites[MG_SP_BOSS_MOVE_3].wsg    = &self->wsgs[MG_WSG_BOSS_MOVE_3];
    self->sprites[MG_SP_BOSS_MOVE_3].origin = &origin_15_15;
    self->sprites[MG_SP_BOSS_MOVE_3].hitBox = &box_16_32;

    self->sprites[MG_SP_BOSS_DOOR].wsg    = &self->wsgs[MG_WSG_BOSS_DOOR];
    self->sprites[MG_SP_BOSS_DOOR].origin = &origin_7_31;
    self->sprites[MG_SP_BOSS_DOOR].hitBox = &box_16_64;

    self->sprites[MG_SP_PLAYER_HURT_2].wsg    = &self->wsgs[MG_WSG_PLAYER_HURT_2];
    self->sprites[MG_SP_PLAYER_HURT_2].origin = &origin_15_15;
    self->sprites[MG_SP_PLAYER_HURT_2].hitBox = &box_16_32;

    self->sprites[MG_SP_PLAYER_HURT_3].wsg    = &self->wsgs[MG_WSG_PLAYER_HURT_3];
    self->sprites[MG_SP_PLAYER_HURT_3].origin = &origin_15_15;
    self->sprites[MG_SP_PLAYER_HURT_3].hitBox = &box_16_32;
}

void mg_initializeTiles(mgWsgManager_t* self)
{
    for (uint8_t i = 0; i < MG_TILE_SET_SIZE; i++)
    {
        self->tiles[i] = NULL;
    }
    /*self->tiles[0] = &self->wsgs[MG_WSG_GRASS];
    self->tiles[1] = &self->wsgs[MG_WSG_GROUND];
    self->tiles[2] = &self->wsgs[MG_WSG_BRICK_BLOCK];
    self->tiles[3] = &self->wsgs[MG_WSG_BLOCK];
    self->tiles[4] = &self->wsgs[MG_WSG_METAL_BLOCK];
    self->tiles[5] = &self->wsgs[MG_WSG_METAL_PIPE_H];
    self->tiles[6] = &self->wsgs[MG_WSG_METAL_PIPE_V];
    self->tiles[7] = &self->wsgs[MG_WSG_BOUNCE_BLOCK];
    self->tiles[8] = &self->wsgs[MG_WSG_DIRT_PATH];
    self->tiles[9] = &self->wsgs[MG_WSG_GIRDER];

    self->tiles[10] = &self->wsgs[MG_WSG_GRASS];
    self->tiles[11] = &self->wsgs[MG_WSG_GRASS];
    self->tiles[12] = &self->wsgs[MG_WSG_GRASS];
    self->tiles[13] = &self->wsgs[MG_WSG_GRASS];
    self->tiles[14] = &self->wsgs[MG_WSG_GRASS];
    self->tiles[15] = &self->wsgs[MG_WSG_GRASS];
    self->tiles[16] = &self->wsgs[MG_WSG_GRASS];
    self->tiles[17] = &self->wsgs[MG_WSG_GRASS];
    self->tiles[18] = &self->wsgs[MG_WSG_GRASS];
    self->tiles[19] = &self->wsgs[MG_WSG_GRASS];
    self->tiles[20] = &self->wsgs[MG_WSG_GRASS];
    self->tiles[21] = &self->wsgs[MG_WSG_GRASS];
    self->tiles[22] = &self->wsgs[MG_WSG_GRASS];
    self->tiles[23] = &self->wsgs[MG_WSG_GRASS];
    self->tiles[24] = &self->wsgs[MG_WSG_GRASS];
    self->tiles[25] = &self->wsgs[MG_WSG_GRASS];
    self->tiles[26] = &self->wsgs[MG_WSG_GRASS];

    self->tiles[27] = &self->wsgs[MG_WSG_GOAL_100PTS];
    self->tiles[28] = &self->wsgs[MG_WSG_GOAL_500PTS];
    self->tiles[29] = &self->wsgs[MG_WSG_GOAL_1000PTS];
    self->tiles[30] = &self->wsgs[MG_WSG_GOAL_2000PTS];
    self->tiles[31] = &self->wsgs[MG_WSG_GOAL_5000PTS];
    self->tiles[32] = &self->wsgs[MG_WSG_CONTAINER_1];
    self->tiles[33] = &self->wsgs[MG_WSG_CONTAINER_2];
    self->tiles[34] = &self->wsgs[MG_WSG_CONTAINER_3];
    self->tiles[35] = &self->wsgs[MG_WSG_COIN_1];
    self->tiles[36] = &self->wsgs[MG_WSG_COIN_2];
    self->tiles[37] = &self->wsgs[MG_WSG_COIN_3];
    self->tiles[38] = &self->wsgs[MG_WSG_LADDER];

    self->tiles[39] = &self->wsgs[MG_WSG_GRASS];
    self->tiles[40] = &self->wsgs[MG_WSG_GRASS];
    self->tiles[41] = &self->wsgs[MG_WSG_GRASS];
    self->tiles[42] = &self->wsgs[MG_WSG_GRASS];
    self->tiles[43] = &self->wsgs[MG_WSG_GRASS];
    self->tiles[44] = &self->wsgs[MG_WSG_GRASS];
    self->tiles[45] = &self->wsgs[MG_WSG_GRASS];
    self->tiles[46] = &self->wsgs[MG_WSG_GRASS];
    self->tiles[47] = &self->wsgs[MG_WSG_GRASS];

    self->tiles[48] = &self->wsgs[MG_WSG_BG_GOAL_ZONE];
    self->tiles[49] = &self->wsgs[MG_WSG_BG_ARROW_L];
    self->tiles[50] = &self->wsgs[MG_WSG_BG_ARROW_R];
    self->tiles[51] = &self->wsgs[MG_WSG_BG_ARROW_U];
    self->tiles[52] = &self->wsgs[MG_WSG_BG_ARROW_D];
    self->tiles[53] = &self->wsgs[MG_WSG_BG_ARROW_LU];
    self->tiles[54] = &self->wsgs[MG_WSG_BG_ARROW_RU];
    self->tiles[55] = &self->wsgs[MG_WSG_BG_ARROW_LD];
    self->tiles[56] = &self->wsgs[MG_WSG_BG_ARROW_RD];
    self->tiles[57] = &self->wsgs[MG_WSG_BG_CLOUD_LD];
    self->tiles[58] = &self->wsgs[MG_WSG_BG_CLOUD_M];
    self->tiles[59] = &self->wsgs[MG_WSG_BG_CLOUD_RD];
    self->tiles[60] = &self->wsgs[MG_WSG_BG_CLOUD_LU];
    self->tiles[61] = &self->wsgs[MG_WSG_BG_CLOUD_RU];
    self->tiles[62] = &self->wsgs[MG_WSG_BG_CLOUD_D];
    self->tiles[63] = &self->wsgs[MG_WSG_BG_CLOUD];
    self->tiles[64] = &self->wsgs[MG_WSG_BG_TALL_GRASS];
    self->tiles[65] = &self->wsgs[MG_WSG_BG_MOUNTAIN_L];
    self->tiles[66] = &self->wsgs[MG_WSG_BG_MOUNTAIN_U];
    self->tiles[67] = &self->wsgs[MG_WSG_BG_MOUNTAIN_R];
    self->tiles[68] = &self->wsgs[MG_WSG_BG_MOUNTAIN];
    self->tiles[69] = &self->wsgs[MG_WSG_BG_METAL];
    self->tiles[70] = &self->wsgs[MG_WSG_BG_CHAINS];
    self->tiles[71] = &self->wsgs[MG_WSG_BG_WALL];*/
}

void mg_remapWsgToSprite(mgWsgManager_t* self, uint16_t spriteIndex, uint16_t wsgIndex)
{
    self->sprites[spriteIndex].wsg = &self->wsgs[wsgIndex];
}

void mg_remapWsgToTile(mgWsgManager_t* self, uint16_t tileIndex, uint16_t wsgIndex)
{
    self->tiles[tileIndex] = &self->wsgs[wsgIndex];
}

void mg_remapPlayerCharacter(mgWsgManager_t* self, uint16_t newBaseIndex)
{
    for (uint16_t i = 0; i < (MG_WSG_PLAYER_WIN + 1); i++)
    {
        mg_remapWsgToSprite(self, i, newBaseIndex + i);
    }
}

void mg_animateTiles(mgWsgManager_t* self)
{
    self->globalTileAnimationTimer--;
    if (self->globalTileAnimationTimer < 0)
    {
        // Assumption: all animated tiles have 6 frames of animation
        self->globalTileAnimationFrame = ((self->globalTileAnimationFrame + 1) % 6);

        // mg_remapWsgToTile(self, 9, PA_WSG_BLOCK_BLUE + self->globalTileAnimationFrame);
        // mg_remapWsgToSprite(self, PA_SP_BONUS_BLOCK, PA_WSG_BLOCK_BLUE + self->globalTileAnimationFrame);

        self->globalTileAnimationTimer = 6;
    }
}

void mg_remapBlockTile(mgWsgManager_t* self, uint16_t newBlockWsgIndex)
{
    mg_remapWsgToTile(self, 8, newBlockWsgIndex);
    // mg_remapWsgToSprite(self, PA_SP_BLOCK, newBlockWsgIndex);
}

void mg_remapPlayerShootWsg(mgWsgManager_t* self)
{
    for (uint16_t i = MG_SP_PLAYER_IDLE; i < (MG_SP_PLAYER_JUMP4 + 1); i++)
    {
        mg_remapWsgToSprite(self, i, MG_WSG_PLAYER_SHOOT_IDLE + i);
    }
}

void mg_remapPlayerNotShootWsg(mgWsgManager_t* self)
{
    for (uint16_t i = MG_SP_PLAYER_IDLE; i < (MG_SP_PLAYER_JUMP4 + 1); i++)
    {
        mg_remapWsgToSprite(self, i, MG_WSG_PLAYER_IDLE + i);
    }
}

void mg_loadWsgSet(mgWsgManager_t* self, mgWsgSetIndex_t index)
{
    if (self->wsgSetIndex == index)
    {
        return;
    }

    // TODO: Free ALL tile WSGs here?

    switch (index)
    {
        case MG_WSGSET_LEVEL_SELECT:
            for (uint16_t i = 0; i < MG_LEVEL_SELECT_TILESET_MAP_LENGTH; i++)
            {
                uint16_t wsgIndex = mg_levelSelectTileset[i * MG_TILESET_MAP_ROW_LENGTH + MG_WSG_INDEX_LOOKUP_OFFSET];

                if (self->wsgs[wsgIndex].w && self->wsgs[wsgIndex].h)
                {
                    freeWsg(&self->wsgs[wsgIndex]);
                }

                loadWsg(mg_levelSelectTileset[i * MG_TILESET_MAP_ROW_LENGTH + MG_IMAGE_FILENAME_LOOKUP_OFFSET],
                        &self->wsgs[wsgIndex], false);
                self->tiles[mg_levelSelectTileset[(i * MG_TILESET_MAP_ROW_LENGTH + MG_TILE_INDEX_LOOKUP_OFFSET)] - 32]
                    = &self->wsgs[wsgIndex];
                self->transparencyFunction = &mg_levelSelectTileset_needsTransparency;
            }
            break;
        case MG_WSGSET_KINETIC_DONUT:
        default:
            for (uint16_t i = 0; i < MG_KINETIC_DONUT_TILESET_MAP_LENGTH; i++)
            {
                uint16_t wsgIndex = mg_kineticDonutTileset[i * MG_TILESET_MAP_ROW_LENGTH + MG_WSG_INDEX_LOOKUP_OFFSET];

                if (self->wsgs[wsgIndex].w && self->wsgs[wsgIndex].h)
                {
                    freeWsg(&self->wsgs[wsgIndex]);
                }

                loadWsg(mg_kineticDonutTileset[i * MG_TILESET_MAP_ROW_LENGTH + MG_IMAGE_FILENAME_LOOKUP_OFFSET],
                        &self->wsgs[wsgIndex], false);
                self->tiles[mg_kineticDonutTileset[(i * MG_TILESET_MAP_ROW_LENGTH + MG_TILE_INDEX_LOOKUP_OFFSET)] - 32]
                    = &self->wsgs[wsgIndex];
                self->transparencyFunction = &mg_kineticDonutTileset_needsTransparency;
            }
            break;
    }

    self->wsgSetIndex = index;
}

bool mg_dummyTileset_needsTransparency(uint8_t tile)
{
    return false;
}