#include "ray_cutscene_npc_loot.h"

/**
 * @brief Initialize the cutscene for Loot
 *
 * Note that Tomi is style 0 and Hyut is style 1
 *
 * @param cutscene
 */
void initNpcCutsceneLoot(cutscene_t* cutscene)
{
    addCutsceneStyle(cutscene, c000, TQ_LOOT_PORTRAIT_WSG, CUTSCENE_TEXTBOX_WSG, "Loot", 1, true, true, true);

    addCutsceneLine(cutscene, 0, "I'm Tomi", false, 0, NULL);
    addCutsceneLine(cutscene, 1, "Hyaa", false, 0, NULL);
    addCutsceneLine(cutscene, 2, "I'm Loot", false, 0, NULL);
}
