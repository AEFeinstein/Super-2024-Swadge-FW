#ifndef _RAY_CUTSCENE_H_
#define _RAY_CUTSCENE_H_

#include "mode_ray.h"

#include "ray_cutscene_npc_barry_the_fairy.h"
#include "ray_cutscene_npc_bean.h"
#include "ray_cutscene_npc_cho.h"
#include "ray_cutscene_npc_dacc.h"
#include "ray_cutscene_npc_doria.h"
#include "ray_cutscene_npc_ember_demon.h"
#include "ray_cutscene_npc_fredward.h"
#include "ray_cutscene_npc_great_dejay_tree.h"
#include "ray_cutscene_npc_kaitie.h"
#include "ray_cutscene_npc_kiki_bouba.h"
#include "ray_cutscene_npc_king_donut.h"
#include "ray_cutscene_npc_loot.h"
#include "ray_cutscene_npc_mega_pulse.h"
#include "ray_cutscene_npc_sable.h"
#include "ray_cutscene_npc_the_mayor.h"
#include "ray_cutscene_npc_trash_man.h"

#include "ray_cutscene_scene_00_intro.h"
#include "ray_cutscene_scene_01_pairing.h"
#include "ray_cutscene_scene_02_hyut_intro.h"
#include "ray_cutscene_scene_03_hyut_pairing.h"
#include "ray_cutscene_scene_04_loot_intro.h"
#include "ray_cutscene_scene_05_doria_intro.h"
#include "ray_cutscene_scene_06_dungeon_1_complete.h"
#include "ray_cutscene_scene_07_dungeon_2_complete.h"
#include "ray_cutscene_scene_08_dungeon_3_complete.h"
#include "ray_cutscene_scene_09_dungeon_4_complete.h"
#include "ray_cutscene_scene_10_singing.h"
#include "ray_cutscene_scene_11_final_fight.h"
#include "ray_cutscene_scene_12_fight_complete.h"
#include "ray_cutscene_scene_13_great_fairy.h"
#include "ray_cutscene_scene_14_dream_1.h"
#include "ray_cutscene_scene_15_dream_2.h"
#include "ray_cutscene_scene_16_dream_3.h"
#include "ray_cutscene_scene_17_final_warp.h"
#include "ray_cutscene_scene_18_statue.h"

#define CS_STYLE_TOMI 0
#define CS_STYLE_HYUT 1

typedef enum
{
    TOMI_COCKY,
    TOMI_EMBARASSED,
    TOMI_FIREDUP,
    TOMI_PLEADING,
    TOMI_ANGRY,
    TOMI_BORED,
    TOMI_SUSPICIOUS,
    TOMI_CALM,
    TOMI_SAD,
    TOMI_SHOCKED,
    TOMI_PLEADING_TEARS,
    NUM_TOMI_EMOTIONS,
} tomiEmotion_t;

typedef enum
{
    HYUT_NEUTRAL,
    HYUT_YELLING,
    NUM_HYUT_EMOTIONS,
} hyutEmotion_t;

typedef enum
{
    TQS_INTRO              = (1 << 0),
    TQS_PAIRING            = (1 << 1),
    TQS_HYUT_INTRO         = (1 << 2),
    TQS_HYUT_PAIRING       = (1 << 3),
    TQS_LOOT_INTRO         = (1 << 4),
    TQS_DORIA_INTRO        = (1 << 5),
    TQS_DUNGEON_1_COMPLETE = (1 << 6),
    TQS_DUNGEON_2_COMPLETE = (1 << 7),
    TQS_DUNGEON_3_COMPLETE = (1 << 8),
    TQS_DUNGEON_4_COMPLETE = (1 << 9),
    TQS_SINGING            = (1 << 10),
    TQS_FINAL_FIGHT        = (1 << 11),
    TQS_FIGHT_COMPLETE     = (1 << 12),
    TQS_GREAT_FAIRY        = (1 << 13),
    TQS_DREAM_1            = (1 << 14),
    TQS_DREAM_2            = (1 << 15),
    TQS_DREAM_3            = (1 << 16),
    TQS_DJ_TREE_1          = (1 << 17),
    TQS_DJ_TREE_2          = (1 << 18),
    TQS_DJ_TREE_3          = (1 << 19),
    TQS_DJ_TREE_4          = (1 << 20),
    TQS_DJ_TREE_5          = (1 << 21),
    TQS_FREDWARD           = (1 << 22),
    TQS_LOOT_2             = (1 << 24),
    TQS_FINAL_WARP         = (1 << 25),
    TQS_STATUE             = (1 << 26),
} tomisQuestSceneId_t;

typedef bool (*rayCutsceneSceneInitFn_t)(ray_t* ray, cutscene_t* cutscene, const char* title);

void rayShowCutscene(ray_t* ray, rayMapCellType_t type, char* text);
void rayCutsceneCheckButtons(ray_t* ray);
void rayCutsceneRender(ray_t* ray, uint32_t elapsedUs);
void addCutsceneStyleNarrator(cutscene_t* cutscene);
void addCutsceneStyleUnknown(cutscene_t* cutscene);
void addCutsceneStyleHankW(cutscene_t* cutscene);
void addCutsceneStyleGreatFairy(cutscene_t* cutscene);
void addCutsceneStyleGreatFairies(cutscene_t* cutscene);
void deleteDoria(void);

#endif