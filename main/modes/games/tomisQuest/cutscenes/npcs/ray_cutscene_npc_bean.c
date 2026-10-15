#include "ray_cutscene_npc_bean.h"

/**
 * @brief Add a cutscene style for Bean
 *
 * @param cutscene The cutscene to add to
 */
void addCutsceneStyleBean(cutscene_t* cutscene)
{
    addCutsceneStyle(cutscene, c000, TQ_BEAN_PORTRAIT_WSG, CUTSCENE_TEXTBOX_WSG, "Bean", 1, true, true, true);
}

/**
 * @brief Initialize the cutscene for Bean
 *
 * Note that Tomi is style 0 and Hyut is style 1
 *
 * @param cutscene The cutscene to add to
 */
void initNpcCutsceneBean(cutscene_t* cutscene)
{
    addCutsceneStyleBean(cutscene);
    addCutsceneLine(cutscene, 0, "I'm Tomi", false, 0, NULL);
    addCutsceneLine(cutscene, 1, "Hyaa", false, 0, NULL);
    addCutsceneLine(cutscene, 2, "I'm Bean", false, 0, NULL);
}
