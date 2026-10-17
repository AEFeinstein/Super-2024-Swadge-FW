#include "ray_cutscene_npc_mega_pulse.h"

/**
 * @brief Add a cutscene style for Mega Pulse
 *
 * @param cutscene The cutscene to add to
 */
void addCutsceneStyleMegaPulse(cutscene_t* cutscene)
{
    addCutsceneStyle(cutscene, c000, TQ_MEGA_PULSE_PORTRAIT_WSG, CUTSCENE_TEXTBOX_WSG, "Mega Pulse", 1, true, true,
                     true);
}

/**
 * @brief Initialize the cutscene for Mega Pulse
 *
 * Note that Tomi is style 0 and Hyut is style 1
 *
 * @param ray The entire game state
 * @param cutscene The cutscene to add to
 */
void initNpcCutsceneMegaPulse(ray_t* ray, cutscene_t* cutscene)
{
    addCutsceneStyleMegaPulse(cutscene);
    addCutsceneLine(cutscene, 0, "I'm Tomi", false, 0, NULL);
    addCutsceneLine(cutscene, 1, "Hyaa", false, 0, NULL);
    addCutsceneLine(cutscene, 2, "I'm Mega Pulse", false, 0, NULL);
}
