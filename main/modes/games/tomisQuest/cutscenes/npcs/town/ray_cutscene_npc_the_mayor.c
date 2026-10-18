#include "ray_cutscene.h"
#include "ray_cutscene_npc_the_mayor.h"

/**
 * @brief Add a cutscene style for The Mayor
 *
 * @param cutscene The cutscene to add to
 */
void addCutsceneStyleTheMayor(cutscene_t* cutscene)
{
    addCutsceneStyle(cutscene, c000, TQ_THE_MAYOR_PORTRAIT_WSG, CUTSCENE_TEXTBOX_WSG, "The Mayor", 1, true, true, true);
}

/**
 * @brief Initialize the cutscene for The Mayor
 *
 * Note that Tomi is style 0 and Hyut is style 1
 *
 * @param ray The entire game state
 * @param cutscene The cutscene to add to
 */
void initNpcCutsceneTheMayor(ray_t* ray, cutscene_t* cutscene)
{
#define CS_STYLE_THE_MAYOR (CS_STYLE_HYUT + 1)
    addCutsceneStyleTheMayor(cutscene);

    if (!ray->p.i.haveEwiOfTime)
    {
        // Before getting Ewi
        addCutsceneLine(cutscene, CS_STYLE_THE_MAYOR, "TODO", false, 0, NULL);
    }
    // After getting Ewi, before visiting Loot
    else if (!(ray->p.cutsceneViewed & TQS_LOOT_INTRO))
    {
        addCutsceneLine(cutscene, CS_STYLE_THE_MAYOR, "TODO", false, 0, NULL);
    }
    // After visiting Loot, before getting shield
    else if (!ray->p.i.haveShield)
    {
        addCutsceneLine(cutscene, CS_STYLE_THE_MAYOR, "TODO", false, 0, NULL);
    }
    // After getting shield, before getting turntables
    else if (!ray->p.i.haveTurntables)
    {
        addCutsceneLine(cutscene, CS_STYLE_THE_MAYOR, "TODO", false, 0, NULL);
    }
    // After getting turntables, before getting lullaby
    else if (!ray->p.i.haveDoriasLullaby)
    {
        addCutsceneLine(cutscene, CS_STYLE_THE_MAYOR, "TODO", false, 0, NULL);
    }
    // After getting lullaby
    else
    {
        addCutsceneLine(cutscene, CS_STYLE_THE_MAYOR, "TODO", false, 0, NULL);
    }
}
