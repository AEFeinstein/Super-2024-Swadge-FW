#ifndef _RAY_CUTSCENE_H_
#define _RAY_CUTSCENE_H_

#include "mode_ray.h"

typedef enum
{
    // Misc
    TQ_NARRATOR,
    // Protagonists
    TQ_TOMI,
    TQ_HYUT,
    // Forest-folk
    TQ_FAIRY_1,
    TQ_FAIRY_2,
    TQ_FAIRY_3,
    TQ_FAIRY_4,
    TQ_FAIRY_5,
    TQ_FAIRY_6,
    TQ_FAIRY_BARRY,
    TQ_DJ_TREE,
    // Field-folk
    TQ_LOOT,
    TQ_DORIA,
    // Towns-folk
    TQ_KING_DONUT,
    TQ_MEGA_PULSE,
    TQ_CHO,
    TQ_GARBOTNIK,
    TQ_EMBER,
    TQ_MAYOR,
    TQ_FREDWARD,
    // Final Area
    TQ_WADDLEDORF,
    TQ_GREAT_FAIRIES,
    TQ_GREAT_FAIRY,
} tqCharacter_t;

void rayShowCutscene(ray_t* ray, rayMapCellType_t type, int32_t sceneID);
void rayCutsceneCheckButtons(ray_t* ray);
void rayCutsceneRender(ray_t* ray, uint32_t elapsedUs);

#endif