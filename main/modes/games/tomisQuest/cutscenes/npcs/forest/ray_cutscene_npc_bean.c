#include "ray_cutscene.h"
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
 * @param ray The entire game state
 * @param cutscene The cutscene to add to
 */
void initNpcCutsceneBean(ray_t* ray, cutscene_t* cutscene)
{
#define CS_STYLE_BEAN (CS_STYLE_HYUT + 1)
    addCutsceneStyleBean(cutscene);

    // clang-format off

    if (!ray->p.i.haveEwiOfTime)
    {
        // Before getting Ewi
        addCutsceneLine(cutscene, CS_STYLE_BEAN, "Oh... sorry, Tomi. Maybe you'll meet someone next time?", false, 0, NULL);
    }
    // After getting Ewi, before visiting Loot
    else if (!(ray->p.cutsceneViewed & TQS_LOOT_INTRO))
    {
        addCutsceneLine(cutscene, CS_STYLE_BEAN, "Nice, Tomi! You found a kid - nice to meet ya! My name's -", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_HYUT, "HYAYAAAA!! HYUT! HA!!", false, HYUT_YELLING, NULL);
        addCutsceneLine(cutscene, CS_STYLE_BEAN, "Uhh... can't you talk, kid?", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_HYUT, "HAA!! HUP HYAA!", false, HYUT_YELLING, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Uhhh, that's just... they're just a little excited to be on an adventure, that's all.", false, TOMI_EMBARASSED, NULL);
        addCutsceneLine(cutscene, CS_STYLE_BEAN, "Uh, okay...", false, 0, NULL);
    }
    // After visiting Loot, before getting shield
    else if (!ray->p.i.haveShield)
    {
        addCutsceneLine(cutscene, CS_STYLE_BEAN, "Hi Tomi! What was your kids' name again? Emily?", false, 0, NULL);
    }
    // After getting shield, before getting turntables
    else if (!ray->p.i.haveTurntables)
    {
        addCutsceneLine(cutscene, CS_STYLE_BEAN, "Hi Tomi! What was your kids' name again? Adam?", false, 0, NULL);
    }
    // After getting turntables, before getting lullaby
    else if (!ray->p.i.haveDoriasLullaby)
    {
        addCutsceneLine(cutscene, CS_STYLE_BEAN, "Hi Tomi! What was your kids' name again? Faith? Deb?", false, 0, NULL);
    }
    // After getting lullaby
    else
    {
        addCutsceneLine(cutscene, CS_STYLE_BEAN, "Hi Tomi! What was your kids' name again? Joe?", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "It's Hyut.", false, TOMI_BORED, NULL);
        addCutsceneLine(cutscene, CS_STYLE_HYUT, "Hyut!", false, HYUT_NEUTRAL, NULL);
        addCutsceneLine(cutscene, CS_STYLE_BEAN, "Hyut? I think I'd remember a name like that.", false, 0, NULL);
    }

    // clang-format on
}
