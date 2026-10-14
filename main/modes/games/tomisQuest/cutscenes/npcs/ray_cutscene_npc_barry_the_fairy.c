#include "ray_cutscene_npc_barry_the_fairy.h"

/**
 * @brief Add a cutscene style for Barry the Fairy
 *
 * @param cutscene The cutscene to add to
 */
void addCutsceneStyleBarryTheFairy(cutscene_t* cutscene)
{
    addCutsceneStyle(cutscene, c000, TQ_BARRY_THE_FAIRY_PORTRAIT_WSG, CUTSCENE_TEXTBOX_WSG, "Barry the Fairy", 1, true,
                     true, true);
}

/**
 * @brief Initialize the cutscene for Barry the Fairy
 *
 * Note that Tomi is style 0 and Hyut is style 1
 *
 * @param cutscene The cutscene to add to
 */
void initNpcCutsceneBarryTheFairy(cutscene_t* cutscene)
{
    addCutsceneStyleBarryTheFairy(cutscene);
    addCutsceneLine(cutscene, 0, "I'm Tomi", false, 0, NULL);
    addCutsceneLine(cutscene, 1, "Hyaa", false, 0, NULL);
    addCutsceneLine(cutscene, 2, "I'm Barry the Fairy", false, 0, NULL);
}
