#include "ray_cutscene_npc_doria.h"

/**
 * @brief Initialize the cutscene for Doria
 *
 * Note that Tomi is style 0 and Hyut is style 1
 *
 * @param cutscene
 */
void initNpcCutsceneDoria(cutscene_t* cutscene)
{
    addCutsceneStyle(cutscene, c000, TQ_DORIA_PORTRAIT_WSG, CUTSCENE_TEXTBOX_WSG, "Doria", 1, true, true, true);

    addCutsceneLine(cutscene, 0, "I'm Tomi", false, 0, NULL);
    addCutsceneLine(cutscene, 1, "Hyaa", false, 0, NULL);
    addCutsceneLine(cutscene, 2, "I'm Doria", false, 0, NULL);
}
