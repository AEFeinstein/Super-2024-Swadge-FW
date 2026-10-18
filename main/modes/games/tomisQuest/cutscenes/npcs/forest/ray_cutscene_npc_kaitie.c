#include "ray_cutscene.h"
#include "ray_cutscene_npc_kaitie.h"

/**
 * @brief Add a cutscene style for Kaitie
 *
 * @param cutscene The cutscene to add to
 */
void addCutsceneStyleKaitie(cutscene_t* cutscene)
{
    addCutsceneStyle(cutscene, c000, TQ_KAITIE_PORTRAIT_WSG, CUTSCENE_TEXTBOX_WSG, "Kaitie", 1, true, true, true);
}

/**
 * @brief Initialize the cutscene for Kaitie
 *
 * Note that Tomi is style 0 and Hyut is style 1
 *
 * @param ray The entire game state
 * @param cutscene The cutscene to add to
 */
void initNpcCutsceneKaitie(ray_t* ray, cutscene_t* cutscene)
{
#define CS_STYLE_KAITIE (CS_STYLE_HYUT + 1)
    addCutsceneStyleKaitie(cutscene);

    // clang-format off

    if (!ray->p.i.haveEwiOfTime)
    {
        // Before getting Ewi
        addCutsceneLine(cutscene, CS_STYLE_KAITIE, "Oh shoot Tomi, does the Great DeJay Tree know you didn't get a kid?", false, 0, NULL);
    }
    // After getting Ewi, before visiting Loot
    else if (!(ray->p.cutsceneViewed & TQS_LOOT_INTRO))
    {
        addCutsceneLine(cutscene, CS_STYLE_KAITIE, "Woah! Tomi! Who's that with you?", false, 0, NULL);
    }
    // After visiting Loot, before getting shield
    else if (!ray->p.i.haveShield)
    {
        addCutsceneLine(cutscene, CS_STYLE_KAITIE, "Oh, Hi Tomi! Back so soon?", false, 0, NULL);
    }
    // After getting shield, before getting turntables
    else if (!ray->p.i.haveTurntables)
    {
        addCutsceneLine(cutscene, CS_STYLE_KAITIE, "Oh hey Tomi! Cool SHIELD you got there!", false, 0, NULL);
    }
    // After getting turntables, before getting lullaby
    else if (!ray->p.i.haveDoriasLullaby)
    {
        addCutsceneLine(cutscene, CS_STYLE_KAITIE, "Oh hey Tomi! The Great DeJay tree was asking about you.", false, 0, NULL);
    }
    // After getting lullaby
    else
    {
        addCutsceneLine(cutscene, CS_STYLE_KAITIE, "Welcome back Tomi! It's good to see you!", false, 0, NULL);
    }

    // clang-format on
}
