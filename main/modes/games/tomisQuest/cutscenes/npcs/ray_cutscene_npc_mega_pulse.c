#include "ray_cutscene_npc_mega_pulse.h"

/**
 * @brief Initialize the cutscene for Mega Pulse
 *
 * Note that Tomi is style 0 and Hyut is style 1
 *
 * @param cutscene
 */
void initNpcCutsceneMegaPulse(cutscene_t* cutscene)
{
    addCutsceneStyle(cutscene, c000, TQ_MEGA_PULSE_PORTRAIT_WSG, CUTSCENE_TEXTBOX_WSG, "Mega Pulse", 1, true, true,
                     true);

    addCutsceneLine(cutscene, 0, "I'm Tomi", false, 0, NULL);
    addCutsceneLine(cutscene, 1, "Hyaa", false, 0, NULL);
    addCutsceneLine(cutscene, 2, "I'm Mega Pulse", false, 0, NULL);
}
