#include "ray_cutscene_npc_kiki.h"

/**
 * @brief Add a cutscene style for Kiki
 *
 * @param cutscene The cutscene to add to
 */
void addCutsceneStyleKiki(cutscene_t* cutscene)
{
    addCutsceneStyle(cutscene, c000, TQ_KIKI_PORTRAIT_WSG, CUTSCENE_TEXTBOX_WSG, "Kiki", 1, true, true, true);
}

/**
 * @brief Initialize the cutscene for Kiki
 *
 * Note that Tomi is style 0 and Hyut is style 1
 *
 * @param cutscene The cutscene to add to
 */
void initNpcCutsceneKiki(cutscene_t* cutscene)
{
    addCutsceneStyleKiki(cutscene);
    addCutsceneLine(cutscene, 0, "I'm Tomi", false, 0, NULL);
    addCutsceneLine(cutscene, 1, "Hyaa", false, 0, NULL);
    addCutsceneLine(cutscene, 2, "I'm Kiki", false, 0, NULL);
}
