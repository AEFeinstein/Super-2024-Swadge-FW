#include "ray_cutscene.h"
#include "ray_cutscene_npc_barry_the_fairy.h"

/**
 * @brief Add a cutscene style for Barry the Fairy
 *
 * @param cutscene The cutscene to add to
 */
void addCutsceneStyleBarryTheFairy(cutscene_t* cutscene)
{
    addCutsceneStyle(cutscene, c000, TQ_BARRY_THE_FAIRY_PORTRAIT_WSG, CUTSCENE_TEXTBOX_WSG, "Barry the Fairy", 1, true,
                     true, true);
}

/**
 * @brief Initialize the cutscene for Barry the Fairy
 *
 * Note that Tomi is style 0 and Hyut is style 1
 *
 * @param ray The entire game state
 * @param cutscene The cutscene to add to
 */
void initNpcCutsceneBarryTheFairy(ray_t* ray, cutscene_t* cutscene)
{
#define CS_STYLE_BARRY (CS_STYLE_HYUT + 1)
    addCutsceneStyleBarryTheFairy(cutscene);

    // clang-format off

    if (!ray->p.i.haveEwiOfTime)
    {
        // Before getting Ewi
        addCutsceneLine(cutscene, CS_STYLE_BARRY, "Eyy!! Whaddaya want?", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Uh, I just was hoping to get through?", false, TOMI_SUSPICIOUS, NULL);
        addCutsceneLine(cutscene, CS_STYLE_BARRY, "Ha! Just you? By ya self? FUGGEDABOUTIT.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Hey!", false, TOMI_ANGRY, NULL);
        addCutsceneLine(cutscene, CS_STYLE_BARRY, "Look, Tomi. You 'member da rules. Without a kiddo, us fairies ain't never leavin' the Forest. Sorry, bub.", false, 0, NULL);
    }
    // After getting Ewi, before visiting Loot
    else if (!(ray->p.cutsceneViewed & TQS_LOOT_INTRO))
    {
        addCutsceneLine(cutscene, CS_STYLE_BARRY, "Hey, who's the small fry, small fry?", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "This is Hyut.", false, TOMI_COCKY, NULL);
        addCutsceneLine(cutscene, CS_STYLE_HYUT, "Hyut!", false, HYUT_NEUTRAL, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "...Yes. Well. We're off to have a grand adventure!", false, TOMI_EMBARASSED, NULL);
        addCutsceneLine(cutscene, CS_STYLE_HYUT, "HYAA! Hup!", false, HYUT_YELLING, NULL);
        addCutsceneLine(cutscene, CS_STYLE_BARRY, "They, uh, ok in the head?", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Yeah, they're fine. It's like a classic \"brains plus brawn\" dynamic. We're good.", false, TOMI_BORED, NULL);
        addCutsceneLine(cutscene, CS_STYLE_BARRY, "You off to go conquer the Great Fairy Trials, then?", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Yep! The next time you talk to me, you'll be addressing the newest Great Fairy.", false, TOMI_COCKY, NULL);
        addCutsceneLine(cutscene, CS_STYLE_BARRY, "Heh heh - sure, bub. I'll be here waiting as soon as you come crawling back home.", false, 0, NULL);
    }
    // After visiting Loot, before getting shield
    else
    {
        addCutsceneLine(cutscene, CS_STYLE_BARRY, "Oh hey, look - it's the newest Great Fairy! What, you forget something at home? Heh heh heh!", false, 0, NULL);
    }

    // clang-format on
}
