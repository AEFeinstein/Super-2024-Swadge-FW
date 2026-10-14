#include "ray_cutscene_npc_great_deejay_tree.h"

/**
 * @brief Add a cutscene style for Great DeeJay Tree
 *
 * @param cutscene The cutscene to add to
 */
void addCutsceneStyleGreatDeejayTree(cutscene_t* cutscene)
{
    addCutsceneStyle(cutscene, c000, TQ_GREAT_DEEJAY_TREE_PORTRAIT_WSG, CUTSCENE_TEXTBOX_WSG, "Great DeeJay Tree", 1,
                     true, true, true);
}

/**
 * @brief Initialize the cutscene for Great DeeJay Tree
 *
 * Note that Tomi is style 0 and Hyut is style 1
 *
 * @param cutscene The cutscene to add to
 */
void initNpcCutsceneGreatDeejayTree(cutscene_t* cutscene)
{
    addCutsceneStyleGreatDeejayTree(cutscene);
    addCutsceneLine(cutscene, 0, "I'm Tomi", false, 0, NULL);
    addCutsceneLine(cutscene, 1, "Hyaa", false, 0, NULL);
    addCutsceneLine(cutscene, 2, "I'm Great DeeJay Tree", false, 0, NULL);
}
