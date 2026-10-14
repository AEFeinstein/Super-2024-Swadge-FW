#include "ray_cutscene_npc_fairy_3.h"

/**
 * @brief Add a cutscene style for Fairy 3
 *
 * @param cutscene The cutscene to add to
 */
void addCutsceneStyleFairy3(cutscene_t* cutscene)
{
    addCutsceneStyle(cutscene, c000, TQ_FAIRY_3_PORTRAIT_WSG, CUTSCENE_TEXTBOX_WSG, "Fairy 3", 1, true, true, true);
}

/**
 * @brief Initialize the cutscene for Fairy 3
 *
 * Note that Tomi is style 0 and Hyut is style 1
 *
 * @param cutscene The cutscene to add to
 */
void initNpcCutsceneFairy3(cutscene_t* cutscene)
{
    addCutsceneStyleFairy3(cutscene);
    addCutsceneLine(cutscene, 0, "I'm Tomi", false, 0, NULL);
    addCutsceneLine(cutscene, 1, "Hyaa", false, 0, NULL);
    addCutsceneLine(cutscene, 2, "I'm Fairy 3", false, 0, NULL);
}
