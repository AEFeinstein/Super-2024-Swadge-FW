#include "ray_cutscene_npc_king_donut.h"

/**
 * @brief Initialize the cutscene for King Donut
 *
 * Note that Tomi is style 0 and Hyut is style 1
 *
 * @param cutscene
 */
void initNpcCutsceneKingDonut(cutscene_t* cutscene)
{
    addCutsceneStyle(cutscene, c000, TQ_KING_DONUT_PORTRAIT_WSG, CUTSCENE_TEXTBOX_WSG, "King Donut", 1, true, true,
                     true);

    addCutsceneLine(cutscene, 0, "I'm Tomi", false, 0, NULL);
    addCutsceneLine(cutscene, 1, "Hyaa", false, 0, NULL);
    addCutsceneLine(cutscene, 2, "I'm King Donut", false, 0, NULL);
}
