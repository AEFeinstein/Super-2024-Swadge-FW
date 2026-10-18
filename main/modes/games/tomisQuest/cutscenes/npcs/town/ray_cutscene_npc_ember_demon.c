#include "ray_cutscene.h"
#include "ray_cutscene_npc_ember_demon.h"

/**
 * @brief Add a cutscene style for Ember Demon
 *
 * @param cutscene The cutscene to add to
 */
void addCutsceneStyleEmberDemon(cutscene_t* cutscene)
{
    addCutsceneStyle(cutscene, c000, TQ_EMBER_DEMON_PORTRAIT_WSG, CUTSCENE_TEXTBOX_WSG, "Ember Demon", 1, true, true,
                     true);
}

/**
 * @brief Initialize the cutscene for Ember Demon
 *
 * Note that Tomi is style 0 and Hyut is style 1
 *
 * @param ray The entire game state
 * @param cutscene The cutscene to add to
 */
void initNpcCutsceneEmberDemon(ray_t* ray, cutscene_t* cutscene)
{
#define CS_STYLE_EMBER_DEMON (CS_STYLE_HYUT + 1)
    addCutsceneStyleEmberDemon(cutscene);

    if (!ray->p.i.haveEwiOfTime)
    {
        // Before getting Ewi
        addCutsceneLine(cutscene, CS_STYLE_EMBER_DEMON, "TODO", false, 0, NULL);
    }
    // After getting Ewi, before visiting Loot
    else if (!(ray->p.cutsceneViewed & TQS_LOOT_INTRO))
    {
        addCutsceneLine(cutscene, CS_STYLE_EMBER_DEMON, "TODO", false, 0, NULL);
    }
    // After visiting Loot, before getting shield
    else if (!ray->p.i.haveShield)
    {
        addCutsceneLine(cutscene, CS_STYLE_EMBER_DEMON, "TODO", false, 0, NULL);
    }
    // After getting shield, before getting turntables
    else if (!ray->p.i.haveTurntables)
    {
        addCutsceneLine(cutscene, CS_STYLE_EMBER_DEMON, "TODO", false, 0, NULL);
    }
    // After getting turntables, before getting lullaby
    else if (!ray->p.i.haveDoriasLullaby)
    {
        addCutsceneLine(cutscene, CS_STYLE_EMBER_DEMON, "TODO", false, 0, NULL);
    }
    // After getting lullaby
    else
    {
        addCutsceneLine(cutscene, CS_STYLE_EMBER_DEMON, "TODO", false, 0, NULL);
    }
}
