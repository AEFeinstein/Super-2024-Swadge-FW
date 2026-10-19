#include "ray_cutscene.h"
#include "ray_cutscene_npc_king_donut.h"

/**
 * @brief Add a cutscene style for King Donut
 *
 * @param cutscene The cutscene to add to
 */
void addCutsceneStyleKingDonut(cutscene_t* cutscene)
{
    addCutsceneStyle(cutscene, c000, TQ_KING_DONUT_PORTRAIT_WSG, CUTSCENE_TEXTBOX_WSG, "King Donut", 1, true, true,
                     true);
}

/**
 * @brief Initialize the cutscene for King Donut
 *
 * Note that Tomi is style 0 and Hyut is style 1
 *
 * @param ray The entire game state
 * @param cutscene The cutscene to add to
 */
void initNpcCutsceneKingDonut(ray_t* ray, cutscene_t* cutscene)
{
#define CS_STYLE_KING_DONUT (CS_STYLE_HYUT + 1)
    addCutsceneStyleKingDonut(cutscene);

    // clang-format off

    // After visiting Loot, before getting shield
    if (!ray->p.i.haveShield)
    {
        addCutsceneLine(cutscene, CS_STYLE_KING_DONUT, "Man, have you sampled any of the fine goods over at the ITEM SHOP? They're delicious!", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "um... none of the items in the ITEM SHOP looked like food...", false, TOMI_SUSPICIOUS, NULL);
        addCutsceneLine(cutscene, CS_STYLE_KING_DONUT, "Sure, not if you have THAT attitude!", false, 0, NULL);
    }
    // After getting shield, before getting turntables
    else if (!ray->p.i.haveTurntables)
    {
        addCutsceneLine(cutscene, CS_STYLE_KING_DONUT, "Welcome back, brave adventurers!", false, 0, NULL);
    }
    // After getting turntables, before getting lullaby
    else if (!ray->p.i.haveDoriasLullaby)
    {
        addCutsceneLine(cutscene, CS_STYLE_KING_DONUT, "Back again, brave Adventurers! Need to stock up on provisions?", false, 0, NULL);
    }
    // After getting lullaby
    else
    {
        addCutsceneLine(cutscene, CS_STYLE_KING_DONUT, "Hello Tomi!", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Hello, your majesty!", false, TOMI_CALM, NULL);
        addCutsceneLine(cutscene, CS_STYLE_KING_DONUT, "I will be returning to my kingdom soon, sadly. No rest for visiting royalty!", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Did you get to do everything you wanted in Gaylordia?", false, TOMI_CALM, NULL);
        addCutsceneLine(cutscene, CS_STYLE_KING_DONUT, "never enough time to see everyone and do everything. But alas, it's always this way. The good news is, there's always next year!", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Until then!", false, TOMI_CALM, NULL);
    }
    // clang-format on
}
