#include "ray_cutscene_npc_hyut.h"

/**
 * @brief Initialize the cutscene for Hyut
 *
 * Note that Tomi is style 0 and Hyut is style 1
 *
 * @param cutscene
 */
void initNpcCutsceneHyut(cutscene_t* cutscene)
{
    addCutsceneStyle(cutscene, c000, TQ_HYUT_PORTRAIT_WSG, CUTSCENE_TEXTBOX_WSG, "Hyut", 1, true, true, true);

    addCutsceneLine(cutscene, 0, "I'm Tomi", false, 0, NULL);
    addCutsceneLine(cutscene, 1, "Hyaa", false, 0, NULL);
    addCutsceneLine(cutscene, 2, "I'm Hyut", false, 0, NULL);
}
