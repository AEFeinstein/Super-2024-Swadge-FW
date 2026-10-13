#include "ray_cutscene_npc_the_mayor.h"

/**
 * @brief Initialize the cutscene for The Mayor
 *
 * Note that Tomi is style 0 and Hyut is style 1
 *
 * @param cutscene
 */
void initNpcCutsceneTheMayor(cutscene_t* cutscene)
{
    addCutsceneStyle(cutscene, c000, TQ_THE_MAYOR_PORTRAIT_WSG, CUTSCENE_TEXTBOX_WSG, "The Mayor", 1, true, true, true);

    addCutsceneLine(cutscene, 0, "I'm Tomi", false, 0, NULL);
    addCutsceneLine(cutscene, 1, "Hyaa", false, 0, NULL);
    addCutsceneLine(cutscene, 2, "I'm The Mayor", false, 0, NULL);
}
