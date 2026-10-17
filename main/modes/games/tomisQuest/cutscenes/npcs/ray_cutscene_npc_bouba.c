#include "ray_cutscene_npc_bouba.h"

/**
 * @brief Add a cutscene style for Bouba
 *
 * @param cutscene The cutscene to add to
 */
void addCutsceneStyleBouba(cutscene_t* cutscene)
{
    addCutsceneStyle(cutscene, c000, TQ_BOUBA_PORTRAIT_WSG, CUTSCENE_TEXTBOX_WSG, "Bouba", 1, true, true, true);
}

/**
 * @brief Initialize the cutscene for Bouba
 *
 * Note that Tomi is style 0 and Hyut is style 1
 *
 * @param ray The entire game state
 * @param cutscene The cutscene to add to
 */
void initNpcCutsceneBouba(ray_t* ray, cutscene_t* cutscene)
{
    addCutsceneStyleBouba(cutscene);
    addCutsceneLine(cutscene, 0, "I'm Tomi", false, 0, NULL);
    addCutsceneLine(cutscene, 1, "Hyaa", false, 0, NULL);
    addCutsceneLine(cutscene, 2, "I'm Bouba", false, 0, NULL);
}
