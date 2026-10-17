#include "ray_cutscene_npc_sable.h"

/**
 * @brief Add a cutscene style for Sable
 *
 * @param cutscene The cutscene to add to
 */
void addCutsceneStyleSable(cutscene_t* cutscene)
{
    addCutsceneStyle(cutscene, c000, TQ_SABLE_PORTRAIT_WSG, CUTSCENE_TEXTBOX_WSG, "Sable", 1, true, true, true);
}

/**
 * @brief Initialize the cutscene for Sable
 *
 * Note that Tomi is style 0 and Hyut is style 1
 *
 * @param ray The entire game state
 * @param cutscene The cutscene to add to
 */
void initNpcCutsceneSable(ray_t* ray, cutscene_t* cutscene)
{
    addCutsceneStyleSable(cutscene);
    addCutsceneLine(cutscene, 0, "I'm Tomi", false, 0, NULL);
    addCutsceneLine(cutscene, 1, "Hyaa", false, 0, NULL);
    addCutsceneLine(cutscene, 2, "I'm Sable", false, 0, NULL);
}
