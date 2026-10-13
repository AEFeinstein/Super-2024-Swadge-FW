#include "ray_cutscene_npc_ember_demon.h"

/**
 * @brief Initialize the cutscene for Ember Demon
 *
 * Note that Tomi is style 0 and Hyut is style 1
 *
 * @param cutscene
 */
void initNpcCutsceneEmberDemon(cutscene_t* cutscene)
{
    addCutsceneStyle(cutscene, c000, TQ_EMBER_DEMON_PORTRAIT_WSG, CUTSCENE_TEXTBOX_WSG, "Ember Demon", 1, true, true,
                     true);

    addCutsceneLine(cutscene, 0, "I'm Tomi", false, 0, NULL);
    addCutsceneLine(cutscene, 1, "Hyaa", false, 0, NULL);
    addCutsceneLine(cutscene, 2, "I'm Ember Demon", false, 0, NULL);
}
