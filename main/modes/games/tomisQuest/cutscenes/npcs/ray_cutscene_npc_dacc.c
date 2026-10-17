#include "ray_cutscene_npc_dacc.h"

/**
 * @brief Add a cutscene style for Dacc
 *
 * @param cutscene The cutscene to add to
 */
void addCutsceneStyleDacc(cutscene_t* cutscene)
{
    addCutsceneStyle(cutscene, c000, TQ_DACC_PORTRAIT_WSG, CUTSCENE_TEXTBOX_WSG, "Dacc", 1, true, true, true);
}

/**
 * @brief Initialize the cutscene for Dacc
 *
 * Note that Tomi is style 0 and Hyut is style 1
 *
 * @param ray The entire game state
 * @param cutscene The cutscene to add to
 */
void initNpcCutsceneDacc(ray_t* ray, cutscene_t* cutscene)
{
    addCutsceneStyleDacc(cutscene);
    addCutsceneLine(cutscene, 0, "I'm Tomi", false, 0, NULL);
    addCutsceneLine(cutscene, 1, "Hyaa", false, 0, NULL);
    addCutsceneLine(cutscene, 2, "I'm Dacc", false, 0, NULL);
}
