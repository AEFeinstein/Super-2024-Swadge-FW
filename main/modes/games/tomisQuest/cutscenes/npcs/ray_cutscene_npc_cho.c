#include "ray_cutscene_npc_cho.h"

/**
 * @brief Add a cutscene style for Cho
 *
 * @param cutscene The cutscene to add to
 */
void addCutsceneStyleCho(cutscene_t* cutscene)
{
    addCutsceneStyle(cutscene, c000, TQ_CHO_PORTRAIT_WSG, CUTSCENE_TEXTBOX_WSG, "Cho", 1, true, true, true);
}

/**
 * @brief Initialize the cutscene for Cho
 *
 * Note that Tomi is style 0 and Hyut is style 1
 *
 * @param cutscene The cutscene to add to
 */
void initNpcCutsceneCho(cutscene_t* cutscene)
{
    addCutsceneStyleCho(cutscene);
    addCutsceneLine(cutscene, 0, "I'm Tomi", false, 0, NULL);
    addCutsceneLine(cutscene, 1, "Hyaa", false, 0, NULL);
    addCutsceneLine(cutscene, 2, "I'm Cho", false, 0, NULL);
}
