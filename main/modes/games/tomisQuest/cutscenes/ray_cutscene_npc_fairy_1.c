#include "ray_cutscene_npc_fairy_1.h"

/**
 * @brief Initialize the cutscene for Fairy 1
 *
 * Note that Tomi is style 0 and Hyut is style 1
 *
 * @param cutscene
 */
void initNpcCutsceneFairy1(cutscene_t* cutscene)
{
    addCutsceneStyle(cutscene, c000, NAVI_PORTRAIT_WSG, CUTSCENE_TEXTBOX_WSG, "Fairy 1", 1, true, true, true);

    addCutsceneLine(cutscene, 0, "I'm Tomi", false, 0, NULL);
    addCutsceneLine(cutscene, 1, "Hyaa", false, 0, NULL);
    addCutsceneLine(cutscene, 2, "I'm Fairy 1", false, 0, NULL);
}
