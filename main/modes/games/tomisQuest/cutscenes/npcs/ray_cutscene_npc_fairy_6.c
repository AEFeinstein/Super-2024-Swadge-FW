#include "ray_cutscene_npc_fairy_6.h"

/**
 * @brief Initialize the cutscene for Fairy 6
 *
 * Note that Tomi is style 0 and Hyut is style 1
 *
 * @param cutscene
 */
void initNpcCutsceneFairy6(cutscene_t* cutscene)
{
    addCutsceneStyle(cutscene, c000, TQ_FAIRY_6_PORTRAIT_WSG, CUTSCENE_TEXTBOX_WSG, "Fairy 6", 1, true, true, true);

    addCutsceneLine(cutscene, 0, "I'm Tomi", false, 0, NULL);
    addCutsceneLine(cutscene, 1, "Hyaa", false, 0, NULL);
    addCutsceneLine(cutscene, 2, "I'm Fairy 6", false, 0, NULL);
}
