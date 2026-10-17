#ifndef _RAY_CUTSCENE_H_
#define _RAY_CUTSCENE_H_

#include "mode_ray.h"

#include "ray_cutscene_npc_barry_the_fairy.h"
#include "ray_cutscene_npc_bean.h"
#include "ray_cutscene_npc_bouba.h"
#include "ray_cutscene_npc_cho.h"
#include "ray_cutscene_npc_dacc.h"
#include "ray_cutscene_npc_doria.h"
#include "ray_cutscene_npc_ember_demon.h"
#include "ray_cutscene_npc_fredward.h"
#include "ray_cutscene_npc_great_deejay_tree.h"
#include "ray_cutscene_npc_hyut.h"
#include "ray_cutscene_npc_kaitie.h"
#include "ray_cutscene_npc_kiki.h"
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

typedef bool (*rayCutsceneSceneInitFn_t)(cutscene_t* cutscene, const char* title);

void rayShowCutscene(ray_t* ray, rayMapCellType_t type, char* text);
void rayCutsceneCheckButtons(ray_t* ray);
void rayCutsceneRender(ray_t* ray, uint32_t elapsedUs);
void addCutsceneStyleNarrator(cutscene_t* cutscene);

#endif