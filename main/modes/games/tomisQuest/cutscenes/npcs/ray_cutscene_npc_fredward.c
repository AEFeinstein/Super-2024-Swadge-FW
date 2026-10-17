#include "ray_cutscene_npc_fredward.h"

/**
 * @brief Add a cutscene style for Fredward
 *
 * @param cutscene The cutscene to add to
 */
void addCutsceneStyleFredward(cutscene_t* cutscene)
{
    addCutsceneStyle(cutscene, c000, TQ_FREDWARD_PORTRAIT_WSG, CUTSCENE_TEXTBOX_WSG, "Fredward", 1, true, true, true);
}

/**
 * @brief Initialize the cutscene for Fredward
 *
 * Note that Tomi is style 0 and Hyut is style 1
 *
 * @param ray The entire game state
 * @param cutscene The cutscene to add to
 */
void initNpcCutsceneFredward(ray_t* ray, cutscene_t* cutscene)
{
    addCutsceneStyleFredward(cutscene);
    addCutsceneLine(cutscene, 0, "I'm Tomi", false, 0, NULL);
    addCutsceneLine(cutscene, 1, "Hyaa", false, 0, NULL);
    addCutsceneLine(cutscene, 2, "I'm Fredward", false, 0, NULL);
}
