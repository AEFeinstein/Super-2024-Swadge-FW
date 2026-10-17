#include "ray_cutscene_npc_ember_demon.h"

/**
 * @brief Add a cutscene style for Ember Demon
 *
 * @param cutscene The cutscene to add to
 */
void addCutsceneStyleEmberDemon(cutscene_t* cutscene)
{
    addCutsceneStyle(cutscene, c000, TQ_EMBER_DEMON_PORTRAIT_WSG, CUTSCENE_TEXTBOX_WSG, "Ember Demon", 1, true, true,
                     true);
}

/**
 * @brief Initialize the cutscene for Ember Demon
 *
 * Note that Tomi is style 0 and Hyut is style 1
 *
 * @param ray The entire game state
 * @param cutscene The cutscene to add to
 */
void initNpcCutsceneEmberDemon(ray_t* ray, cutscene_t* cutscene)
{
    addCutsceneStyleEmberDemon(cutscene);
    addCutsceneLine(cutscene, 0, "I'm Tomi", false, 0, NULL);
    addCutsceneLine(cutscene, 1, "Hyaa", false, 0, NULL);
    addCutsceneLine(cutscene, 2, "I'm Ember Demon", false, 0, NULL);
}
