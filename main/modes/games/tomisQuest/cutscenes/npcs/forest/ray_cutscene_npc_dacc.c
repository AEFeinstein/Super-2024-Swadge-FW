#include "ray_cutscene.h"
#include "ray_cutscene_npc_dacc.h"

/**
 * @brief Add a cutscene style for Dacc
 *
 * @param cutscene The cutscene to add to
 */
void addCutsceneStyleDacc(cutscene_t* cutscene)
{
    addCutsceneStyle(cutscene, c000, TQ_DACC_PORTRAIT_WSG, CUTSCENE_TEXTBOX_WSG, "Dacc", 1, true, true, true);
}

/**
 * @brief Initialize the cutscene for Dacc
 *
 * Note that Tomi is style 0 and Hyut is style 1
 *
 * @param ray The entire game state
 * @param cutscene The cutscene to add to
 */
void initNpcCutsceneDacc(ray_t* ray, cutscene_t* cutscene)
{
#define CS_STYLE_DACC (CS_STYLE_HYUT + 1)
    addCutsceneStyleDacc(cutscene);

    // clang-format off

    if (!ray->p.i.haveEwiOfTime)
    {
        // Before getting Ewi
        addCutsceneLine(cutscene, CS_STYLE_DACC, "Oh, hey, Tomi. Where's your kid?", false, 0, NULL);
    }
    // After getting Ewi, before visiting Loot
    else if (!(ray->p.cutsceneViewed & TQS_LOOT_INTRO))
    {
        addCutsceneLine(cutscene, CS_STYLE_DACC, "Woah, Tomi! How did you find a kid so fast? Did you tell the Great DeJay Tree?", false, 0, NULL);
    }
    // After visiting Loot, before getting shield
    else if (!ray->p.i.haveShield)
    {
        addCutsceneLine(cutscene, CS_STYLE_DACC, "Oh man, Tomi is back already... what a loser...", false, 0, NULL);
    }
    // After getting shield, before getting turntables
    else if (!ray->p.i.haveTurntables)
    {
        addCutsceneLine(cutscene, CS_STYLE_DACC, "Hmph, she doesn't look any stronger to me...", false, 0, NULL);
    }
    // After getting turntables, before getting lullaby
    else if (!ray->p.i.haveDoriasLullaby)
    {
        addCutsceneLine(cutscene, CS_STYLE_DACC, "Heh, Tomi's back once again. She's clearly lost.", false, 0, NULL);
    }
    // After getting lullaby
    else
    {
        addCutsceneLine(cutscene, CS_STYLE_DACC, "Tomi, you're back again? I figured you'd be a big star in Gaylordia by now. Forget all about where you came from.", false, 0, NULL);
    }

    // clang-format on
}
