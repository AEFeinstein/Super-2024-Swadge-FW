#include "ray_cutscene.h"
#include "ray_cutscene_npc_great_deejay_tree.h"

/**
 * @brief Add a cutscene style for Great DeeJay Tree
 *
 * @param cutscene The cutscene to add to
 */
void addCutsceneStyleGreatDeejayTree(cutscene_t* cutscene)
{
    addCutsceneStyle(cutscene, c000, TQ_GREAT_DEEJAY_TREE_PORTRAIT_WSG, CUTSCENE_TEXTBOX_WSG, "Great DeeJay Tree", 1,
                     true, true, true);
}

/**
 * @brief Initialize the cutscene for Great DeeJay Tree
 *
 * Note that Tomi is style 0 and Hyut is style 1
 *
 * @param ray The entire game state
 * @param cutscene The cutscene to add to
 */
void initNpcCutsceneGreatDeejayTree(ray_t* ray, cutscene_t* cutscene)
{
#define CS_STYLE_GREAT_DEEJAY_TREE (CS_STYLE_HYUT + 1)
    addCutsceneStyleGreatDeejayTree(cutscene);

    // clang-format off

    if (!ray->p.i.haveEwiOfTime)
    {
        // Before getting Ewi
        addCutsceneLine(cutscene, CS_STYLE_GREAT_DEEJAY_TREE, "TODO", false, 0, NULL);
    }
    // After getting Ewi, before visiting Loot
    else if (!(ray->p.cutsceneViewed & TQS_LOOT_INTRO))
    {
        addCutsceneLine(cutscene, CS_STYLE_GREAT_DEEJAY_TREE, "TODO", false, 0, NULL);
    }
    // After visiting Loot, before getting shield
    else if (!ray->p.i.haveShield)
    {
        addCutsceneLine(cutscene, CS_STYLE_GREAT_DEEJAY_TREE, "TODO", false, 0, NULL);
    }
    // After getting shield, before getting turntables
    else if (!ray->p.i.haveTurntables)
    {
        addCutsceneLine(cutscene, CS_STYLE_GREAT_DEEJAY_TREE, "TODO", false, 0, NULL);
    }
    // After getting turntables, before getting lullaby
    else if (!ray->p.i.haveDoriasLullaby)
    {
        addCutsceneLine(cutscene, CS_STYLE_GREAT_DEEJAY_TREE, "TODO", false, 0,
                        NULL);
    }
    // After getting lullaby
    else
    {
        addCutsceneLine(cutscene, CS_STYLE_GREAT_DEEJAY_TREE, "TODO", false, 0, NULL);
    }

    // clang-format on
}
