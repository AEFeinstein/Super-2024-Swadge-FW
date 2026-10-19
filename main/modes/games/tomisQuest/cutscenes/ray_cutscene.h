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
} tomiEmotion_t;

typedef enum
{
    HYUT_NEUTRAL,
    HYUT_YELLING,
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
} tomisQuestSceneId_t;

typedef bool (*rayCutsceneSceneInitFn_t)(ray_t* ray, cutscene_t* cutscene, const char* title);

void rayShowCutscene(ray_t* ray, rayMapCellType_t type, char* text);
void rayCutsceneCheckButtons(ray_t* ray);
void rayCutsceneRender(ray_t* ray, uint32_t elapsedUs);
void addCutsceneStyleNarrator(cutscene_t* cutscene);

#endif