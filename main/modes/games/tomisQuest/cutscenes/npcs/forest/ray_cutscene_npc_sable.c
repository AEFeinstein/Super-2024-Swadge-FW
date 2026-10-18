#include "ray_cutscene.h"
#include "ray_cutscene_npc_sable.h"

/**
 * @brief Add a cutscene style for Sable
 *
 * @param cutscene The cutscene to add to
 */
void addCutsceneStyleSable(cutscene_t* cutscene)
{
    addCutsceneStyle(cutscene, c000, TQ_SABLE_PORTRAIT_WSG, CUTSCENE_TEXTBOX_WSG, "Sable", 1, true, true, true);
}

/**
 * @brief Initialize the cutscene for Sable
 *
 * Note that Tomi is style 0 and Hyut is style 1
 *
 * @param ray The entire game state
 * @param cutscene The cutscene to add to
 */
void initNpcCutsceneSable(ray_t* ray, cutscene_t* cutscene)
{
#define CS_STYLE_SABLE (CS_STYLE_HYUT + 1)
    addCutsceneStyleSable(cutscene);

    // clang-format off

    if (!ray->p.i.haveEwiOfTime)
    {
        // Before getting Ewi
        addCutsceneLine(cutscene, CS_STYLE_SABLE, "Hey Tomi! Isn't my kid cool? Look, he's even got a sword!", false, 0, NULL);
    }
    // After getting Ewi, before visiting Loot
    else if (!(ray->p.cutsceneViewed & TQS_LOOT_INTRO))
    {
        addCutsceneLine(cutscene, CS_STYLE_SABLE, "...Who's the kid? I didn't see them before... Are they... ok?", false, 0, NULL);
    }
    // After visiting Loot, before getting shield
    else if (!ray->p.i.haveShield)
    {
        addCutsceneLine(cutscene, CS_STYLE_SABLE, "It's fine, we can just set off on our adventure when you're ready, Ben.", false, 0, NULL);
    }
    // After getting shield, before getting turntables
    else if (!ray->p.i.haveTurntables)
    {
        addCutsceneLine(cutscene, CS_STYLE_SABLE, "My kid Ben here is just getting himself psyched for our adventure. Hope he's not afraid to get his feet wet!", false, 0, NULL);
    }
    // After getting turntables, before getting lullaby
    else if (!ray->p.i.haveDoriasLullaby)
    {
        addCutsceneLine(cutscene, CS_STYLE_SABLE, "Hey Tomi...", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "What happened to your kid? Weren't you going to go swimming?", false, TOMI_SUSPICIOUS, NULL);
        addCutsceneLine(cutscene, CS_STYLE_SABLE, "I don't want to talk about it.", false, 0, NULL);
    }
    // After getting lullaby
    else
    {
        addCutsceneLine(cutscene, CS_STYLE_SABLE, "Hi Tomi!", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Hey! Your kid's OK! Ben, right?", false, TOMI_CALM, NULL);
        addCutsceneLine(cutscene, CS_STYLE_SABLE, "Yeah! Our adventure threw us into the deep end, but we made it through..", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Glad to hear! I was worried for a bit there.", false, TOMI_CALM, NULL);
        addCutsceneLine(cutscene, CS_STYLE_SABLE, "I'm a \"sink or swim\" kind of teacher.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Cool. What are you gonna do now?", false, TOMI_SUSPICIOUS, NULL);
        addCutsceneLine(cutscene, CS_STYLE_SABLE, "I dunno. Maybe get some pasta.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Copy that.", false, TOMI_CALM, NULL);
    }

    // clang-format on
}
