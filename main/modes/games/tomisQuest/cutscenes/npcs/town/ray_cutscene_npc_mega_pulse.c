#include "ray_cutscene.h"
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
 * @brief Add a cutscene style for Sawtooth
 *
 * @param cutscene The cutscene to add to
 */
void addCutsceneStyleSawtooth(cutscene_t* cutscene)
{
    addCutsceneStyle(cutscene, c000, TQ_SAWTOOTH_PORTRAIT_WSG, CUTSCENE_TEXTBOX_WSG, "Sawtooth", 1, true, true, true);
}

/**
 * @brief Initialize the cutscene for MEGA_PULSE
 *
 * Note that Tomi is style 0 and Hyut is style 1
 *
 * @param ray The entire game state
 * @param cutscene The cutscene to add to
 */
void initNpcCutsceneMegaPulse(ray_t* ray, cutscene_t* cutscene)
{
#define CS_STYLE_MEGA_PULSE (CS_STYLE_HYUT + 1)
    addCutsceneStyleMegaPulse(cutscene);

#define CS_STYLE_SAWTOOTH (CS_STYLE_MEGA_PULSE + 1)
    addCutsceneStyleSawtooth(cutscene);

    // clang-format off

    // After visiting Loot, before getting shield
    if (!ray->p.i.haveShield)
    {
        addCutsceneLine(cutscene, CS_STYLE_MEGA_PULSE, "Sawtooth? Come in, Sawtooth? I think the teleporter malfunctioned again..", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "you don't look like you're from around here... or like, this time period...", false, TOMI_SUSPICIOUS, NULL);
        addCutsceneLine(cutscene, CS_STYLE_MEGA_PULSE, "uhh, forsooth! Carry on!", false, 0, NULL);
    }
    // After getting shield, before getting turntables
    else if (!ray->p.i.haveTurntables)
    {
        addCutsceneLine(cutscene, CS_STYLE_SAWTOOTH, "Hey there. Have you seen my partner, Pulse? They were trying to fix a broken teleporter...", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Yeah, we saw them a little while back.", false, TOMI_SUSPICIOUS, NULL);
        addCutsceneLine(cutscene, CS_STYLE_SAWTOOTH, "Well that's a good sign. At least I'm in the right world. I spent waaaay too much time in this kart-racing world. Got a cool trophy though!", false, 0, NULL);
    }
    // After getting turntables, before getting lullaby
    else if (!ray->p.i.haveDoriasLullaby)
    {
        addCutsceneLine(cutscene, CS_STYLE_MEGA_PULSE, "You again! Did you see Sawtooth? Man, I just missed her again, didn't I...", false, 0, NULL);
    }
    // After getting lullaby
    else
    {
        addCutsceneLine(cutscene, CS_STYLE_MEGA_PULSE, "Look Tomi, Sawtooth and I finally found each other again!", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "I'm glad to hear it!", false, TOMI_CALM, NULL);
        addCutsceneLine(cutscene, CS_STYLE_SAWTOOTH, "Yeah, this \"Land of MAGrule\" is pretty neat. I'd come back here. Maybe in a few years or so after we revisit some other themes.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Themes?", false, TOMI_SUSPICIOUS, NULL);
        addCutsceneLine(cutscene, CS_STYLE_SAWTOOTH, "Oh, I mean, like... actually, never mind. It'd be too complicated to explain.", false, 0, NULL);
    }
    // clang-format on
}
