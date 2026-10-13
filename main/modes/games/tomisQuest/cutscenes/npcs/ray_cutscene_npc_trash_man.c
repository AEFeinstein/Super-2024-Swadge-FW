#include "ray_cutscene_npc_trash_man.h"

/**
 * @brief Initialize the cutscene for Trash Man
 *
 * Note that Tomi is style 0 and Hyut is style 1
 *
 * @param cutscene
 */
void initNpcCutsceneTrashMan(cutscene_t* cutscene)
{
    addCutsceneStyle(cutscene, c000, TQ_TRASH_MAN_PORTRAIT_WSG, CUTSCENE_TEXTBOX_WSG, "Trash Man", 1, true, true, true);

    addCutsceneLine(cutscene, 0, "I'm Tomi", false, 0, NULL);
    addCutsceneLine(cutscene, 1, "Hyaa", false, 0, NULL);
    addCutsceneLine(cutscene, 2, "I'm Trash Man", false, 0, NULL);
}
