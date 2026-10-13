#include "ray_cutscene_npc_fredward.h"

/**
 * @brief Initialize the cutscene for Fredward
 *
 * Note that Tomi is style 0 and Hyut is style 1
 *
 * @param cutscene
 */
void initNpcCutsceneFredward(cutscene_t* cutscene)
{
    addCutsceneStyle(cutscene, c000, TQ_FREDWARD_PORTRAIT_WSG, CUTSCENE_TEXTBOX_WSG, "Fredward", 1, true, true, true);

    addCutsceneLine(cutscene, 0, "I'm Tomi", false, 0, NULL);
    addCutsceneLine(cutscene, 1, "Hyaa", false, 0, NULL);
    addCutsceneLine(cutscene, 2, "I'm Fredward", false, 0, NULL);
}
