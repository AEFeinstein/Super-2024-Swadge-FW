#include "ray_cutscene.h"
#include "ray_cutscene_npc_doria.h"

/**
 * @brief Add a cutscene style for Doria
 *
 * @param cutscene The cutscene to add to
 */
void addCutsceneStyleDoria(cutscene_t* cutscene)
{
    addCutsceneStyle(cutscene, c000, TQ_DORIA_PORTRAIT_WSG, CUTSCENE_TEXTBOX_WSG, "Doria", 1, true, true, true);
}

/**
 * @brief Initialize the cutscene for Doria
 *
 * Note that Tomi is style 0 and Hyut is style 1
 *
 * @param ray The entire game state
 * @param cutscene The cutscene to add to
 */
void initNpcCutsceneDoria(ray_t* ray, cutscene_t* cutscene)
{
    // Doria isn't an interactble NPC, she's only in cutscenes.
    return;
}
